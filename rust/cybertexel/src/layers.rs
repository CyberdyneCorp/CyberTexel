//! Layer-stack value types.
//!
//! Plain owned Rust values. The audited FFI boundary converts them into native
//! descriptors; nothing here reaches the C ABI directly.
/// One channel's participation in a layer entry.
#[derive(Clone, Debug, PartialEq)]
pub struct LayerChannel {
    pub semantic_id: String,
    pub enabled: bool,
    pub opacity: f64,
}

/// The explicit kind a layer-stack entry declares.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum LayerKind {
    Paint = 0,
    Fill = 1,
    Group = 2,
    Mask = 3,
    Filter = 4,
    Instance = 5,
    EditableDecal = 6,
    EditableText = 7,
    SurfacePath = 8,
}

/// An ordered layer-stack entry.
#[derive(Clone, Debug, PartialEq)]
pub struct LayerEntry {
    pub identifier: String,
    pub display_name: String,
    pub kind: LayerKind,
    pub parent_identifier: Option<String>,
    pub target_identifier: Option<String>,
    pub source_identifier: Option<String>,
    pub enabled: bool,
    pub opacity: f64,
    pub blend_mode: String,
    pub channels: Vec<LayerChannel>,
}

impl LayerEntry {
    /// A visible paint entry at full opacity with normal blending.
    pub fn paint(identifier: &str, display_name: &str) -> Self {
        Self {
            identifier: identifier.to_string(),
            display_name: display_name.to_string(),
            kind: LayerKind::Paint,
            parent_identifier: None,
            target_identifier: None,
            source_identifier: None,
            enabled: true,
            opacity: 1.0,
            blend_mode: "normal".to_string(),
            channels: Vec::new(),
        }
    }
}
