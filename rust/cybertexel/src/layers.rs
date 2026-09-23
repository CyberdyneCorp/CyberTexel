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

/// What one undo or redo exchanged.
///
/// `texture-document` performs undo by exchanging exact storage owners rather
/// than copying pixels, so `copied_pixel_bytes` is the evidence that a restore
/// moved no pixel data.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct HistoryRestore {
    pub tile_count: usize,
    pub exchanged_storage_count: usize,
    pub copied_pixel_bytes: usize,
    pub layer_stack_exchanged: bool,
}

/// Tile-history occupancy against its declared ceiling.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct HistoryBudget {
    pub budget_bytes: usize,
    pub retained_bytes: usize,
    pub available_bytes: usize,
    pub undo_steps: usize,
    pub redo_steps: usize,
}

/// One channel/tile target a history step declares before editing.
///
/// `texture-document` requires the write set to be declared up front, so a step
/// retains exactly the tiles it changed and nothing else.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct HistoryTarget {
    pub semantic_id: String,
    pub tile_x: u32,
    pub tile_y: u32,
}

/// What committing a history step retained.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct HistoryCommit {
    pub committed: bool,
    pub tile_count: usize,
    pub retained_bytes: usize,
    pub layer_stack_changed: bool,
}
