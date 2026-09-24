//! Desktop WGSL reference host.
//!
//! CyberTexel does not own a GPU device. It emits shader source and an ordered
//! pass plan, and the host runs them on the device it already has. This binary
//! is the smallest program that is honestly such a host: it asks the library
//! for a material program, executes the plan on a real `wgpu` device, reports
//! completion so the library publishes the revision, and then drives the
//! explicit transport path — cursor, snapshot, tile layout, host readback.
//!
//! It is an integration host, not an example. The display-free Python examples
//! prove the contract with a software stand-in; this proves it on a device API.
//!
//! With `--benchmark` it also opens a window, configures a surface on it and
//! measures input-to-visible latency there. See `benchmark.rs` for what each
//! stage covers and where the figure's boundaries are.
//!
//! Exit codes:
//!   0  the route ran on a device and every assertion held
//!   2  the route failed
//!   3  no adapter, display or surface was available, so the run is
//!      unmeasured (never a pass)

mod authoring;
mod benchmark;
mod plan;
mod residency;

use std::path::PathBuf;

use cybertexel::{
    emit_default_host_material, CompletedHostResource, CompletionDisposition, Document,
    HostExecutionSession, HostRecovery, HostResource, LayerEntry, MeshData, ReplaySemantics,
    ResourceOwner, ResourceState, ShaderTarget, SnapshotPool,
};
use plan::PassPlan;

const STABLE_IDENTITY: &str = "reference-host/desktop-wgsl";
const OUTPUT_IDENTITY: &str = "material-output";
const SEMANTIC: &str = "pbr.base_color";
/// The interactive-4k configuration device-gate declares.
const CHANNELS: [&str; 4] = [
    "pbr.base_color",
    "pbr.roughness",
    "pbr.metallic",
    "pbr.normal",
];
const TEXTURE_EXTENT: u32 = 4096;
const MESH_TRIANGLES: usize = 250_000;
const LAYER_COUNT: usize = 8;
const UNMEASURED: i32 = 3;

/// Frames the windowed benchmark records once warm-up has passed.
const BENCHMARK_FRAMES: usize = 600;

pub struct Device {
    pub device: wgpu::Device,
    pub queue: wgpu::Queue,
    backend: String,
    adapter: String,
}

/// Acquire any adapter, including a software one, so CI without a GPU still
/// exercises the route. An absent adapter is reported, never worked around.
fn acquire_device() -> Result<Device, String> {
    let instance = wgpu::Instance::default();
    let adapter = pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
        power_preference: wgpu::PowerPreference::HighPerformance,
        force_fallback_adapter: false,
        compatible_surface: None,
        ..Default::default()
    }))
    .or_else(|_| {
        pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
            power_preference: wgpu::PowerPreference::LowPower,
            force_fallback_adapter: true,
            compatible_surface: None,
            ..Default::default()
        }))
    })
    .map_err(|error| format!("no wgpu adapter is available: {error}"))?;
    let information = adapter.get_info();
    let (device, queue) = pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor {
        label: Some("cybertexel-reference-host"),
        required_features: wgpu::Features::empty(),
        required_limits: wgpu::Limits::downlevel_defaults(),
        memory_hints: wgpu::MemoryHints::default(),
        trace: wgpu::Trace::Off,
        ..Default::default()
    }))
    .map_err(|error| format!("wgpu adapter refused a device: {error}"))?;
    Ok(Device {
        device,
        queue,
        backend: format!("{:?}", information.backend),
        adapter: information.name,
    })
}

/// Acquire a device that can present to this window's surface.
///
/// The headless route above asks for any adapter; a presenting route must ask
/// for one the surface is compatible with, and reports its absence rather than
/// falling back to an adapter that cannot reach the display.
fn acquire_presenting_device(
    instance: &wgpu::Instance,
    surface: &wgpu::Surface<'static>,
) -> Result<Option<(wgpu::Adapter, Device)>, String> {
    let Ok(adapter) = pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
        power_preference: wgpu::PowerPreference::HighPerformance,
        force_fallback_adapter: false,
        compatible_surface: Some(surface),
        ..Default::default()
    })) else {
        return Ok(None);
    };
    let information = adapter.get_info();
    // The measured configuration is 4096 square, which downlevel defaults
    // forbid, so the device asks for what this adapter actually offers.
    let (device, queue) = pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor {
        label: Some("cybertexel-reference-host-presenting"),
        required_features: wgpu::Features::empty(),
        required_limits: adapter.limits(),
        memory_hints: wgpu::MemoryHints::default(),
        trace: wgpu::Trace::Off,
        ..Default::default()
    }))
    .map_err(|error| format!("the presenting adapter refused a device: {error}"))?;
    Ok(Some((
        adapter,
        Device {
            device,
            queue,
            backend: format!("{:?}", information.backend),
            adapter: information.name,
        },
    )))
}

/// Execute one render pass of the plan and return the rendered texels.
fn execute(
    gpu: &Device,
    parsed: &PassPlan,
    vertex_source: &str,
    fragment_source: &str,
) -> Result<(Vec<u8>, u32, u32), String> {
    let pass = parsed
        .passes
        .first()
        .ok_or_else(|| "pass plan carries no pass".to_string())?;
    if pass.kind != "render" {
        return Err(format!(
            "this host runs render passes; the plan names {}",
            pass.kind
        ));
    }
    if pass.depth_target.is_some() {
        return Err("this host does not carry a depth target yet".to_string());
    }
    if pass.command.kind != "draw" || pass.command.indexed {
        return Err(format!(
            "this host runs non-indexed draws; the plan names {}",
            pass.command.kind
        ));
    }
    let target = pass
        .render_targets
        .first()
        .ok_or_else(|| "render pass names no render target".to_string())?;
    let format = PassPlan::texture_format(&target.format)?;
    let topology = PassPlan::topology(&pass.command.topology)?;

    // The plan's declared resource must agree with the target it is drawn into.
    let resource = parsed
        .resources
        .iter()
        .find(|entry| entry.version.logical_id == target.resource.logical_id)
        .ok_or_else(|| format!("no resource declares {}", target.resource.logical_id))?;
    if resource.extent.width != target.width || resource.extent.height != target.height {
        return Err("resource extent and render target size disagree".to_string());
    }
    if resource.format != target.format {
        return Err("resource format and render target format disagree".to_string());
    }

    let texture = gpu.device.create_texture(&wgpu::TextureDescriptor {
        label: Some(&target.resource.logical_id),
        size: wgpu::Extent3d {
            width: target.width,
            height: target.height,
            depth_or_array_layers: resource.extent.layers,
        },
        mip_level_count: 1,
        sample_count: 1,
        dimension: wgpu::TextureDimension::D2,
        format,
        usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::COPY_SRC,
        view_formats: &[],
    });
    let view = texture.create_view(&wgpu::TextureViewDescriptor::default());

    // The plan declares the vertex layout; the host owns the geometry. A
    // material preview is one full-screen triangle in clip space, which the
    // emitted vertex stage maps to the 0..1 material coordinate.
    if pass.vertex_buffers.len() != 1 {
        return Err(format!(
            "this host supplies one vertex buffer; the plan declares {}",
            pass.vertex_buffers.len()
        ));
    }
    if !pass.texture_bindings.is_empty()
        || !pass.sampler_bindings.is_empty()
        || !pass.uniform_blocks.is_empty()
    {
        return Err(
            "this host does not carry bound textures, samplers or uniforms yet".to_string(),
        );
    }
    let declared = &pass.vertex_buffers[0];
    let attributes: Vec<wgpu::VertexAttribute> = declared
        .attributes
        .iter()
        .map(|attribute| {
            Ok(wgpu::VertexAttribute {
                format: attribute.format()?,
                offset: attribute.offset,
                shader_location: attribute.location,
            })
        })
        .collect::<Result<_, String>>()?;
    if !declared.attributes.iter().any(|a| a.semantic == "position") {
        return Err("the declared vertex layout carries no position attribute".to_string());
    }
    const FULLSCREEN_TRIANGLE: [f32; 6] = [-1.0, -1.0, 3.0, -1.0, -1.0, 3.0];
    if declared.stride != 8 {
        return Err(format!(
            "this host supplies a vec2 position stream; the plan declares stride {}",
            declared.stride
        ));
    }
    let geometry = gpu.device.create_buffer(&wgpu::BufferDescriptor {
        label: Some("cybertexel-fullscreen-triangle"),
        size: std::mem::size_of_val(&FULLSCREEN_TRIANGLE) as u64,
        usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
        mapped_at_creation: false,
    });
    gpu.queue
        .write_buffer(&geometry, 0, bytemuck_cast(&FULLSCREEN_TRIANGLE));

    let vertex = gpu
        .device
        .create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("cybertexel-vertex"),
            source: wgpu::ShaderSource::Wgsl(vertex_source.into()),
        });
    let fragment = gpu
        .device
        .create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("cybertexel-fragment"),
            source: wgpu::ShaderSource::Wgsl(fragment_source.into()),
        });
    let layout = gpu
        .device
        .create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("cybertexel-pipeline-layout"),
            bind_group_layouts: &[],
            ..Default::default()
        });
    let pipeline = gpu
        .device
        .create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some(&pass.identifier),
            layout: Some(&layout),
            vertex: wgpu::VertexState {
                module: &vertex,
                entry_point: Some(&pass.entry_points.vertex),
                compilation_options: Default::default(),
                buffers: &[Some(wgpu::VertexBufferLayout {
                    array_stride: declared.stride,
                    step_mode: declared.step_mode()?,
                    attributes: &attributes,
                })],
            },
            fragment: Some(wgpu::FragmentState {
                module: &fragment,
                entry_point: Some(&pass.entry_points.fragment),
                compilation_options: Default::default(),
                targets: &[Some(wgpu::ColorTargetState {
                    format,
                    blend: None,
                    write_mask: wgpu::ColorWrites::ALL,
                })],
            }),
            primitive: wgpu::PrimitiveState {
                topology,
                ..Default::default()
            },
            depth_stencil: None,
            multisample: wgpu::MultisampleState::default(),
            multiview_mask: None,
            cache: None,
        });

    // rgba8 is four bytes per texel; the copy row must be 256-byte aligned.
    let bytes_per_texel = 4u32;
    let unpadded = target.width * bytes_per_texel;
    let align = wgpu::COPY_BYTES_PER_ROW_ALIGNMENT;
    let padded = unpadded.div_ceil(align) * align;
    let readback = gpu.device.create_buffer(&wgpu::BufferDescriptor {
        label: Some("cybertexel-readback"),
        size: u64::from(padded) * u64::from(target.height),
        usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
        mapped_at_creation: false,
    });

    let load = if target.load == "clear" {
        wgpu::LoadOp::Clear(wgpu::Color {
            r: target.clear_colour[0],
            g: target.clear_colour[1],
            b: target.clear_colour[2],
            a: target.clear_colour[3],
        })
    } else {
        wgpu::LoadOp::Load
    };

    let mut encoder = gpu
        .device
        .create_command_encoder(&wgpu::CommandEncoderDescriptor { label: None });
    {
        let mut render = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some(&pass.identifier),
            color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                view: &view,
                depth_slice: None,
                resolve_target: None,
                ops: wgpu::Operations {
                    load,
                    store: wgpu::StoreOp::Store,
                },
            })],
            depth_stencil_attachment: None,
            timestamp_writes: None,
            occlusion_query_set: None,
            ..Default::default()
        });
        render.set_pipeline(&pipeline);
        render.set_vertex_buffer(declared.slot, geometry.slice(..));
        let first = pass.command.first_vertex;
        let instances =
            pass.command.first_instance..pass.command.first_instance + pass.command.instance_count;
        render.draw(first..first + pass.command.vertex_count, instances);
    }
    encoder.copy_texture_to_buffer(
        wgpu::TexelCopyTextureInfo {
            texture: &texture,
            mip_level: 0,
            origin: wgpu::Origin3d::ZERO,
            aspect: wgpu::TextureAspect::All,
        },
        wgpu::TexelCopyBufferInfo {
            buffer: &readback,
            layout: wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(padded),
                rows_per_image: Some(target.height),
            },
        },
        wgpu::Extent3d {
            width: target.width,
            height: target.height,
            depth_or_array_layers: 1,
        },
    );
    gpu.queue.submit(Some(encoder.finish()));

    let slice = readback.slice(..);
    let (sender, receiver) = std::sync::mpsc::channel();
    slice.map_async(wgpu::MapMode::Read, move |result| {
        let _ = sender.send(result);
    });
    gpu.device
        .poll(wgpu::PollType::wait_indefinitely())
        .map_err(|error| format!("device poll failed: {error}"))?;
    receiver
        .recv()
        .map_err(|error| format!("readback never resolved: {error}"))?
        .map_err(|error| format!("readback failed: {error}"))?;

    let mapped = slice
        .get_mapped_range()
        .map_err(|error| format!("mapped range unavailable: {error}"))?;
    let mut pixels = Vec::with_capacity((unpadded * target.height) as usize);
    for row in 0..target.height {
        let start = (row * padded) as usize;
        pixels.extend_from_slice(&mapped[start..start + unpadded as usize]);
    }
    drop(mapped);
    readback.unmap();
    Ok((pixels, target.width, target.height))
}

/// Reinterpret a float array as the bytes a vertex buffer upload needs.
fn bytemuck_cast(values: &[f32]) -> &[u8] {
    // Safety: f32 has no padding or invalid bit patterns, and the returned
    // slice borrows the same memory for the same lifetime.
    unsafe {
        std::slice::from_raw_parts(values.as_ptr().cast::<u8>(), std::mem::size_of_val(values))
    }
}

/// A text shader artifact crosses the C ABI as bytes with a terminating NUL.
/// Naga rejects that byte as a global item, so the host trims it rather than
/// handing the driver a string the library never meant as source.
fn wgsl_source(artifact: &[u8], stage: &str) -> Result<String, String> {
    let trimmed = artifact
        .iter()
        .rposition(|byte| *byte != 0)
        .map_or(&artifact[..0], |last| &artifact[..=last]);
    std::str::from_utf8(trimmed)
        .map(str::to_owned)
        .map_err(|error| format!("{stage} artifact is not UTF-8 WGSL: {error}"))
}

fn run() -> Result<serde_json::Value, String> {
    let gpu = acquire_device()?;

    // 1. The library emits WGSL and a device-independent plan; it never sees
    //    the device handle above.
    let program =
        emit_default_host_material(STABLE_IDENTITY, OUTPUT_IDENTITY, 64, 64, ShaderTarget::Wgsl)
            .map_err(|error| format!("emission failed: {error}"))?;
    let parsed: PassPlan = serde_json::from_str(&program.pass_plan)
        .map_err(|error| format!("pass plan did not parse: {error}"))?;
    if parsed.stable_identity != STABLE_IDENTITY {
        return Err("pass plan carries a different stable identity".to_string());
    }
    let vertex_source = wgsl_source(&program.vertex_artifact, "vertex")?;
    let fragment_source = wgsl_source(&program.fragment_artifact, "fragment")?;

    // 2. The host executes the plan on its own device.
    let (pixels, width, height) = execute(&gpu, &parsed, &vertex_source, &fragment_source)?;
    if pixels.len() != (width * height * 4) as usize {
        return Err("device produced the wrong number of bytes".to_string());
    }
    let first = &pixels[0..4];
    if pixels
        .as_chunks::<4>()
        .0
        .iter()
        .any(|texel| *texel != first)
    {
        return Err("the constant material did not render a constant image".to_string());
    }

    // 3. The host reports completion; the library publishes the revision
    //    without the pixels ever leaving the device.
    let resource = &parsed.resources[0];
    let mut session =
        HostExecutionSession::new(0).map_err(|error| format!("session failed: {error}"))?;
    let declared = HostResource {
        logical_id: OUTPUT_IDENTITY.to_string(),
        generation: resource.version.generation,
        role: resource.role.clone(),
        format: 2,
        width,
        height,
        layers: resource.extent.layers,
        mip_levels: resource.mip_levels,
        tile_width: resource.tile_shape.width,
        tile_height: resource.tile_shape.height,
        externally_initialized: resource.externally_initialized,
        owner: ResourceOwner::Host,
        required_state: ResourceState::RenderTarget,
        output: true,
    };
    let token = session
        .submit(
            "material-graph",
            0,
            &[declared],
            ReplaySemantics::Deterministic,
        )
        .map_err(|error| format!("submit failed: {error}"))?;
    let completion = session
        .complete(
            token,
            &[CompletedHostResource {
                logical_id: OUTPUT_IDENTITY.to_string(),
                generation: resource.version.generation,
                format: 2,
                width,
                height,
                layers: resource.extent.layers,
            }],
            Some(&HostRecovery {
                operation_record_version: "material-graph-v1".to_string(),
                checkpoint_revision: 0,
                retained_bytes: pixels.len(),
                inputs_pinned: true,
            }),
        )
        .map_err(|error| format!("completion failed: {error}"))?;
    if completion.disposition != CompletionDisposition::Published {
        return Err(format!(
            "completion was not published: {}",
            completion.message
        ));
    }

    drop(session);

    // 4. Residency traffic: the explicit transport path, instrumented.
    //    device-gate budgets ordinary resident paint and undo at zero bytes of
    //    synchronous pixel readback, so the host counts what it actually moved.
    //    The configuration is the one device-gate declares as interactive-4k:
    //    a 4096-square texture set over a 250k-triangle mesh, four enabled
    //    channels and eight visible layers. A host that measured something
    //    smaller would have its figures refused as a configuration mismatch.
    let mut document = Document::new().map_err(|error| format!("document failed: {error}"))?;
    let mesh = MeshData::grid(MESH_TRIANGLES);
    let texture_set = document
        .create_texture_sets_from_mesh(&mesh, TEXTURE_EXTENT, TEXTURE_EXTENT)
        .map_err(|error| format!("texture sets failed: {error}"))?
        .into_iter()
        .next()
        .ok_or_else(|| "the mesh derived no texture set".to_string())?;
    for semantic in CHANNELS {
        document
            .set_channel_enabled(&texture_set, semantic, 0)
            .map_err(|error| format!("channel failed: {error}"))?;
    }
    document
        .append_layers(
            &texture_set,
            &(0..LAYER_COUNT)
                .map(|index| {
                    LayerEntry::paint(&format!("paint.{index}"), &format!("Layer {index}"))
                })
                .collect::<Vec<_>>(),
        )
        .map_err(|error| format!("layers failed: {error}"))?;
    document
        .configure_tile_history(&texture_set, 64 << 20)
        .map_err(|error| format!("history budget failed: {error}"))?;
    let pool = SnapshotPool::new(4 << 20).map_err(|error| format!("pool failed: {error}"))?;
    let traffic = residency::measure(&gpu, &mut document, &texture_set, SEMANTIC, &pool)?;
    if traffic.paint.synchronous_readback_bytes != 0 || traffic.undo.synchronous_readback_bytes != 0
    {
        return Err("resident paint or undo performed a synchronous pixel readback".to_string());
    }
    if traffic.undo.copied_pixel_bytes != 0 {
        return Err(format!(
            "undo copied {} pixel bytes; it must exchange storage owners",
            traffic.undo.copied_pixel_bytes
        ));
    }

    Ok(serde_json::json!({
        "host": "desktop-wgsl",
        "adapter": gpu.adapter,
        "backend": gpu.backend,
        "stable_identity": parsed.stable_identity,
        "pass_count": parsed.passes.len(),
        "target": { "width": width, "height": height, "format": parsed.resources[0].format },
        "rendered_texel": [first[0], first[1], first[2]],
        "published_revision": completion.published_revision,
        "residency": {
            "tiles_uploaded": traffic.tiles_uploaded,
            "paint_upload_bytes": traffic.paint.upload_bytes,
            "paint_synchronous_readback_bytes": traffic.paint.synchronous_readback_bytes,
            "paint_ms": traffic.paint.elapsed_ms,
            "undo_synchronous_readback_bytes": traffic.undo.synchronous_readback_bytes,
            "undo_copied_pixel_bytes": traffic.undo.copied_pixel_bytes,
            "undo_ms": traffic.undo.elapsed_ms,
            "history_retained_bytes": traffic.history_retained_bytes,
        },
        "configuration": {
            "id": "interactive-4k",
            "texture_extent": TEXTURE_EXTENT,
            "mesh_triangles": mesh.triangle_count(),
            "layers": LAYER_COUNT,
            "channels": CHANNELS.len(),
        },
    }))
}

/// What the command line asked for.
struct Options {
    report: Option<PathBuf>,
    measurements: Option<PathBuf>,
    benchmark: bool,
    frames: usize,
}

impl Options {
    fn parse() -> Self {
        let arguments: Vec<String> = std::env::args().collect();
        let value = |name: &str| {
            arguments
                .iter()
                .position(|argument| argument == name)
                .and_then(|index| arguments.get(index + 1))
                .cloned()
        };
        Self {
            report: value("--report").map(PathBuf::from),
            measurements: value("--measurements").map(PathBuf::from),
            benchmark: arguments.iter().any(|argument| argument == "--benchmark"),
            frames: value("--frames")
                .and_then(|frames| frames.parse().ok())
                .filter(|frames| *frames > 0)
                .unwrap_or(BENCHMARK_FRAMES),
        }
    }
}

/// The three latency measurements, emitted only when a surface produced them.
fn latency_measurements(measured: &benchmark::Measured) -> Vec<serde_json::Value> {
    [
        ("desktop-visible-median", measured.median_ms),
        ("desktop-visible-p95", measured.p95_ms),
        ("desktop-visible-p99", measured.p99_ms),
    ]
    .into_iter()
    .map(|(budget_id, value)| {
        serde_json::json!({
            "budget_id": budget_id,
            "value": value,
            "unit": "ms",
            "configuration": "interactive-4k",
            "batch_size": 1,
        })
    })
    .collect()
}

/// The command that produced this run.
///
/// The headless route is the named recipe; a benchmark run records the
/// invocation it actually ran, because the recipe does not carry the flag.
fn command_line(benchmark: bool) -> String {
    if !benchmark {
        return "just host-desktop".to_string();
    }
    let mut parts: Vec<String> = std::env::args().collect();
    if let Some(first) = parts.first_mut() {
        *first = std::path::Path::new(first.as_str())
            .file_name()
            .map_or_else(|| first.clone(), |name| name.to_string_lossy().into_owned());
    }
    parts.join(" ")
}

/// Why a figure is absent, stated precisely enough to be actionable.
fn unmeasured_reasons(measured: Option<&benchmark::Measured>) -> Vec<String> {
    let mut reasons = vec![
        "the instrumented residency paint step uses write_channel_pixel, a convenience that \
zeroes a whole-canvas coverage buffer, so its wall time is not a latency figure"
            .to_string(),
    ];
    if measured.is_none() {
        reasons.insert(
            0,
            "desktop-visible-* needs a presenting surface; run with --benchmark".to_string(),
        );
    }
    reasons
}

/// Write the schema-1 measurement run device-gate decides on.
///
/// The residency byte budgets come from the headless route. The three
/// input-to-visible budgets are emitted only when `--benchmark` actually
/// presented frames; an absent figure is listed with its reason rather than
/// approximated, because device-gate forbids substituting a missing
/// measurement and a fabricated latency is worse than none.
fn write_measurements(
    path: &std::path::Path,
    report: &serde_json::Value,
    measured: Option<&benchmark::Measured>,
) -> Result<(), String> {
    let residency = &report["residency"];
    let mut measurements = vec![
        serde_json::json!({
            "budget_id": "desktop-paint-sync-readback",
            "value": residency["paint_synchronous_readback_bytes"],
            "unit": "bytes",
            "configuration": "interactive-4k",
            "batch_size": 1,
        }),
        serde_json::json!({
            "budget_id": "desktop-undo-sync-readback",
            "value": residency["undo_synchronous_readback_bytes"],
            "unit": "bytes",
            "configuration": "interactive-4k",
            "batch_size": 1,
        }),
    ];
    if let Some(measured) = measured {
        measurements.extend(latency_measurements(measured));
    }
    let mut run = serde_json::json!({
        "schema": 1,
        "device_id": "macbook-pro-m3-pro-18gpu-36gb",
        "date": report["date"],
        "commit": report["commit"],
        "command": command_line(measured.is_some()),
        "measurements": measurements,
        "not_measured": unmeasured_reasons(measured),
    });
    if let Some(measured) = measured {
        run["interaction"] = measured.trace();
        let interval = if measured.refresh_rate_hz > 0.0 {
            1000.0 / measured.refresh_rate_hz
        } else {
            0.0
        };
        run["notes"] = serde_json::json!([
            format!(
                "a handed-over frame becomes visible at the next refresh, so the pessimistic \
reading of every figure is the measured value plus one {interval:.3} ms refresh interval"
            ),
            "the presentation stage ends when the display frees the drawable the presented frame \
was queued behind; wgpu exposes no scanout timestamp, so the remaining wait is bounded rather than \
measured",
            "the benchmark waits for the device between stages, so the figure is a serialized \
upper bound rather than a pipelined best case",
        ]);
    }
    if let Some(parent) = path.parent() {
        std::fs::create_dir_all(parent).map_err(|error| error.to_string())?;
    }
    std::fs::write(
        path,
        serde_json::to_string_pretty(&run).map_err(|e| e.to_string())? + "\n",
    )
    .map_err(|error| error.to_string())
}

fn write_report(path: &std::path::Path, text: &str) {
    if let Some(parent) = path.parent() {
        let _ = std::fs::create_dir_all(parent);
    }
    std::fs::write(path, format!("{text}\n")).expect("report writes");
}

/// Run the windowed benchmark, or report why it is unmeasured.
fn run_benchmark(options: &Options) -> Result<benchmark::Outcome, String> {
    benchmark::measure(
        benchmark::Plan {
            frames: options.frames,
            extent: TEXTURE_EXTENT,
            mesh_triangles: MESH_TRIANGLES,
            layers: LAYER_COUNT,
        },
        &CHANNELS,
    )
}

fn publish(options: &Options, value: &serde_json::Value, measured: Option<&benchmark::Measured>) {
    if let Some(path) = &options.measurements {
        if let Err(error) = write_measurements(path, value, measured) {
            eprintln!("desktop reference host failed: {error}");
            std::process::exit(2);
        }
    }
    let text = serde_json::to_string_pretty(value).expect("report serializes");
    if let Some(path) = &options.report {
        write_report(path, &text);
    }
    println!("{text}");
}

fn main() {
    let options = Options::parse();
    let mut value = match run() {
        Ok(value) => value,
        Err(message) if message.starts_with("no wgpu adapter") => {
            eprintln!("unmeasured: {message}");
            std::process::exit(UNMEASURED);
        }
        Err(message) => {
            eprintln!("desktop reference host failed: {message}");
            std::process::exit(2);
        }
    };
    value["date"] = serde_json::Value::String(
        std::env::var("CTEX_RUN_DATE").unwrap_or_else(|_| "unknown".to_string()),
    );
    value["commit"] = serde_json::Value::String(
        std::env::var("CTEX_RUN_COMMIT").unwrap_or_else(|_| "unknown".to_string()),
    );
    if !options.benchmark {
        publish(&options, &value, None);
        println!("ok: the desktop WGSL reference host ran the pass plan on a device");
        return;
    }
    match run_benchmark(&options) {
        Ok(benchmark::Outcome::Measured(measured)) => {
            value["benchmark"] = measured.report();
            publish(&options, &value, Some(&measured));
            println!(
                "ok: input-to-visible median {:.3} ms, p95 {:.3} ms, p99 {:.3} ms over {} frames",
                measured.median_ms,
                measured.p95_ms,
                measured.p99_ms,
                measured.frames.len()
            );
        }
        Ok(benchmark::Outcome::Unavailable(reason)) => {
            value["benchmark"] = serde_json::json!({ "not_measured": reason });
            let text = serde_json::to_string_pretty(&value).expect("report serializes");
            if let Some(path) = &options.report {
                write_report(path, &text);
            }
            println!("{text}");
            eprintln!("unmeasured: desktop-visible-* was not measured: {reason}");
            std::process::exit(UNMEASURED);
        }
        Err(message) => {
            eprintln!("desktop reference host failed: {message}");
            std::process::exit(2);
        }
    }
}
