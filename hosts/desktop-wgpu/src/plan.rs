//! The subset of a CyberTexel pass plan this reference host consumes.
//!
//! Deserialization is deliberately strict: an unknown field fails rather than
//! being ignored, so a change to the emitted plan breaks this host and CI
//! rather than being silently skipped.

use serde::Deserialize;

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct PassPlan {
    pub stable_identity: String,
    pub resources: Vec<Resource>,
    pub passes: Vec<Pass>,
    #[allow(dead_code)]
    pub lifetimes: Vec<serde_json::Value>,
}

#[derive(Debug, Deserialize)]
pub struct Resource {
    pub version: ResourceVersion,
    pub format: String,
    pub extent: Extent,
    #[allow(dead_code)]
    pub role: String,
    #[allow(dead_code)]
    pub mip_levels: u32,
    #[allow(dead_code)]
    pub externally_initialized: bool,
    pub tile_shape: TileShape,
}

#[derive(Clone, Debug, Deserialize)]
pub struct ResourceVersion {
    pub logical_id: String,
    pub generation: u64,
}

#[derive(Clone, Copy, Debug, Deserialize)]
pub struct Extent {
    pub width: u32,
    pub height: u32,
    pub layers: u32,
}

#[derive(Clone, Copy, Debug, Deserialize)]
pub struct TileShape {
    pub width: u32,
    pub height: u32,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Pass {
    pub identifier: String,
    pub kind: String,
    pub entry_points: EntryPoints,
    pub render_targets: Vec<RenderTarget>,
    pub command: Command,
    #[serde(default)]
    pub depth_target: Option<serde_json::Value>,
    #[allow(dead_code)]
    pub accesses: Vec<serde_json::Value>,
    #[allow(dead_code)]
    pub dependencies: Vec<serde_json::Value>,
    #[allow(dead_code)]
    pub depth_state: serde_json::Value,
    pub vertex_buffers: Vec<VertexBuffer>,
    pub texture_bindings: Vec<serde_json::Value>,
    pub sampler_bindings: Vec<serde_json::Value>,
    pub uniform_blocks: Vec<serde_json::Value>,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct VertexBuffer {
    pub slot: u32,
    pub stride: u64,
    pub step_mode: String,
    pub attributes: Vec<VertexAttribute>,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct VertexAttribute {
    pub location: u32,
    pub offset: u64,
    pub format: String,
    pub semantic: String,
}

impl VertexBuffer {
    pub fn step_mode(&self) -> Result<wgpu::VertexStepMode, String> {
        match self.step_mode.as_str() {
            "vertex" => Ok(wgpu::VertexStepMode::Vertex),
            "instance" => Ok(wgpu::VertexStepMode::Instance),
            other => Err(format!(
                "pass plan names an unknown vertex step mode: {other}"
            )),
        }
    }
}

impl VertexAttribute {
    pub fn format(&self) -> Result<wgpu::VertexFormat, String> {
        match self.format.as_str() {
            "float32" => Ok(wgpu::VertexFormat::Float32),
            "float32x2" => Ok(wgpu::VertexFormat::Float32x2),
            "float32x3" => Ok(wgpu::VertexFormat::Float32x3),
            "float32x4" => Ok(wgpu::VertexFormat::Float32x4),
            other => Err(format!(
                "pass plan names a vertex format this host does not carry: {other}"
            )),
        }
    }
}

#[derive(Debug, Deserialize)]
pub struct EntryPoints {
    pub vertex: String,
    pub fragment: String,
    #[allow(dead_code)]
    pub compute: String,
}

#[derive(Debug, Deserialize)]
pub struct RenderTarget {
    pub resource: ResourceVersion,
    pub format: String,
    pub width: u32,
    pub height: u32,
    pub load: String,
    pub clear_colour: [f64; 4],
    #[allow(dead_code)]
    pub store: String,
    #[allow(dead_code)]
    pub role: String,
    #[allow(dead_code)]
    pub blend: serde_json::Value,
    #[allow(dead_code)]
    pub subresources: serde_json::Value,
}

#[derive(Debug, Deserialize)]
pub struct Command {
    pub kind: String,
    pub topology: String,
    pub vertex_count: u32,
    pub instance_count: u32,
    pub first_vertex: u32,
    pub first_instance: u32,
    pub indexed: bool,
    #[allow(dead_code)]
    pub index_count: u32,
    #[allow(dead_code)]
    pub first_index: u32,
    #[allow(dead_code)]
    pub base_vertex: i64,
}

impl PassPlan {
    /// Translate a plan format name into the wgpu format that carries it.
    pub fn texture_format(name: &str) -> Result<wgpu::TextureFormat, String> {
        match name {
            "rgba8_unorm" => Ok(wgpu::TextureFormat::Rgba8Unorm),
            "rgba8_unorm_srgb" => Ok(wgpu::TextureFormat::Rgba8UnormSrgb),
            "rgba16_float" => Ok(wgpu::TextureFormat::Rgba16Float),
            "rgba32_float" => Ok(wgpu::TextureFormat::Rgba32Float),
            "r8_unorm" => Ok(wgpu::TextureFormat::R8Unorm),
            "r16_float" => Ok(wgpu::TextureFormat::R16Float),
            "r32_float" => Ok(wgpu::TextureFormat::R32Float),
            other => Err(format!(
                "pass plan names a texture format this host does not carry: {other}"
            )),
        }
    }

    pub fn topology(name: &str) -> Result<wgpu::PrimitiveTopology, String> {
        match name {
            "triangle_list" => Ok(wgpu::PrimitiveTopology::TriangleList),
            "triangle_strip" => Ok(wgpu::PrimitiveTopology::TriangleStrip),
            "line_list" => Ok(wgpu::PrimitiveTopology::LineList),
            "point_list" => Ok(wgpu::PrimitiveTopology::PointList),
            other => Err(format!(
                "pass plan names a topology this host does not carry: {other}"
            )),
        }
    }
}
