//! Raw declarations for the stable CyberTexel C ABI.

#![allow(non_camel_case_types)]

use std::ffi::{c_char, c_void};

pub type ctex_result = u32;
pub const CTEX_RESULT_SUCCESS: ctex_result = 0;
pub const CTEX_RESULT_INVALID_ARGUMENT: ctex_result = 1;
pub const CTEX_RESULT_MISSING_RESOURCE: ctex_result = 2;
pub const CTEX_RESULT_UNSUPPORTED_OPERATION: ctex_result = 3;
pub const CTEX_RESULT_OUT_OF_MEMORY: ctex_result = 4;
pub const CTEX_RESULT_OVER_BUDGET: ctex_result = 5;
pub const CTEX_RESULT_CANCELLED: ctex_result = 6;
pub const CTEX_RESULT_INTERNAL_ERROR: ctex_result = 7;
pub const CTEX_RESULT_BUFFER_TOO_SMALL: ctex_result = 8;
pub const CTEX_RESULT_NO_UNDO: ctex_result = 9;
pub const CTEX_RESULT_NO_REDO: ctex_result = 10;
pub const CTEX_RESULT_STALE_STATE: ctex_result = 11;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctex_version {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
    pub string: *const c_char,
}

#[repr(C)]
pub struct ctex_document {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ctex_texture_set_descriptor {
    pub size: u32,
    pub display_name: *const c_char,
    pub partition_kind: u32,
    pub partition_key: *const c_char,
    pub uv_set: *const c_char,
    pub width: u32,
    pub height: u32,
    pub default_bit_depth: u8,
    pub udim_tiling: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_paint_preview_info {
    pub size: u32,
    pub state: u32,
    pub width: u32,
    pub height: u32,
    pub component_count: u32,
    pub scalar_representation: u32,
    pub bit_depth: u32,
    pub pixel_byte_count: usize,
    pub resolved_dilation_radius: u32,
    pub dilation_radius_clamped: u32,
    pub dilated_texel_count: usize,
    pub zero_gradient_texel_count: usize,
    pub baseline_epoch: u64,
    pub baseline_revision: u64,
    pub preview_epoch: u64,
    pub preview_revision: u64,
    pub committed_epoch: u64,
    pub committed_revision: u64,
    pub changed_tile_count: usize,
    pub maximum_component_error: f64,
}

#[repr(C)]
pub struct ctex_paint_preview_session {
    _private: [u8; 0],
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_transport_revision_cursor {
    pub epoch: u64,
    pub revision: u64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_transport_tile_version {
    pub x: u32,
    pub y: u32,
    pub revision: u64,
    pub generation: u64,
    pub residency: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_transport_delta_info {
    pub size: u32,
    pub disposition: u32,
    pub synchronized_cursor: ctex_transport_revision_cursor,
    pub current_cursor: ctex_transport_revision_cursor,
    pub changed_tile_count: usize,
    pub indexed_tiles_visited: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_transport_snapshot_query_info {
    pub size: u32,
    pub disposition: u32,
    pub synchronized_cursor: ctex_transport_revision_cursor,
    pub current_cursor: ctex_transport_revision_cursor,
    pub changed_tile_count: usize,
    pub indexed_tiles_visited: usize,
    pub retained_bytes: usize,
    pub additional_pinned_bytes: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_transport_tile_memory_layout {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub row_pitch_bytes: usize,
    pub pixel_stride_bytes: usize,
    pub channel_order: u32,
    pub component_type: u32,
    pub component_byte_order: u32,
    pub tile_contiguity: u32,
    pub byte_size: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctex_transport_tile_readback_destination {
    pub size: u32,
    pub version: ctex_transport_tile_version,
    pub layout: ctex_transport_tile_memory_layout,
    pub output: *mut c_void,
    pub output_size: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctex_transport_host_tile_completion {
    pub size: u32,
    pub version: ctex_transport_tile_version,
    pub layout: ctex_transport_tile_memory_layout,
    pub bytes: *const c_void,
    pub byte_size: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_transport_readback_info {
    pub size: u32,
    pub status: u32,
    pub output_readable: u32,
    pub tile_count: usize,
    pub required_detail_size: usize,
}

#[repr(C)]
pub struct ctex_transport_snapshot_pool {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ctex_transport_snapshot {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ctex_transport_readback {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ctex_host_execution_session {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ctex_host_completion_result {
    _private: [u8; 0],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctex_host_resource_descriptor {
    pub size: u32,
    pub logical_id: *const c_char,
    pub generation: u64,
    pub role: *const c_char,
    pub format: u32,
    pub width: u32,
    pub height: u32,
    pub layers: u32,
    pub mip_levels: u32,
    pub tile_width: u32,
    pub tile_height: u32,
    pub externally_initialized: u32,
    pub owner: u32,
    pub required_state: u32,
    pub output: u32,
}

#[repr(C)]
pub struct ctex_host_submission_descriptor {
    pub size: u32,
    pub operation: *const c_char,
    pub base_revision: u64,
    pub resources: *const ctex_host_resource_descriptor,
    pub resource_count: usize,
    pub replay_semantics: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_host_submission_info {
    pub size: u32,
    pub completion_token: u64,
    pub base_revision: u64,
    pub resource_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctex_host_completed_resource_descriptor {
    pub size: u32,
    pub logical_id: *const c_char,
    pub generation: u64,
    pub format: u32,
    pub width: u32,
    pub height: u32,
    pub layers: u32,
}

#[repr(C)]
pub struct ctex_host_recovery_descriptor {
    pub size: u32,
    pub kind: u32,
    pub checkpoint_complete: u32,
    pub checkpoint_revision: u64,
    pub operation_record_version: *const c_char,
    pub inputs_pinned: u32,
    pub retained_bytes: usize,
}

#[repr(C)]
pub struct ctex_host_completion_descriptor {
    pub size: u32,
    pub completion_token: u64,
    pub status: u32,
    pub outputs: *const ctex_host_completed_resource_descriptor,
    pub output_count: usize,
    pub recovery: *const ctex_host_recovery_descriptor,
    pub detail: *const c_char,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_host_resource_version {
    pub logical_id_offset: usize,
    pub generation: u64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_host_completion_result_info {
    pub size: u32,
    pub disposition: u32,
    pub completion_token: u64,
    pub has_published_revision: u32,
    pub published_revision: u64,
    pub released_resource_count: usize,
    pub required_released_identity_size: usize,
    pub required_message_size: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_material_graph_info {
    pub size: u32,
    pub output_node_id: u64,
    pub node_count: usize,
    pub link_count: usize,
    pub output_channel_count: usize,
    pub canonical_size: usize,
    pub report_size: usize,
}

#[repr(C)]
pub struct ctex_shader_device_features_descriptor {
    pub size: u32,
    pub binding_budget: u32,
    pub maximum_texture_dimension: u32,
    pub supported_texture_formats: *const u32,
    pub supported_texture_format_count: usize,
    pub floating_point_filtering: u32,
    pub compute_available: u32,
}

#[repr(C)]
pub struct ctex_shader_texture_descriptor {
    pub size: u32,
    pub logical_id: *const c_char,
    pub generation: u64,
    pub role: *const c_char,
    pub format: u32,
    pub width: u32,
    pub height: u32,
    pub layers: u32,
    pub mip_levels: u32,
    pub tile_width: u32,
    pub tile_height: u32,
    pub externally_initialized: u32,
}

#[repr(C)]
pub struct ctex_shader_material_request {
    pub size: u32,
    pub stable_identity: *const c_char,
    pub target: u32,
    pub features: ctex_shader_device_features_descriptor,
    pub resources: *const c_void,
    pub resource_count: usize,
    pub output: ctex_shader_texture_descriptor,
    pub requested_filter: u32,
    pub vertex_count: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ctex_shader_material_info {
    pub size: u32,
    pub target: u32,
    pub vertex_artifact_size: usize,
    pub fragment_artifact_size: usize,
    pub pass_plan_size: usize,
    pub workaround_report_size: usize,
    pub pass_count: usize,
    pub logical_resource_count: usize,
    pub binding_count: usize,
    pub workaround_count: usize,
}

extern "C" {
    pub fn ctex_get_version() -> ctex_version;
    pub fn ctex_get_abi_version() -> ctex_version;
    pub fn ctex_get_last_diagnostic() -> *const c_char;
    pub fn ctex_get_last_diagnostic_code() -> u32;
    pub fn ctex_document_create(out_document: *mut *mut ctex_document) -> ctex_result;
    pub fn ctex_document_destroy(document: *mut ctex_document);
    pub fn ctex_document_create_texture_set(
        document: *mut ctex_document,
        descriptor: *const ctex_texture_set_descriptor,
    ) -> ctex_result;
    pub fn ctex_document_get_texture_set_ids(
        document: *const ctex_document,
        buffer: *mut c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
        out_count: *mut usize,
    ) -> ctex_result;
    pub fn ctex_texture_set_set_channel_enabled(
        document: *mut ctex_document,
        texture_set_id: *const c_char,
        semantic_id: *const c_char,
        enabled: u32,
        bit_depth: u32,
    ) -> ctex_result;
    pub fn ctex_paint_preview_session_create(
        document: *mut ctex_document,
        texture_set_id: *const c_char,
        semantic_id: *const c_char,
        out_session: *mut *mut ctex_paint_preview_session,
    ) -> ctex_result;
    pub fn ctex_paint_preview_session_destroy(session: *mut ctex_paint_preview_session);
    pub fn ctex_paint_preview_session_write_pixel(
        session: *mut ctex_paint_preview_session,
        x: u32,
        y: u32,
        pixel: *const c_void,
        pixel_size: usize,
    ) -> ctex_result;
    pub fn ctex_paint_preview_session_finalize(
        session: *mut ctex_paint_preview_session,
        coverage: *const u8,
        coverage_count: usize,
        dilation_radius: u32,
        out_info: *mut ctex_paint_preview_info,
    ) -> ctex_result;
    pub fn ctex_paint_preview_session_commit(
        session: *mut ctex_paint_preview_session,
        out_info: *mut ctex_paint_preview_info,
    ) -> ctex_result;
    pub fn ctex_host_execution_session_create(
        initial_revision: u64,
        out_session: *mut *mut ctex_host_execution_session,
    ) -> ctex_result;
    pub fn ctex_host_execution_session_destroy(session: *mut ctex_host_execution_session);
    pub fn ctex_host_execution_session_submit(
        session: *mut ctex_host_execution_session,
        descriptor: *const ctex_host_submission_descriptor,
        out_info: *mut ctex_host_submission_info,
    ) -> ctex_result;
    pub fn ctex_host_execution_session_complete(
        session: *mut ctex_host_execution_session,
        descriptor: *const ctex_host_completion_descriptor,
        out_result: *mut *mut ctex_host_completion_result,
    ) -> ctex_result;
    pub fn ctex_host_execution_session_get_committed_resource(
        session: *const ctex_host_execution_session,
        logical_id: *const c_char,
        out_found: *mut u32,
        out_generation: *mut u64,
    ) -> ctex_result;
    pub fn ctex_host_execution_session_resource_is_held(
        session: *const ctex_host_execution_session,
        logical_id: *const c_char,
        generation: u64,
        out_held: *mut u32,
    ) -> ctex_result;
    pub fn ctex_host_completion_result_destroy(result: *mut ctex_host_completion_result);
    pub fn ctex_host_completion_result_get_info(
        result: *const ctex_host_completion_result,
        out_info: *mut ctex_host_completion_result_info,
        released_resources: *mut ctex_host_resource_version,
        released_resource_capacity: usize,
        released_identities: *mut c_char,
        released_identity_size: usize,
        message: *mut c_char,
        message_size: usize,
    ) -> ctex_result;
    pub fn ctex_texture_set_query_channel_delta(
        document: *const ctex_document,
        texture_set_id: *const c_char,
        semantic_id: *const c_char,
        synchronized_cursor: ctex_transport_revision_cursor,
        changed_tiles: *mut ctex_transport_tile_version,
        changed_tile_capacity: usize,
        out_info: *mut ctex_transport_delta_info,
    ) -> ctex_result;
    pub fn ctex_transport_snapshot_pool_create(
        budget_bytes: usize,
        out_pool: *mut *mut ctex_transport_snapshot_pool,
    ) -> ctex_result;
    pub fn ctex_transport_snapshot_pool_destroy(pool: *mut ctex_transport_snapshot_pool);
    pub fn ctex_texture_set_query_channel_snapshot(
        pool: *mut ctex_transport_snapshot_pool,
        document: *const ctex_document,
        texture_set_id: *const c_char,
        semantic_id: *const c_char,
        synchronized_cursor: ctex_transport_revision_cursor,
        out_snapshot: *mut *mut ctex_transport_snapshot,
        out_info: *mut ctex_transport_snapshot_query_info,
    ) -> ctex_result;
    pub fn ctex_transport_snapshot_destroy(snapshot: *mut ctex_transport_snapshot);
    pub fn ctex_transport_snapshot_get_tile_versions(
        snapshot: *const ctex_transport_snapshot,
        versions: *mut ctex_transport_tile_version,
        version_capacity: usize,
        out_version_count: *mut usize,
    ) -> ctex_result;
    pub fn ctex_transport_snapshot_get_tile_memory_layout(
        snapshot: *const ctex_transport_snapshot,
        version: ctex_transport_tile_version,
        format: *const c_void,
        out_layout: *mut ctex_transport_tile_memory_layout,
    ) -> ctex_result;
    pub fn ctex_transport_snapshot_begin_host_readback(
        snapshot: *const ctex_transport_snapshot,
        format: *const c_void,
        destinations: *const ctex_transport_tile_readback_destination,
        destination_count: usize,
        out_readback: *mut *mut ctex_transport_readback,
    ) -> ctex_result;
    pub fn ctex_transport_readback_destroy(readback: *mut ctex_transport_readback);
    pub fn ctex_transport_readback_get_info(
        readback: *const ctex_transport_readback,
        out_info: *mut ctex_transport_readback_info,
        detail: *mut c_char,
        detail_size: usize,
    ) -> ctex_result;
    pub fn ctex_transport_readback_complete_host(
        readback: *mut ctex_transport_readback,
        completed_tiles: *const ctex_transport_host_tile_completion,
        completed_tile_count: usize,
    ) -> ctex_result;
    pub fn ctex_material_graph_create_default(
        out_info: *mut ctex_material_graph_info,
        canonical_output: *mut c_void,
        canonical_output_size: usize,
        report_output: *mut c_char,
        report_output_size: usize,
    ) -> ctex_result;
    pub fn ctex_shader_emit_material(
        registry: *const c_void,
        graph_serialized: *const c_void,
        graph_serialized_size: usize,
        request: *const ctex_shader_material_request,
        out_info: *mut ctex_shader_material_info,
        vertex_artifact: *mut c_void,
        vertex_artifact_size: usize,
        fragment_artifact: *mut c_void,
        fragment_artifact_size: usize,
        pass_plan_output: *mut c_char,
        pass_plan_output_size: usize,
        workaround_report_output: *mut c_char,
        workaround_report_output_size: usize,
    ) -> ctex_result;
}

pub type ctex_user_data = *mut c_void;
