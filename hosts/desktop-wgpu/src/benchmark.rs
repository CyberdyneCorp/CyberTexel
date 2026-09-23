//! Interaction latency, measured against a real presenting surface.
//!
//! The host opens a window, configures a `wgpu` surface on it, and drives a
//! synthetic pointer stream at the interactive-4k configuration. Every frame
//! is timed in five contiguous stages, so the stage figures sum to the frame's
//! input-to-visible latency by construction rather than by assertion:
//!
//! | stage | what it covers |
//! | --- | --- |
//! | `input` | from the instant the synthetic sample was due to the authored edit being committed |
//! | `upload` | publishing the changed tiles into the device texture, waiting for that copy to complete |
//! | `queue` | encoding the frame's commands |
//! | `execution` | submitting the frame and waiting for the GPU to finish it |
//! | `presentation` | presenting the frame and waiting for the display to free the drawable it was queued behind |
//!
//! Two properties of the figure are deliberate and are reported alongside it:
//!
//! * The loop waits for the GPU between stages. That serializes work a shipping
//!   application would pipeline, so the number is an upper bound, not a best
//!   case.
//! * `presentation` ends when the display has taken a frame off the queue the
//!   presented frame joined — the closest observable proxy for it going out.
//!   The scanout instant itself is not exposed by `wgpu`, so the remainder is
//!   bounded rather than measured: a frame is on the display within one refresh
//!   interval of the stage ending, and the report carries that bound beside
//!   every figure.

use std::sync::Arc;
use std::time::{Duration, Instant};

use cybertexel::MeshData;
use winit::application::ApplicationHandler;
use winit::dpi::LogicalSize;
use winit::event::WindowEvent;
use winit::event_loop::{ActiveEventLoop, ControlFlow, EventLoop};
use winit::window::{Window, WindowId, WindowLevel};

use crate::authoring::{Canvas, Sample, Stamp, DEVICE_TEXEL_BYTES, TILE_EXTENT};

/// Synthetic pointer samples per second, as interactive-4k declares.
const INPUT_RATE_HZ: f64 = 120.0;
/// The brush radius the synthetic stroke stamps, in texels.
const STAMP_RADIUS: u32 = 16;
/// Frames run before recording starts, so first-frame compilation and
/// swapchain warm-up never reach a reported percentile.
const WARMUP_FRAMES: usize = 60;
/// Phases the input clock is stepped through against the display.
///
/// A 120 Hz pointer stream against a 120 Hz display sits at one arbitrary
/// phase for a whole run, and that phase alone moves input-to-visible by up to
/// a frame interval. A real digitizer is not phase-locked to the display, so
/// the run steps the input clock through the interval instead of reporting
/// whichever alignment it happened to start at.
const PHASE_STEPS: usize = 8;
/// Consecutive hidden acquisitions that end a run before its warm-up does.
///
/// A window takes a few frames to reach the screen; one that has not arrived
/// after this many is not going to, and the run says so rather than painting
/// into it until the deadline.
const HIDDEN_ACQUISITION_LIMIT: usize = 240;
/// The run gives up rather than hanging if the window never produces frames.
const DEADLINE: Duration = Duration::from_secs(120);

const EDIT_NOTE: &str = "ctex_paint_plan_work plans the reachable storage tiles; a texture-set transaction declares that write set, writes the stamp and commits one undoable step";
const STAGE_NOTE: &str =
    "five contiguous intervals covering the whole frame, so they sum to the reported latency";
const PRESENTATION_NOTE: &str = "presents the frame and blocks until the display frees the drawable it was queued behind; wgpu exposes no scanout timestamp, so the remaining wait is bounded by one refresh interval rather than measured";
const PIPELINING_NOTE: &str = "the loop waits for the device between stages, so the figure is a serialized upper bound rather than a pipelined best case";

const SHADER: &str = r#"
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) coordinate: vec2<f32>,
};

@group(0) @binding(0) var authored_texture: texture_2d<f32>;
@group(0) @binding(1) var authored_sampler: sampler;

@vertex
fn vertex_main(@builtin(vertex_index) index: u32) -> VertexOutput {
    var corners = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>(3.0, -1.0),
        vec2<f32>(-1.0, 3.0),
    );
    let corner = corners[index];
    var output: VertexOutput;
    output.position = vec4<f32>(corner, 0.0, 1.0);
    output.coordinate = vec2<f32>((corner.x + 1.0) * 0.5, 1.0 - (corner.y + 1.0) * 0.5);
    return output;
}

@fragment
fn fragment_main(input: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(authored_texture, authored_sampler, input.coordinate);
}
"#;

/// One frame's five contiguous stages, in milliseconds.
#[derive(Clone, Copy, Debug, Default)]
pub struct Frame {
    pub input: f64,
    pub queue: f64,
    pub upload: f64,
    pub execution: f64,
    pub presentation: f64,
    pub upload_bytes: u64,
    pub uploaded_tiles: usize,
    pub declared_tiles: usize,
    pub committed_tiles: usize,
    pub authored_texels: usize,
    /// The authored edit's own cost, inside `input`.
    ///
    /// The rest of `input` is the age of the sample when the loop reached it.
    /// The split is reported because the whole point of the bounded paint-work
    /// route is that this figure stays in the tenths of a millisecond.
    pub authored_edit: f64,
    /// How long the sample had been waiting when the loop reached it.
    pub sample_age: f64,
}

impl Frame {
    /// The frame's input-to-visible latency.
    ///
    /// Summed in the order the emitted trace lists the stages, so the value
    /// the gate recomputes from `stages_ms` is bit-identical to this one.
    pub fn input_to_visible(&self) -> f64 {
        self.input + self.queue + self.upload + self.execution + self.presentation
    }

    pub fn stages(&self) -> serde_json::Value {
        serde_json::json!({
            "input": self.input,
            "queue": self.queue,
            "upload": self.upload,
            "execution": self.execution,
            "presentation": self.presentation,
        })
    }
}

/// What the benchmark measured, or why it could not.
pub enum Outcome {
    Measured(Box<Measured>),
    Unavailable(String),
}

/// The recorded run.
pub struct Measured {
    pub frames: Vec<Frame>,
    pub median: Frame,
    pub slowest: Frame,
    pub median_ms: f64,
    pub p95_ms: f64,
    pub p99_ms: f64,
    pub skipped: usize,
    pub refresh_rate_hz: f64,
    pub observed_input_rate_hz: f64,
    pub mesh_triangles: usize,
    pub layers: usize,
    pub channels: usize,
    pub extent: u32,
    pub retained_bytes: usize,
    pub adapter: String,
    pub backend: String,
    pub surface_format: String,
    pub surface_size: (u32, u32),
    pub present_mode: String,
    pub texture_set: String,
}

impl Measured {
    /// The interaction trace `tools/device_gate.py` validates.
    ///
    /// It reports the median frame rather than per-stage medians: the stages
    /// of one frame that actually happened sum to that frame's latency, while
    /// a stage-wise median would sum to nothing that ever ran.
    pub fn trace(&self) -> serde_json::Value {
        serde_json::json!({
            "input_to_visible_ms": self.median.input_to_visible(),
            "stages_ms": self.median.stages(),
            "input_rate_hz": INPUT_RATE_HZ,
            // Samples the loop could not keep up with are coalesced to the
            // newest, so the rate the run consumed is reported beside the rate
            // it generated rather than in place of it.
            "consumed_sample_rate_hz": self.observed_input_rate_hz,
            "refresh_rate_hz": self.refresh_rate_hz,
            "mesh_triangles": self.mesh_triangles,
            "layers": self.layers,
            "channels": self.channels,
            "residency": {
                "tiles_uploaded": self.median.uploaded_tiles,
                "upload_bytes": self.median.upload_bytes,
                "synchronous_readback_bytes": 0,
                "declared_write_set_tiles": self.median.declared_tiles,
                "history_committed_tiles": self.median.committed_tiles,
                "history_retained_bytes": self.retained_bytes,
            },
            "sample": "median frame of the recorded run",
            "input_phase_steps": PHASE_STEPS,
        })
    }

    /// The full benchmark section of the host report.
    pub fn report(&self) -> serde_json::Value {
        serde_json::json!({
            "interaction": self.trace(),
            "slowest_frame_ms": {
                "input_to_visible": self.slowest.input_to_visible(),
                "stages": self.slowest.stages(),
                "sample_age": self.slowest.sample_age,
                "authored_edit": self.slowest.authored_edit,
            },
            "scanout_bound_ms": self.scanout_bound(),
            "statistics_ms": {
                "median": self.median_ms,
                "p95": self.p95_ms,
                "p99": self.p99_ms,
                "minimum": self.extreme(true),
                "maximum": self.extreme(false),
            },
            "configuration": {
                "id": "interactive-4k",
                "texture_extent": self.extent,
                "texture_set": self.texture_set,
                "mesh_triangles": self.mesh_triangles,
                "layers": self.layers,
                "channels": self.channels,
            },
            "stage_medians_ms": self.stage_medians(),
            "recorded_frames": self.frames.len(),
            "skipped_frames": self.skipped,
            "observed_input_rate_hz": self.observed_input_rate_hz,
            "authored_texels_per_frame": self.median.authored_texels,
            "median_frame_ms": {
                "authored_edit": self.median.authored_edit,
                "sample_age": self.median.sample_age,
            },
            "surface": {
                "adapter": self.adapter,
                "backend": self.backend,
                "format": self.surface_format,
                "present_mode": self.present_mode,
                "width": self.surface_size.0,
                "height": self.surface_size.1,
            },
            "method": {
                "edit": EDIT_NOTE,
                "stage_boundaries": STAGE_NOTE,
                "presentation_boundary": PRESENTATION_NOTE,
                "pipelining": PIPELINING_NOTE,
            },
        })
    }

    /// The measured figures plus one display interval.
    ///
    /// Derived, not measured, and labelled as such. A frame handed to the
    /// compositor becomes visible at the next refresh, so this is the longest
    /// the unobservable remainder can be. It is reported because a frame whose
    /// measured latency is shorter than a refresh interval certainly was not on
    /// the display that early, and the budget verdict should survive the
    /// pessimistic reading as well as the measured one.
    fn scanout_bound(&self) -> serde_json::Value {
        let interval = if self.refresh_rate_hz > 0.0 {
            1000.0 / self.refresh_rate_hz
        } else {
            0.0
        };
        serde_json::json!({
            "derivation": "measured latency plus one refresh interval",
            "refresh_interval_ms": interval,
            "median": self.median_ms + interval,
            "p95": self.p95_ms + interval,
            "p99": self.p99_ms + interval,
        })
    }

    /// The median of each stage across the run.
    ///
    /// Informational only: these do not sum to any frame that ran, which is
    /// why the emitted trace carries one frame's stages instead.
    fn stage_medians(&self) -> serde_json::Value {
        let median = |pick: fn(&Frame) -> f64| {
            let mut values: Vec<f64> = self.frames.iter().map(pick).collect();
            values.sort_by(f64::total_cmp);
            order_statistic(&values, 0.5)
        };
        serde_json::json!({
            "input": median(|frame| frame.input),
            "queue": median(|frame| frame.queue),
            "upload": median(|frame| frame.upload),
            "execution": median(|frame| frame.execution),
            "presentation": median(|frame| frame.presentation),
            "authored_edit": median(|frame| frame.authored_edit),
            "sample_age": median(|frame| frame.sample_age),
        })
    }

    /// The fastest or slowest recorded frame. A run only reaches this with at
    /// least one frame, so the neutral start value is never what is reported.
    fn extreme(&self, minimum: bool) -> f64 {
        let latencies = self.frames.iter().map(Frame::input_to_visible);
        if minimum {
            latencies.fold(f64::INFINITY, f64::min)
        } else {
            latencies.fold(f64::NEG_INFINITY, f64::max)
        }
    }
}

/// How the run is shaped.
#[derive(Clone, Copy, Debug)]
pub struct Plan {
    pub frames: usize,
    pub extent: u32,
    pub mesh_triangles: usize,
    pub layers: usize,
}

/// Run the windowed benchmark and report what it measured.
pub fn measure(plan: Plan, channels: &[&str]) -> Result<Outcome, String> {
    let event_loop = match EventLoop::new() {
        Ok(value) => value,
        Err(error) => {
            return Ok(Outcome::Unavailable(format!(
                "no display is available for a presenting surface: {error}"
            )))
        }
    };
    event_loop.set_control_flow(ControlFlow::Poll);
    let mesh = MeshData::grid(plan.mesh_triangles);
    let canvas = Canvas::build(&mesh, plan.extent, channels, plan.layers, 512 << 20)?;
    let mut session = Benchmark::new(plan, canvas, mesh.triangle_count(), channels.len());
    event_loop
        .run_app(&mut session)
        .map_err(|error| format!("the window event loop failed: {error}"))?;
    session.finish()
}

/// The device, surface and pipeline one window owns.
struct Surface {
    window: Arc<Window>,
    surface: wgpu::Surface<'static>,
    configuration: wgpu::SurfaceConfiguration,
    gpu: crate::Device,
    pipeline: wgpu::RenderPipeline,
    bindings: wgpu::BindGroup,
    texture: wgpu::Texture,
}

struct Benchmark {
    plan: Plan,
    canvas: Canvas,
    mesh_triangles: usize,
    channels: usize,
    surface: Option<Surface>,
    /// The drawable acquired after the previous frame was presented.
    pending: Option<wgpu::SurfaceTexture>,
    /// The input clock's origin. Stepped through the display interval so the
    /// recorded distribution covers every input-to-vsync phase.
    origin: Option<Instant>,
    started: Option<Instant>,
    next_sample: u64,
    warmup: usize,
    frames: Vec<Frame>,
    skipped: usize,
    retained_bytes: usize,
    authored_samples: u64,
    /// Whether the compositor hid the window where it mattered.
    occluded: bool,
    hidden_streak: usize,
    failure: Option<String>,
    unavailable: Option<String>,
}

impl Benchmark {
    fn new(plan: Plan, canvas: Canvas, mesh_triangles: usize, channels: usize) -> Self {
        Self {
            plan,
            canvas,
            mesh_triangles,
            channels,
            surface: None,
            pending: None,
            origin: None,
            started: None,
            next_sample: 0,
            warmup: WARMUP_FRAMES,
            frames: Vec::new(),
            skipped: 0,
            retained_bytes: 0,
            authored_samples: 0,
            occluded: false,
            hidden_streak: 0,
            failure: None,
            unavailable: None,
        }
    }

    fn finish(mut self) -> Result<Outcome, String> {
        // A held drawable must be released while its surface is still alive.
        drop(self.pending.take());
        if let Some(message) = self.failure {
            return Err(message);
        }
        if let Some(message) = self.unavailable {
            return Ok(Outcome::Unavailable(message));
        }
        let surface = self
            .surface
            .ok_or_else(|| "the window never produced a surface".to_string())?;
        if self.occluded {
            return Ok(Outcome::Unavailable(
                "the window was hidden during the run; a hidden window is throttled by the \
                 compositor, and throttled frames are not an interaction measurement"
                    .to_string(),
            ));
        }
        if self.frames.is_empty() || self.frames.len() < self.plan.frames {
            return Ok(Outcome::Unavailable(format!(
                "the window produced {} of the {} frames the run asked for",
                self.frames.len(),
                self.plan.frames
            )));
        }
        let mut ordered: Vec<f64> = self.frames.iter().map(Frame::input_to_visible).collect();
        ordered.sort_by(f64::total_cmp);
        let median_ms = order_statistic(&ordered, 0.5);
        let median = self
            .frames
            .iter()
            .find(|frame| frame.input_to_visible() == median_ms)
            .copied()
            .unwrap_or_default();
        let elapsed = self
            .started
            .map_or(0.0, |started| started.elapsed().as_secs_f64());
        let texture_set_identifier = self.canvas.texture_set_identifier();
        let slowest = self
            .frames
            .iter()
            .copied()
            .reduce(|a, b| {
                if b.input_to_visible() > a.input_to_visible() {
                    b
                } else {
                    a
                }
            })
            .unwrap_or_default();
        Ok(Outcome::Measured(Box::new(Measured {
            median,
            slowest,
            median_ms,
            p95_ms: order_statistic(&ordered, 0.95),
            p99_ms: order_statistic(&ordered, 0.99),
            frames: self.frames,
            skipped: self.skipped,
            refresh_rate_hz: refresh_rate(&surface.window),
            observed_input_rate_hz: if elapsed > 0.0 {
                self.authored_samples as f64 / elapsed
            } else {
                0.0
            },
            mesh_triangles: self.mesh_triangles,
            layers: self.plan.layers,
            channels: self.channels,
            extent: self.plan.extent,
            retained_bytes: self.retained_bytes,
            adapter: surface.gpu.adapter.clone(),
            backend: surface.gpu.backend.clone(),
            surface_format: format!("{:?}", surface.configuration.format),
            surface_size: (surface.configuration.width, surface.configuration.height),
            present_mode: format!("{:?}", surface.configuration.present_mode),
            texture_set: texture_set_identifier,
        })))
    }

    /// Wait until the next synthetic sample is due and report when it arrived.
    ///
    /// A sample that arrived while the previous frame was still being
    /// presented is already late; its age belongs to this frame's latency, so
    /// the returned instant is when the sample was due, not when it was read.
    /// Samples that arrived during a longer frame are coalesced to the newest,
    /// which is what an application does with a pointer queue.
    fn await_sample(&mut self, origin: Instant) -> Instant {
        let period = Duration::from_secs_f64(1.0 / INPUT_RATE_HZ);
        let mut due = origin + period.mul_f64(self.next_sample as f64);
        let now = Instant::now();
        if now >= due {
            let elapsed = now.duration_since(origin).as_secs_f64();
            self.next_sample = (elapsed * INPUT_RATE_HZ) as u64;
            due = origin + period.mul_f64(self.next_sample as f64);
        } else {
            spin_until(due);
        }
        self.next_sample += 1;
        self.authored_samples += 1;
        due
    }

    /// A pointer stroke that wanders the canvas rather than repainting a texel.
    fn sample(&self, ordinal: u64) -> Sample {
        let phase = ordinal as f64 * 0.031;
        let reach = f64::from(self.plan.extent) * 0.36;
        let centre = f64::from(self.plan.extent) * 0.5;
        let radius = f64::from(STAMP_RADIUS);
        let last = f64::from(self.plan.extent - 1) - radius;
        Sample {
            ordinal,
            x: (centre + reach * (phase * 1.7).sin()).clamp(radius, last) as u32,
            y: (centre + reach * (phase * 1.1 + 0.7).sin()).clamp(radius, last) as u32,
            radius: STAMP_RADIUS,
            // The three components cycle on periods coprime with each other
            // and with the path's, and the offsets keep them apart, so no
            // sample ever repaints the value a texel already holds — including
            // the channel's uniform default. An edit that changed nothing
            // would publish nothing, and there would be no frame to measure.
            colour: [
                (ordinal % 251) as u8,
                (ordinal % 241) as u8 ^ 0x5A,
                (ordinal % 239) as u8 ^ 0xA5,
            ],
        }
    }

    /// One measured frame, or `None` when the surface asked to skip it.
    ///
    /// The drawable is acquired before the sample exists and the next one is
    /// acquired straight after presenting, so the loop is paced by the display
    /// rather than by its own idling. That matters for honesty: with a
    /// double-buffered surface, an application that idles between frames always
    /// finds a free drawable and never observes the queue its frame is sitting
    /// in. Acquiring immediately after presenting blocks until the display has
    /// taken a frame off that queue, which is the closest thing to "the frame
    /// went out" that the API exposes.
    fn render(&mut self) -> Result<Option<Frame>, String> {
        let Some(target) = self.held()? else {
            return Ok(None);
        };
        let origin = *self.origin.get_or_insert_with(Instant::now);
        self.started.get_or_insert(origin);
        let due = self.await_sample(origin);
        // The stroke advances once per authored stamp. The input ordinal may
        // step back when the clock's phase moves, and a stamp that repeated an
        // earlier one would repaint texels that already hold its colour and
        // publish nothing.
        let ordinal = self.authored_samples;
        let taken = Instant::now();
        let stamp = self.canvas.stamp(self.sample(ordinal))?;
        self.retained_bytes = stamp.retained_bytes;
        let after_input = Instant::now();
        let published = self.publish(&stamp)?;
        let after_upload = Instant::now();
        let commands = self.encode(&target)?;
        let after_queue = Instant::now();
        self.execute(commands)?;
        let after_execution = Instant::now();
        self.present(target)?;
        self.pending = self.acquire()?;
        let after_slot = Instant::now();
        Ok(Some(Frame {
            input: since(due, after_input),
            upload: since(after_input, after_upload),
            queue: since(after_upload, after_queue),
            execution: since(after_queue, after_execution),
            presentation: since(after_execution, after_slot),
            upload_bytes: published.0,
            uploaded_tiles: published.1,
            declared_tiles: stamp.declared_tiles.len(),
            committed_tiles: stamp.committed_tiles,
            authored_texels: stamp.texels,
            authored_edit: since(taken, after_input),
            sample_age: since(due, taken),
        }))
    }

    /// The drawable this frame renders into, acquiring one if none is held.
    fn held(&mut self) -> Result<Option<wgpu::SurfaceTexture>, String> {
        if let Some(target) = self.pending.take() {
            return Ok(Some(target));
        }
        self.acquire()
    }

    /// Publish the changed tiles into the device texture and wait for the copy.
    ///
    /// Nothing is mapped and nothing is read back: the host uploads the texels
    /// it authored, which is why the residency figure it reports is zero bytes
    /// of synchronous readback rather than a small one.
    fn publish(&self, stamp: &Stamp) -> Result<(u64, usize), String> {
        let surface = self.require()?;
        let mut bytes = 0_u64;
        let mut tiles = 0_usize;
        for tile in &stamp.changed_tiles {
            let Some(payload) = self.canvas.tile_payload(*tile) else {
                continue;
            };
            surface.gpu.queue.write_texture(
                wgpu::TexelCopyTextureInfo {
                    texture: &surface.texture,
                    mip_level: 0,
                    origin: wgpu::Origin3d {
                        x: tile.0 * TILE_EXTENT,
                        y: tile.1 * TILE_EXTENT,
                        z: 0,
                    },
                    aspect: wgpu::TextureAspect::All,
                },
                payload,
                wgpu::TexelCopyBufferLayout {
                    offset: 0,
                    bytes_per_row: Some(TILE_EXTENT * DEVICE_TEXEL_BYTES as u32),
                    rows_per_image: Some(TILE_EXTENT),
                },
                wgpu::Extent3d {
                    width: TILE_EXTENT,
                    height: TILE_EXTENT,
                    depth_or_array_layers: 1,
                },
            );
            bytes += payload.len() as u64;
            tiles += 1;
        }
        let index = surface.gpu.queue.submit(std::iter::empty());
        wait(&surface.gpu.device, index)?;
        Ok((bytes, tiles))
    }

    fn acquire(&mut self) -> Result<Option<wgpu::SurfaceTexture>, String> {
        let surface = self.require()?;
        match surface.surface.get_current_texture() {
            wgpu::CurrentSurfaceTexture::Success(texture)
            | wgpu::CurrentSurfaceTexture::Suboptimal(texture) => {
                self.hidden_streak = 0;
                Ok(Some(texture))
            }
            wgpu::CurrentSurfaceTexture::Timeout => Ok(None),
            // A hidden window is a throttled window: the compositor stops
            // scheduling it and the frames that do get through are not what a
            // user would have seen. Before the first recorded frame this is
            // just the window not being on screen yet, which warm-up exists to
            // absorb; after it, or once it has persisted, the run is not a
            // measurement and says so.
            wgpu::CurrentSurfaceTexture::Occluded => {
                self.hidden_streak += 1;
                self.occluded |= self.warmup == 0 || self.hidden_streak > HIDDEN_ACQUISITION_LIMIT;
                Ok(None)
            }
            wgpu::CurrentSurfaceTexture::Outdated | wgpu::CurrentSurfaceTexture::Lost => {
                self.reconfigure();
                Ok(None)
            }
            wgpu::CurrentSurfaceTexture::Validation => {
                Err("the surface rejected an acquisition".to_string())
            }
        }
    }

    fn encode(&self, target: &wgpu::SurfaceTexture) -> Result<wgpu::CommandBuffer, String> {
        let surface = self.require()?;
        let view = target
            .texture
            .create_view(&wgpu::TextureViewDescriptor::default());
        let mut encoder =
            surface
                .gpu
                .device
                .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("cybertexel-interaction-frame"),
                });
        {
            let mut pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("cybertexel-interaction-present"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view,
                    depth_slice: None,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color::BLACK),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
                ..Default::default()
            });
            pass.set_pipeline(&surface.pipeline);
            pass.set_bind_group(0, &surface.bindings, &[]);
            pass.draw(0..3, 0..1);
        }
        Ok(encoder.finish())
    }

    fn execute(&self, commands: wgpu::CommandBuffer) -> Result<(), String> {
        let surface = self.require()?;
        let index = surface.gpu.queue.submit(Some(commands));
        wait(&surface.gpu.device, index)
    }

    fn present(&self, target: wgpu::SurfaceTexture) -> Result<(), String> {
        let surface = self.require()?;
        surface.window.pre_present_notify();
        surface.gpu.queue.present(target);
        Ok(())
    }

    fn require(&self) -> Result<&Surface, String> {
        self.surface
            .as_ref()
            .ok_or_else(|| "the benchmark has no configured surface".to_string())
    }

    fn reconfigure(&mut self) {
        if let Some(surface) = &mut self.surface {
            let size = surface.window.inner_size();
            surface.configuration.width = size.width.max(1);
            surface.configuration.height = size.height.max(1);
            surface
                .surface
                .configure(&surface.gpu.device, &surface.configuration);
        }
    }

    fn start(&mut self, event_loop: &ActiveEventLoop) -> Result<Option<Surface>, String> {
        // The window stays above the others for the length of the run: a
        // window the compositor may hide is a window it may throttle, and a
        // throttled window measures the compositor rather than the host.
        let attributes = Window::default_attributes()
            .with_title("CyberTexel desktop reference host")
            .with_inner_size(LogicalSize::new(1280.0, 800.0))
            .with_window_level(WindowLevel::AlwaysOnTop);
        let window = match event_loop.create_window(attributes) {
            Ok(window) => Arc::new(window),
            Err(error) => {
                self.unavailable = Some(format!("no window could be opened: {error}"));
                return Ok(None);
            }
        };
        // A window the compositor is free to hide is a window it is free to
        // throttle, so the benchmark asks for the front before it measures.
        window.focus_window();
        let surface = build_surface(window, self.plan.extent)?;
        if surface.is_none() {
            self.unavailable = Some("no adapter can present to this window's surface".to_string());
        }
        Ok(surface)
    }
}

impl ApplicationHandler for Benchmark {
    fn resumed(&mut self, event_loop: &ActiveEventLoop) {
        if self.surface.is_some() || self.failure.is_some() || self.unavailable.is_some() {
            return;
        }
        match self.start(event_loop) {
            Ok(Some(surface)) => self.surface = Some(surface),
            Ok(None) => event_loop.exit(),
            Err(error) => {
                self.failure = Some(error);
                event_loop.exit();
            }
        }
    }

    fn window_event(&mut self, event_loop: &ActiveEventLoop, _: WindowId, event: WindowEvent) {
        match event {
            WindowEvent::CloseRequested => event_loop.exit(),
            WindowEvent::Resized(_) => self.reconfigure(),
            WindowEvent::Occluded(true) => self.occluded = true,
            WindowEvent::RedrawRequested => self.step(event_loop),
            _ => {}
        }
    }

    fn about_to_wait(&mut self, _: &ActiveEventLoop) {
        if let Some(surface) = &self.surface {
            surface.window.request_redraw();
        }
    }
}

impl Benchmark {
    fn step(&mut self, event_loop: &ActiveEventLoop) {
        if self.frames.len() >= self.plan.frames {
            event_loop.exit();
            return;
        }
        if self.occluded {
            // The run is already unmeasurable; stop rather than painting into
            // a window nobody can see.
            event_loop.exit();
            return;
        }

        match self.render() {
            Ok(Some(_)) if self.warmup > 0 => self.warmup -= 1,
            Ok(Some(frame)) => self.frames.push(frame),
            Ok(None) => self.skipped += 1,
            Err(error) => {
                self.failure = Some(error);
                event_loop.exit();
                return;
            }
        }
        self.step_phase();
        let expired = self
            .started
            .is_some_and(|started| started.elapsed() > DEADLINE);
        if self.frames.len() >= self.plan.frames || expired {
            event_loop.exit();
        }
    }
}

impl Benchmark {
    /// Advance the input clock by a fraction of the display interval once a
    /// segment of the run has been recorded.
    fn step_phase(&mut self) {
        let segment = self.plan.frames / PHASE_STEPS;
        if segment == 0 || self.frames.is_empty() || !self.frames.len().is_multiple_of(segment) {
            return;
        }
        let interval = Duration::from_secs_f64(1.0 / INPUT_RATE_HZ.max(1.0));
        if let Some(origin) = &mut self.origin {
            *origin += interval / PHASE_STEPS as u32;
        }
    }
}

/// Milliseconds between two instants of the same frame, in order.
fn since(from: Instant, to: Instant) -> f64 {
    to.saturating_duration_since(from).as_secs_f64() * 1000.0
}

/// The nearest-rank order statistic, so every reported figure is a frame that
/// actually ran rather than an interpolation between two that did.
fn order_statistic(sorted: &[f64], fraction: f64) -> f64 {
    if sorted.is_empty() {
        return f64::NAN;
    }
    let rank = (fraction * sorted.len() as f64).ceil().max(1.0) as usize;
    sorted[rank.min(sorted.len()) - 1]
}

fn spin_until(instant: Instant) {
    while Instant::now() < instant {
        let remaining = instant.saturating_duration_since(Instant::now());
        if remaining > Duration::from_millis(2) {
            std::thread::sleep(remaining - Duration::from_millis(1));
        } else {
            std::hint::spin_loop();
        }
    }
}

fn wait(device: &wgpu::Device, index: wgpu::SubmissionIndex) -> Result<(), String> {
    device
        .poll(wgpu::PollType::Wait {
            submission_index: Some(index),
            timeout: None,
        })
        .map(|_| ())
        .map_err(|error| format!("waiting for the device failed: {error}"))
}

fn refresh_rate(window: &Window) -> f64 {
    window
        .current_monitor()
        .and_then(|monitor| monitor.refresh_rate_millihertz())
        .map_or(0.0, |millihertz| f64::from(millihertz) / 1000.0)
}

/// Create the device that can present to this window and the resident texture.
fn build_surface(window: Arc<Window>, extent: u32) -> Result<Option<Surface>, String> {
    let instance = wgpu::Instance::default();
    let surface = instance
        .create_surface(window.clone())
        .map_err(|error| format!("the window carries no usable surface: {error}"))?;
    let Some((adapter, gpu)) = crate::acquire_presenting_device(&instance, &surface)? else {
        return Ok(None);
    };
    if gpu.device.limits().max_texture_dimension_2d < extent {
        return Err(format!(
            "the device caps 2D textures at {}, below the {extent}-square configuration",
            gpu.device.limits().max_texture_dimension_2d
        ));
    }
    let size = window.inner_size();
    let mut configuration = surface
        .get_default_config(&adapter, size.width.max(1), size.height.max(1))
        .ok_or_else(|| "the adapter cannot present to this surface".to_string())?;
    configuration.present_mode = present_mode(&surface, &adapter);
    configuration.desired_maximum_frame_latency = 1;
    surface.configure(&gpu.device, &configuration);
    let texture = authored_texture(&gpu.device, extent);
    let (pipeline, bindings) = present_pipeline(&gpu.device, &texture, configuration.format);
    Ok(Some(Surface {
        window,
        surface,
        configuration,
        gpu,
        pipeline,
        bindings,
        texture,
    }))
}

/// Present in lockstep with the display when it is offered: the measurement is
/// of what the user sees, and a tear-free path is what a paint application
/// ships.
fn present_mode(surface: &wgpu::Surface<'static>, adapter: &wgpu::Adapter) -> wgpu::PresentMode {
    let modes = surface.get_capabilities(adapter).present_modes;
    if modes.contains(&wgpu::PresentMode::Fifo) {
        wgpu::PresentMode::Fifo
    } else {
        modes
            .first()
            .copied()
            .unwrap_or(wgpu::PresentMode::AutoVsync)
    }
}

fn authored_texture(device: &wgpu::Device, extent: u32) -> wgpu::Texture {
    device.create_texture(&wgpu::TextureDescriptor {
        label: Some("cybertexel-authored-canvas"),
        size: wgpu::Extent3d {
            width: extent,
            height: extent,
            depth_or_array_layers: 1,
        },
        mip_level_count: 1,
        sample_count: 1,
        dimension: wgpu::TextureDimension::D2,
        format: wgpu::TextureFormat::Rgba8Unorm,
        usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
        view_formats: &[],
    })
}

fn present_pipeline(
    device: &wgpu::Device,
    texture: &wgpu::Texture,
    format: wgpu::TextureFormat,
) -> (wgpu::RenderPipeline, wgpu::BindGroup) {
    let module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
        label: Some("cybertexel-present"),
        source: wgpu::ShaderSource::Wgsl(SHADER.into()),
    });
    let layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
        label: Some("cybertexel-present-bindings"),
        entries: &[
            wgpu::BindGroupLayoutEntry {
                binding: 0,
                visibility: wgpu::ShaderStages::FRAGMENT,
                ty: wgpu::BindingType::Texture {
                    sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    view_dimension: wgpu::TextureViewDimension::D2,
                    multisampled: false,
                },
                count: None,
            },
            wgpu::BindGroupLayoutEntry {
                binding: 1,
                visibility: wgpu::ShaderStages::FRAGMENT,
                ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                count: None,
            },
        ],
    });
    let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
        label: Some("cybertexel-present-sampler"),
        mag_filter: wgpu::FilterMode::Linear,
        min_filter: wgpu::FilterMode::Linear,
        ..Default::default()
    });
    let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
    let bindings = device.create_bind_group(&wgpu::BindGroupDescriptor {
        label: Some("cybertexel-present-bindings"),
        layout: &layout,
        entries: &[
            wgpu::BindGroupEntry {
                binding: 0,
                resource: wgpu::BindingResource::TextureView(&view),
            },
            wgpu::BindGroupEntry {
                binding: 1,
                resource: wgpu::BindingResource::Sampler(&sampler),
            },
        ],
    });
    let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
        label: Some("cybertexel-present-layout"),
        bind_group_layouts: &[Some(&layout)],
        ..Default::default()
    });
    let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
        label: Some("cybertexel-present"),
        layout: Some(&pipeline_layout),
        vertex: wgpu::VertexState {
            module: &module,
            entry_point: Some("vertex_main"),
            compilation_options: Default::default(),
            buffers: &[],
        },
        fragment: Some(wgpu::FragmentState {
            module: &module,
            entry_point: Some("fragment_main"),
            compilation_options: Default::default(),
            targets: &[Some(wgpu::ColorTargetState {
                format,
                blend: None,
                write_mask: wgpu::ColorWrites::ALL,
            })],
        }),
        primitive: wgpu::PrimitiveState::default(),
        depth_stencil: None,
        multisample: wgpu::MultisampleState::default(),
        multiview_mask: None,
        cache: None,
    });
    (pipeline, bindings)
}
