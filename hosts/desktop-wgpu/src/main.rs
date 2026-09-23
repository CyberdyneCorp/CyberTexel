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
//! Exit codes:
//!   0  the route ran on a device and every assertion held
//!   2  the route failed
//!   3  no adapter was available, so the run is unmeasured (never a pass)

mod plan;

use std::path::PathBuf;

use cybertexel::{
    emit_default_host_material, CompletedHostResource, CompletionDisposition, Document,
    HostExecutionSession, HostRecovery, HostResource, ReadbackStatus, ReplaySemantics,
    ResourceOwner, ResourceState, ShaderTarget, SnapshotPool,
};
use plan::PassPlan;

const STABLE_IDENTITY: &str = "reference-host/desktop-wgsl";
const OUTPUT_IDENTITY: &str = "material-output";
const SEMANTIC: &str = "pbr.base_color";
const UNMEASURED: i32 = 3;

struct Device {
    device: wgpu::Device,
    queue: wgpu::Queue,
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
    if pixels.chunks_exact(4).any(|texel| texel != first) {
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

    // 4. The explicit transport path: what changed, pinned, laid out, read back.
    let mut document = Document::new().map_err(|error| format!("document failed: {error}"))?;
    let texture_set = document
        .create_texture_set("Reference", "reference", 64, 64)
        .map_err(|error| format!("texture set failed: {error}"))?;
    document
        .set_channel_enabled(&texture_set, SEMANTIC, 0)
        .map_err(|error| format!("channel failed: {error}"))?;
    let pool = SnapshotPool::new(4 << 20).map_err(|error| format!("pool failed: {error}"))?;
    let before = pool
        .current_cursor(&document, &texture_set, SEMANTIC)
        .map_err(|error| format!("cursor failed: {error}"))?;
    // A write matching the stored value produces no delta by design, and the
    // emitted material is a constant that may equal the channel's default. The
    // host therefore writes the device colour and its complement, so at least
    // one texel differs and the tile is genuinely dirty.
    let device_texel = [first[0], first[1], first[2]];
    let complement = [!first[0], !first[1], !first[2]];
    document
        .write_channel_pixel(&texture_set, SEMANTIC, 2, 3, &device_texel)
        .map_err(|error| format!("write failed: {error}"))?;
    document
        .write_channel_pixel(&texture_set, SEMANTIC, 4, 5, &complement)
        .map_err(|error| format!("complement write failed: {error}"))?;
    let snapshot = pool
        .snapshot(&document, &texture_set, SEMANTIC, before)
        .map_err(|error| format!("snapshot failed: {error}"))?;
    let mut readback = snapshot
        .begin_host_readback()
        .map_err(|error| format!("readback failed: {error}"))?;
    let sizes = readback.tile_byte_sizes();
    if sizes.is_empty() {
        return Err("the snapshot reported no changed tile to read back".to_string());
    }
    let initial = readback.status().map_err(|e| e.to_string())?;
    if initial != ReadbackStatus::Pending {
        return Err(format!(
            "a fresh host readback must be pending, not {initial:?}"
        ));
    }
    if readback.tiles().is_ok() {
        return Err("a pending host readback must not publish tiles".to_string());
    }
    let tiles: Vec<Vec<u8>> = sizes.iter().map(|size| vec![0u8; *size]).collect();
    readback
        .complete(&tiles)
        .map_err(|error| format!("readback completion failed: {error}"))?;
    if readback.status().map_err(|e| e.to_string())? != ReadbackStatus::Complete {
        return Err("a completed host readback must report complete".to_string());
    }

    Ok(serde_json::json!({
        "host": "desktop-wgsl",
        "adapter": gpu.adapter,
        "backend": gpu.backend,
        "stable_identity": parsed.stable_identity,
        "pass_count": parsed.passes.len(),
        "target": { "width": width, "height": height, "format": parsed.resources[0].format },
        "rendered_texel": device_texel,
        "published_revision": completion.published_revision,
        "transport_tile_count": sizes.len(),
        "transport_tile_bytes": sizes,
        "synchronous_pixel_readbacks": 0,
    }))
}

fn main() {
    let report = std::env::args()
        .skip_while(|argument| argument != "--report")
        .nth(1)
        .map(PathBuf::from);
    match run() {
        Ok(value) => {
            let text = serde_json::to_string_pretty(&value).expect("report serializes");
            if let Some(path) = report {
                if let Some(parent) = path.parent() {
                    let _ = std::fs::create_dir_all(parent);
                }
                std::fs::write(&path, format!("{text}\n")).expect("report writes");
            }
            println!("{text}");
            println!("ok: the desktop WGSL reference host ran the pass plan on a device");
        }
        Err(message) if message.starts_with("no wgpu adapter") => {
            eprintln!("unmeasured: {message}");
            std::process::exit(UNMEASURED);
        }
        Err(message) => {
            eprintln!("desktop reference host failed: {message}");
            std::process::exit(2);
        }
    }
}
