//! Raw declarations generated from `include/ctex/capi.h`.
//! Regenerate with `python3 tools/generate_rust_sys.py`.

//! C header SHA-256: e089804504db69d4d6e7eca9ea4124ac8328cf07c4b1fb2bfd559e9012cbad0c

#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals)]

pub const CTEX_NO_SURFACE_TRIANGLE: u32 = 4294967295;
pub const CTEX_NO_UV_ISLAND: u32 = 4294967295;
pub const CTEX_MESH_REPLACEMENT_NO_PARTITION: u32 = 4294967295;
pub const CTEX_MESH_MAP_BAKE_PROVIDER_ENTRY_POINT_V1: &[u8; 31] =
    b"ctex_mesh_map_bake_provider_v1\0";
pub const ctex_result_CTEX_RESULT_SUCCESS: ctex_result = 0;
pub const ctex_result_CTEX_RESULT_INVALID_ARGUMENT: ctex_result = 1;
pub const ctex_result_CTEX_RESULT_MISSING_RESOURCE: ctex_result = 2;
pub const ctex_result_CTEX_RESULT_UNSUPPORTED_OPERATION: ctex_result = 3;
pub const ctex_result_CTEX_RESULT_OUT_OF_MEMORY: ctex_result = 4;
pub const ctex_result_CTEX_RESULT_OVER_BUDGET: ctex_result = 5;
pub const ctex_result_CTEX_RESULT_CANCELLED: ctex_result = 6;
pub const ctex_result_CTEX_RESULT_INTERNAL_ERROR: ctex_result = 7;
pub const ctex_result_CTEX_RESULT_BUFFER_TOO_SMALL: ctex_result = 8;
pub const ctex_result_CTEX_RESULT_NO_UNDO: ctex_result = 9;
pub const ctex_result_CTEX_RESULT_NO_REDO: ctex_result = 10;
pub const ctex_result_CTEX_RESULT_STALE_STATE: ctex_result = 11;
pub type ctex_result = ::std::os::raw::c_uint;
pub const CTEX_RESULT_SUCCESS: ctex_result = ctex_result_CTEX_RESULT_SUCCESS;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_NONE: ctex_diagnostic_code = 0;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_NULL_ARGUMENT: ctex_diagnostic_code = 1;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE: ctex_diagnostic_code = 2;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE: ctex_diagnostic_code = 3;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_VALUE: ctex_diagnostic_code = 4;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL: ctex_diagnostic_code = 5;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_ALLOCATION_FAILED: ctex_diagnostic_code = 6;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_UNEXPECTED_EXCEPTION: ctex_diagnostic_code = 7;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_DISPLAY_NAME:
    ctex_diagnostic_code = 8;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_PARTITION_KEY:
    ctex_diagnostic_code = 9;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_UV_SET: ctex_diagnostic_code = 10;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_RESOLUTION:
    ctex_diagnostic_code = 11;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_BIT_DEPTH: ctex_diagnostic_code =
    12;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_DUPLICATE_TEXTURE_SET: ctex_diagnostic_code = 13;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_ALLOCATOR_CONTRACT_VIOLATION: ctex_diagnostic_code =
    14;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_MISSING_TEXTURE_SET: ctex_diagnostic_code = 15;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_EMPTY_CHANNEL_SEMANTIC_ID: ctex_diagnostic_code = 16;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_EMPTY_CHANNEL_EXPORT_MAPPING: ctex_diagnostic_code =
    17;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_CHANNEL_COMPONENT_COUNT:
    ctex_diagnostic_code = 18;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_CHANNEL_DEFAULT_VALUE_COUNT:
    ctex_diagnostic_code = 19;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_CHANNEL_BIT_DEPTH: ctex_diagnostic_code = 20;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_DUPLICATE_CHANNEL: ctex_diagnostic_code = 21;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_MISSING_CHANNEL: ctex_diagnostic_code = 22;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_UNSUPPORTED_COLOR_SPACE: ctex_diagnostic_code = 23;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_UNSUPPORTED_CHANNEL_SEMANTIC: ctex_diagnostic_code =
    24;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_COLOR_COMPONENT: ctex_diagnostic_code = 25;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_COLOR_BIT_DEPTH: ctex_diagnostic_code = 26;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_CUBE_LUT: ctex_diagnostic_code = 27;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_MESH_LIMIT_EXCEEDED: ctex_diagnostic_code = 28;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_MESH: ctex_diagnostic_code = 29;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_MISSING_UV_SET: ctex_diagnostic_code = 30;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_FORMAT: ctex_diagnostic_code = 31;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA: ctex_diagnostic_code = 32;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED: ctex_diagnostic_code = 33;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_COMBINATION: ctex_diagnostic_code =
    34;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_IMAGE_ENCODING_FAILED: ctex_diagnostic_code = 35;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_STROKE: ctex_diagnostic_code = 36;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_STROKE_PRESET: ctex_diagnostic_code = 37;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_COVERAGE: ctex_diagnostic_code = 38;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED: ctex_diagnostic_code = 39;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_BLEND: ctex_diagnostic_code = 40;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_MASK: ctex_diagnostic_code = 41;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_COORDINATES: ctex_diagnostic_code = 42;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_REJECTION: ctex_diagnostic_code = 43;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_WORK: ctex_diagnostic_code = 44;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION: ctex_diagnostic_code = 45;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER: ctex_diagnostic_code = 46;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE: ctex_diagnostic_code =
    47;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_PREVIEW: ctex_diagnostic_code = 48;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PICK_QUERY: ctex_diagnostic_code = 49;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_TEXTURE_EXPORT: ctex_diagnostic_code = 50;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PROJECT_CONTAINER: ctex_diagnostic_code = 51;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_SMART_MATERIAL: ctex_diagnostic_code = 52;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PRESET_LIBRARY: ctex_diagnostic_code = 53;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_HOST_TRANSPORT: ctex_diagnostic_code = 54;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_EXECUTOR: ctex_diagnostic_code = 55;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_PAINT_TOOL: ctex_diagnostic_code = 56;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_MATERIAL_GRAPH: ctex_diagnostic_code = 57;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_SHADER_EMISSION: ctex_diagnostic_code = 58;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_MESH_MAP: ctex_diagnostic_code = 59;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_TILE_HISTORY: ctex_diagnostic_code = 60;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_OPERATION_RECORD: ctex_diagnostic_code = 61;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_EDITABLE_AUTHORING: ctex_diagnostic_code =
    62;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_MESH_REPROJECTION: ctex_diagnostic_code = 63;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_INVALID_RESOLUTION_CHANGE: ctex_diagnostic_code = 64;
pub const ctex_diagnostic_code_CTEX_DIAGNOSTIC_IMAGE_DECODE_CANCELLED: ctex_diagnostic_code = 65;
pub type ctex_diagnostic_code = ::std::os::raw::c_uint;
pub const ctex_log_severity_CTEX_LOG_SEVERITY_TRACE: ctex_log_severity = 0;
pub const ctex_log_severity_CTEX_LOG_SEVERITY_DEBUG: ctex_log_severity = 1;
pub const ctex_log_severity_CTEX_LOG_SEVERITY_INFO: ctex_log_severity = 2;
pub const ctex_log_severity_CTEX_LOG_SEVERITY_WARNING: ctex_log_severity = 3;
pub const ctex_log_severity_CTEX_LOG_SEVERITY_ERROR: ctex_log_severity = 4;
pub const ctex_log_severity_CTEX_LOG_SEVERITY_FATAL: ctex_log_severity = 5;
pub type ctex_log_severity = ::std::os::raw::c_uint;
pub type ctex_log_callback = ::std::option::Option<
    unsafe extern "C" fn(
        severity: ctex_log_severity,
        category: *const ::std::os::raw::c_char,
        message: *const ::std::os::raw::c_char,
        user_data: *mut ::std::os::raw::c_void,
    ),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_log_sink_descriptor {
    pub size: u32,
    pub callback: ctex_log_callback,
    pub user_data: *mut ::std::os::raw::c_void,
    pub minimum_severity: u32,
}
impl Default for ctex_log_sink_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type ctex_allocate_callback = ::std::option::Option<
    unsafe extern "C" fn(
        size: usize,
        alignment: usize,
        user_data: *mut ::std::os::raw::c_void,
    ) -> *mut ::std::os::raw::c_void,
>;
pub type ctex_deallocate_callback = ::std::option::Option<
    unsafe extern "C" fn(
        allocation: *mut ::std::os::raw::c_void,
        size: usize,
        alignment: usize,
        user_data: *mut ::std::os::raw::c_void,
    ),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_allocator_descriptor {
    pub size: u32,
    pub allocate: ctex_allocate_callback,
    pub deallocate: ctex_deallocate_callback,
    pub user_data: *mut ::std::os::raw::c_void,
}
impl Default for ctex_allocator_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_document {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_cube_lut {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_replacement_plan {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_dilation_session {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_surface_map_cache {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_preview_session {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_pick_index {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_uv_pick_index {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_transport_snapshot_pool {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_transport_snapshot {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_transport_readback {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_resource_ledger {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_resource_reservation {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_set {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_bake_session {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_bake_request_token {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_project_autosave_session {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_executor_registry {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_cpu_execution_result {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_parity_gate_result {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_host_execution_session {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_host_completion_result {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_host_recovery_report {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_workspace {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_node_registry {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_emission_cache {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_tile_history_capture {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_layer_snapshot {
    _unused: [u8; 0],
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_set_transaction {
    _unused: [u8; 0],
}
pub const ctex_partition_source_kind_CTEX_PARTITION_SOURCE_MATERIAL: ctex_partition_source_kind = 0;
pub const ctex_partition_source_kind_CTEX_PARTITION_SOURCE_OBJECT: ctex_partition_source_kind = 1;
pub const ctex_partition_source_kind_CTEX_PARTITION_SOURCE_SUBMESH: ctex_partition_source_kind = 2;
pub const ctex_partition_source_kind_CTEX_PARTITION_SOURCE_EXPLICIT_FACES:
    ctex_partition_source_kind = 3;
pub type ctex_partition_source_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_vec2f {
    pub x: f32,
    pub y: f32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_vec3f {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_vec4f {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub w: f32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_vec2d {
    pub x: f64,
    pub y: f64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_vec3d {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_stroke_frame {
    pub tangent: ctex_vec3d,
    pub bitangent: ctex_vec3d,
    pub normal: ctex_vec3d,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_stroke_input_sample {
    pub size: u32,
    pub position: ctex_vec3d,
    pub frame: ctex_stroke_frame,
    pub timestamp_nanoseconds: u64,
    pub has_pressure: u32,
    pub pressure: f64,
    pub tilt: ctex_vec2d,
}
pub const ctex_stroke_tip_mode_CTEX_STROKE_TIP_CONTINUOUS_SWEEP: ctex_stroke_tip_mode = 0;
pub const ctex_stroke_tip_mode_CTEX_STROKE_TIP_DISCRETE_ALPHA: ctex_stroke_tip_mode = 1;
pub type ctex_stroke_tip_mode = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_response_curve_point {
    pub input: f64,
    pub output: f64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_response_mapping_descriptor {
    pub size: u32,
    pub enabled: u32,
    pub points: *const ctex_response_curve_point,
    pub point_count: usize,
    pub minimum_output: f64,
    pub maximum_output: f64,
}
impl Default for ctex_response_mapping_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_stroke_stabilizer_descriptor {
    pub size: u32,
    pub radius: f64,
    pub time_constant_seconds: f64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_stroke_jitter_descriptor {
    pub size: u32,
    pub seed: u64,
    pub position_fraction: f64,
    pub radius_fraction: f64,
    pub rotation_radians: f64,
    pub opacity: f64,
    pub flow: f64,
}
pub const ctex_stroke_taper_unit_CTEX_STROKE_TAPER_NONE: ctex_stroke_taper_unit = 0;
pub const ctex_stroke_taper_unit_CTEX_STROKE_TAPER_STAMP_COUNT: ctex_stroke_taper_unit = 1;
pub const ctex_stroke_taper_unit_CTEX_STROKE_TAPER_DISTANCE: ctex_stroke_taper_unit = 2;
pub type ctex_stroke_taper_unit = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_stroke_taper_span_descriptor {
    pub size: u32,
    pub unit: u32,
    pub extent: f64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_stroke_taper_descriptor {
    pub size: u32,
    pub entry: ctex_stroke_taper_span_descriptor,
    pub exit: ctex_stroke_taper_span_descriptor,
    pub floor: f64,
    pub affect_radius: u32,
    pub affect_opacity: u32,
}
pub const ctex_stroke_constraint_mode_CTEX_STROKE_CONSTRAINT_NONE: ctex_stroke_constraint_mode = 0;
pub const ctex_stroke_constraint_mode_CTEX_STROKE_CONSTRAINT_STRAIGHT_LINE:
    ctex_stroke_constraint_mode = 1;
pub const ctex_stroke_constraint_mode_CTEX_STROKE_CONSTRAINT_DOMINANT_AXIS:
    ctex_stroke_constraint_mode = 2;
pub const ctex_stroke_constraint_mode_CTEX_STROKE_CONSTRAINT_GRID: ctex_stroke_constraint_mode = 3;
pub type ctex_stroke_constraint_mode = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_stroke_constraint_descriptor {
    pub size: u32,
    pub mode: u32,
    pub grid_step: f64,
}
pub const ctex_stroke_symmetry_axis_CTEX_STROKE_SYMMETRY_AXIS_X: ctex_stroke_symmetry_axis = 0;
pub const ctex_stroke_symmetry_axis_CTEX_STROKE_SYMMETRY_AXIS_Y: ctex_stroke_symmetry_axis = 1;
pub const ctex_stroke_symmetry_axis_CTEX_STROKE_SYMMETRY_AXIS_Z: ctex_stroke_symmetry_axis = 2;
pub type ctex_stroke_symmetry_axis = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_stroke_symmetry_descriptor {
    pub size: u32,
    pub mirror_x: u32,
    pub mirror_y: u32,
    pub mirror_z: u32,
    pub radial_count: u32,
    pub radial_axis: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_stroke_settings_descriptor {
    pub size: u32,
    pub reconstruction_version: u32,
    pub tip_mode: u32,
    pub spacing_fraction: f64,
    pub radius: f64,
    pub opacity: f64,
    pub hardness: f64,
    pub rotation_radians: f64,
    pub elongation: f64,
    pub flow: f64,
    pub tip_resource_identity: *const ::std::os::raw::c_char,
    pub stabilizer: ctex_stroke_stabilizer_descriptor,
    pub pressure_radius: ctex_response_mapping_descriptor,
    pub pressure_opacity: ctex_response_mapping_descriptor,
    pub pressure_hardness: ctex_response_mapping_descriptor,
    pub pressure_flow: ctex_response_mapping_descriptor,
    pub pressure_rotation: ctex_response_mapping_descriptor,
    pub tilt_rotation: ctex_response_mapping_descriptor,
    pub tilt_elongation: ctex_response_mapping_descriptor,
    pub jitter: ctex_stroke_jitter_descriptor,
    pub taper: ctex_stroke_taper_descriptor,
    pub constraint: ctex_stroke_constraint_descriptor,
    pub symmetry: ctex_stroke_symmetry_descriptor,
}
impl Default for ctex_stroke_settings_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_resolved_stamp {
    pub position: ctex_vec3d,
    pub frame: ctex_stroke_frame,
    pub radius: f64,
    pub opacity: f64,
    pub hardness: f64,
    pub rotation_radians: f64,
    pub elongation: f64,
    pub flow: f64,
    pub tip_resource_identity: *const ::std::os::raw::c_char,
    pub source_ordinal: u64,
    pub symmetry_instance: u64,
    pub ordinal: u64,
}
impl Default for ctex_resolved_stamp {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_swept_segment {
    pub start_stamp_ordinal: u64,
    pub end_stamp_ordinal: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_resolved_stroke_info {
    pub size: u32,
    pub reconstruction_version: u32,
    pub tip_mode: u32,
    pub symmetry_instance_count: u64,
    pub stamp_count: usize,
    pub swept_segment_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_resolved_stroke_descriptor {
    pub size: u32,
    pub reconstruction_version: u32,
    pub tip_mode: u32,
    pub symmetry_instance_count: u64,
    pub stamps: *const ctex_resolved_stamp,
    pub stamp_count: usize,
    pub swept_segments: *const ctex_swept_segment,
    pub swept_segment_count: usize,
}
impl Default for ctex_resolved_stroke_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_tile_coverage_descriptor {
    pub size: u32,
    pub uv_set: *const ::std::os::raw::c_char,
    pub width: u32,
    pub height: u32,
    pub tile_origin: ctex_vec2d,
}
impl Default for ctex_paint_tile_coverage_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_paint_deposition_mode_CTEX_PAINT_DEPOSITION_NON_BUILDING:
    ctex_paint_deposition_mode = 0;
pub const ctex_paint_deposition_mode_CTEX_PAINT_DEPOSITION_BUILD_UP: ctex_paint_deposition_mode = 1;
pub type ctex_paint_deposition_mode = ::std::os::raw::c_uint;
pub const ctex_alpha_discard_format_CTEX_ALPHA_DISCARD_UNORM8: ctex_alpha_discard_format = 0;
pub const ctex_alpha_discard_format_CTEX_ALPHA_DISCARD_UNORM16: ctex_alpha_discard_format = 1;
pub const ctex_alpha_discard_format_CTEX_ALPHA_DISCARD_FLOATING_POINT: ctex_alpha_discard_format =
    2;
pub type ctex_alpha_discard_format = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_mask_view {
    pub values: *const f64,
    pub value_count: usize,
}
impl Default for ctex_paint_mask_view {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_mask_inputs_descriptor {
    pub size: u32,
    pub active_layer_masks: *const ctex_paint_mask_view,
    pub active_layer_mask_count: usize,
    pub colour_id_selection: *const ctex_paint_mask_view,
    pub geometry_selection: *const ctex_paint_mask_view,
    pub screen_selection: *const ctex_paint_mask_view,
    pub uv_island_selection: *const ctex_paint_mask_view,
}
impl Default for ctex_paint_mask_inputs_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_mask_info {
    pub size: u32,
    pub active_input_count: usize,
}
pub const ctex_paint_material_coordinate_mode_CTEX_PAINT_MATERIAL_COORDINATE_UV:
    ctex_paint_material_coordinate_mode = 0;
pub const ctex_paint_material_coordinate_mode_CTEX_PAINT_MATERIAL_COORDINATE_TRIPLANAR:
    ctex_paint_material_coordinate_mode = 1;
pub const ctex_paint_material_coordinate_mode_CTEX_PAINT_MATERIAL_COORDINATE_PLANAR:
    ctex_paint_material_coordinate_mode = 2;
pub type ctex_paint_material_coordinate_mode = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_material_coordinate_descriptor {
    pub size: u32,
    pub mode: u32,
    pub planar_origin: ctex_vec3d,
    pub planar_u_axis: ctex_vec3d,
    pub planar_v_axis: ctex_vec3d,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_material_coordinate_sample {
    pub covered: u32,
    pub projection_count: u32,
    pub coordinates: [ctex_vec2d; 3usize],
    pub weights: [f64; 3usize],
}
pub const ctex_paint_symmetry_depth_policy_CTEX_PAINT_SYMMETRY_DEPTH_REQUIRE_CONSISTENT:
    ctex_paint_symmetry_depth_policy = 0;
pub const ctex_paint_symmetry_depth_policy_CTEX_PAINT_SYMMETRY_DEPTH_DISABLE_DERIVED:
    ctex_paint_symmetry_depth_policy = 1;
pub type ctex_paint_symmetry_depth_policy = ::std::os::raw::c_uint;
pub const ctex_paint_depth_disposition_CTEX_PAINT_DEPTH_DISABLED_BY_OPERATION:
    ctex_paint_depth_disposition = 0;
pub const ctex_paint_depth_disposition_CTEX_PAINT_DEPTH_CONSISTENT_PER_INSTANCE:
    ctex_paint_depth_disposition = 1;
pub const ctex_paint_depth_disposition_CTEX_PAINT_DEPTH_DISABLED_FOR_DERIVED_SYMMETRY:
    ctex_paint_depth_disposition = 2;
pub type ctex_paint_depth_disposition = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_depth_context_descriptor {
    pub size: u32,
    pub symmetry_instance: u64,
    pub viewport_width: u32,
    pub viewport_height: u32,
    pub screen_positions: *const ctex_vec2d,
    pub surface_depth: *const f64,
    pub surface_sample_count: usize,
    pub visible_depth: *const f64,
    pub visible_depth_count: usize,
    pub transform_consistent: u32,
}
impl Default for ctex_paint_depth_context_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_rejection_descriptor {
    pub size: u32,
    pub depth_enabled: u32,
    pub depth_bias: f64,
    pub symmetry_depth_policy: u32,
    pub angle_enabled: u32,
    pub minimum_normal_dot: f64,
    pub backface_enabled: u32,
    pub depth_contexts: *const ctex_paint_depth_context_descriptor,
    pub depth_context_count: usize,
    pub view_directions: *const ctex_vec3d,
    pub view_direction_count: usize,
}
impl Default for ctex_paint_rejection_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_rejection_info {
    pub size: u32,
    pub depth_disposition: u32,
    pub depth_rejected_contributions: usize,
    pub angle_rejected_contributions: usize,
    pub backface_rejected_texels: usize,
    pub resolved_depth_bias: f64,
    pub resolved_minimum_normal_dot: f64,
    pub depth_bias_clamped: u32,
    pub minimum_normal_dot_clamped: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_stamp_footprint {
    pub stamp_ordinal: u64,
    pub minimum_x: u32,
    pub minimum_y: u32,
    pub maximum_x: u32,
    pub maximum_y: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_tile_coordinate {
    pub x: u32,
    pub y: u32,
}
pub const ctex_paint_preview_state_CTEX_PAINT_PREVIEW_PROVISIONAL: ctex_paint_preview_state = 0;
pub const ctex_paint_preview_state_CTEX_PAINT_PREVIEW_FINAL: ctex_paint_preview_state = 1;
pub const ctex_paint_preview_state_CTEX_PAINT_PREVIEW_COMMITTED: ctex_paint_preview_state = 2;
pub const ctex_paint_preview_state_CTEX_PAINT_PREVIEW_CANCELLED: ctex_paint_preview_state = 3;
pub type ctex_paint_preview_state = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
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
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_work_descriptor {
    pub size: u32,
    pub canvas_width: u32,
    pub canvas_height: u32,
    pub tile_size: u32,
    pub dilation_radius: u32,
    pub stamp_footprints: *const ctex_paint_stamp_footprint,
    pub stamp_footprint_count: usize,
}
impl Default for ctex_paint_work_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_work_info {
    pub size: u32,
    pub canvas_tile_count: u64,
    pub footprint_count: usize,
    pub candidate_tile_visits: usize,
    pub processed_tile_count: usize,
    pub resolved_dilation_radius: u32,
    pub dilation_radius_clamped: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_seam_dilation_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub component_count: u32,
    pub radius: u32,
    pub pixels: *const f64,
    pub pixel_count: usize,
    pub coverage: *const u8,
    pub coverage_count: usize,
}
impl Default for ctex_paint_seam_dilation_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_seam_dilation_info {
    pub size: u32,
    pub required_pixel_count: usize,
    pub dilated_texel_count: usize,
    pub zero_gradient_texel_count: usize,
    pub resolved_radius: u32,
    pub radius_clamped: u32,
}
pub const ctex_paint_dilation_state_CTEX_PAINT_DILATION_PROVISIONAL: ctex_paint_dilation_state = 0;
pub const ctex_paint_dilation_state_CTEX_PAINT_DILATION_FINAL: ctex_paint_dilation_state = 1;
pub type ctex_paint_dilation_state = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_dilation_tile_descriptor {
    pub size: u32,
    pub u: i32,
    pub v: i32,
    pub width: u32,
    pub height: u32,
    pub component_count: u32,
    pub pixels: *const f64,
    pub pixel_count: usize,
    pub coverage: *const u8,
    pub coverage_count: usize,
}
impl Default for ctex_paint_dilation_tile_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_dilation_tile_info {
    pub u: i32,
    pub v: i32,
    pub width: u32,
    pub height: u32,
    pub component_count: u32,
    pub pixel_offset: usize,
    pub pixel_count: usize,
    pub dilated_texel_count: usize,
    pub zero_gradient_texel_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_dilation_session_info {
    pub size: u32,
    pub state: u32,
    pub tile_count: usize,
    pub required_pixel_count: usize,
    pub dilation_pass_count: usize,
    pub resolved_radius: u32,
    pub radius_clamped: u32,
}
pub const ctex_paint_surface_filter_operation_CTEX_PAINT_SURFACE_FILTER_BLUR:
    ctex_paint_surface_filter_operation = 0;
pub const ctex_paint_surface_filter_operation_CTEX_PAINT_SURFACE_FILTER_SMEAR:
    ctex_paint_surface_filter_operation = 1;
pub const ctex_paint_surface_filter_operation_CTEX_PAINT_SURFACE_FILTER_DERIVATIVE:
    ctex_paint_surface_filter_operation = 2;
pub const ctex_paint_surface_filter_operation_CTEX_PAINT_SURFACE_FILTER_MIP_GENERATION:
    ctex_paint_surface_filter_operation = 3;
pub type ctex_paint_surface_filter_operation = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_surface_filter_sample {
    pub texel_index: usize,
    pub tangent_frame: ctex_stroke_frame,
    pub offset_x: i32,
    pub offset_y: i32,
    pub weight: f64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_surface_filter_descriptor {
    pub size: u32,
    pub operation: u32,
    pub radius_x: u32,
    pub radius_y: u32,
    pub output_frame: ctex_stroke_frame,
    pub samples: *const ctex_paint_surface_filter_sample,
    pub sample_count: usize,
}
impl Default for ctex_paint_surface_filter_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_island_padding_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub component_count: u32,
    pub radius_x: u32,
    pub radius_y: u32,
    pub requested_mip_levels: u32,
    pub island_identity: *const u32,
    pub island_identity_count: usize,
    pub pixels: *const f64,
    pub pixel_count: usize,
}
impl Default for ctex_paint_island_padding_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_unsupported_mip_level {
    pub mip_level: u32,
    pub required_gutter_radius: u32,
    pub affected_island_offset: usize,
    pub affected_island_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_island_padding_info {
    pub size: u32,
    pub required_ownership_count: usize,
    pub unsupported_mip_level_count: usize,
    pub required_affected_island_count: usize,
    pub required_pixel_count: usize,
    pub padded_texel_count: usize,
    pub padding_radius: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_surface_map_request {
    pub size: u32,
    pub partition_index: usize,
    pub uv_set: *const ::std::os::raw::c_char,
    pub width: u32,
    pub height: u32,
    pub tile_origin: ctex_vec2d,
}
impl Default for ctex_paint_surface_map_request {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_surface_texel {
    pub position: ctex_vec3d,
    pub normal: ctex_vec3d,
    pub geometric_normal: ctex_vec3d,
    pub uv: ctex_vec2d,
    pub triangle: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_surface_map_info {
    pub size: u32,
    pub cache_hit: u32,
    pub mesh_revision: u64,
    pub required_texture_set_id_size: usize,
    pub required_uv_set_size: usize,
    pub required_texel_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_surface_map_buffers {
    pub size: u32,
    pub texture_set_id: *mut ::std::os::raw::c_char,
    pub texture_set_id_size: usize,
    pub uv_set: *mut ::std::os::raw::c_char,
    pub uv_set_size: usize,
    pub surface_texels: *mut ctex_paint_surface_texel,
    pub surface_texel_capacity: usize,
    pub coverage: *mut u8,
    pub coverage_capacity: usize,
    pub triangle_identity: *mut u32,
    pub triangle_identity_capacity: usize,
    pub uv_island_identity: *mut u32,
    pub uv_island_identity_capacity: usize,
}
impl Default for ctex_paint_surface_map_buffers {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_surface_map_statistics {
    pub size: u32,
    pub entries: usize,
    pub hits: usize,
    pub misses: usize,
    pub invalidated_entries: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_deposition_descriptor {
    pub size: u32,
    pub mode: u32,
    pub alpha_discard_format: u32,
    pub has_custom_alpha_discard_threshold: u32,
    pub custom_alpha_discard_threshold: f64,
    pub masks: *const ctex_paint_mask_inputs_descriptor,
}
impl Default for ctex_paint_deposition_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_deposition_info {
    pub size: u32,
    pub mode: u32,
    pub applied_stamp_count: usize,
    pub alpha_discard_threshold: f64,
    pub alpha_discard_threshold_clamped: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_deposition_sample {
    pub non_building_coverage: f64,
    pub build_up_deposition: f64,
    pub strength: f64,
    pub retained_strength: f64,
    pub write: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_blend_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub blend_mode: *const ::std::os::raw::c_char,
    pub stroke_start_snapshot: *const ctex_vec4f,
    pub paint: *const ctex_vec4f,
    pub deposition: *const ctex_paint_deposition_sample,
    pub pixel_count: usize,
}
impl Default for ctex_paint_blend_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_tool_channel_descriptor {
    pub size: u32,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub component_count: u32,
    pub pixels: *const ctex_vec4f,
    pub pixel_count: usize,
}
impl Default for ctex_paint_tool_channel_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_tool_channel_output {
    pub size: u32,
    pub pixels: *mut ctex_vec4f,
    pub pixel_capacity: usize,
}
impl Default for ctex_paint_tool_channel_output {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_brush_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub enabled_layer_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub enabled_layer_channel_count: usize,
    pub material: *const ctex_paint_tool_channel_descriptor,
    pub material_channel_count: usize,
    pub deposition: *const ctex_paint_deposition_sample,
    pub deposition_count: usize,
    pub blend_mode: *const ::std::os::raw::c_char,
}
impl Default for ctex_paint_brush_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_brush_info {
    pub size: u32,
    pub applied_channel_count: usize,
    pub required_pixels_per_channel: usize,
}
pub const ctex_paint_eraser_target_CTEX_PAINT_ERASER_TARGET_LAYER_OPACITY:
    ctex_paint_eraser_target = 0;
pub const ctex_paint_eraser_target_CTEX_PAINT_ERASER_TARGET_MASK: ctex_paint_eraser_target = 1;
pub type ctex_paint_eraser_target = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_eraser_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub target: u32,
    pub stroke_start_values: *const f64,
    pub value_count: usize,
    pub deposition: *const ctex_paint_deposition_sample,
    pub deposition_count: usize,
}
impl Default for ctex_paint_eraser_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_eraser_info {
    pub size: u32,
    pub target: u32,
    pub required_value_count: usize,
}
pub const ctex_paint_fill_scope_CTEX_PAINT_FILL_WHOLE_SET: ctex_paint_fill_scope = 0;
pub const ctex_paint_fill_scope_CTEX_PAINT_FILL_TRIANGLE: ctex_paint_fill_scope = 1;
pub const ctex_paint_fill_scope_CTEX_PAINT_FILL_CONNECTED_BY_ANGLE: ctex_paint_fill_scope = 2;
pub const ctex_paint_fill_scope_CTEX_PAINT_FILL_UV_ISLAND: ctex_paint_fill_scope = 3;
pub const ctex_paint_fill_scope_CTEX_PAINT_FILL_UV_TILE: ctex_paint_fill_scope = 4;
pub const ctex_paint_fill_scope_CTEX_PAINT_FILL_SELECTION: ctex_paint_fill_scope = 5;
pub type ctex_paint_fill_scope = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_fill_triangle_topology {
    pub triangle_identity: u32,
    pub geometric_normal: ctex_vec3d,
    pub adjacent_triangles: *const u32,
    pub adjacent_triangle_count: usize,
}
impl Default for ctex_paint_fill_triangle_topology {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_fill_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub scope: u32,
    pub has_picked_texel: u32,
    pub picked_texel: usize,
    pub maximum_angle_degrees: f64,
    pub surface_texels: *const ctex_paint_surface_texel,
    pub surface_texel_count: usize,
    pub coverage: *const u8,
    pub coverage_count: usize,
    pub triangle_identity: *const u32,
    pub triangle_identity_count: usize,
    pub uv_island_identity: *const u32,
    pub uv_island_identity_count: usize,
    pub triangle_topology: *const ctex_paint_fill_triangle_topology,
    pub triangle_topology_count: usize,
    pub selection: *const f64,
    pub selection_count: usize,
    pub enabled_layer_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub enabled_layer_channel_count: usize,
    pub material: *const ctex_paint_tool_channel_descriptor,
    pub material_channel_count: usize,
    pub masks: *const ctex_paint_mask_inputs_descriptor,
    pub rejection_acceptance: *const f64,
    pub rejection_acceptance_count: usize,
    pub blend_mode: *const ::std::os::raw::c_char,
}
impl Default for ctex_paint_fill_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_fill_info {
    pub size: u32,
    pub scope: u32,
    pub resolved_maximum_angle_degrees: f64,
    pub maximum_angle_clamped: u32,
    pub selected_texel_count: usize,
    pub selected_triangle_count: usize,
    pub applied_channel_count: usize,
    pub required_pixels_per_channel: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_fill_outputs {
    pub size: u32,
    pub scope_values: *mut f64,
    pub scope_value_capacity: usize,
    pub selected_triangle_ids: *mut u32,
    pub selected_triangle_capacity: usize,
    pub channels: *const ctex_paint_tool_channel_output,
    pub channel_count: usize,
}
impl Default for ctex_paint_fill_outputs {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_paint_clone_mode_CTEX_PAINT_CLONE_ALIGNED: ctex_paint_clone_mode = 0;
pub const ctex_paint_clone_mode_CTEX_PAINT_CLONE_FIXED: ctex_paint_clone_mode = 1;
pub type ctex_paint_clone_mode = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_clone_source_descriptor {
    pub size: u32,
    pub texture_set_id: *const ::std::os::raw::c_char,
    pub uv: ctex_vec2d,
}
impl Default for ctex_paint_clone_source_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_clone_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub mode: u32,
    pub destination_texture_set_id: *const ::std::os::raw::c_char,
    pub tile_origin: ctex_vec2d,
    pub destination_anchor_uv: ctex_vec2d,
    pub source: *const ctex_paint_clone_source_descriptor,
    pub destination_surface_texels: *const ctex_paint_surface_texel,
    pub destination_surface_texel_count: usize,
    pub destination_coverage: *const u8,
    pub destination_coverage_count: usize,
    pub enabled_layer_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub enabled_layer_channel_count: usize,
    pub source_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub source_channel_count: usize,
    pub deposition: *const ctex_paint_deposition_sample,
    pub deposition_count: usize,
    pub blend_mode: *const ::std::os::raw::c_char,
}
impl Default for ctex_paint_clone_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_clone_info {
    pub size: u32,
    pub mode: u32,
    pub source_anchor_uv: ctex_vec2d,
    pub destination_anchor_uv: ctex_vec2d,
    pub required_source_sample_count: usize,
    pub applied_channel_count: usize,
    pub required_pixels_per_channel: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_clone_outputs {
    pub size: u32,
    pub source_sample_indices: *mut usize,
    pub source_sample_capacity: usize,
    pub channels: *const ctex_paint_tool_channel_output,
    pub channel_count: usize,
}
impl Default for ctex_paint_clone_outputs {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_blur_neighborhood_descriptor {
    pub size: u32,
    pub output_frame: ctex_stroke_frame,
    pub horizontal_samples: *const ctex_paint_surface_filter_sample,
    pub horizontal_sample_count: usize,
    pub vertical_samples: *const ctex_paint_surface_filter_sample,
    pub vertical_sample_count: usize,
}
impl Default for ctex_paint_blur_neighborhood_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_blur_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub radius: u32,
    pub stroke_start_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub channel_count: usize,
    pub deposition: *const ctex_paint_deposition_sample,
    pub deposition_count: usize,
    pub blend_mode: *const ::std::os::raw::c_char,
    pub neighborhoods: *const ctex_paint_blur_neighborhood_descriptor,
    pub neighborhood_count: usize,
}
impl Default for ctex_paint_blur_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_blur_info {
    pub size: u32,
    pub resolved_radius: u32,
    pub radius_clamped: u32,
    pub applied_channel_count: usize,
    pub required_pixels_per_channel: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_smear_mapping_descriptor {
    pub size: u32,
    pub output_frame: ctex_stroke_frame,
    pub upstream_sample: ctex_paint_surface_filter_sample,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_smear_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub strength: f64,
    pub footprint_radius_x: u32,
    pub footprint_radius_y: u32,
    pub stroke_start_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub channel_count: usize,
    pub deposition: *const ctex_paint_deposition_sample,
    pub deposition_count: usize,
    pub blend_mode: *const ::std::os::raw::c_char,
    pub mappings: *const ctex_paint_smear_mapping_descriptor,
    pub mapping_count: usize,
}
impl Default for ctex_paint_smear_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_smear_info {
    pub size: u32,
    pub resolved_strength: f64,
    pub strength_clamped: u32,
    pub resolved_footprint_radius_x: u32,
    pub resolved_footprint_radius_y: u32,
    pub footprint_radius_x_clamped: u32,
    pub footprint_radius_y_clamped: u32,
    pub applied_channel_count: usize,
    pub required_pixels_per_channel: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_stencil_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub screen_positions: *const ctex_vec2d,
    pub screen_position_count: usize,
    pub image_width: u32,
    pub image_height: u32,
    pub image_opacity: *const f64,
    pub image_opacity_count: usize,
    pub position: ctex_vec2d,
    pub rotation_radians: f64,
    pub scale: ctex_vec2d,
    pub inverted: u32,
}
impl Default for ctex_paint_stencil_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_stencil_info {
    pub size: u32,
    pub resolved_position: ctex_vec2d,
    pub resolved_rotation_radians: f64,
    pub resolved_scale: ctex_vec2d,
    pub inverted: u32,
    pub position_x_clamped: u32,
    pub position_y_clamped: u32,
    pub rotation_clamped: u32,
    pub scale_x_clamped: u32,
    pub scale_y_clamped: u32,
    pub required_mask_value_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_decal_transform {
    pub rotation_radians: f64,
    pub uniform_scale: f64,
    pub axis_scale: ctex_vec2d,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_decal_placement {
    pub position: ctex_vec3d,
    pub surface_normal: ctex_vec3d,
    pub transform: ctex_paint_decal_transform,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_decal_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub surface_texels: *const ctex_paint_surface_texel,
    pub surface_texel_count: usize,
    pub coverage: *const u8,
    pub coverage_count: usize,
    pub placement: ctex_paint_decal_placement,
    pub material_width: u32,
    pub material_height: u32,
    pub material: *const ctex_paint_tool_channel_descriptor,
    pub material_channel_count: usize,
    pub material_opacity: *const f64,
    pub material_opacity_count: usize,
    pub enabled_layer_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub enabled_layer_channel_count: usize,
    pub masks: *const ctex_paint_mask_inputs_descriptor,
    pub rejection_acceptance: *const f64,
    pub rejection_acceptance_count: usize,
    pub blend_mode: *const ::std::os::raw::c_char,
}
impl Default for ctex_paint_decal_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_decal_info {
    pub size: u32,
    pub resolved_placement: ctex_paint_decal_placement,
    pub frame_tangent: ctex_vec3d,
    pub frame_bitangent: ctex_vec3d,
    pub frame_scale: ctex_vec2d,
    pub rotation_clamped: u32,
    pub uniform_scale_clamped: u32,
    pub axis_scale_x_clamped: u32,
    pub axis_scale_y_clamped: u32,
    pub applied_channel_count: usize,
    pub required_pixels_per_channel: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_decal_outputs {
    pub size: u32,
    pub source_sample_indices: *mut usize,
    pub source_sample_capacity: usize,
    pub strength: *mut f64,
    pub strength_capacity: usize,
    pub channels: *const ctex_paint_tool_channel_output,
    pub channel_count: usize,
}
impl Default for ctex_paint_decal_outputs {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_paint_projection_mode_CTEX_PAINT_PROJECTION_CAMERA: ctex_paint_projection_mode = 0;
pub const ctex_paint_projection_mode_CTEX_PAINT_PROJECTION_PLANAR: ctex_paint_projection_mode = 1;
pub const ctex_paint_projection_mode_CTEX_PAINT_PROJECTION_TRIPLANAR: ctex_paint_projection_mode =
    2;
pub type ctex_paint_projection_mode = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_projection_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub surface_texels: *const ctex_paint_surface_texel,
    pub surface_texel_count: usize,
    pub coverage: *const u8,
    pub coverage_count: usize,
    pub mode: u32,
    pub camera_view_projection: [f32; 16usize],
    pub camera_visible_surface: *const f64,
    pub camera_visible_surface_count: usize,
    pub planar_origin: ctex_vec3d,
    pub planar_u_axis: ctex_vec3d,
    pub planar_v_axis: ctex_vec3d,
    pub planar_extent: ctex_vec2d,
    pub triplanar_scale: f64,
    pub triplanar_offset: ctex_vec2d,
    pub material_width: u32,
    pub material_height: u32,
    pub material: *const ctex_paint_tool_channel_descriptor,
    pub material_channel_count: usize,
    pub material_opacity: *const f64,
    pub material_opacity_count: usize,
    pub enabled_layer_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub enabled_layer_channel_count: usize,
    pub masks: *const ctex_paint_mask_inputs_descriptor,
    pub rejection_acceptance: *const f64,
    pub rejection_acceptance_count: usize,
    pub blend_mode: *const ::std::os::raw::c_char,
}
impl Default for ctex_paint_projection_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_projection_sample {
    pub source_indices: [usize; 3usize],
    pub weights: [f64; 3usize],
    pub count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_projection_info {
    pub size: u32,
    pub resolved_mode: u32,
    pub resolved_planar_extent: ctex_vec2d,
    pub resolved_triplanar_scale: f64,
    pub resolved_triplanar_offset: ctex_vec2d,
    pub planar_extent_x_clamped: u32,
    pub planar_extent_y_clamped: u32,
    pub triplanar_scale_clamped: u32,
    pub triplanar_offset_x_clamped: u32,
    pub triplanar_offset_y_clamped: u32,
    pub applied_channel_count: usize,
    pub required_sample_count: usize,
    pub required_pixels_per_channel: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_projection_outputs {
    pub size: u32,
    pub samples: *mut ctex_paint_projection_sample,
    pub sample_capacity: usize,
    pub strength: *mut f64,
    pub strength_capacity: usize,
    pub channels: *const ctex_paint_tool_channel_output,
    pub channel_count: usize,
}
impl Default for ctex_paint_projection_outputs {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_paint_text_alignment_CTEX_PAINT_TEXT_ALIGN_LEFT: ctex_paint_text_alignment = 0;
pub const ctex_paint_text_alignment_CTEX_PAINT_TEXT_ALIGN_CENTRE: ctex_paint_text_alignment = 1;
pub const ctex_paint_text_alignment_CTEX_PAINT_TEXT_ALIGN_RIGHT: ctex_paint_text_alignment = 2;
pub type ctex_paint_text_alignment = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_font_glyph_descriptor {
    pub size: u32,
    pub codepoint: u32,
    pub width: u32,
    pub height: u32,
    pub bearing_x: f64,
    pub bearing_y: f64,
    pub advance: f64,
    pub coverage: *const f64,
    pub coverage_count: usize,
}
impl Default for ctex_paint_font_glyph_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_font_descriptor {
    pub size: u32,
    pub identity: *const ::std::os::raw::c_char,
    pub pixels_per_em: f64,
    pub ascent: f64,
    pub descent: f64,
    pub line_gap: f64,
    pub glyphs: *const ctex_paint_font_glyph_descriptor,
    pub glyph_count: usize,
}
impl Default for ctex_paint_font_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_text_material_value {
    pub size: u32,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub component_count: u32,
    pub value: ctex_vec4f,
}
impl Default for ctex_paint_text_material_value {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_text_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub surface_texels: *const ctex_paint_surface_texel,
    pub surface_texel_count: usize,
    pub coverage: *const u8,
    pub coverage_count: usize,
    pub font: *const ctex_paint_font_descriptor,
    pub utf8: *const ::std::os::raw::c_char,
    pub utf8_size: usize,
    pub tracking_em: f64,
    pub alignment: u32,
    pub text_size: f64,
    pub placement: ctex_paint_decal_placement,
    pub material: *const ctex_paint_text_material_value,
    pub material_channel_count: usize,
    pub enabled_layer_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub enabled_layer_channel_count: usize,
    pub masks: *const ctex_paint_mask_inputs_descriptor,
    pub rejection_acceptance: *const f64,
    pub rejection_acceptance_count: usize,
    pub blend_mode: *const ::std::os::raw::c_char,
}
impl Default for ctex_paint_text_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_text_info {
    pub size: u32,
    pub resolved_tracking_em: f64,
    pub resolved_alignment: u32,
    pub resolved_text_size: f64,
    pub tracking_clamped: u32,
    pub text_size_clamped: u32,
    pub raster_width: u32,
    pub raster_height: u32,
    pub line_count: usize,
    pub width_em: f64,
    pub height_em: f64,
    pub resolved_placement: ctex_paint_decal_placement,
    pub frame_tangent: ctex_vec3d,
    pub frame_bitangent: ctex_vec3d,
    pub frame_scale: ctex_vec2d,
    pub rotation_clamped: u32,
    pub uniform_scale_clamped: u32,
    pub axis_scale_x_clamped: u32,
    pub axis_scale_y_clamped: u32,
    pub editable_revision: u64,
    pub applied_channel_count: usize,
    pub required_codepoint_count: usize,
    pub required_raster_opacity_count: usize,
    pub required_pixels_per_channel: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_text_outputs {
    pub size: u32,
    pub codepoints: *mut u32,
    pub codepoint_capacity: usize,
    pub raster_opacity: *mut f64,
    pub raster_opacity_capacity: usize,
    pub source_sample_indices: *mut usize,
    pub source_sample_capacity: usize,
    pub strength: *mut f64,
    pub strength_capacity: usize,
    pub channels: *const ctex_paint_tool_channel_output,
    pub channel_count: usize,
}
impl Default for ctex_paint_text_outputs {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_particle_settings {
    pub count: u32,
    pub lifetime_seconds: f64,
    pub initial_speed: f64,
    pub mass: f64,
    pub gravity: ctex_vec3d,
    pub friction: f64,
    pub restitution: f64,
    pub randomness: f64,
    pub seed: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_particle_contact {
    pub particle_ordinal: u32,
    pub collision_ordinal: u32,
    pub time_seconds: f64,
    pub position: ctex_vec3d,
    pub normal: ctex_vec3d,
    pub uv: ctex_vec2d,
    pub triangle: u32,
    pub impact_speed: f64,
    pub impulse: f64,
    pub strength: f64,
    pub texture_set_id_offset: usize,
    pub texture_set_id_size: usize,
    pub mapped_texel: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_particle_state {
    pub position: ctex_vec3d,
    pub velocity: ctex_vec3d,
    pub simulated_seconds: f64,
    pub collision_count: u32,
    pub resting: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_particle_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub tile_origin: ctex_vec2d,
    pub texture_set_id: *const ::std::os::raw::c_char,
    pub mesh_revision: u64,
    pub surface_texels: *const ctex_paint_surface_texel,
    pub surface_texel_count: usize,
    pub coverage: *const u8,
    pub coverage_count: usize,
    pub triangle_identity: *const u32,
    pub triangle_identity_count: usize,
    pub texture_sets: *const ctex_pick_texture_set_binding_descriptor,
    pub texture_set_count: usize,
    pub emitter_position: ctex_vec3d,
    pub emitter_direction: ctex_vec3d,
    pub simulation: ctex_paint_particle_settings,
    pub material: *const ctex_paint_tool_channel_descriptor,
    pub material_channel_count: usize,
    pub enabled_layer_snapshot: *const ctex_paint_tool_channel_descriptor,
    pub enabled_layer_channel_count: usize,
    pub masks: *const ctex_paint_mask_inputs_descriptor,
    pub rejection_acceptance: *const f64,
    pub rejection_acceptance_count: usize,
    pub blend_mode: *const ::std::os::raw::c_char,
}
impl Default for ctex_paint_particle_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_particle_info {
    pub size: u32,
    pub resolved_settings: ctex_paint_particle_settings,
    pub count_clamped: u32,
    pub lifetime_clamped: u32,
    pub initial_speed_clamped: u32,
    pub mass_clamped: u32,
    pub gravity_x_clamped: u32,
    pub gravity_y_clamped: u32,
    pub gravity_z_clamped: u32,
    pub friction_clamped: u32,
    pub restitution_clamped: u32,
    pub randomness_clamped: u32,
    pub emitted_count: u32,
    pub mapped_contact_count: usize,
    pub applied_channel_count: usize,
    pub required_contact_count: usize,
    pub required_final_state_count: usize,
    pub required_texture_set_id_size: usize,
    pub required_pixels_per_channel: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_particle_outputs {
    pub size: u32,
    pub contacts: *mut ctex_paint_particle_contact,
    pub contact_capacity: usize,
    pub final_states: *mut ctex_paint_particle_state,
    pub final_state_capacity: usize,
    pub texture_set_ids: *mut ::std::os::raw::c_char,
    pub texture_set_id_size: usize,
    pub strength: *mut f64,
    pub strength_capacity: usize,
    pub channels: *const ctex_paint_tool_channel_output,
    pub channel_count: usize,
}
impl Default for ctex_paint_particle_outputs {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_stroke_preset_info {
    pub size: u32,
    pub schema_version: u32,
    pub required_name_size: usize,
    pub required_tip_resource_identity_size: usize,
    pub required_curve_point_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_stroke_preset_buffers_descriptor {
    pub size: u32,
    pub name_buffer: *mut ::std::os::raw::c_char,
    pub name_buffer_size: usize,
    pub tip_resource_identity_buffer: *mut ::std::os::raw::c_char,
    pub tip_resource_identity_buffer_size: usize,
    pub curve_points: *mut ctex_response_curve_point,
    pub curve_point_capacity: usize,
}
impl Default for ctex_stroke_preset_buffers_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_uv_set_descriptor {
    pub size: u32,
    pub name: *const ::std::os::raw::c_char,
    pub values: *const ctex_vec2f,
    pub value_count: usize,
}
impl Default for ctex_uv_set_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_partition_descriptor {
    pub size: u32,
    pub kind: u32,
    pub stable_key: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
}
impl Default for ctex_mesh_partition_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_descriptor {
    pub size: u32,
    pub positions: *const ctex_vec3f,
    pub position_count: usize,
    pub normals: *const ctex_vec3f,
    pub normal_count: usize,
    pub vertex_colors: *const ctex_vec4f,
    pub vertex_color_count: usize,
    pub triangle_indices: *const u32,
    pub triangle_index_count: usize,
    pub uv_sets: *const ctex_uv_set_descriptor,
    pub uv_set_count: usize,
    pub default_uv_set: *const ::std::os::raw::c_char,
    pub partitions: *const ctex_mesh_partition_descriptor,
    pub partition_count: usize,
    pub face_partition_indices: *const u32,
    pub face_partition_index_count: usize,
    pub face_material_ids: *const u32,
    pub face_material_id_count: usize,
}
impl Default for ctex_mesh_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_info {
    pub size: u32,
    pub vertex_count: usize,
    pub triangle_count: usize,
    pub uv_set_count: usize,
    pub partition_count: usize,
    pub has_vertex_colors: u32,
    pub revision: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_uv_overlap_info {
    pub size: u32,
    pub required_face_count: usize,
    pub overlap_pair_count: usize,
    pub candidate_pair_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_uv_coverage_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub selected_face_count: usize,
    pub covered_texel_count: usize,
    pub uncovered_texel_count: usize,
    pub tested_texel_count: usize,
    pub required_outside_face_count: usize,
    pub uncovered_fraction: f64,
}
pub const ctex_mesh_uv_change_CTEX_MESH_UV_UNCHANGED: ctex_mesh_uv_change = 0;
pub const ctex_mesh_uv_change_CTEX_MESH_UV_CHANGED: ctex_mesh_uv_change = 1;
pub const ctex_mesh_uv_change_CTEX_MESH_SOURCE_PARTITION_MISSING: ctex_mesh_uv_change = 2;
pub const ctex_mesh_uv_change_CTEX_MESH_REPLACEMENT_PARTITION_MISSING: ctex_mesh_uv_change = 3;
pub const ctex_mesh_uv_change_CTEX_MESH_UV_SET_MISSING: ctex_mesh_uv_change = 4;
pub type ctex_mesh_uv_change = ::std::os::raw::c_uint;
pub const ctex_mesh_replacement_policy_CTEX_MESH_REPLACEMENT_KEEP_TEXELS:
    ctex_mesh_replacement_policy = 0;
pub const ctex_mesh_replacement_policy_CTEX_MESH_REPLACEMENT_REQUEST_REPROJECTION:
    ctex_mesh_replacement_policy = 1;
pub const ctex_mesh_replacement_policy_CTEX_MESH_REPLACEMENT_CLEAR: ctex_mesh_replacement_policy =
    2;
pub type ctex_mesh_replacement_policy = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_replacement_entry {
    pub uv_change: u32,
    pub source_partition_index: u32,
    pub replacement_partition_index: u32,
    pub source_face_count: usize,
    pub replacement_face_count: usize,
    pub texture_set_id_offset: usize,
    pub texture_set_id_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_replacement_plan_info {
    pub size: u32,
    pub source_mesh_revision: u64,
    pub texture_set_count: usize,
    pub changed_texture_set_count: usize,
    pub required_texture_set_id_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_replacement_decision {
    pub size: u32,
    pub texture_set_id: *const ::std::os::raw::c_char,
    pub policy: u32,
}
impl Default for ctex_mesh_replacement_decision {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_replacement_apply_info {
    pub size: u32,
    pub replacement_applied: u32,
    pub kept_texture_set_count: usize,
    pub cleared_texture_set_count: usize,
    pub reprojection_pending_texture_set_count: usize,
    pub replacement_mesh_revision: u64,
}
pub const ctex_mesh_reprojection_hole_policy_CTEX_MESH_REPROJECTION_RETAIN_TARGET:
    ctex_mesh_reprojection_hole_policy = 0;
pub const ctex_mesh_reprojection_hole_policy_CTEX_MESH_REPROJECTION_CHANNEL_DEFAULT:
    ctex_mesh_reprojection_hole_policy = 1;
pub type ctex_mesh_reprojection_hole_policy = ::std::os::raw::c_uint;
pub const ctex_mesh_reprojection_ambiguity_policy_CTEX_MESH_REPROJECTION_REFUSE_AMBIGUITY:
    ctex_mesh_reprojection_ambiguity_policy = 0;
pub const ctex_mesh_reprojection_ambiguity_policy_CTEX_MESH_REPROJECTION_NEAREST_LOWEST_TRIANGLE:
    ctex_mesh_reprojection_ambiguity_policy = 1;
pub type ctex_mesh_reprojection_ambiguity_policy = ::std::os::raw::c_uint;
pub type ctex_mesh_reprojection_cancel_callback =
    ::std::option::Option<unsafe extern "C" fn(user_data: *mut ::std::os::raw::c_void) -> u32>;
pub type ctex_mesh_reprojection_progress_callback = ::std::option::Option<
    unsafe extern "C" fn(completed_work_items: usize, user_data: *mut ::std::os::raw::c_void),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_reprojection_descriptor {
    pub size: u32,
    pub maximum_distance: f64,
    pub maximum_normal_angle_radians: f64,
    pub require_visibility: u32,
    pub visibility_epsilon: f64,
    pub ambiguity_distance_epsilon: f64,
    pub maximum_work_items: usize,
    pub progress_interval: usize,
    pub user_data: *mut ::std::os::raw::c_void,
    pub is_cancelled: ctex_mesh_reprojection_cancel_callback,
    pub report_progress: ctex_mesh_reprojection_progress_callback,
}
impl Default for ctex_mesh_reprojection_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_reprojection_preflight_info {
    pub size: u32,
    pub source_mesh_revision: u64,
    pub replacement_mesh_revision: u64,
    pub mapped_texel_count: usize,
    pub unmapped_texel_count: usize,
    pub ambiguous_texel_count: usize,
    pub affected_entry_count: usize,
    pub tested_candidate_count: usize,
    pub required_mapping_json_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_reprojection_commit_info {
    pub size: u32,
    pub replacement_mesh_revision: u64,
    pub reprojected_texel_count: usize,
    pub retained_hole_count: usize,
    pub defaulted_hole_count: usize,
    pub resolved_ambiguity_count: usize,
    pub transformed_tangent_normal_count: usize,
    pub reprojected_entry_count: usize,
}
pub const ctex_pick_occlusion_policy_CTEX_PICK_OCCLUSION_NEAREST: ctex_pick_occlusion_policy = 0;
pub const ctex_pick_occlusion_policy_CTEX_PICK_OCCLUSION_ALL_HITS: ctex_pick_occlusion_policy = 1;
pub type ctex_pick_occlusion_policy = ::std::os::raw::c_uint;
pub const ctex_pick_backface_policy_CTEX_PICK_BACKFACE_ACCEPT: ctex_pick_backface_policy = 0;
pub const ctex_pick_backface_policy_CTEX_PICK_BACKFACE_REJECT: ctex_pick_backface_policy = 1;
pub type ctex_pick_backface_policy = ::std::os::raw::c_uint;
pub const ctex_pick_projection_kind_CTEX_PICK_PROJECTION_PERSPECTIVE: ctex_pick_projection_kind = 0;
pub const ctex_pick_projection_kind_CTEX_PICK_PROJECTION_ORTHOGRAPHIC: ctex_pick_projection_kind =
    1;
pub type ctex_pick_projection_kind = ::std::os::raw::c_uint;
pub const ctex_pick_batch_status_CTEX_PICK_BATCH_COMPLETE: ctex_pick_batch_status = 0;
pub const ctex_pick_batch_status_CTEX_PICK_BATCH_CANCELLED: ctex_pick_batch_status = 1;
pub const ctex_pick_batch_status_CTEX_PICK_BATCH_MEMORY_CEILING_EXCEEDED: ctex_pick_batch_status =
    2;
pub type ctex_pick_batch_status = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_pick_ray {
    pub origin: ctex_vec3f,
    pub direction: ctex_vec3f,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_pick_texture_set_binding_descriptor {
    pub size: u32,
    pub partition_index: u32,
    pub uv_set: *const ::std::os::raw::c_char,
}
impl Default for ctex_pick_texture_set_binding_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_pick_options_descriptor {
    pub size: u32,
    pub maximum_distance: f32,
    pub occlusion_policy: u32,
    pub backface_policy: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_pick_screen_view_descriptor {
    pub size: u32,
    pub viewport_width: u32,
    pub viewport_height: u32,
    pub view: [f32; 16usize],
    pub projection: [f32; 16usize],
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_pick_hit {
    pub has_hit: u32,
    pub position: ctex_vec3f,
    pub interpolated_normal: ctex_vec3f,
    pub geometric_normal: ctex_vec3f,
    pub uv: ctex_vec2f,
    pub udim_u: i32,
    pub udim_v: i32,
    pub udim_number: i64,
    pub triangle_index: u32,
    pub barycentric: ctex_vec3f,
    pub material_id: u32,
    pub distance: f32,
    pub texture_set_id_offset: usize,
    pub texture_set_id_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_pick_index_info {
    pub size: u32,
    pub mesh_revision: u64,
    pub build_count: usize,
    pub node_count: usize,
    pub triangle_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_pick_query_info {
    pub size: u32,
    pub result_count: usize,
    pub required_texture_set_id_size: usize,
    pub visited_nodes: usize,
    pub tested_leaf_triangles: usize,
    pub index_build_count: usize,
    pub mesh_revision: u64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_picker_texture_view_descriptor {
    pub size: u32,
    pub texture_set_id: *const ::std::os::raw::c_char,
    pub tile_origin: ctex_vec2d,
    pub width: u32,
    pub height: u32,
    pub enabled_channels: *const ctex_paint_tool_channel_descriptor,
    pub enabled_channel_count: usize,
    pub material_identities: *const *const ::std::os::raw::c_char,
    pub material_identity_count: usize,
}
impl Default for ctex_paint_picker_texture_view_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_picker_descriptor {
    pub size: u32,
    pub hit: ctex_pick_hit,
    pub hit_texture_set_id: *const ::std::os::raw::c_char,
    pub texture_views: *const ctex_paint_picker_texture_view_descriptor,
    pub texture_view_count: usize,
}
impl Default for ctex_paint_picker_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_picker_channel_value {
    pub component_count: u32,
    pub value: ctex_vec4f,
    pub semantic_id_offset: usize,
    pub semantic_id_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_picker_info {
    pub size: u32,
    pub tile_origin: ctex_vec2d,
    pub uv: ctex_vec2d,
    pub texel: usize,
    pub has_material_identity: u32,
    pub texture_set_id_offset: usize,
    pub texture_set_id_size: usize,
    pub material_identity_offset: usize,
    pub material_identity_size: usize,
    pub required_channel_count: usize,
    pub required_string_size: usize,
}
pub const CTEX_PAINT_COLOUR_ID_SELECTION_EMPTY: _bindgen_ty_1 = 0;
pub const CTEX_PAINT_COLOUR_ID_SELECTION_MATCHED: _bindgen_ty_1 = 1;
pub type _bindgen_ty_1 = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_colour_id_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub pixels: *const ctex_vec4f,
    pub pixel_count: usize,
    pub picked_colour: ctex_vec4f,
    pub tolerance: f64,
}
impl Default for ctex_paint_colour_id_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_colour_id_info {
    pub size: u32,
    pub resolved_tolerance: f64,
    pub tolerance_clamped: u32,
    pub status: u32,
    pub selected_texel_count: usize,
    pub required_value_count: usize,
}
pub const ctex_paint_parameter_context_CTEX_PAINT_PARAMETER_CONTEXT_GENERAL:
    ctex_paint_parameter_context = 0;
pub const ctex_paint_parameter_context_CTEX_PAINT_PARAMETER_CONTEXT_TAPER_DISABLED:
    ctex_paint_parameter_context = 1;
pub const ctex_paint_parameter_context_CTEX_PAINT_PARAMETER_CONTEXT_TAPER_STAMP_COUNT:
    ctex_paint_parameter_context = 2;
pub const ctex_paint_parameter_context_CTEX_PAINT_PARAMETER_CONTEXT_TAPER_DISTANCE:
    ctex_paint_parameter_context = 3;
pub const ctex_paint_parameter_context_CTEX_PAINT_PARAMETER_CONTEXT_ALPHA_UNORM8:
    ctex_paint_parameter_context = 4;
pub const ctex_paint_parameter_context_CTEX_PAINT_PARAMETER_CONTEXT_ALPHA_HIGH_PRECISION:
    ctex_paint_parameter_context = 5;
pub type ctex_paint_parameter_context = ::std::os::raw::c_uint;
pub const ctex_paint_parameter_value_kind_CTEX_PAINT_PARAMETER_CONTINUOUS:
    ctex_paint_parameter_value_kind = 0;
pub const ctex_paint_parameter_value_kind_CTEX_PAINT_PARAMETER_INTEGER:
    ctex_paint_parameter_value_kind = 1;
pub type ctex_paint_parameter_value_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_parameter_descriptor {
    pub size: u32,
    pub context: u32,
    pub value_kind: u32,
    pub default_value: f64,
    pub minimum: f64,
    pub maximum: f64,
    pub name_offset: usize,
    pub name_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_parameter_catalogue_info {
    pub size: u32,
    pub required_parameter_count: usize,
    pub required_name_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_parameter_validation_info {
    pub size: u32,
    pub supplied: f64,
    pub resolved: f64,
    pub clamped: u32,
}
pub const ctex_paint_selection_kind_CTEX_PAINT_SELECTION_SCREEN_RECTANGLE:
    ctex_paint_selection_kind = 0;
pub const ctex_paint_selection_kind_CTEX_PAINT_SELECTION_SCREEN_LASSO: ctex_paint_selection_kind =
    1;
pub const ctex_paint_selection_kind_CTEX_PAINT_SELECTION_POLYGON_TRIANGLE:
    ctex_paint_selection_kind = 2;
pub const ctex_paint_selection_kind_CTEX_PAINT_SELECTION_POLYGON_UV_ISLAND:
    ctex_paint_selection_kind = 3;
pub const ctex_paint_selection_kind_CTEX_PAINT_SELECTION_POLYGON_CONNECTED_BY_ANGLE:
    ctex_paint_selection_kind = 4;
pub type ctex_paint_selection_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_selection_surface_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub tile_origin: ctex_vec2d,
    pub texture_set_id: *const ::std::os::raw::c_char,
    pub uv_set: *const ::std::os::raw::c_char,
    pub mesh_revision: u64,
    pub surface_texels: *const ctex_paint_surface_texel,
    pub surface_texel_count: usize,
    pub coverage: *const u8,
    pub coverage_count: usize,
    pub triangle_identity: *const u32,
    pub triangle_identity_count: usize,
    pub uv_island_identity: *const u32,
    pub uv_island_identity_count: usize,
}
impl Default for ctex_paint_selection_surface_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_screen_selection_descriptor {
    pub size: u32,
    pub kind: u32,
    pub surface: *const ctex_paint_selection_surface_descriptor,
    pub minimum: ctex_vec2f,
    pub maximum: ctex_vec2f,
    pub lasso_points: *const ctex_vec2f,
    pub lasso_point_count: usize,
    pub view: ctex_pick_screen_view_descriptor,
}
impl Default for ctex_paint_screen_selection_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_polygon_selection_descriptor {
    pub size: u32,
    pub kind: u32,
    pub surface: *const ctex_paint_selection_surface_descriptor,
    pub picked_texel: usize,
    pub maximum_angle_degrees: f64,
    pub triangle_topology: *const ctex_paint_fill_triangle_topology,
    pub triangle_topology_count: usize,
}
impl Default for ctex_paint_polygon_selection_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_paint_selection_info {
    pub size: u32,
    pub kind: u32,
    pub resolved_maximum_angle_degrees: f64,
    pub maximum_angle_clamped: u32,
    pub selected_texel_count: usize,
    pub selected_triangle_count: usize,
    pub visited_nodes: usize,
    pub tested_leaf_triangles: usize,
    pub required_value_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_paint_selection_outputs {
    pub size: u32,
    pub values: *mut f64,
    pub value_capacity: usize,
    pub selected_triangle_ids: *mut u32,
    pub selected_triangle_capacity: usize,
}
impl Default for ctex_paint_selection_outputs {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type ctex_pick_cancel_callback =
    ::std::option::Option<unsafe extern "C" fn(user_data: *mut ::std::os::raw::c_void) -> u32>;
pub type ctex_pick_progress_callback = ::std::option::Option<
    unsafe extern "C" fn(
        completed_rays: usize,
        total_rays: usize,
        user_data: *mut ::std::os::raw::c_void,
    ),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_pick_batch_control_descriptor {
    pub size: u32,
    pub memory_ceiling_bytes: usize,
    pub progress_interval: usize,
    pub user_data: *mut ::std::os::raw::c_void,
    pub is_cancelled: ctex_pick_cancel_callback,
    pub report_progress: ctex_pick_progress_callback,
}
impl Default for ctex_pick_batch_control_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_pick_batch_info {
    pub size: u32,
    pub status: u32,
    pub processed_rays: usize,
    pub required_memory_bytes: usize,
    pub required_hit_count: usize,
    pub required_texture_set_id_size: usize,
    pub visited_nodes: usize,
    pub tested_leaf_triangles: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_set_descriptor {
    pub size: u32,
    pub display_name: *const ::std::os::raw::c_char,
    pub partition_kind: u32,
    pub partition_key: *const ::std::os::raw::c_char,
    pub uv_set: *const ::std::os::raw::c_char,
    pub width: u32,
    pub height: u32,
    pub default_bit_depth: u8,
    pub udim_tiling: u32,
}
impl Default for ctex_texture_set_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_udim_pixel_write_descriptor {
    pub size: u32,
    pub u: f64,
    pub v: f64,
    pub pixel: *const ::std::os::raw::c_void,
    pub pixel_size: usize,
}
impl Default for ctex_udim_pixel_write_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_udim_write_info {
    pub size: u32,
    pub changed_tile_count: usize,
    pub allocated_tile_count: usize,
    pub changed_pixel_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_atlas_region_descriptor {
    pub size: u32,
    pub texture_set_id: *const ::std::os::raw::c_char,
    pub x: u32,
    pub y: u32,
    pub width: u32,
    pub height: u32,
}
impl Default for ctex_atlas_region_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_atlas_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub width: u32,
    pub height: u32,
    pub regions: *const ctex_atlas_region_descriptor,
    pub region_count: usize,
}
impl Default for ctex_atlas_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_atlas_region {
    pub texture_set_id_offset: usize,
    pub texture_set_id_size: usize,
    pub x: u32,
    pub y: u32,
    pub width: u32,
    pub height: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_atlas_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub region_count: usize,
    pub required_display_name_size: usize,
    pub required_texture_set_id_size: usize,
}
pub const ctex_layer_entry_kind_CTEX_LAYER_ENTRY_PAINT: ctex_layer_entry_kind = 0;
pub const ctex_layer_entry_kind_CTEX_LAYER_ENTRY_FILL: ctex_layer_entry_kind = 1;
pub const ctex_layer_entry_kind_CTEX_LAYER_ENTRY_GROUP: ctex_layer_entry_kind = 2;
pub const ctex_layer_entry_kind_CTEX_LAYER_ENTRY_MASK: ctex_layer_entry_kind = 3;
pub const ctex_layer_entry_kind_CTEX_LAYER_ENTRY_FILTER: ctex_layer_entry_kind = 4;
pub const ctex_layer_entry_kind_CTEX_LAYER_ENTRY_INSTANCE: ctex_layer_entry_kind = 5;
pub const ctex_layer_entry_kind_CTEX_LAYER_ENTRY_EDITABLE_DECAL: ctex_layer_entry_kind = 6;
pub const ctex_layer_entry_kind_CTEX_LAYER_ENTRY_EDITABLE_TEXT: ctex_layer_entry_kind = 7;
pub const ctex_layer_entry_kind_CTEX_LAYER_ENTRY_SURFACE_PATH: ctex_layer_entry_kind = 8;
pub type ctex_layer_entry_kind = ::std::os::raw::c_uint;
pub const ctex_layer_source_deletion_policy_CTEX_LAYER_SOURCE_DELETION_REFUSE:
    ctex_layer_source_deletion_policy = 0;
pub const ctex_layer_source_deletion_policy_CTEX_LAYER_SOURCE_DELETION_MAKE_INSTANCES_INDEPENDENT : ctex_layer_source_deletion_policy = 1 ;
pub type ctex_layer_source_deletion_policy = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_layer_channel_descriptor {
    pub size: u32,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub enabled: u32,
    pub opacity: f64,
}
impl Default for ctex_layer_channel_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_layer_entry_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub kind: u32,
    pub parent_identifier: *const ::std::os::raw::c_char,
    pub target_identifier: *const ::std::os::raw::c_char,
    pub source_identifier: *const ::std::os::raw::c_char,
    pub enabled: u32,
    pub opacity: f64,
    pub blend_mode: *const ::std::os::raw::c_char,
    pub channels: *const ctex_layer_channel_descriptor,
    pub channel_count: usize,
}
impl Default for ctex_layer_entry_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_layer_mask_sample {
    pub size: u32,
    pub mask_identifier: *const ::std::os::raw::c_char,
    pub value: f64,
}
impl Default for ctex_layer_mask_sample {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layer_participation_info {
    pub size: u32,
    pub participates: u32,
    pub effective_opacity: f64,
    pub mask_count: usize,
    pub required_mask_id_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_tile_history_target_descriptor {
    pub size: u32,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub tile_x: u32,
    pub tile_y: u32,
}
impl Default for ctex_tile_history_target_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_channel_region_descriptor {
    pub size: u32,
    pub x: u32,
    pub y: u32,
    pub width: u32,
    pub height: u32,
    pub row_pitch_bytes: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_tile_history_budget_report {
    pub size: u32,
    pub budget_bytes: usize,
    pub retained_bytes: usize,
    pub available_bytes: usize,
    pub proposed_step_bytes: usize,
    pub additional_steps_at_proposed_size: usize,
    pub undo_steps: usize,
    pub redo_steps: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_tile_history_commit_info {
    pub size: u32,
    pub committed: u32,
    pub tile_count: usize,
    pub retained_bytes: usize,
    pub layer_stack_changed: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_tile_history_restore_info {
    pub size: u32,
    pub tile_count: usize,
    pub exchanged_storage_count: usize,
    pub copied_pixel_bytes: usize,
    pub layer_stack_exchanged: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_layer_composite_raster_descriptor {
    pub size: u32,
    pub entry_identifier: *const ::std::os::raw::c_char,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub width: u32,
    pub height: u32,
    pub pixels: *const ctex_vec4f,
    pub pixel_count: usize,
    pub coverage: *const f32,
    pub coverage_count: usize,
}
impl Default for ctex_layer_composite_raster_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_layer_composite_mask_descriptor {
    pub size: u32,
    pub mask_identifier: *const ::std::os::raw::c_char,
    pub width: u32,
    pub height: u32,
    pub values: *const f64,
    pub value_count: usize,
}
impl Default for ctex_layer_composite_mask_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layer_composite_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub channel_count: usize,
    pub required_semantic_id_size: usize,
    pub required_pixel_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layer_composite_channel_info {
    pub component_count: u32,
    pub semantic_id_offset: usize,
    pub semantic_id_size: usize,
    pub pixel_offset: usize,
    pub pixel_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layer_snapshot_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub content_count: usize,
    pub mask_count: usize,
    pub required_string_size: usize,
    pub required_pixel_count: usize,
    pub required_coverage_count: usize,
    pub required_mask_value_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layer_snapshot_content_info {
    pub width: u32,
    pub height: u32,
    pub entry_identifier_offset: usize,
    pub entry_identifier_size: usize,
    pub semantic_id_offset: usize,
    pub semantic_id_size: usize,
    pub pixel_offset: usize,
    pub pixel_count: usize,
    pub coverage_offset: usize,
    pub coverage_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layer_snapshot_mask_info {
    pub width: u32,
    pub height: u32,
    pub mask_identifier_offset: usize,
    pub mask_identifier_size: usize,
    pub value_offset: usize,
    pub value_count: usize,
}
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_CREATE: ctex_layer_operation_kind = 0;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_DUPLICATE: ctex_layer_operation_kind = 1;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_DELETE: ctex_layer_operation_kind = 2;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_REORDER: ctex_layer_operation_kind = 3;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_REPARENT: ctex_layer_operation_kind = 4;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_CLEAR: ctex_layer_operation_kind = 5;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_INVERT: ctex_layer_operation_kind = 6;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_MERGE_DOWN: ctex_layer_operation_kind = 7;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_MERGE_GROUP: ctex_layer_operation_kind = 8;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_FLATTEN: ctex_layer_operation_kind = 9;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_CONVERT: ctex_layer_operation_kind = 10;
pub const ctex_layer_operation_kind_CTEX_LAYER_OPERATION_APPLY_MASK: ctex_layer_operation_kind = 11;
pub type ctex_layer_operation_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_layer_operation_descriptor {
    pub size: u32,
    pub kind: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub duplicate_identifier: *const ::std::os::raw::c_char,
    pub parent_identifier: *const ::std::os::raw::c_char,
    pub before_identifier: *const ::std::os::raw::c_char,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub entry: *const ctex_layer_entry_descriptor,
    pub replacement_content: *const ctex_layer_composite_raster_descriptor,
    pub replacement_content_count: usize,
    pub replacement_masks: *const ctex_layer_composite_mask_descriptor,
    pub replacement_mask_count: usize,
    pub source_deletion_policy: u32,
    pub target_kind: u32,
    pub graph_serialized: *const ::std::os::raw::c_void,
    pub graph_serialized_size: usize,
    pub maximum_output_bytes: usize,
    pub appearance_tolerance: f32,
}
impl Default for ctex_layer_operation_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layer_operation_info {
    pub size: u32,
    pub affected_count: usize,
    pub required_affected_id_size: usize,
}
pub const ctex_scalar_representation_CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED:
    ctex_scalar_representation = 0;
pub const ctex_scalar_representation_CTEX_SCALAR_REPRESENTATION_FLOATING_POINT:
    ctex_scalar_representation = 1;
pub type ctex_scalar_representation = ::std::os::raw::c_uint;
pub const ctex_image_file_format_CTEX_IMAGE_FILE_FORMAT_UNKNOWN: ctex_image_file_format = 0;
pub const ctex_image_file_format_CTEX_IMAGE_FILE_FORMAT_PNG: ctex_image_file_format = 1;
pub const ctex_image_file_format_CTEX_IMAGE_FILE_FORMAT_JPEG: ctex_image_file_format = 2;
pub const ctex_image_file_format_CTEX_IMAGE_FILE_FORMAT_BMP: ctex_image_file_format = 3;
pub const ctex_image_file_format_CTEX_IMAGE_FILE_FORMAT_TIFF: ctex_image_file_format = 4;
pub const ctex_image_file_format_CTEX_IMAGE_FILE_FORMAT_OPENEXR: ctex_image_file_format = 5;
pub const ctex_image_file_format_CTEX_IMAGE_FILE_FORMAT_RADIANCE_HDR: ctex_image_file_format = 6;
pub const ctex_image_file_format_CTEX_IMAGE_FILE_FORMAT_PSD: ctex_image_file_format = 7;
pub const ctex_image_file_format_CTEX_IMAGE_FILE_FORMAT_TGA: ctex_image_file_format = 8;
pub type ctex_image_file_format = ::std::os::raw::c_uint;
pub const ctex_color_space_source_CTEX_COLOR_SPACE_SOURCE_CALLER: ctex_color_space_source = 0;
pub const ctex_color_space_source_CTEX_COLOR_SPACE_SOURCE_EMBEDDED_SRGB: ctex_color_space_source =
    1;
pub const ctex_color_space_source_CTEX_COLOR_SPACE_SOURCE_AUTOMATIC_RULE: ctex_color_space_source =
    2;
pub const ctex_color_space_source_CTEX_COLOR_SPACE_SOURCE_EMBEDDED_PROFILE:
    ctex_color_space_source = 3;
pub type ctex_color_space_source = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_image_decode_limits_descriptor {
    pub size: u32,
    pub maximum_width: u32,
    pub maximum_height: u32,
    pub maximum_decoded_bytes: usize,
}
pub const ctex_image_decode_phase_CTEX_IMAGE_DECODE_PHASE_INSPECTION: ctex_image_decode_phase = 0;
pub const ctex_image_decode_phase_CTEX_IMAGE_DECODE_PHASE_CODEC: ctex_image_decode_phase = 1;
pub const ctex_image_decode_phase_CTEX_IMAGE_DECODE_PHASE_UNPACK: ctex_image_decode_phase = 2;
pub const ctex_image_decode_phase_CTEX_IMAGE_DECODE_PHASE_COMPLETE: ctex_image_decode_phase = 3;
pub type ctex_image_decode_phase = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_image_decode_progress_info {
    pub size: u32,
    pub phase: u32,
    pub completed_rows: u32,
    pub total_rows: u32,
    pub estimated_peak_working_bytes: usize,
}
pub type ctex_image_decode_cancel_callback =
    ::std::option::Option<unsafe extern "C" fn(user_data: *mut ::std::os::raw::c_void) -> u32>;
pub type ctex_image_decode_progress_callback = ::std::option::Option<
    unsafe extern "C" fn(
        user_data: *mut ::std::os::raw::c_void,
        progress: *const ctex_image_decode_progress_info,
    ),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_image_decode_control_descriptor {
    pub size: u32,
    pub maximum_working_bytes: usize,
    pub progress_interval_rows: u32,
    pub user_data: *mut ::std::os::raw::c_void,
    pub is_cancelled: ctex_image_decode_cancel_callback,
    pub report_progress: ctex_image_decode_progress_callback,
}
impl Default for ctex_image_decode_control_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_image_decode_execution_info {
    pub size: u32,
    pub estimated_peak_working_bytes: usize,
    pub progress_event_count: usize,
    pub cancelled: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_decoded_image_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub channel_count: u32,
    pub scalar_representation: u32,
    pub bit_depth: u32,
    pub color_space: u32,
    pub detected_format: u32,
    pub extension_mismatch: u32,
    pub color_space_source: u32,
    pub uninterpretable_profile: u32,
}
pub const ctex_layered_image_decode_mode_CTEX_LAYERED_IMAGE_DECODE_COMPOSITE:
    ctex_layered_image_decode_mode = 0;
pub const ctex_layered_image_decode_mode_CTEX_LAYERED_IMAGE_DECODE_INDIVIDUAL:
    ctex_layered_image_decode_mode = 1;
pub type ctex_layered_image_decode_mode = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layered_image_decode_descriptor {
    pub size: u32,
    pub mode: u32,
    pub intended_channel: u32,
    pub input_color_space: u32,
    pub maximum_image_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layered_image_decode_info {
    pub size: u32,
    pub detected_format: u32,
    pub source_was_layered: u32,
    pub image_count: usize,
    pub required_image_info_count: usize,
    pub required_name_buffer_size: usize,
    pub required_pixel_buffer_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_layered_decoded_image_info {
    pub size: u32,
    pub origin_x: i32,
    pub origin_y: i32,
    pub width: u32,
    pub height: u32,
    pub channel_count: u32,
    pub scalar_representation: u32,
    pub bit_depth: u32,
    pub color_space: u32,
    pub name_offset: usize,
    pub name_size: usize,
    pub pixel_offset: usize,
    pub pixel_size: usize,
}
pub const ctex_image_channel_expansion_rule_CTEX_IMAGE_CHANNEL_EXPANSION_IDENTITY:
    ctex_image_channel_expansion_rule = 0;
pub const ctex_image_channel_expansion_rule_CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_TO_RGB:
    ctex_image_channel_expansion_rule = 1;
pub const ctex_image_channel_expansion_rule_CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_TO_RGBA:
    ctex_image_channel_expansion_rule = 2;
pub const ctex_image_channel_expansion_rule_CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_ALPHA_TO_RGBA:
    ctex_image_channel_expansion_rule = 3;
pub const ctex_image_channel_expansion_rule_CTEX_IMAGE_CHANNEL_EXPANSION_RGB_TO_RGBA:
    ctex_image_channel_expansion_rule = 4;
pub type ctex_image_channel_expansion_rule = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_image_channel_expansion_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub source_channel_count: u32,
    pub scalar_representation: u32,
    pub bit_depth: u32,
    pub source_row_stride_bytes: usize,
    pub target_channel_count: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_image_channel_expansion_info {
    pub size: u32,
    pub channel_count: u32,
    pub scalar_representation: u32,
    pub bit_depth: u32,
    pub rule: u32,
    pub required_pixel_buffer_size: usize,
}
pub const ctex_image_resample_filter_CTEX_IMAGE_RESAMPLE_FILTER_DEFAULT:
    ctex_image_resample_filter = 0;
pub const ctex_image_resample_filter_CTEX_IMAGE_RESAMPLE_FILTER_NEAREST:
    ctex_image_resample_filter = 1;
pub const ctex_image_resample_filter_CTEX_IMAGE_RESAMPLE_FILTER_BILINEAR:
    ctex_image_resample_filter = 2;
pub type ctex_image_resample_filter = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_image_resample_descriptor {
    pub size: u32,
    pub source_width: u32,
    pub source_height: u32,
    pub channel_count: u32,
    pub scalar_representation: u32,
    pub bit_depth: u32,
    pub source_row_stride_bytes: usize,
    pub output_width: u32,
    pub output_height: u32,
    pub filter: u32,
    pub maximum_output_bytes: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_image_resample_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub channel_count: u32,
    pub scalar_representation: u32,
    pub bit_depth: u32,
    pub filter: u32,
    pub required_pixel_buffer_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_image_encode_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub channel_count: u32,
    pub scalar_representation: u32,
    pub input_bit_depth: u32,
    pub row_stride_bytes: usize,
    pub color_space: u32,
    pub output_format: u32,
    pub output_bit_depth: u32,
    pub jpeg_quality: u32,
}
pub const ctex_channel_classification_CTEX_CHANNEL_CLASSIFICATION_COLOR:
    ctex_channel_classification = 0;
pub const ctex_channel_classification_CTEX_CHANNEL_CLASSIFICATION_DATA:
    ctex_channel_classification = 1;
pub type ctex_channel_classification = ::std::os::raw::c_uint;
pub const ctex_blending_policy_CTEX_BLENDING_POLICY_COLOR: ctex_blending_policy = 0;
pub const ctex_blending_policy_CTEX_BLENDING_POLICY_SCALAR: ctex_blending_policy = 1;
pub const ctex_blending_policy_CTEX_BLENDING_POLICY_NORMAL_VECTOR: ctex_blending_policy = 2;
pub const ctex_blending_policy_CTEX_BLENDING_POLICY_ADDITIVE: ctex_blending_policy = 3;
pub type ctex_blending_policy = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_channel_descriptor {
    pub size: u32,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub component_count: u32,
    pub scalar_representation: u32,
    pub preferred_bit_depth: u32,
    pub default_value: [f64; 4usize],
    pub default_value_count: u32,
    pub classification: u32,
    pub blending_policy: u32,
    pub export_mapping: *const ::std::os::raw::c_char,
    pub evaluable: u32,
}
impl Default for ctex_channel_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_channel_info {
    pub size: u32,
    pub component_count: u32,
    pub scalar_representation: u32,
    pub preferred_bit_depth: u32,
    pub default_value: [f64; 4usize],
    pub default_value_count: u32,
    pub classification: u32,
    pub blending_policy: u32,
    pub evaluable: u32,
    pub enabled: u32,
    pub storage_bit_depth: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_texture_set_memory_report {
    pub size: u32,
    pub enabled_channel_count: usize,
    pub channel_pixel_bytes: usize,
    pub mesh_map_pixel_bytes: usize,
    pub total_resident_bytes: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_document_texture_set_memory_info {
    pub texture_set_id_offset: usize,
    pub texture_set_id_size: usize,
    pub channel_pixel_bytes: usize,
    pub history_retained_bytes: usize,
    pub mesh_map_pixel_bytes: usize,
    pub total_resident_bytes: usize,
    pub estimated_save_bytes: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_document_memory_info {
    pub size: u32,
    pub texture_set_count: usize,
    pub required_texture_set_id_size: usize,
    pub channel_pixel_bytes: usize,
    pub history_retained_bytes: usize,
    pub mesh_map_pixel_bytes: usize,
    pub total_resident_bytes: usize,
    pub estimated_save_bytes: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_transport_revision_cursor {
    pub epoch: u64,
    pub revision: u64,
}
pub const ctex_transport_delta_disposition_CTEX_TRANSPORT_DELTA_COMPLETE:
    ctex_transport_delta_disposition = 0;
pub const ctex_transport_delta_disposition_CTEX_TRANSPORT_FULL_RESYNCHRONIZATION_REQUIRED:
    ctex_transport_delta_disposition = 1;
pub type ctex_transport_delta_disposition = ::std::os::raw::c_uint;
pub const ctex_transport_tile_residency_CTEX_TRANSPORT_TILE_CPU: ctex_transport_tile_residency = 0;
pub const ctex_transport_tile_residency_CTEX_TRANSPORT_TILE_HOST_DEVICE:
    ctex_transport_tile_residency = 1;
pub type ctex_transport_tile_residency = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_transport_tile_version {
    pub x: u32,
    pub y: u32,
    pub revision: u64,
    pub generation: u64,
    pub residency: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_transport_delta_info {
    pub size: u32,
    pub disposition: u32,
    pub synchronized_cursor: ctex_transport_revision_cursor,
    pub current_cursor: ctex_transport_revision_cursor,
    pub changed_tile_count: usize,
    pub indexed_tiles_visited: usize,
}
pub const ctex_transport_component_type_CTEX_TRANSPORT_COMPONENT_UINT8_UNORM:
    ctex_transport_component_type = 0;
pub const ctex_transport_component_type_CTEX_TRANSPORT_COMPONENT_UINT16_UNORM:
    ctex_transport_component_type = 1;
pub const ctex_transport_component_type_CTEX_TRANSPORT_COMPONENT_FLOAT32:
    ctex_transport_component_type = 2;
pub type ctex_transport_component_type = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_transport_pixel_format {
    pub component_type: u32,
    pub channel_count: u32,
}
pub const ctex_transport_conversion_policy_CTEX_TRANSPORT_EXACT_FORMAT_ONLY:
    ctex_transport_conversion_policy = 0;
pub const ctex_transport_conversion_policy_CTEX_TRANSPORT_ALLOW_FORMAT_CONVERSION:
    ctex_transport_conversion_policy = 1;
pub type ctex_transport_conversion_policy = ::std::os::raw::c_uint;
pub const ctex_transport_format_conversion_CTEX_TRANSPORT_CONVERSION_NONE:
    ctex_transport_format_conversion = 0;
pub const ctex_transport_format_conversion_CTEX_TRANSPORT_CONVERSION_UINT8_TO_UINT16:
    ctex_transport_format_conversion = 1;
pub const ctex_transport_format_conversion_CTEX_TRANSPORT_CONVERSION_UINT8_TO_FLOAT32:
    ctex_transport_format_conversion = 2;
pub const ctex_transport_format_conversion_CTEX_TRANSPORT_CONVERSION_UINT16_TO_UINT8:
    ctex_transport_format_conversion = 3;
pub const ctex_transport_format_conversion_CTEX_TRANSPORT_CONVERSION_UINT16_TO_FLOAT32:
    ctex_transport_format_conversion = 4;
pub const ctex_transport_format_conversion_CTEX_TRANSPORT_CONVERSION_FLOAT32_TO_UINT8:
    ctex_transport_format_conversion = 5;
pub const ctex_transport_format_conversion_CTEX_TRANSPORT_CONVERSION_FLOAT32_TO_UINT16:
    ctex_transport_format_conversion = 6;
pub type ctex_transport_format_conversion = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_transport_format_selection {
    pub size: u32,
    pub source_format: ctex_transport_pixel_format,
    pub output_format: ctex_transport_pixel_format,
    pub conversion: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
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
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_transport_snapshot_memory_report {
    pub size: u32,
    pub budget_bytes: usize,
    pub pinned_bytes: usize,
    pub active_snapshots: usize,
    pub pinned_allocations: usize,
}
pub const ctex_resource_category_CTEX_RESOURCE_DOCUMENT_STORAGE: ctex_resource_category = 0;
pub const ctex_resource_category_CTEX_RESOURCE_HISTORY: ctex_resource_category = 1;
pub const ctex_resource_category_CTEX_RESOURCE_RECOVERY_RECORD: ctex_resource_category = 2;
pub const ctex_resource_category_CTEX_RESOURCE_MESH_MAP: ctex_resource_category = 3;
pub const ctex_resource_category_CTEX_RESOURCE_COMPOSITE: ctex_resource_category = 4;
pub const ctex_resource_category_CTEX_RESOURCE_CACHE: ctex_resource_category = 5;
pub const ctex_resource_category_CTEX_RESOURCE_TEMPORARY: ctex_resource_category = 6;
pub const ctex_resource_category_CTEX_RESOURCE_CATEGORY_COUNT: ctex_resource_category = 7;
pub type ctex_resource_category = ::std::os::raw::c_uint;
pub const ctex_resource_role_CTEX_RESOURCE_CPU_RESIDENT: ctex_resource_role = 1;
pub const ctex_resource_role_CTEX_RESOURCE_GPU_RESIDENT: ctex_resource_role = 2;
pub const ctex_resource_role_CTEX_RESOURCE_BACKING_STORE: ctex_resource_role = 4;
pub const ctex_resource_role_CTEX_RESOURCE_PINNED: ctex_resource_role = 8;
pub const ctex_resource_role_CTEX_RESOURCE_IN_FLIGHT: ctex_resource_role = 16;
pub type ctex_resource_role = ::std::os::raw::c_uint;
pub type ctex_resource_cache_eviction_callback = ::std::option::Option<
    unsafe extern "C" fn(allocation_identity: u64, user_data: *mut ::std::os::raw::c_void),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_resource_allocation_descriptor {
    pub size: u32,
    pub allocation_identity: u64,
    pub category: u32,
    pub physical_bytes: usize,
    pub roles: u32,
    pub device_backend: *const ::std::os::raw::c_char,
    pub device_identifier: *const ::std::os::raw::c_char,
    pub heap_identifier: *const ::std::os::raw::c_char,
}
impl Default for ctex_resource_allocation_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_resource_category_report {
    pub size: u32,
    pub category: u32,
    pub allocation_count: usize,
    pub physical_bytes: usize,
    pub cpu_resident_bytes: usize,
    pub gpu_resident_bytes: usize,
    pub backing_store_bytes: usize,
    pub pinned_bytes: usize,
    pub in_flight_bytes: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_resource_accounting_report {
    pub size: u32,
    pub allocation_count: usize,
    pub physical_bytes: usize,
    pub cpu_resident_bytes: usize,
    pub gpu_resident_bytes: usize,
    pub backing_store_bytes: usize,
    pub pinned_bytes: usize,
    pub in_flight_bytes: usize,
    pub category_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_resource_budget_limits {
    pub size: u32,
    pub cpu_bytes: usize,
    pub gpu_bytes: usize,
    pub backing_store_bytes: usize,
    pub temporary_bytes: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_resource_requirement {
    pub size: u32,
    pub category: u32,
    pub physical_bytes: usize,
    pub roles: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_resource_admission_descriptor {
    pub size: u32,
    pub operation: *const ::std::os::raw::c_char,
    pub limits: ctex_resource_budget_limits,
    pub fixed_requirements: *const ctex_resource_requirement,
    pub fixed_requirement_count: usize,
    pub per_work_item: ctex_resource_requirement,
    pub work_item_count: usize,
}
impl Default for ctex_resource_admission_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_resource_admission_status_CTEX_RESOURCE_ADMITTED_WHOLE:
    ctex_resource_admission_status = 0;
pub const ctex_resource_admission_status_CTEX_RESOURCE_ADMITTED_TILED:
    ctex_resource_admission_status = 1;
pub const ctex_resource_admission_status_CTEX_RESOURCE_OVER_BUDGET: ctex_resource_admission_status =
    2;
pub const ctex_resource_admission_status_CTEX_RESOURCE_QUIESCING: ctex_resource_admission_status =
    3;
pub type ctex_resource_admission_status = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_resource_admission_report {
    pub size: u32,
    pub status: u32,
    pub work_item_count: usize,
    pub admitted_work_items: usize,
    pub projected_cpu_bytes: usize,
    pub projected_gpu_bytes: usize,
    pub projected_backing_store_bytes: usize,
    pub projected_temporary_bytes: usize,
    pub evicted_allocation_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_preview_quality_option {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub requirements: *const ctex_resource_requirement,
    pub requirement_count: usize,
    pub derived_work_deferred: u32,
}
impl Default for ctex_preview_quality_option {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_preview_quality_admission_descriptor {
    pub size: u32,
    pub operation: *const ::std::os::raw::c_char,
    pub limits: ctex_resource_budget_limits,
    pub full_quality_width: u32,
    pub full_quality_height: u32,
    pub options: *const ctex_preview_quality_option,
    pub option_count: usize,
}
impl Default for ctex_preview_quality_admission_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_preview_quality_status_CTEX_PREVIEW_FULL_QUALITY: ctex_preview_quality_status = 0;
pub const ctex_preview_quality_status_CTEX_PREVIEW_REDUCED_RESOLUTION: ctex_preview_quality_status =
    1;
pub const ctex_preview_quality_status_CTEX_PREVIEW_DEFERRED_DERIVED: ctex_preview_quality_status =
    2;
pub const ctex_preview_quality_status_CTEX_PREVIEW_REDUCED_AND_DEFERRED:
    ctex_preview_quality_status = 3;
pub const ctex_preview_quality_status_CTEX_PREVIEW_OVER_BUDGET: ctex_preview_quality_status = 4;
pub const ctex_preview_quality_status_CTEX_PREVIEW_QUIESCING: ctex_preview_quality_status = 5;
pub type ctex_preview_quality_status = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_preview_quality_admission_report {
    pub size: u32,
    pub status: u32,
    pub selected_option: usize,
    pub full_quality_width: u32,
    pub full_quality_height: u32,
    pub selected_width: u32,
    pub selected_height: u32,
    pub derived_work_deferred: u32,
    pub projected_cpu_bytes: usize,
    pub projected_gpu_bytes: usize,
    pub projected_backing_store_bytes: usize,
    pub projected_temporary_bytes: usize,
    pub evicted_allocation_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_tile_backing_key {
    pub namespace_identity: u64,
    pub tile_x: u32,
    pub tile_y: u32,
    pub generation: u64,
}
pub type ctex_tile_backing_store_callback = ::std::option::Option<
    unsafe extern "C" fn(
        key: ctex_tile_backing_key,
        bytes: *const ::std::os::raw::c_void,
        byte_count: usize,
        user_data: *mut ::std::os::raw::c_void,
    ) -> u32,
>;
pub type ctex_tile_backing_load_callback = ::std::option::Option<
    unsafe extern "C" fn(
        key: ctex_tile_backing_key,
        bytes: *mut ::std::os::raw::c_void,
        byte_count: usize,
        user_data: *mut ::std::os::raw::c_void,
    ) -> u32,
>;
pub type ctex_tile_backing_discard_callback = ::std::option::Option<
    unsafe extern "C" fn(key: ctex_tile_backing_key, user_data: *mut ::std::os::raw::c_void),
>;
pub type ctex_tile_backing_release_callback = ::std::option::Option<
    unsafe extern "C" fn(namespace_identity: u64, user_data: *mut ::std::os::raw::c_void),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_tile_backing_store_descriptor {
    pub size: u32,
    pub store: ctex_tile_backing_store_callback,
    pub load: ctex_tile_backing_load_callback,
    pub discard: ctex_tile_backing_discard_callback,
    pub release_namespace: ctex_tile_backing_release_callback,
    pub user_data: *mut ::std::os::raw::c_void,
}
impl Default for ctex_tile_backing_store_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_tile_eviction_status_CTEX_TILE_EVICTED: ctex_tile_eviction_status = 0;
pub const ctex_tile_eviction_status_CTEX_TILE_SPARSE: ctex_tile_eviction_status = 1;
pub const ctex_tile_eviction_status_CTEX_TILE_ALREADY_EVICTED: ctex_tile_eviction_status = 2;
pub const ctex_tile_eviction_status_CTEX_TILE_PINNED: ctex_tile_eviction_status = 3;
pub const ctex_tile_eviction_status_CTEX_TILE_NO_BACKING_STORE: ctex_tile_eviction_status = 4;
pub const ctex_tile_eviction_status_CTEX_TILE_BACKING_STORE_FAILED: ctex_tile_eviction_status = 5;
pub type ctex_tile_eviction_status = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_tile_eviction_report {
    pub size: u32,
    pub status: u32,
    pub resident_bytes_released: usize,
    pub backing_bytes_written: usize,
    pub resident_pixel_bytes: usize,
    pub backed_pixel_bytes: usize,
}
pub const ctex_transport_channel_order_CTEX_TRANSPORT_CHANNEL_ORDER_R:
    ctex_transport_channel_order = 0;
pub const ctex_transport_channel_order_CTEX_TRANSPORT_CHANNEL_ORDER_RG:
    ctex_transport_channel_order = 1;
pub const ctex_transport_channel_order_CTEX_TRANSPORT_CHANNEL_ORDER_RGB:
    ctex_transport_channel_order = 2;
pub const ctex_transport_channel_order_CTEX_TRANSPORT_CHANNEL_ORDER_RGBA:
    ctex_transport_channel_order = 3;
pub type ctex_transport_channel_order = ::std::os::raw::c_uint;
pub const ctex_transport_component_byte_order_CTEX_TRANSPORT_COMPONENT_BYTE_ORDER_NATIVE:
    ctex_transport_component_byte_order = 0;
pub type ctex_transport_component_byte_order = ::std::os::raw::c_uint;
pub const ctex_transport_tile_contiguity_CTEX_TRANSPORT_SEPARATE_TILE_BUFFERS:
    ctex_transport_tile_contiguity = 0;
pub type ctex_transport_tile_contiguity = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
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
#[derive(Debug, Copy, Clone)]
pub struct ctex_transport_tile_readback_destination {
    pub size: u32,
    pub version: ctex_transport_tile_version,
    pub layout: ctex_transport_tile_memory_layout,
    pub output: *mut ::std::os::raw::c_void,
    pub output_size: usize,
}
impl Default for ctex_transport_tile_readback_destination {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_transport_readback_status_CTEX_TRANSPORT_READBACK_PENDING:
    ctex_transport_readback_status = 0;
pub const ctex_transport_readback_status_CTEX_TRANSPORT_READBACK_COMPLETE:
    ctex_transport_readback_status = 1;
pub const ctex_transport_readback_status_CTEX_TRANSPORT_READBACK_CANCELLED:
    ctex_transport_readback_status = 2;
pub const ctex_transport_readback_status_CTEX_TRANSPORT_READBACK_FAILED:
    ctex_transport_readback_status = 3;
pub type ctex_transport_readback_status = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_transport_host_tile_completion {
    pub size: u32,
    pub version: ctex_transport_tile_version,
    pub layout: ctex_transport_tile_memory_layout,
    pub bytes: *const ::std::os::raw::c_void,
    pub byte_size: usize,
}
impl Default for ctex_transport_host_tile_completion {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_transport_readback_info {
    pub size: u32,
    pub status: u32,
    pub output_readable: u32,
    pub tile_count: usize,
    pub required_detail_size: usize,
}
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_TANGENT_SPACE_NORMAL: ctex_mesh_map_kind = 0;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_OBJECT_SPACE_NORMAL: ctex_mesh_map_kind = 1;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_WORLD_SPACE_DIRECTION: ctex_mesh_map_kind = 2;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_AMBIENT_OCCLUSION: ctex_mesh_map_kind = 3;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_CURVATURE: ctex_mesh_map_kind = 4;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_THICKNESS: ctex_mesh_map_kind = 5;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_POSITION: ctex_mesh_map_kind = 6;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_HEIGHT: ctex_mesh_map_kind = 7;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_BENT_NORMAL: ctex_mesh_map_kind = 8;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_MATERIAL_ID: ctex_mesh_map_kind = 9;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_OBJECT_ID: ctex_mesh_map_kind = 10;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_UV_DENSITY: ctex_mesh_map_kind = 11;
pub const ctex_mesh_map_kind_CTEX_MESH_MAP_VERTEX_COLOUR: ctex_mesh_map_kind = 12;
pub type ctex_mesh_map_kind = ::std::os::raw::c_uint;
pub const ctex_mesh_map_channel_meaning_CTEX_MESH_MAP_SCALAR_DATA: ctex_mesh_map_channel_meaning =
    0;
pub const ctex_mesh_map_channel_meaning_CTEX_MESH_MAP_NORMAL_XYZ: ctex_mesh_map_channel_meaning = 1;
pub const ctex_mesh_map_channel_meaning_CTEX_MESH_MAP_DIRECTION_XYZ: ctex_mesh_map_channel_meaning =
    2;
pub const ctex_mesh_map_channel_meaning_CTEX_MESH_MAP_POSITION_XYZ: ctex_mesh_map_channel_meaning =
    3;
pub const ctex_mesh_map_channel_meaning_CTEX_MESH_MAP_IDENTIFIER: ctex_mesh_map_channel_meaning = 4;
pub const ctex_mesh_map_channel_meaning_CTEX_MESH_MAP_COLOUR_RGB: ctex_mesh_map_channel_meaning = 5;
pub const ctex_mesh_map_channel_meaning_CTEX_MESH_MAP_COLOUR_RGBA: ctex_mesh_map_channel_meaning =
    6;
pub type ctex_mesh_map_channel_meaning = ::std::os::raw::c_uint;
pub const ctex_mesh_map_normal_convention_CTEX_MESH_MAP_NORMAL_OPENGL:
    ctex_mesh_map_normal_convention = 0;
pub const ctex_mesh_map_normal_convention_CTEX_MESH_MAP_NORMAL_DIRECTX:
    ctex_mesh_map_normal_convention = 1;
pub type ctex_mesh_map_normal_convention = ::std::os::raw::c_uint;
pub const ctex_tangent_basis_algorithm_CTEX_TANGENT_BASIS_UV_DERIVATIVE:
    ctex_tangent_basis_algorithm = 0;
pub const ctex_tangent_basis_algorithm_CTEX_TANGENT_BASIS_LENGYEL_ORTHONORMALIZED:
    ctex_tangent_basis_algorithm = 1;
pub const ctex_tangent_basis_algorithm_CTEX_TANGENT_BASIS_MIKKTSPACE: ctex_tangent_basis_algorithm =
    2;
pub type ctex_tangent_basis_algorithm = ::std::os::raw::c_uint;
pub const ctex_tangent_normal_orientation_CTEX_TANGENT_NORMAL_VERTEX:
    ctex_tangent_normal_orientation = 0;
pub const ctex_tangent_normal_orientation_CTEX_TANGENT_NORMAL_INVERTED_VERTEX:
    ctex_tangent_normal_orientation = 1;
pub type ctex_tangent_normal_orientation = ::std::os::raw::c_uint;
pub const ctex_coordinate_handedness_CTEX_COORDINATE_RIGHT_HANDED: ctex_coordinate_handedness = 0;
pub const ctex_coordinate_handedness_CTEX_COORDINATE_LEFT_HANDED: ctex_coordinate_handedness = 1;
pub type ctex_coordinate_handedness = ::std::os::raw::c_uint;
pub const ctex_uv_v_axis_CTEX_UV_V_AXIS_UPWARD: ctex_uv_v_axis = 0;
pub const ctex_uv_v_axis_CTEX_UV_V_AXIS_DOWNWARD: ctex_uv_v_axis = 1;
pub type ctex_uv_v_axis = ::std::os::raw::c_uint;
pub const ctex_tangent_handedness_encoding_CTEX_TANGENT_HANDEDNESS_W_SIGN:
    ctex_tangent_handedness_encoding = 0;
pub type ctex_tangent_handedness_encoding = ::std::os::raw::c_uint;
pub const ctex_tangent_frame_source_CTEX_TANGENT_FRAME_SUPPLIED: ctex_tangent_frame_source = 0;
pub const ctex_tangent_frame_source_CTEX_TANGENT_FRAME_GENERATED: ctex_tangent_frame_source = 1;
pub type ctex_tangent_frame_source = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_tangent_frame_descriptor {
    pub size: u32,
    pub algorithm: u32,
    pub algorithm_version: u32,
    pub normal_orientation: u32,
    pub coordinate_handedness: u32,
    pub uv_v_axis: u32,
    pub handedness_encoding: u32,
    pub uv_set: *const ::std::os::raw::c_char,
}
impl Default for ctex_tangent_frame_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_tangent_data_descriptor {
    pub size: u32,
    pub frame: ctex_tangent_frame_descriptor,
    pub corner_tangents: *const ctex_vec4f,
    pub corner_tangent_count: usize,
}
impl Default for ctex_mesh_tangent_data_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_tangent_frame_info {
    pub size: u32,
    pub source: u32,
    pub algorithm: u32,
    pub algorithm_version: u32,
    pub normal_orientation: u32,
    pub coordinate_handedness: u32,
    pub uv_v_axis: u32,
    pub handedness_encoding: u32,
    pub corner_tangent_count: usize,
    pub required_uv_set_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_pixel_buffer_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub component_type: u32,
    pub component_count: u32,
    pub row_stride_bytes: usize,
    pub pixels: *const ::std::os::raw::c_void,
    pub pixel_bytes: usize,
}
impl Default for ctex_mesh_map_pixel_buffer_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_import_descriptor {
    pub size: u32,
    pub kind: u32,
    pub channel_meaning: u32,
    pub color_space: u32,
    pub has_normal_convention: u32,
    pub normal_convention: u32,
    pub tangent_frame: *const ctex_tangent_frame_descriptor,
    pub buffer: ctex_mesh_map_pixel_buffer_descriptor,
}
impl Default for ctex_mesh_map_import_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_import_info {
    pub size: u32,
    pub replaced_existing: u32,
    pub resolution_mismatch: u32,
    pub stale: u32,
    pub converted_to_working_space: u32,
    pub storage_color_space: u32,
    pub channel_meaning: u32,
    pub map_width: u32,
    pub map_height: u32,
    pub texture_set_width: u32,
    pub texture_set_height: u32,
    pub produced_mesh_revision: u64,
    pub current_mesh_revision: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_set_info {
    pub size: u32,
    pub texture_set_width: u32,
    pub texture_set_height: u32,
    pub mesh_revision: u64,
    pub bound_map_count: usize,
    pub resident_pixel_bytes: usize,
    pub required_texture_set_id_size: usize,
    pub required_uv_set_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_entry_info {
    pub kind: u32,
    pub width: u32,
    pub height: u32,
    pub resident_pixel_bytes: usize,
    pub produced_mesh_revision: u64,
    pub stale: u32,
    pub has_normal_convention: u32,
    pub normal_convention: u32,
    pub has_tangent_frame: u32,
    pub tangent_algorithm: u32,
    pub tangent_algorithm_version: u32,
    pub tangent_normal_orientation: u32,
    pub tangent_coordinate_handedness: u32,
    pub tangent_uv_v_axis: u32,
    pub tangent_handedness_encoding: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_sample_info {
    pub size: u32,
    pub component_count: u32,
    pub values: [f64; 4usize],
    pub stale: u32,
    pub produced_mesh_revision: u64,
    pub current_mesh_revision: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_staleness {
    pub kind: u32,
    pub produced_mesh_revision: u64,
    pub current_mesh_revision: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_requirement_info {
    pub size: u32,
    pub required_missing_map_count: usize,
    pub required_stale_map_count: usize,
    pub required_message_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_release_info {
    pub size: u32,
    pub released_map_count: usize,
    pub resident_pixel_bytes_released: usize,
}
pub const ctex_mesh_map_generator_kind_CTEX_MESH_MAP_GENERATOR_AMBIENT_OCCLUSION:
    ctex_mesh_map_generator_kind = 0;
pub const ctex_mesh_map_generator_kind_CTEX_MESH_MAP_GENERATOR_CURVATURE:
    ctex_mesh_map_generator_kind = 1;
pub const ctex_mesh_map_generator_kind_CTEX_MESH_MAP_GENERATOR_THICKNESS:
    ctex_mesh_map_generator_kind = 2;
pub const ctex_mesh_map_generator_kind_CTEX_MESH_MAP_GENERATOR_POSITION_GRADIENT:
    ctex_mesh_map_generator_kind = 3;
pub const ctex_mesh_map_generator_kind_CTEX_MESH_MAP_GENERATOR_WORLD_SPACE_DIRECTION:
    ctex_mesh_map_generator_kind = 4;
pub const ctex_mesh_map_generator_kind_CTEX_MESH_MAP_GENERATOR_DIRT: ctex_mesh_map_generator_kind =
    5;
pub const ctex_mesh_map_generator_kind_CTEX_MESH_MAP_GENERATOR_EDGE_WEAR:
    ctex_mesh_map_generator_kind = 6;
pub const ctex_mesh_map_generator_kind_CTEX_MESH_MAP_GENERATOR_SCRATCHES:
    ctex_mesh_map_generator_kind = 7;
pub type ctex_mesh_map_generator_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_generator_parameter_descriptor {
    pub size: u32,
    pub name_offset: usize,
    pub name_size: usize,
    pub default_value: f64,
    pub minimum: f64,
    pub maximum: f64,
    pub meaning_offset: usize,
    pub meaning_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_generator_info {
    pub size: u32,
    pub kind: u32,
    pub name_offset: usize,
    pub name_size: usize,
    pub required_map_count: usize,
    pub parameter_count: usize,
    pub required_string_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_generator_parameter {
    pub size: u32,
    pub name: *const ::std::os::raw::c_char,
    pub value: f64,
}
impl Default for ctex_mesh_map_generator_parameter {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_generator_resolved_parameter {
    pub name_offset: usize,
    pub name_size: usize,
    pub value: f64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_generator_parameter_clamp {
    pub name_offset: usize,
    pub name_size: usize,
    pub supplied: f64,
    pub resolved: f64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_generator_result_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub row_stride_bytes: usize,
    pub required_mask_value_count: usize,
    pub required_resolved_parameter_count: usize,
    pub required_parameter_clamp_count: usize,
    pub required_stale_map_count: usize,
    pub message_offset: usize,
    pub message_size: usize,
    pub required_string_size: usize,
}
pub const ctex_mesh_map_bake_provider_status_CTEX_MESH_MAP_BAKE_PROVIDER_COMPLETED:
    ctex_mesh_map_bake_provider_status = 0;
pub const ctex_mesh_map_bake_provider_status_CTEX_MESH_MAP_BAKE_PROVIDER_CANCELLED:
    ctex_mesh_map_bake_provider_status = 1;
pub const ctex_mesh_map_bake_provider_status_CTEX_MESH_MAP_BAKE_PROVIDER_FAILED:
    ctex_mesh_map_bake_provider_status = 2;
pub type ctex_mesh_map_bake_provider_status = ::std::os::raw::c_uint;
pub const ctex_mesh_map_bake_request_status_CTEX_MESH_MAP_BAKE_COMPLETED:
    ctex_mesh_map_bake_request_status = 0;
pub const ctex_mesh_map_bake_request_status_CTEX_MESH_MAP_BAKE_CANCELLED:
    ctex_mesh_map_bake_request_status = 1;
pub const ctex_mesh_map_bake_request_status_CTEX_MESH_MAP_BAKE_UNSUPPORTED:
    ctex_mesh_map_bake_request_status = 2;
pub const ctex_mesh_map_bake_request_status_CTEX_MESH_MAP_BAKE_REQUEST_PROVIDER_FAILED:
    ctex_mesh_map_bake_request_status = 3;
pub type ctex_mesh_map_bake_request_status = ::std::os::raw::c_uint;
pub const ctex_mesh_map_bake_completion_disposition_CTEX_MESH_MAP_BAKE_BOUND:
    ctex_mesh_map_bake_completion_disposition = 0;
pub const ctex_mesh_map_bake_completion_disposition_CTEX_MESH_MAP_BAKE_STALE:
    ctex_mesh_map_bake_completion_disposition = 1;
pub const ctex_mesh_map_bake_completion_disposition_CTEX_MESH_MAP_BAKE_COMPLETION_CANCELLED:
    ctex_mesh_map_bake_completion_disposition = 2;
pub const ctex_mesh_map_bake_completion_disposition_CTEX_MESH_MAP_BAKE_INVALID_OUTPUT:
    ctex_mesh_map_bake_completion_disposition = 3;
pub const ctex_mesh_map_bake_completion_disposition_CTEX_MESH_MAP_BAKE_UNKNOWN_TOKEN:
    ctex_mesh_map_bake_completion_disposition = 4;
pub type ctex_mesh_map_bake_completion_disposition = ::std::os::raw::c_uint;
pub type ctex_mesh_map_bake_can_produce_fn = ::std::option::Option<
    unsafe extern "C" fn(user_data: *mut ::std::os::raw::c_void, kind: u32) -> u32,
>;
pub type ctex_mesh_map_bake_is_cancelled_fn =
    ::std::option::Option<unsafe extern "C" fn(user_data: *mut ::std::os::raw::c_void) -> u32>;
pub type ctex_mesh_map_bake_report_progress_fn = ::std::option::Option<
    unsafe extern "C" fn(user_data: *mut ::std::os::raw::c_void, fraction: f64),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_bake_request_descriptor {
    pub size: u32,
    pub kind: u32,
    pub texture_set_id: *const ::std::os::raw::c_char,
    pub uv_set: *const ::std::os::raw::c_char,
    pub mesh_revision: u64,
    pub bake_settings_revision: u64,
    pub request_generation: u64,
    pub tangent_frame: *const ctex_tangent_frame_descriptor,
    pub width: u32,
    pub height: u32,
}
impl Default for ctex_mesh_map_bake_request_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_bake_control {
    pub size: u32,
    pub user_data: *mut ::std::os::raw::c_void,
    pub is_cancelled: ctex_mesh_map_bake_is_cancelled_fn,
    pub report_progress: ctex_mesh_map_bake_report_progress_fn,
}
impl Default for ctex_mesh_map_bake_control {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_bake_output_descriptor {
    pub size: u32,
    pub buffer: ctex_mesh_map_pixel_buffer_descriptor,
    pub has_normal_convention: u32,
    pub normal_convention: u32,
    pub tangent_frame: *const ctex_tangent_frame_descriptor,
    pub detail: *const ::std::os::raw::c_char,
}
impl Default for ctex_mesh_map_bake_output_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type ctex_mesh_map_bake_request_fn = ::std::option::Option<
    unsafe extern "C" fn(
        user_data: *mut ::std::os::raw::c_void,
        request: *const ctex_mesh_map_bake_request_descriptor,
        control: *const ctex_mesh_map_bake_control,
        output: *mut ctex_mesh_map_bake_output_descriptor,
    ) -> u32,
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_bake_provider_descriptor {
    pub size: u32,
    pub name: *const ::std::os::raw::c_char,
    pub user_data: *mut ::std::os::raw::c_void,
    pub can_produce: ctex_mesh_map_bake_can_produce_fn,
    pub request: ctex_mesh_map_bake_request_fn,
}
impl Default for ctex_mesh_map_bake_provider_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type ctex_mesh_map_bake_provider_entry_point_v1_fn = ::std::option::Option<
    unsafe extern "C" fn(out_provider: *mut ctex_mesh_map_bake_provider_descriptor) -> ctex_result,
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_bake_control_descriptor {
    pub size: u32,
    pub user_data: *mut ::std::os::raw::c_void,
    pub is_cancelled: ctex_mesh_map_bake_is_cancelled_fn,
    pub report_progress: ctex_mesh_map_bake_report_progress_fn,
}
impl Default for ctex_mesh_map_bake_control_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_bake_result_info {
    pub size: u32,
    pub status: u32,
    pub has_binding: u32,
    pub replaced_existing: u32,
    pub resolution_mismatch: u32,
    pub stale: u32,
    pub produced_mesh_revision: u64,
    pub current_mesh_revision: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_bake_session_info {
    pub size: u32,
    pub settings_revision: u64,
    pub pending_request_count: usize,
    pub undo_step_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_mesh_map_bake_token_info {
    pub size: u32,
    pub session_identity: u64,
    pub kind: u32,
    pub mesh_revision: u64,
    pub bake_settings_revision: u64,
    pub request_generation: u64,
    pub width: u32,
    pub height: u32,
    pub has_tangent_frame: u32,
    pub tangent_frame: ctex_tangent_frame_descriptor,
    pub required_texture_set_id_size: usize,
    pub required_uv_set_size: usize,
    pub required_tangent_uv_set_size: usize,
}
impl Default for ctex_mesh_map_bake_token_info {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_bake_completion_info {
    pub size: u32,
    pub disposition: u32,
    pub has_binding: u32,
    pub replaced_existing: u32,
    pub resolution_mismatch: u32,
    pub stale: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_bake_settings_edit_info {
    pub size: u32,
    pub previous_revision: u64,
    pub current_revision: u64,
    pub invalidated_request_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_mesh_map_bake_settings_undo_info {
    pub size: u32,
    pub restored: u32,
    pub previous_revision: u64,
    pub restored_revision: u64,
    pub restored_map_count: usize,
    pub invalidated_request_count: usize,
}
pub const ctex_executor_route_CTEX_EXECUTOR_ROUTE_HOST_EXECUTED: ctex_executor_route = 0;
pub const ctex_executor_route_CTEX_EXECUTOR_ROUTE_CPU_REFERENCE: ctex_executor_route = 1;
pub const ctex_executor_route_CTEX_EXECUTOR_ROUTE_OWNED_GPU: ctex_executor_route = 2;
pub type ctex_executor_route = ::std::os::raw::c_uint;
pub const ctex_executor_availability_CTEX_EXECUTOR_AVAILABLE: ctex_executor_availability = 0;
pub const ctex_executor_availability_CTEX_EXECUTOR_DEVICE_UNAVAILABLE: ctex_executor_availability =
    1;
pub const ctex_executor_availability_CTEX_EXECUTOR_HOST_NOT_ATTACHED: ctex_executor_availability =
    2;
pub type ctex_executor_availability = ::std::os::raw::c_uint;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_R8_UNORM:
    ctex_executor_texture_format = 0;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_RG8_UNORM:
    ctex_executor_texture_format = 1;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM:
    ctex_executor_texture_format = 2;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_R16_UNORM:
    ctex_executor_texture_format = 3;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_RG16_UNORM:
    ctex_executor_texture_format = 4;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_RGBA16_UNORM:
    ctex_executor_texture_format = 5;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_R16_FLOAT:
    ctex_executor_texture_format = 6;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_RG16_FLOAT:
    ctex_executor_texture_format = 7;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT:
    ctex_executor_texture_format = 8;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_R32_FLOAT:
    ctex_executor_texture_format = 9;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_RG32_FLOAT:
    ctex_executor_texture_format = 10;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_RGBA32_FLOAT:
    ctex_executor_texture_format = 11;
pub const ctex_executor_texture_format_CTEX_EXECUTOR_TEXTURE_DEPTH32_FLOAT:
    ctex_executor_texture_format = 12;
pub type ctex_executor_texture_format = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_host_executor_descriptor {
    pub size: u32,
    pub device_name: *const ::std::os::raw::c_char,
    pub binding_budget: u32,
    pub maximum_texture_dimension: u32,
    pub supported_texture_formats: *const u32,
    pub supported_texture_format_count: usize,
    pub floating_point_filtering: u32,
    pub compute_available: u32,
    pub attached: u32,
}
impl Default for ctex_host_executor_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_executor_info {
    pub size: u32,
    pub route: u32,
    pub availability: u32,
    pub binding_budget: u32,
    pub maximum_texture_dimension: u32,
    pub supported_texture_format_count: usize,
    pub floating_point_filtering: u32,
    pub compute_available: u32,
    pub required_identifier_size: usize,
    pub required_display_name_size: usize,
    pub required_device_name_size: usize,
}
pub const ctex_executor_selection_source_CTEX_EXECUTOR_SELECTION_AUTOMATIC:
    ctex_executor_selection_source = 0;
pub const ctex_executor_selection_source_CTEX_EXECUTOR_SELECTION_EXPLICIT:
    ctex_executor_selection_source = 1;
pub const ctex_executor_selection_source_CTEX_EXECUTOR_SELECTION_ENVIRONMENT:
    ctex_executor_selection_source = 2;
pub type ctex_executor_selection_source = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_executor_selection_info {
    pub size: u32,
    pub source: u32,
    pub selected_executor_index: usize,
    pub required_requested_identifier_size: usize,
    pub required_message_size: usize,
}
pub const ctex_execution_failure_code_CTEX_EXECUTION_FAILURE_DEVICE_UNAVAILABLE:
    ctex_execution_failure_code = 0;
pub const ctex_execution_failure_code_CTEX_EXECUTION_FAILURE_DEVICE_LOST:
    ctex_execution_failure_code = 1;
pub const ctex_execution_failure_code_CTEX_EXECUTION_FAILURE_OPERATION_FAILED:
    ctex_execution_failure_code = 2;
pub const ctex_execution_failure_code_CTEX_EXECUTION_FAILURE_CANCELLED:
    ctex_execution_failure_code = 3;
pub type ctex_execution_failure_code = ::std::os::raw::c_uint;
pub const ctex_executor_fallback_disposition_CTEX_EXECUTOR_NO_FALLBACK:
    ctex_executor_fallback_disposition = 0;
pub const ctex_executor_fallback_disposition_CTEX_EXECUTOR_CPU_FALLBACK:
    ctex_executor_fallback_disposition = 1;
pub const ctex_executor_fallback_disposition_CTEX_EXECUTOR_RECOVERY_REQUIRED:
    ctex_executor_fallback_disposition = 2;
pub type ctex_executor_fallback_disposition = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_executor_fallback_descriptor {
    pub size: u32,
    pub failed_executor: *const ::std::os::raw::c_char,
    pub failure: u32,
    pub failure_detail: *const ::std::os::raw::c_char,
    pub disposition: u32,
    pub fallback_executor: *const ::std::os::raw::c_char,
    pub recovery_restored: u32,
}
impl Default for ctex_executor_fallback_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_executor_fallback_info {
    pub size: u32,
    pub disposition: u32,
    pub recovery_restored: u32,
    pub required_message_size: usize,
}
pub const ctex_cpu_execution_status_CTEX_CPU_EXECUTION_COMPLETED: ctex_cpu_execution_status = 0;
pub const ctex_cpu_execution_status_CTEX_CPU_EXECUTION_CANCELLED: ctex_cpu_execution_status = 1;
pub const ctex_cpu_execution_status_CTEX_CPU_EXECUTION_MEMORY_CEILING_EXCEEDED:
    ctex_cpu_execution_status = 2;
pub type ctex_cpu_execution_status = ::std::os::raw::c_uint;
pub type ctex_cpu_work_item_callback = ::std::option::Option<
    unsafe extern "C" fn(
        work_item: usize,
        shared_working_memory: *mut ::std::os::raw::c_void,
        shared_working_memory_size: usize,
        worker_working_memory: *mut ::std::os::raw::c_void,
        worker_working_memory_size: usize,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result,
>;
pub type ctex_cpu_commit_callback = ::std::option::Option<
    unsafe extern "C" fn(
        shared_working_memory: *const ::std::os::raw::c_void,
        shared_working_memory_size: usize,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result,
>;
pub type ctex_cpu_cancel_callback =
    ::std::option::Option<unsafe extern "C" fn(user_data: *mut ::std::os::raw::c_void) -> u32>;
pub type ctex_cpu_progress_callback = ::std::option::Option<
    unsafe extern "C" fn(
        completed_work_items: usize,
        total_work_items: usize,
        user_data: *mut ::std::os::raw::c_void,
    ),
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_cpu_bounded_execution_descriptor {
    pub size: u32,
    pub operation: *const ::std::os::raw::c_char,
    pub work_item_count: usize,
    pub shared_working_memory_bytes: usize,
    pub working_memory_bytes_per_worker: usize,
    pub maximum_workers: usize,
    pub memory_ceiling_bytes: usize,
    pub progress_interval: usize,
    pub execute_work_item: ctex_cpu_work_item_callback,
    pub commit: ctex_cpu_commit_callback,
    pub is_cancelled: ctex_cpu_cancel_callback,
    pub report_progress: ctex_cpu_progress_callback,
    pub user_data: *mut ::std::os::raw::c_void,
}
impl Default for ctex_cpu_bounded_execution_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_cpu_execution_info {
    pub size: u32,
    pub status: u32,
    pub completed_work_items: usize,
    pub total_work_items: usize,
    pub required_memory_bytes: usize,
    pub worker_count: usize,
    pub required_message_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_cpu_raster_mesh_descriptor {
    pub size: u32,
    pub positions: *const ctex_vec3f,
    pub uv: *const ctex_vec2f,
    pub vertex_count: usize,
    pub triangle_indices: *const u32,
    pub triangle_index_count: usize,
}
impl Default for ctex_cpu_raster_mesh_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_cpu_raster_camera_descriptor {
    pub size: u32,
    pub view_projection: [f32; 16usize],
    pub width: u32,
    pub height: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_cpu_viewport_raster_descriptor {
    pub size: u32,
    pub mesh: *const ctex_cpu_raster_mesh_descriptor,
    pub camera: *const ctex_cpu_raster_camera_descriptor,
    pub maximum_output_pixels: usize,
}
impl Default for ctex_cpu_viewport_raster_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_cpu_uv_raster_descriptor {
    pub size: u32,
    pub mesh: *const ctex_cpu_raster_mesh_descriptor,
    pub camera: *const ctex_cpu_raster_camera_descriptor,
    pub width: u32,
    pub height: u32,
    pub tile_origin: ctex_vec2f,
    pub maximum_output_pixels: usize,
}
impl Default for ctex_cpu_uv_raster_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_cpu_raster_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub pixel_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_cpu_raster_outputs {
    pub size: u32,
    pub depth: *mut f32,
    pub depth_capacity: usize,
    pub coordinates: *mut ctex_vec2f,
    pub coordinate_capacity: usize,
    pub coverage: *mut u8,
    pub coverage_capacity: usize,
    pub triangle_identity: *mut u32,
    pub triangle_identity_capacity: usize,
}
impl Default for ctex_cpu_raster_outputs {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_parity_value_class_CTEX_PARITY_UNORM8: ctex_parity_value_class = 0;
pub const ctex_parity_value_class_CTEX_PARITY_UNORM16: ctex_parity_value_class = 1;
pub const ctex_parity_value_class_CTEX_PARITY_FLOATING_POINT: ctex_parity_value_class = 2;
pub type ctex_parity_value_class = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_parity_tolerance_info {
    pub size: u32,
    pub absolute: f64,
    pub relative: f64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_parity_comparison_info {
    pub size: u32,
    pub matches: u32,
    pub compared_value_count: usize,
    pub maximum_absolute_deviation: f64,
    pub has_failure: u32,
    pub failure_value_index: usize,
    pub failure_reference: f64,
    pub failure_measured: f64,
    pub failure_absolute_deviation: f64,
    pub failure_allowed_deviation: f64,
    pub required_message_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_parity_fixture_channel_descriptor {
    pub size: u32,
    pub semantic: *const ::std::os::raw::c_char,
    pub value_class: u32,
    pub filtered: u32,
    pub component_count: u32,
}
impl Default for ctex_parity_fixture_channel_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_parity_fixture_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub document: *const ::std::os::raw::c_char,
    pub stroke: *const ::std::os::raw::c_char,
    pub camera: *const ::std::os::raw::c_char,
    pub material: *const ::std::os::raw::c_char,
    pub channels: *const ctex_parity_fixture_channel_descriptor,
    pub channel_count: usize,
}
impl Default for ctex_parity_fixture_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_parity_rendered_channel_descriptor {
    pub size: u32,
    pub semantic: *const ::std::os::raw::c_char,
    pub values: *const f64,
    pub value_count: usize,
}
impl Default for ctex_parity_rendered_channel_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_parity_rendered_fixture_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub channels: *const ctex_parity_rendered_channel_descriptor,
    pub channel_count: usize,
}
impl Default for ctex_parity_rendered_fixture_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type ctex_parity_render_callback = ::std::option::Option<
    unsafe extern "C" fn(
        fixture: *const ctex_parity_fixture_descriptor,
        out_rendered: *mut ctex_parity_rendered_fixture_descriptor,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result,
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_parity_executor_binding_descriptor {
    pub size: u32,
    pub executor_index: usize,
    pub render: ctex_parity_render_callback,
    pub user_data: *mut ::std::os::raw::c_void,
}
impl Default for ctex_parity_executor_binding_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_parity_gate_info {
    pub size: u32,
    pub passed: u32,
    pub executor_count: usize,
    pub reference_count: usize,
    pub passed_count: usize,
    pub failed_count: usize,
    pub unmeasured_count: usize,
    pub required_report_size: usize,
}
pub const ctex_host_resource_owner_CTEX_HOST_RESOURCE_LIBRARY: ctex_host_resource_owner = 0;
pub const ctex_host_resource_owner_CTEX_HOST_RESOURCE_HOST: ctex_host_resource_owner = 1;
pub type ctex_host_resource_owner = ::std::os::raw::c_uint;
pub const ctex_host_resource_state_CTEX_HOST_RESOURCE_SHADER_READ: ctex_host_resource_state = 0;
pub const ctex_host_resource_state_CTEX_HOST_RESOURCE_STORAGE_READ: ctex_host_resource_state = 1;
pub const ctex_host_resource_state_CTEX_HOST_RESOURCE_STORAGE_WRITE: ctex_host_resource_state = 2;
pub const ctex_host_resource_state_CTEX_HOST_RESOURCE_RENDER_TARGET: ctex_host_resource_state = 3;
pub const ctex_host_resource_state_CTEX_HOST_RESOURCE_DEPTH_TARGET: ctex_host_resource_state = 4;
pub type ctex_host_resource_state = ::std::os::raw::c_uint;
pub const ctex_host_replay_semantics_CTEX_HOST_REPLAY_DETERMINISTIC: ctex_host_replay_semantics = 0;
pub const ctex_host_replay_semantics_CTEX_HOST_REPLAY_CHECKPOINT_ONLY: ctex_host_replay_semantics =
    1;
pub type ctex_host_replay_semantics = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_host_resource_descriptor {
    pub size: u32,
    pub logical_id: *const ::std::os::raw::c_char,
    pub generation: u64,
    pub role: *const ::std::os::raw::c_char,
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
impl Default for ctex_host_resource_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_host_submission_descriptor {
    pub size: u32,
    pub operation: *const ::std::os::raw::c_char,
    pub base_revision: u64,
    pub resources: *const ctex_host_resource_descriptor,
    pub resource_count: usize,
    pub replay_semantics: u32,
}
impl Default for ctex_host_submission_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_host_submission_info {
    pub size: u32,
    pub completion_token: u64,
    pub base_revision: u64,
    pub resource_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_host_execution_session_info {
    pub size: u32,
    pub revision: u64,
    pub active_submission_count: usize,
    pub retained_recovery_bytes: usize,
}
pub const ctex_host_execution_status_CTEX_HOST_EXECUTION_SUCCEEDED: ctex_host_execution_status = 0;
pub const ctex_host_execution_status_CTEX_HOST_EXECUTION_FAILED: ctex_host_execution_status = 1;
pub const ctex_host_execution_status_CTEX_HOST_EXECUTION_CANCELLED: ctex_host_execution_status = 2;
pub type ctex_host_execution_status = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_host_completed_resource_descriptor {
    pub size: u32,
    pub logical_id: *const ::std::os::raw::c_char,
    pub generation: u64,
    pub format: u32,
    pub width: u32,
    pub height: u32,
    pub layers: u32,
}
impl Default for ctex_host_completed_resource_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_host_recovery_kind_CTEX_HOST_RECOVERY_RESULT_CHECKPOINT: ctex_host_recovery_kind = 0;
pub const ctex_host_recovery_kind_CTEX_HOST_RECOVERY_DETERMINISTIC_RECORD: ctex_host_recovery_kind =
    1;
pub type ctex_host_recovery_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_host_recovery_descriptor {
    pub size: u32,
    pub kind: u32,
    pub checkpoint_complete: u32,
    pub checkpoint_revision: u64,
    pub operation_record_version: *const ::std::os::raw::c_char,
    pub inputs_pinned: u32,
    pub retained_bytes: usize,
}
impl Default for ctex_host_recovery_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_host_completion_descriptor {
    pub size: u32,
    pub completion_token: u64,
    pub status: u32,
    pub outputs: *const ctex_host_completed_resource_descriptor,
    pub output_count: usize,
    pub recovery: *const ctex_host_recovery_descriptor,
    pub detail: *const ::std::os::raw::c_char,
}
impl Default for ctex_host_completion_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_host_completion_disposition_CTEX_HOST_COMPLETION_PUBLISHED:
    ctex_host_completion_disposition = 0;
pub const ctex_host_completion_disposition_CTEX_HOST_COMPLETION_AWAITING_RECOVERY:
    ctex_host_completion_disposition = 1;
pub const ctex_host_completion_disposition_CTEX_HOST_COMPLETION_STALE:
    ctex_host_completion_disposition = 2;
pub const ctex_host_completion_disposition_CTEX_HOST_COMPLETION_CANCELLED:
    ctex_host_completion_disposition = 3;
pub const ctex_host_completion_disposition_CTEX_HOST_COMPLETION_FAILED:
    ctex_host_completion_disposition = 4;
pub const ctex_host_completion_disposition_CTEX_HOST_COMPLETION_REJECTED:
    ctex_host_completion_disposition = 5;
pub const ctex_host_completion_disposition_CTEX_HOST_COMPLETION_DUPLICATE:
    ctex_host_completion_disposition = 6;
pub const ctex_host_completion_disposition_CTEX_HOST_COMPLETION_UNKNOWN_TOKEN:
    ctex_host_completion_disposition = 7;
pub type ctex_host_completion_disposition = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_host_resource_version {
    pub logical_id_offset: usize,
    pub generation: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
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
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_host_device_loss_info {
    pub size: u32,
    pub recovered_revision: u64,
    pub cancelled_submission_count: usize,
    pub released_resource_count: usize,
    pub required_released_identity_size: usize,
    pub retained_recovery_bytes: usize,
    pub restored: u32,
}
pub const ctex_texture_export_texture_set_selection_CTEX_TEXTURE_EXPORT_TEXTURE_SET_ALL:
    ctex_texture_export_texture_set_selection = 0;
pub const ctex_texture_export_texture_set_selection_CTEX_TEXTURE_EXPORT_TEXTURE_SET_SELECTED:
    ctex_texture_export_texture_set_selection = 1;
pub type ctex_texture_export_texture_set_selection = ::std::os::raw::c_uint;
pub const ctex_texture_export_spatial_scope_CTEX_TEXTURE_EXPORT_SCOPE_TEXTURE_SET:
    ctex_texture_export_spatial_scope = 0;
pub const ctex_texture_export_spatial_scope_CTEX_TEXTURE_EXPORT_SCOPE_UDIM:
    ctex_texture_export_spatial_scope = 1;
pub const ctex_texture_export_spatial_scope_CTEX_TEXTURE_EXPORT_SCOPE_ATLAS:
    ctex_texture_export_spatial_scope = 2;
pub type ctex_texture_export_spatial_scope = ::std::os::raw::c_uint;
pub const ctex_texture_export_layer_scope_CTEX_TEXTURE_EXPORT_LAYER_FLATTEN_VISIBLE:
    ctex_texture_export_layer_scope = 0;
pub const ctex_texture_export_layer_scope_CTEX_TEXTURE_EXPORT_LAYER_FLATTEN_SELECTED:
    ctex_texture_export_layer_scope = 1;
pub const ctex_texture_export_layer_scope_CTEX_TEXTURE_EXPORT_LAYER_EACH_SELECTED:
    ctex_texture_export_layer_scope = 2;
pub type ctex_texture_export_layer_scope = ::std::os::raw::c_uint;
pub const ctex_texture_export_layer_kind_CTEX_TEXTURE_EXPORT_LAYER_CONTENT:
    ctex_texture_export_layer_kind = 0;
pub const ctex_texture_export_layer_kind_CTEX_TEXTURE_EXPORT_LAYER_GROUP:
    ctex_texture_export_layer_kind = 1;
pub type ctex_texture_export_layer_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_layer_source_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub parent_identifier: *const ::std::os::raw::c_char,
    pub kind: u32,
    pub visible: u32,
}
impl Default for ctex_texture_export_layer_source_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_texture_set_source_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub width: u32,
    pub height: u32,
    pub occupied_udim_tiles: *const u32,
    pub occupied_udim_tile_count: usize,
    pub layers: *const ctex_texture_export_layer_source_descriptor,
    pub layer_count: usize,
}
impl Default for ctex_texture_export_texture_set_source_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_atlas_source_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub width: u32,
    pub height: u32,
    pub texture_set_identifiers: *const *const ::std::os::raw::c_char,
    pub texture_set_identifier_count: usize,
}
impl Default for ctex_texture_export_atlas_source_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_catalogue_descriptor {
    pub size: u32,
    pub project_name: *const ::std::os::raw::c_char,
    pub texture_sets: *const ctex_texture_export_texture_set_source_descriptor,
    pub texture_set_count: usize,
    pub atlases: *const ctex_texture_export_atlas_source_descriptor,
    pub atlas_count: usize,
}
impl Default for ctex_texture_export_catalogue_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_layer_selection_descriptor {
    pub size: u32,
    pub texture_set_identifier: *const ::std::os::raw::c_char,
    pub layer_identifiers: *const *const ::std::os::raw::c_char,
    pub layer_identifier_count: usize,
}
impl Default for ctex_texture_export_layer_selection_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_plan_descriptor {
    pub size: u32,
    pub texture_set_selection: u32,
    pub selected_texture_set_identifiers: *const *const ::std::os::raw::c_char,
    pub selected_texture_set_identifier_count: usize,
    pub spatial_scope: u32,
    pub layer_scope: u32,
    pub selected_layers: *const ctex_texture_export_layer_selection_descriptor,
    pub selected_layer_count: usize,
    pub output_width: u32,
    pub output_height: u32,
    pub filename_pattern: *const ::std::os::raw::c_char,
}
impl Default for ctex_texture_export_plan_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_texture_descriptor {
    pub size: u32,
    pub suffix: *const ::std::os::raw::c_char,
    pub channel_tokens: [*const ::std::os::raw::c_char; 4usize],
    pub color_space: u32,
    pub bit_depth: u32,
    pub format: u32,
}
impl Default for ctex_texture_export_texture_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_preset_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub textures: *const ctex_texture_export_texture_descriptor,
    pub texture_count: usize,
}
impl Default for ctex_texture_export_preset_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_options_descriptor {
    pub size: u32,
    pub plan: *const ctex_texture_export_plan_descriptor,
    pub padding_radius: u32,
    pub jpeg_quality: u32,
    pub dry_run: u32,
}
impl Default for ctex_texture_export_options_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_named_value {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub component_count: u32,
    pub components: [f64; 4usize],
}
impl Default for ctex_texture_export_named_value {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_sample {
    pub size: u32,
    pub base_color: [f64; 3usize],
    pub opacity: f64,
    pub roughness: f64,
    pub metallic: f64,
    pub normal: [f64; 3usize],
    pub height: f64,
    pub occlusion: f64,
    pub emission: [f64; 3usize],
    pub subsurface: f64,
    pub mesh_maps: *const ctex_texture_export_named_value,
    pub mesh_map_count: usize,
    pub registered_channels: *const ctex_texture_export_named_value,
    pub registered_channel_count: usize,
}
impl Default for ctex_texture_export_sample {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type ctex_texture_export_sample_callback = ::std::option::Option<
    unsafe extern "C" fn(
        x: u32,
        y: u32,
        out_sample: *mut ctex_texture_export_sample,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result,
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_pixel_source_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub sample: ctex_texture_export_sample_callback,
    pub sample_user_data: *mut ::std::os::raw::c_void,
    pub coverage: *const u8,
    pub coverage_count: usize,
}
impl Default for ctex_texture_export_pixel_source_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_layer_selection_view {
    pub size: u32,
    pub texture_set_identifier: *const ::std::os::raw::c_char,
    pub layer_identifiers: *const *const ::std::os::raw::c_char,
    pub layer_identifier_count: usize,
}
impl Default for ctex_texture_export_layer_selection_view {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_planned_output {
    pub size: u32,
    pub relative_path: *const ::std::os::raw::c_char,
    pub texture_set_identifiers: *const *const ::std::os::raw::c_char,
    pub texture_set_identifier_count: usize,
    pub has_udim_tile: u32,
    pub udim_tile: u32,
    pub atlas_identifier: *const ::std::os::raw::c_char,
    pub layer_selections: *const ctex_texture_export_layer_selection_view,
    pub layer_selection_count: usize,
    pub preset_texture_index: usize,
    pub width: u32,
    pub height: u32,
    pub format: u32,
    pub bit_depth: u32,
    pub color_space: u32,
}
impl Default for ctex_texture_export_planned_output {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_encoded_output {
    pub size: u32,
    pub report_entry_index: usize,
    pub relative_path: *const ::std::os::raw::c_char,
    pub width: u32,
    pub height: u32,
    pub format: u32,
    pub bit_depth: u32,
    pub color_space: u32,
    pub bytes: *const ::std::os::raw::c_void,
    pub byte_count: usize,
}
impl Default for ctex_texture_export_encoded_output {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type ctex_texture_export_source_callback = ::std::option::Option<
    unsafe extern "C" fn(
        output: *const ctex_texture_export_planned_output,
        out_source: *mut ctex_texture_export_pixel_source_descriptor,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result,
>;
pub type ctex_texture_export_output_callback = ::std::option::Option<
    unsafe extern "C" fn(
        output: *const ctex_texture_export_encoded_output,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result,
>;
pub type ctex_texture_export_report_callback = ::std::option::Option<
    unsafe extern "C" fn(
        json: *const ::std::os::raw::c_char,
        json_size: usize,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result,
>;
pub type ctex_texture_export_progress_callback = ::std::option::Option<
    unsafe extern "C" fn(
        completed_outputs: usize,
        total_outputs: usize,
        relative_path: *const ::std::os::raw::c_char,
        user_data: *mut ::std::os::raw::c_void,
    ),
>;
pub type ctex_texture_export_cancel_callback =
    ::std::option::Option<unsafe extern "C" fn(user_data: *mut ::std::os::raw::c_void) -> u32>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_export_callbacks_descriptor {
    pub size: u32,
    pub source: ctex_texture_export_source_callback,
    pub output: ctex_texture_export_output_callback,
    pub report: ctex_texture_export_report_callback,
    pub progress: ctex_texture_export_progress_callback,
    pub cancel: ctex_texture_export_cancel_callback,
    pub user_data: *mut ::std::os::raw::c_void,
}
impl Default for ctex_texture_export_callbacks_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_texture_export_info {
    pub size: u32,
    pub planned_output_count: usize,
    pub encoded_output_count: usize,
    pub dry_run: u32,
    pub cancelled: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_container_read_limits_descriptor {
    pub size: u32,
    pub maximum_input_bytes: usize,
    pub maximum_total_allocation_bytes: usize,
    pub maximum_sections: usize,
    pub maximum_images: usize,
    pub maximum_tiles: usize,
    pub maximum_resources: usize,
    pub maximum_assets: usize,
    pub maximum_asset_dependencies: usize,
    pub maximum_string_bytes: usize,
    pub maximum_decoded_tile_bytes: usize,
    pub maximum_packed_resource_bytes: usize,
    pub maximum_asset_payload_bytes: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_container_version {
    pub size: u32,
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_container_info {
    pub size: u32,
    pub source_schema: ctex_project_container_version,
    pub newer_schema: u32,
    pub tiled_image_count: usize,
    pub resource_count: usize,
    pub asset_count: usize,
    pub opaque_section_count: usize,
    pub occupied_tile_count: usize,
    pub packed_resource_bytes: usize,
    pub canonical_size: usize,
    pub report_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_document_mesh_state_descriptor {
    pub size: u32,
    pub document_asset_id: *const ::std::os::raw::c_char,
    pub mesh_resource_id: *const ::std::os::raw::c_char,
    pub current_mesh_revision: u64,
    pub map_sets: *const *const ctex_mesh_map_set,
    pub map_set_count: usize,
}
impl Default for ctex_document_mesh_state_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_document_mesh_state_info {
    pub size: u32,
    pub current_mesh_revision: u64,
    pub map_count: usize,
    pub texture_set_count: usize,
    pub required_mesh_resource_id_size: usize,
    pub required_texture_set_ids_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_project_autosave_config_descriptor {
    pub size: u32,
    pub recovery_directory: *const ::std::os::raw::c_char,
    pub recovery_key: *const ::std::os::raw::c_char,
    pub interval_milliseconds: u64,
}
impl Default for ctex_project_autosave_config_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_project_autosave_submission_status_CTEX_PROJECT_AUTOSAVE_QUEUED:
    ctex_project_autosave_submission_status = 0;
pub const ctex_project_autosave_submission_status_CTEX_PROJECT_AUTOSAVE_STALE_REVISION:
    ctex_project_autosave_submission_status = 1;
pub type ctex_project_autosave_submission_status = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_autosave_info {
    pub size: u32,
    pub has_last_saved_revision: u32,
    pub last_saved_revision: u64,
    pub has_pending_revision: u32,
    pub pending_revision: u64,
    pub has_saving_revision: u32,
    pub saving_revision: u64,
    pub successful_writes: u64,
    pub required_recovery_path_size: usize,
    pub required_last_error_size: usize,
}
pub type ctex_project_quiesce_cancel_callback =
    ::std::option::Option<unsafe extern "C" fn(user_data: *mut ::std::os::raw::c_void)>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_project_quiesce_descriptor {
    pub size: u32,
    pub current_revision: u64,
    pub deadline_milliseconds: u64,
    pub request_cancel: ctex_project_quiesce_cancel_callback,
    pub user_data: *mut ::std::os::raw::c_void,
}
impl Default for ctex_project_quiesce_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_project_quiesce_status_CTEX_PROJECT_QUIESCE_DURABLE: ctex_project_quiesce_status = 0;
pub const ctex_project_quiesce_status_CTEX_PROJECT_QUIESCE_DEADLINE_EXCEEDED:
    ctex_project_quiesce_status = 1;
pub const ctex_project_quiesce_status_CTEX_PROJECT_QUIESCE_CHECKPOINT_FAILED:
    ctex_project_quiesce_status = 2;
pub type ctex_project_quiesce_status = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_quiesce_report {
    pub size: u32,
    pub status: u32,
    pub admissions_stopped: u32,
    pub cancellation_requested: u32,
    pub work_drained: u32,
    pub active_operation_count: usize,
    pub has_durable_revision: u32,
    pub durable_revision: u64,
    pub has_uncheckpointed_range: u32,
    pub uncheckpointed_first_revision: u64,
    pub uncheckpointed_last_revision: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_recovery_checkpoint_info {
    pub size: u32,
    pub has_revision: u32,
    pub revision: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_recovery_entry {
    pub recovery_key_offset: usize,
    pub recovery_key_size: usize,
    pub path_offset: usize,
    pub path_size: usize,
    pub schema: ctex_project_container_version,
    pub file_bytes: u64,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_recovery_rejection {
    pub path_offset: usize,
    pub path_size: usize,
    pub message_offset: usize,
    pub message_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_recovery_enumeration_info {
    pub size: u32,
    pub required_recoverable_count: usize,
    pub required_rejected_count: usize,
    pub required_string_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_project_asset_export_options_descriptor {
    pub size: u32,
    pub self_contained: u32,
    pub source_directory: *const ::std::os::raw::c_char,
}
impl Default for ctex_project_asset_export_options_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_project_asset_search_paths_descriptor {
    pub size: u32,
    pub paths: *const *const ::std::os::raw::c_char,
    pub path_count: usize,
}
impl Default for ctex_project_asset_search_paths_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_project_resource_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub kind: *const ::std::os::raw::c_char,
    pub relative_path: *const ::std::os::raw::c_char,
    pub packed: u32,
    pub packed_bytes: *const ::std::os::raw::c_void,
    pub packed_byte_count: usize,
}
impl Default for ctex_project_resource_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_operation_replay_class_CTEX_OPERATION_REPLAY_CHECKPOINT_ONLY:
    ctex_operation_replay_class = 0;
pub const ctex_operation_replay_class_CTEX_OPERATION_REPLAY_SAME_RESOLUTION:
    ctex_operation_replay_class = 1;
pub const ctex_operation_replay_class_CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT:
    ctex_operation_replay_class = 2;
pub type ctex_operation_replay_class = ::std::os::raw::c_uint;
pub const ctex_operation_payload_kind_CTEX_OPERATION_PAYLOAD_RESOLVED_STAMPS:
    ctex_operation_payload_kind = 0;
pub const ctex_operation_payload_kind_CTEX_OPERATION_PAYLOAD_EDITABLE_SOURCE_PATH:
    ctex_operation_payload_kind = 1;
pub const ctex_operation_payload_kind_CTEX_OPERATION_PAYLOAD_OPAQUE_ALGORITHM_DATA:
    ctex_operation_payload_kind = 2;
pub type ctex_operation_payload_kind = ::std::os::raw::c_uint;
pub const ctex_operation_replay_disposition_CTEX_OPERATION_REPLAY_SAME_RESOLUTION_AVAILABLE:
    ctex_operation_replay_disposition = 0;
pub const ctex_operation_replay_disposition_CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT_AVAILABLE : ctex_operation_replay_disposition = 1 ;
pub const ctex_operation_replay_disposition_CTEX_OPERATION_REPLAY_CHECKPOINT_ONLY_AVAILABLE:
    ctex_operation_replay_disposition = 2;
pub const ctex_operation_replay_disposition_CTEX_OPERATION_REPLAY_RESAMPLE_CHECKPOINT_REQUIRED:
    ctex_operation_replay_disposition = 3;
pub const ctex_operation_replay_disposition_CTEX_OPERATION_REPLAY_UNSUPPORTED_ALGORITHM:
    ctex_operation_replay_disposition = 4;
pub type ctex_operation_replay_disposition = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_operation_channel_descriptor {
    pub size: u32,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub component_count: u32,
    pub scalar_representation: u32,
    pub bit_depth: u32,
    pub color_space: u32,
    pub default_value: *const f64,
    pub default_value_count: usize,
}
impl Default for ctex_operation_channel_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_pinned_operation_resource_descriptor {
    pub size: u32,
    pub role: *const ::std::os::raw::c_char,
    pub content_identity: *const ::std::os::raw::c_char,
    pub bytes: *const ::std::os::raw::c_void,
    pub byte_count: usize,
}
impl Default for ctex_pinned_operation_resource_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_operation_record_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub algorithm_identifier: *const ::std::os::raw::c_char,
    pub algorithm_version: u32,
    pub preset_identifier: *const ::std::os::raw::c_char,
    pub preset_version: u32,
    pub replay_class: u32,
    pub input_document_revision: u64,
    pub seed: u64,
    pub mesh_content_identity: *const ::std::os::raw::c_char,
    pub coordinate_frame: [f64; 16usize],
    pub payload_kind: u32,
    pub payload_version: u32,
    pub channels: *const ctex_operation_channel_descriptor,
    pub channel_count: usize,
    pub pinned_resources: *const ctex_pinned_operation_resource_descriptor,
    pub pinned_resource_count: usize,
    pub checkpoint_image_identifiers: *const *const ::std::os::raw::c_char,
    pub checkpoint_image_count: usize,
    pub payload: *const ::std::os::raw::c_void,
    pub payload_size: usize,
}
impl Default for ctex_operation_record_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_operation_record_info {
    pub size: u32,
    pub schema_version: u32,
    pub replay_class: u32,
    pub payload_kind: u32,
    pub payload_version: u32,
    pub input_document_revision: u64,
    pub seed: u64,
    pub channel_count: usize,
    pub pinned_resource_count: usize,
    pub checkpoint_image_count: usize,
    pub pinned_resource_bytes: usize,
    pub payload_size: usize,
    pub canonical_size: usize,
    pub report_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_operation_algorithm_support_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub minimum_version: u32,
    pub maximum_version: u32,
}
impl Default for ctex_operation_algorithm_support_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_operation_replay_assessment_descriptor {
    pub size: u32,
    pub supported_algorithms: *const ctex_operation_algorithm_support_descriptor,
    pub supported_algorithm_count: usize,
    pub target_resolution_changed: u32,
}
impl Default for ctex_operation_replay_assessment_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_operation_replay_info {
    pub size: u32,
    pub disposition: u32,
    pub declared_replay_class: u32,
    pub replay_available: u32,
    pub checkpoint_available: u32,
    pub target_resolution_changed: u32,
    pub required_report_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_project_operation_replay_info {
    pub size: u32,
    pub record_count: usize,
    pub replay_available_count: usize,
    pub checkpoint_fallback_count: usize,
    pub unsupported_algorithm_count: usize,
    pub required_report_size: usize,
}
pub const ctex_resolution_change_policy_CTEX_RESOLUTION_REPLAY_ELIGIBLE:
    ctex_resolution_change_policy = 0;
pub const ctex_resolution_change_policy_CTEX_RESOLUTION_RESAMPLE_ALL:
    ctex_resolution_change_policy = 1;
pub const ctex_resolution_change_policy_CTEX_RESOLUTION_CANCEL: ctex_resolution_change_policy = 2;
pub type ctex_resolution_change_policy = ::std::os::raw::c_uint;
pub const ctex_checkpoint_resample_policy_CTEX_CHECKPOINT_RESAMPLE_REFUSE:
    ctex_checkpoint_resample_policy = 0;
pub const ctex_checkpoint_resample_policy_CTEX_CHECKPOINT_RESAMPLE_NEAREST:
    ctex_checkpoint_resample_policy = 1;
pub const ctex_checkpoint_resample_policy_CTEX_CHECKPOINT_RESAMPLE_BILINEAR:
    ctex_checkpoint_resample_policy = 2;
pub type ctex_checkpoint_resample_policy = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_resolution_operation_record_descriptor {
    pub size: u32,
    pub canonical_record: *const ::std::os::raw::c_void,
    pub canonical_record_size: usize,
}
impl Default for ctex_resolution_operation_record_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_resolution_replay_raster_descriptor {
    pub size: u32,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub udim_tile_number: u32,
    pub pixels: *const ::std::os::raw::c_void,
    pub pixel_bytes: usize,
}
impl Default for ctex_resolution_replay_raster_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_texture_set_resolution_change_descriptor {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub policy: u32,
    pub checkpoint_policy: u32,
    pub operation_records: *const ctex_resolution_operation_record_descriptor,
    pub operation_record_count: usize,
    pub supported_algorithms: *const ctex_operation_algorithm_support_descriptor,
    pub supported_algorithm_count: usize,
    pub replay_rasters: *const ctex_resolution_replay_raster_descriptor,
    pub replay_raster_count: usize,
    pub maximum_working_bytes: usize,
    pub maximum_history_bytes: usize,
}
impl Default for ctex_texture_set_resolution_change_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_texture_set_resolution_change_info {
    pub size: u32,
    pub committed: u32,
    pub policy: u32,
    pub source_width: u32,
    pub source_height: u32,
    pub target_width: u32,
    pub target_height: u32,
    pub replayed_source_count: usize,
    pub resampled_source_count: usize,
    pub procedural_entry_count: usize,
    pub raster_count: usize,
    pub staged_pixel_bytes: usize,
    pub retained_history_bytes: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_texture_set_resolution_restore_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub retained_history_bytes: usize,
}
pub const ctex_editable_entry_kind_CTEX_EDITABLE_ENTRY_DECAL: ctex_editable_entry_kind = 0;
pub const ctex_editable_entry_kind_CTEX_EDITABLE_ENTRY_TEXT: ctex_editable_entry_kind = 1;
pub const ctex_editable_entry_kind_CTEX_EDITABLE_ENTRY_SURFACE_PATH: ctex_editable_entry_kind = 2;
pub type ctex_editable_entry_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_editable_placement_frame {
    pub position: ctex_vec3d,
    pub normal: ctex_vec3d,
    pub rotation_radians: f64,
    pub uniform_scale: f64,
    pub axis_scale: ctex_vec2d,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_editable_material_parameter_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub component_count: u32,
    pub value: [f64; 4usize],
}
impl Default for ctex_editable_material_parameter_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_editable_tile_dependency_descriptor {
    pub size: u32,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub tile_x: u32,
    pub tile_y: u32,
}
impl Default for ctex_editable_tile_dependency_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_editable_surface_point_descriptor {
    pub size: u32,
    pub position: ctex_vec3d,
    pub normal: ctex_vec3d,
    pub triangle: u32,
    pub barycentric: [f64; 3usize],
    pub width: f64,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_editable_entry_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub kind: u32,
    pub expected_revision: u64,
    pub placement: ctex_editable_placement_frame,
    pub material_identity: *const ::std::os::raw::c_char,
    pub material_parameters: *const ctex_editable_material_parameter_descriptor,
    pub material_parameter_count: usize,
    pub text: *const ::std::os::raw::c_char,
    pub font_identity: *const ::std::os::raw::c_char,
    pub mesh_revision: u64,
    pub surface_points: *const ctex_editable_surface_point_descriptor,
    pub surface_point_count: usize,
    pub dependent_tiles: *const ctex_editable_tile_dependency_descriptor,
    pub dependent_tile_count: usize,
}
impl Default for ctex_editable_entry_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_editable_entry_info {
    pub size: u32,
    pub kind: u32,
    pub entry_present: u32,
    pub entry_revision: u64,
    pub document_revision: u64,
    pub entry_count: usize,
    pub material_parameter_count: usize,
    pub surface_point_count: usize,
    pub invalidated_tile_count: usize,
    pub undo_step_count: usize,
    pub redo_step_count: usize,
    pub required_report_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_preset_shelf_entry_descriptor {
    pub size: u32,
    pub asset_identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub tags: *const *const ::std::os::raw::c_char,
    pub tag_count: usize,
    pub thumbnail_resource_identifier: *const ::std::os::raw::c_char,
}
impl Default for ctex_preset_shelf_entry_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_preset_shelf_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub contents: *const ::std::os::raw::c_void,
    pub contents_size: usize,
    pub entries: *const ctex_preset_shelf_entry_descriptor,
    pub entry_count: usize,
}
impl Default for ctex_preset_shelf_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_preset_library_descriptor {
    pub size: u32,
    pub shelves: *const ctex_preset_shelf_descriptor,
    pub shelf_count: usize,
    pub read_limits: *const ctex_project_container_read_limits_descriptor,
}
impl Default for ctex_preset_library_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_preset_library_info {
    pub size: u32,
    pub shelf_count: usize,
    pub preset_count: usize,
    pub report_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_material_graph_catalogue_info {
    pub size: u32,
    pub node_count: usize,
    pub input_node_count: usize,
    pub texture_node_count: usize,
    pub colour_filter_node_count: usize,
    pub vector_math_node_count: usize,
    pub math_operation_count: usize,
    pub vector_math_operation_count: usize,
    pub report_size: usize,
}
pub const ctex_material_graph_socket_coercion_CTEX_MATERIAL_GRAPH_COERCION_IDENTITY:
    ctex_material_graph_socket_coercion = 0;
pub const ctex_material_graph_socket_coercion_CTEX_MATERIAL_GRAPH_COERCION_SCALAR_TO_VECTOR:
    ctex_material_graph_socket_coercion = 1;
pub const ctex_material_graph_socket_coercion_CTEX_MATERIAL_GRAPH_COERCION_VECTOR_TO_SCALAR:
    ctex_material_graph_socket_coercion = 2;
pub const ctex_material_graph_socket_coercion_CTEX_MATERIAL_GRAPH_COERCION_COLOUR_TO_VECTOR:
    ctex_material_graph_socket_coercion = 3;
pub const ctex_material_graph_socket_coercion_CTEX_MATERIAL_GRAPH_COERCION_COLOUR_TO_SCALAR:
    ctex_material_graph_socket_coercion = 4;
pub type ctex_material_graph_socket_coercion = ::std::os::raw::c_uint;
pub const ctex_material_graph_diagnostic_code_CTEX_MATERIAL_GRAPH_UNCONNECTED_REQUIRED_INPUT:
    ctex_material_graph_diagnostic_code = 0;
pub const ctex_material_graph_diagnostic_code_CTEX_MATERIAL_GRAPH_MISSING_MESH_MAP:
    ctex_material_graph_diagnostic_code = 1;
pub const ctex_material_graph_diagnostic_code_CTEX_MATERIAL_GRAPH_MISSING_IMAGE_RESOURCE:
    ctex_material_graph_diagnostic_code = 2;
pub const ctex_material_graph_diagnostic_code_CTEX_MATERIAL_GRAPH_MISSING_GROUP:
    ctex_material_graph_diagnostic_code = 3;
pub const ctex_material_graph_diagnostic_code_CTEX_MATERIAL_GRAPH_MISSING_NODE_TYPE:
    ctex_material_graph_diagnostic_code = 4;
pub const ctex_material_graph_diagnostic_code_CTEX_MATERIAL_GRAPH_INCOMPATIBLE_NODE_INTERFACE:
    ctex_material_graph_diagnostic_code = 5;
pub const ctex_material_graph_diagnostic_code_CTEX_MATERIAL_GRAPH_UNSUPPORTED_EMISSION_TARGET:
    ctex_material_graph_diagnostic_code = 6;
pub const ctex_material_graph_diagnostic_code_CTEX_MATERIAL_GRAPH_UNREACHABLE_NODE:
    ctex_material_graph_diagnostic_code = 7;
pub type ctex_material_graph_diagnostic_code = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
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
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_link_descriptor {
    pub size: u32,
    pub source_node: u64,
    pub source_socket: *const ::std::os::raw::c_char,
    pub target_node: u64,
    pub target_socket: *const ::std::os::raw::c_char,
}
impl Default for ctex_material_graph_link_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_material_graph_link_info {
    pub size: u32,
    pub graph: ctex_material_graph_info,
    pub coercion: u32,
    pub replaced: u32,
    pub replaced_source_node: u64,
    pub replaced_source_socket_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_validation_resources_descriptor {
    pub size: u32,
    pub image_resources: *const *const ::std::os::raw::c_char,
    pub image_resource_count: usize,
    pub mesh_maps: *const *const ::std::os::raw::c_char,
    pub mesh_map_count: usize,
}
impl Default for ctex_material_graph_validation_resources_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_material_graph_validation_info {
    pub size: u32,
    pub valid: u32,
    pub error_count: usize,
    pub warning_count: usize,
    pub diagnostic_count: usize,
    pub report_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_material_graph_library_info {
    pub size: u32,
    pub preset_count: usize,
    pub canonical_size: usize,
    pub report_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_preset_descriptor {
    pub size: u32,
    pub stable_id: *const ::std::os::raw::c_char,
    pub name: *const ::std::os::raw::c_char,
    pub thumbnail_resource: *const ::std::os::raw::c_char,
    pub graph_serialized: *const ::std::os::raw::c_void,
    pub graph_serialized_size: usize,
}
impl Default for ctex_material_graph_preset_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_smart_material_value_type_CTEX_SMART_MATERIAL_VALUE_SCALAR:
    ctex_smart_material_value_type = 0;
pub const ctex_smart_material_value_type_CTEX_SMART_MATERIAL_VALUE_VECTOR:
    ctex_smart_material_value_type = 1;
pub const ctex_smart_material_value_type_CTEX_SMART_MATERIAL_VALUE_COLOUR:
    ctex_smart_material_value_type = 2;
pub const ctex_smart_material_value_type_CTEX_SMART_MATERIAL_VALUE_STRING:
    ctex_smart_material_value_type = 3;
pub const ctex_smart_material_value_type_CTEX_SMART_MATERIAL_VALUE_IMAGE:
    ctex_smart_material_value_type = 4;
pub const ctex_smart_material_value_type_CTEX_SMART_MATERIAL_VALUE_BOOLEAN:
    ctex_smart_material_value_type = 5;
pub type ctex_smart_material_value_type = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_smart_material_value_descriptor {
    pub size: u32,
    pub type_: u32,
    pub scalar: f64,
    pub vector: ctex_vec3f,
    pub colour: ctex_vec4f,
    pub text: *const ::std::os::raw::c_char,
    pub boolean: u32,
}
impl Default for ctex_smart_material_value_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub const ctex_material_graph_owner_kind_CTEX_MATERIAL_GRAPH_OWNER_MATERIAL:
    ctex_material_graph_owner_kind = 0;
pub const ctex_material_graph_owner_kind_CTEX_MATERIAL_GRAPH_OWNER_GROUP:
    ctex_material_graph_owner_kind = 1;
pub type ctex_material_graph_owner_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_socket_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub type_: u32,
    pub default_value: *const ctex_smart_material_value_descriptor,
}
impl Default for ctex_material_graph_socket_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_group_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub inputs: *const ctex_material_graph_socket_descriptor,
    pub input_count: usize,
    pub outputs: *const ctex_material_graph_socket_descriptor,
    pub output_count: usize,
}
impl Default for ctex_material_graph_group_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_group_interface_descriptor {
    pub size: u32,
    pub inputs: *const ctex_material_graph_socket_descriptor,
    pub input_count: usize,
    pub outputs: *const ctex_material_graph_socket_descriptor,
    pub output_count: usize,
}
impl Default for ctex_material_graph_group_interface_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_material_graph_workspace_info {
    pub size: u32,
    pub material_count: usize,
    pub group_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_material_graph_group_update_info {
    pub size: u32,
    pub group_version: u32,
    pub instances_updated: usize,
    pub removed_link_count: usize,
}
pub const ctex_material_graph_emission_target_CTEX_MATERIAL_GRAPH_TARGET_WGSL:
    ctex_material_graph_emission_target = 0;
pub const ctex_material_graph_emission_target_CTEX_MATERIAL_GRAPH_TARGET_MSL:
    ctex_material_graph_emission_target = 1;
pub const ctex_material_graph_emission_target_CTEX_MATERIAL_GRAPH_TARGET_SPIRV:
    ctex_material_graph_emission_target = 2;
pub const ctex_material_graph_emission_target_CTEX_MATERIAL_GRAPH_TARGET_HLSL:
    ctex_material_graph_emission_target = 3;
pub type ctex_material_graph_emission_target = ::std::os::raw::c_uint;
pub const ctex_shader_filter_mode_CTEX_SHADER_FILTER_NEAREST: ctex_shader_filter_mode = 0;
pub const ctex_shader_filter_mode_CTEX_SHADER_FILTER_LINEAR: ctex_shader_filter_mode = 1;
pub type ctex_shader_filter_mode = ::std::os::raw::c_uint;
pub const ctex_shader_preview_kind_CTEX_SHADER_PREVIEW_LIT: ctex_shader_preview_kind = 0;
pub const ctex_shader_preview_kind_CTEX_SHADER_PREVIEW_CHANNEL_INSPECTION:
    ctex_shader_preview_kind = 1;
pub type ctex_shader_preview_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_texture_descriptor {
    pub size: u32,
    pub logical_id: *const ::std::os::raw::c_char,
    pub generation: u64,
    pub role: *const ::std::os::raw::c_char,
    pub format: u32,
    pub width: u32,
    pub height: u32,
    pub layers: u32,
    pub mip_levels: u32,
    pub tile_width: u32,
    pub tile_height: u32,
    pub externally_initialized: u32,
}
impl Default for ctex_shader_texture_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_material_resource_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub texture: ctex_shader_texture_descriptor,
}
impl Default for ctex_shader_material_resource_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_device_features_descriptor {
    pub size: u32,
    pub binding_budget: u32,
    pub maximum_texture_dimension: u32,
    pub supported_texture_formats: *const u32,
    pub supported_texture_format_count: usize,
    pub floating_point_filtering: u32,
    pub compute_available: u32,
}
impl Default for ctex_shader_device_features_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_material_request {
    pub size: u32,
    pub stable_identity: *const ::std::os::raw::c_char,
    pub target: u32,
    pub features: ctex_shader_device_features_descriptor,
    pub resources: *const ctex_shader_material_resource_descriptor,
    pub resource_count: usize,
    pub output: ctex_shader_texture_descriptor,
    pub requested_filter: u32,
    pub vertex_count: u32,
}
impl Default for ctex_shader_material_request {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
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
pub const ctex_shader_material_source_kind_CTEX_SHADER_MATERIAL_SOURCE_SERIALIZED_GRAPH:
    ctex_shader_material_source_kind = 0;
pub const ctex_shader_material_source_kind_CTEX_SHADER_MATERIAL_SOURCE_WORKSPACE:
    ctex_shader_material_source_kind = 1;
pub type ctex_shader_material_source_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_material_source_descriptor {
    pub size: u32,
    pub kind: u32,
    pub graph_serialized: *const ::std::os::raw::c_void,
    pub graph_serialized_size: usize,
    pub workspace: *const ctex_material_graph_workspace,
    pub material_identifier: *const ::std::os::raw::c_char,
}
impl Default for ctex_shader_material_source_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_shader_material_debug_info {
    pub size: u32,
    pub node_attribution_count: usize,
    pub metadata_size: usize,
    pub binary_companion: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_shader_backend_attribution_info {
    pub size: u32,
    pub report_size: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_layer_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub texture: ctex_shader_texture_descriptor,
}
impl Default for ctex_shader_layer_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_layer_stack_request {
    pub size: u32,
    pub stable_identity: *const ::std::os::raw::c_char,
    pub target: u32,
    pub features: ctex_shader_device_features_descriptor,
    pub layers: *const ctex_shader_layer_descriptor,
    pub layer_count: usize,
    pub output: ctex_shader_texture_descriptor,
    pub requested_filter: u32,
}
impl Default for ctex_shader_layer_stack_request {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_shader_layer_stack_info {
    pub size: u32,
    pub target: u32,
    pub layer_count: usize,
    pub pass_count: usize,
    pub artifact_blob_size: usize,
    pub artifact_report_size: usize,
    pub pass_plan_size: usize,
    pub workaround_report_size: usize,
    pub workaround_count: usize,
    pub compute_used: u32,
    pub cache_hit: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_shader_emission_cache_info {
    pub size: u32,
    pub entry_count: usize,
    pub hit_count: usize,
    pub miss_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_preview_channel_descriptor {
    pub size: u32,
    pub semantic_id: *const ::std::os::raw::c_char,
    pub component_count: u32,
    pub texture: ctex_shader_texture_descriptor,
}
impl Default for ctex_shader_preview_channel_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_preview_environment_descriptor {
    pub size: u32,
    pub radiance: ctex_shader_texture_descriptor,
    pub diffuse_irradiance: ctex_shader_texture_descriptor,
    pub specular_brdf_lookup: ctex_shader_texture_descriptor,
}
impl Default for ctex_shader_preview_environment_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_shader_preview_request {
    pub size: u32,
    pub stable_identity: *const ::std::os::raw::c_char,
    pub target: u32,
    pub features: ctex_shader_device_features_descriptor,
    pub channels: *const ctex_shader_preview_channel_descriptor,
    pub channel_count: usize,
    pub output: ctex_shader_texture_descriptor,
    pub environment: *const ctex_shader_preview_environment_descriptor,
    pub analytic_light_count: usize,
    pub vertex_count: u32,
}
impl Default for ctex_shader_preview_request {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_shader_preview_info {
    pub size: u32,
    pub target: u32,
    pub kind: u32,
    pub fallback_lighting: u32,
    pub cache_hit: u32,
    pub vertex_artifact_size: usize,
    pub fragment_artifact_size: usize,
    pub pass_plan_size: usize,
    pub workaround_report_size: usize,
    pub pass_count: usize,
    pub logical_resource_count: usize,
    pub binding_count: usize,
    pub workaround_count: usize,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_property_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub display_name: *const ::std::os::raw::c_char,
    pub default_value: *const ctex_smart_material_value_descriptor,
    pub allowed_values: *const *const ::std::os::raw::c_char,
    pub allowed_value_count: usize,
}
impl Default for ctex_material_graph_property_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_parity_fixture_descriptor {
    pub size: u32,
    pub identifier: *const ::std::os::raw::c_char,
    pub inputs: *const ctex_smart_material_value_descriptor,
    pub input_count: usize,
    pub expected_outputs: *const ctex_smart_material_value_descriptor,
    pub expected_output_count: usize,
    pub tolerance: f64,
}
impl Default for ctex_material_graph_parity_fixture_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_host_property_value {
    pub identifier: *const ::std::os::raw::c_char,
    pub value: ctex_smart_material_value_descriptor,
}
impl Default for ctex_material_graph_host_property_value {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_host_evaluation_request {
    pub size: u32,
    pub node_id: u64,
    pub type_id: *const ::std::os::raw::c_char,
    pub type_version: u32,
    pub properties: *const ctex_material_graph_host_property_value,
    pub property_count: usize,
    pub inputs: *const ctex_smart_material_value_descriptor,
    pub input_count: usize,
}
impl Default for ctex_material_graph_host_evaluation_request {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type ctex_material_graph_cpu_evaluate_callback = ::std::option::Option<
    unsafe extern "C" fn(
        request: *const ctex_material_graph_host_evaluation_request,
        outputs: *mut ctex_smart_material_value_descriptor,
        output_count: usize,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result,
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_host_emission_request {
    pub size: u32,
    pub node_id: u64,
    pub type_id: *const ::std::os::raw::c_char,
    pub type_version: u32,
    pub target: u32,
    pub properties: *const ctex_material_graph_host_property_value,
    pub property_count: usize,
    pub input_expressions: *const *const ::std::os::raw::c_char,
    pub input_expression_count: usize,
}
impl Default for ctex_material_graph_host_emission_request {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_host_emission_result {
    pub size: u32,
    pub output_expressions: *const *const ::std::os::raw::c_char,
    pub output_expression_count: usize,
    pub resource_identifiers: *const *const ::std::os::raw::c_char,
    pub resource_identifier_count: usize,
}
impl Default for ctex_material_graph_host_emission_result {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
pub type ctex_material_graph_emit_callback = ::std::option::Option<
    unsafe extern "C" fn(
        request: *const ctex_material_graph_host_emission_request,
        out_result: *mut ctex_material_graph_host_emission_result,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result,
>;
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_material_graph_host_node_registration_descriptor {
    pub size: u32,
    pub type_id: *const ::std::os::raw::c_char,
    pub type_version: u32,
    pub display_name: *const ::std::os::raw::c_char,
    pub inputs: *const ctex_material_graph_socket_descriptor,
    pub input_count: usize,
    pub outputs: *const ctex_material_graph_socket_descriptor,
    pub output_count: usize,
    pub properties: *const ctex_material_graph_property_descriptor,
    pub property_count: usize,
    pub cpu_evaluate: ctex_material_graph_cpu_evaluate_callback,
    pub emit: ctex_material_graph_emit_callback,
    pub user_data: *mut ::std::os::raw::c_void,
    pub deterministic: u32,
    pub resource_dependencies: *const *const ::std::os::raw::c_char,
    pub resource_dependency_count: usize,
    pub supported_targets: *const u32,
    pub supported_target_count: usize,
    pub parity_fixtures: *const ctex_material_graph_parity_fixture_descriptor,
    pub parity_fixture_count: usize,
}
impl Default for ctex_material_graph_host_node_registration_descriptor {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_material_graph_node_registry_info {
    pub size: u32,
    pub registration_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_material_graph_host_contract_info {
    pub size: u32,
    pub parity_passed: u32,
    pub parity_failure_count: usize,
    pub replay_eligible: u32,
    pub unpinned_dependency_count: usize,
    pub report_size: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_smart_material_info {
    pub size: u32,
    pub source_schema_version: u32,
    pub canonical_schema_version: u32,
    pub entry_count: usize,
    pub derived_entry_count: usize,
    pub model_specific_entry_count: usize,
    pub model_specific_pixel_bytes: usize,
    pub exposed_parameter_count: usize,
    pub anchor_count: usize,
    pub anchor_reference_count: usize,
    pub resource_reference_count: usize,
    pub canonical_size: usize,
    pub report_size: usize,
}
pub const ctex_applied_preset_kind_CTEX_APPLIED_PRESET_SMART_MATERIAL: ctex_applied_preset_kind = 0;
pub const ctex_applied_preset_kind_CTEX_APPLIED_PRESET_SMART_MASK: ctex_applied_preset_kind = 1;
pub type ctex_applied_preset_kind = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_preset_application_info {
    pub size: u32,
    pub kind: u32,
    pub schema_version: u32,
    pub entry_count: usize,
    pub application_count: usize,
    pub undo_step_count: usize,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_preset_undo_info {
    pub size: u32,
    pub removed: u32,
    pub removed_entry_count: usize,
    pub application_count: usize,
    pub undo_step_count: usize,
}
pub const ctex_color_space_CTEX_COLOR_SPACE_LINEAR_REC709: ctex_color_space = 0;
pub const ctex_color_space_CTEX_COLOR_SPACE_SRGB_REC709: ctex_color_space = 1;
pub type ctex_color_space = ::std::os::raw::c_uint;
pub const ctex_input_color_space_CTEX_INPUT_COLOR_SPACE_AUTOMATIC: ctex_input_color_space = 0;
pub const ctex_input_color_space_CTEX_INPUT_COLOR_SPACE_LINEAR_REC709: ctex_input_color_space = 1;
pub const ctex_input_color_space_CTEX_INPUT_COLOR_SPACE_SRGB_REC709: ctex_input_color_space = 2;
pub type ctex_input_color_space = ::std::os::raw::c_uint;
pub const ctex_channel_semantic_CTEX_CHANNEL_SEMANTIC_BASE_COLOR: ctex_channel_semantic = 0;
pub const ctex_channel_semantic_CTEX_CHANNEL_SEMANTIC_OPACITY: ctex_channel_semantic = 1;
pub const ctex_channel_semantic_CTEX_CHANNEL_SEMANTIC_ROUGHNESS: ctex_channel_semantic = 2;
pub const ctex_channel_semantic_CTEX_CHANNEL_SEMANTIC_METALLIC: ctex_channel_semantic = 3;
pub const ctex_channel_semantic_CTEX_CHANNEL_SEMANTIC_NORMAL: ctex_channel_semantic = 4;
pub const ctex_channel_semantic_CTEX_CHANNEL_SEMANTIC_HEIGHT: ctex_channel_semantic = 5;
pub const ctex_channel_semantic_CTEX_CHANNEL_SEMANTIC_OCCLUSION: ctex_channel_semantic = 6;
pub const ctex_channel_semantic_CTEX_CHANNEL_SEMANTIC_EMISSION: ctex_channel_semantic = 7;
pub const ctex_channel_semantic_CTEX_CHANNEL_SEMANTIC_SUBSURFACE: ctex_channel_semantic = 8;
pub type ctex_channel_semantic = ::std::os::raw::c_uint;
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_rgb_color {
    pub red: f64,
    pub green: f64,
    pub blue: f64,
    pub color_space: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_channel_color_policy {
    pub size: u32,
    pub color_valued: u32,
    pub recommended_bit_depth: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_resolved_input_color_space {
    pub size: u32,
    pub color_space: u32,
    pub inferred: u32,
}
#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct ctex_bit_depth_warning {
    pub size: u32,
    pub warning: u32,
    pub selected_bit_depth: u32,
    pub recommended_bit_depth: u32,
}
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ctex_version {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
    pub string: *const ::std::os::raw::c_char,
}
impl Default for ctex_version {
    fn default() -> Self {
        let mut s = ::std::mem::MaybeUninit::<Self>::uninit();
        unsafe {
            ::std::ptr::write_bytes(s.as_mut_ptr(), 0, 1);
            s.assume_init()
        }
    }
}
unsafe extern "C" {
    pub fn ctex_get_version() -> ctex_version;
}
unsafe extern "C" {
    pub fn ctex_get_abi_version() -> ctex_version;
}
unsafe extern "C" {
    pub fn ctex_get_working_color_space() -> ctex_color_space;
}
unsafe extern "C" {
    pub fn ctex_color_space_get_name(
        color_space: u32,
        buffer: *mut ::std::os::raw::c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_channel_get_color_policy(
        channel_semantic: u32,
        out_policy: *mut ctex_channel_color_policy,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resolve_input_color_space(
        declaration: u32,
        channel_semantic: u32,
        out_resolved: *mut ctex_resolved_input_color_space,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_color_convert(
        input: *const ctex_rgb_color,
        destination_color_space: u32,
        out_color: *mut ctex_rgb_color,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_color_input_to_working(
        input: *const ctex_rgb_color,
        channel_semantic: u32,
        out_color: *mut ctex_rgb_color,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_channel_get_bit_depth_warning(
        channel_semantic: u32,
        selected_bit_depth: u32,
        out_warning: *mut ctex_bit_depth_warning,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_accumulate_height(
        contributions: *const f64,
        contribution_count: usize,
        storage_bit_depth: u32,
        out_accumulated: *mut f64,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_quantize_unorm8(
        value: f64,
        x: u32,
        y: u32,
        dither: u32,
        out_value: *mut u8,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_cube_lut_create(
        cube_source: *const ::std::os::raw::c_char,
        cube_source_size: usize,
        out_lut: *mut *mut ctex_cube_lut,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_cube_lut_destroy(lut: *mut ctex_cube_lut);
}
unsafe extern "C" {
    pub fn ctex_cube_lut_apply_preview(
        lut: *const ctex_cube_lut,
        input: *const ctex_rgb_color,
        out_color: *mut ctex_rgb_color,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_image_decode_memory(
        encoded: *const ::std::os::raw::c_void,
        encoded_size: usize,
        source_name: *const ::std::os::raw::c_char,
        intended_channel: u32,
        input_color_space: u32,
        limits: *const ctex_image_decode_limits_descriptor,
        out_info: *mut ctex_decoded_image_info,
        pixel_buffer: *mut ::std::os::raw::c_void,
        pixel_buffer_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_image_decode_memory_bounded(
        encoded: *const ::std::os::raw::c_void,
        encoded_size: usize,
        source_name: *const ::std::os::raw::c_char,
        intended_channel: u32,
        input_color_space: u32,
        limits: *const ctex_image_decode_limits_descriptor,
        control: *const ctex_image_decode_control_descriptor,
        out_execution_info: *mut ctex_image_decode_execution_info,
        out_info: *mut ctex_decoded_image_info,
        pixel_buffer: *mut ::std::os::raw::c_void,
        pixel_buffer_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_image_decode_layered_memory(
        encoded: *const ::std::os::raw::c_void,
        encoded_size: usize,
        source_name: *const ::std::os::raw::c_char,
        descriptor: *const ctex_layered_image_decode_descriptor,
        limits: *const ctex_image_decode_limits_descriptor,
        control: *const ctex_image_decode_control_descriptor,
        out_execution_info: *mut ctex_image_decode_execution_info,
        out_info: *mut ctex_layered_image_decode_info,
        image_infos: *mut ctex_layered_decoded_image_info,
        image_info_capacity: usize,
        name_buffer: *mut ::std::os::raw::c_char,
        name_buffer_size: usize,
        pixel_buffer: *mut ::std::os::raw::c_void,
        pixel_buffer_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_image_expand_channels(
        source_pixels: *const ::std::os::raw::c_void,
        source_pixel_buffer_size: usize,
        descriptor: *const ctex_image_channel_expansion_descriptor,
        out_info: *mut ctex_image_channel_expansion_info,
        output_pixels: *mut ::std::os::raw::c_void,
        output_pixel_buffer_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_image_resample(
        source_pixels: *const ::std::os::raw::c_void,
        source_pixel_buffer_size: usize,
        descriptor: *const ctex_image_resample_descriptor,
        out_info: *mut ctex_image_resample_info,
        output_pixels: *mut ::std::os::raw::c_void,
        output_pixel_buffer_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_image_encode_memory(
        pixels: *const ::std::os::raw::c_void,
        pixel_buffer_size: usize,
        descriptor: *const ctex_image_encode_descriptor,
        encoded_buffer: *mut ::std::os::raw::c_void,
        encoded_buffer_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_export_get_built_in_preset_ids(
        buffer: *mut ::std::os::raw::c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
        out_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_export_run(
        catalogue: *const ctex_texture_export_catalogue_descriptor,
        preset: *const ctex_texture_export_preset_descriptor,
        options: *const ctex_texture_export_options_descriptor,
        callbacks: *const ctex_texture_export_callbacks_descriptor,
        out_info: *mut ctex_texture_export_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_create_empty(
        output: *mut ::std::os::raw::c_void,
        output_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_probe_version(
        encoded: *const ::std::os::raw::c_void,
        encoded_size: usize,
        out_version: *mut ctex_project_container_version,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_normalize(
        encoded: *const ::std::os::raw::c_void,
        encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        out_info: *mut ctex_project_container_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_get_texture_document_ids(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        buffer: *mut ::std::os::raw::c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
        out_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_upsert_texture_document(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        document: *const ctex_document,
        asset_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_project_container_info,
        project_output: *mut ::std::os::raw::c_void,
        project_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_restore_texture_document(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        asset_identifier: *const ::std::os::raw::c_char,
        document: *mut ctex_document,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_upsert_document_mesh_state(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        state: *const ctex_document_mesh_state_descriptor,
        out_info: *mut ctex_project_container_info,
        project_output: *mut ::std::os::raw::c_void,
        project_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_get_document_mesh_state_info(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        document_asset_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_document_mesh_state_info,
        mesh_resource_id: *mut ::std::os::raw::c_char,
        mesh_resource_id_size: usize,
        texture_set_ids: *mut ::std::os::raw::c_char,
        texture_set_ids_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_restore_document_mesh_state(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        document_asset_id: *const ::std::os::raw::c_char,
        map_sets: *const *mut ctex_mesh_map_set,
        map_set_count: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_save_atomic(
        encoded: *const ::std::os::raw::c_void,
        encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        path: *const ::std::os::raw::c_char,
        out_info: *mut ctex_project_container_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_autosave_session_create(
        config: *const ctex_project_autosave_config_descriptor,
        out_session: *mut *mut ctex_project_autosave_session,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_autosave_session_destroy(session: *mut ctex_project_autosave_session);
}
unsafe extern "C" {
    pub fn ctex_project_autosave_session_submit(
        session: *mut ctex_project_autosave_session,
        revision: u64,
        encoded: *const ::std::os::raw::c_void,
        encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        out_status: *mut u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_autosave_session_wait(
        session: *mut ctex_project_autosave_session,
        timeout_milliseconds: u64,
        out_idle: *mut u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_autosave_session_flush(
        session: *mut ctex_project_autosave_session,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_autosave_session_get_info(
        session: *const ctex_project_autosave_session,
        out_info: *mut ctex_project_autosave_info,
        recovery_path: *mut ::std::os::raw::c_char,
        recovery_path_size: usize,
        last_error: *mut ::std::os::raw::c_char,
        last_error_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_lifecycle_quiesce(
        ledger: *mut ctex_resource_ledger,
        autosave: *mut ctex_project_autosave_session,
        descriptor: *const ctex_project_quiesce_descriptor,
        out_report: *mut ctex_project_quiesce_report,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_lifecycle_resume(ledger: *mut ctex_resource_ledger) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_recovery_enumerate(
        recovery_directory: *const ::std::os::raw::c_char,
        out_info: *mut ctex_project_recovery_enumeration_info,
        recoverable: *mut ctex_project_recovery_entry,
        recoverable_capacity: usize,
        rejected: *mut ctex_project_recovery_rejection,
        rejected_capacity: usize,
        strings: *mut ::std::os::raw::c_char,
        string_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_recovery_read(
        path: *const ::std::os::raw::c_char,
        limits: *const ctex_project_container_read_limits_descriptor,
        out_info: *mut ctex_project_container_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_recovery_resume(
        path: *const ::std::os::raw::c_char,
        limits: *const ctex_project_container_read_limits_descriptor,
        out_checkpoint: *mut ctex_project_recovery_checkpoint_info,
        out_info: *mut ctex_project_container_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_asset_export(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        asset_identifier: *const ::std::os::raw::c_char,
        options: *const ctex_project_asset_export_options_descriptor,
        out_info: *mut ctex_project_container_info,
        asset_output: *mut ::std::os::raw::c_void,
        asset_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_asset_install(
        library_encoded: *const ::std::os::raw::c_void,
        library_encoded_size: usize,
        asset_encoded: *const ::std::os::raw::c_void,
        asset_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        search_paths: *const ctex_project_asset_search_paths_descriptor,
        out_info: *mut ctex_project_container_info,
        library_output: *mut ::std::os::raw::c_void,
        library_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_operation_record_create(
        descriptor: *const ctex_operation_record_descriptor,
        out_info: *mut ctex_operation_record_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_operation_record_inspect(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        out_info: *mut ctex_operation_record_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_operation_record_assess_replay(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        descriptor: *const ctex_operation_replay_assessment_descriptor,
        out_info: *mut ctex_operation_replay_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_upsert_operation_record(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        record_serialized: *const ::std::os::raw::c_void,
        record_serialized_size: usize,
        out_info: *mut ctex_project_container_info,
        project_output: *mut ::std::os::raw::c_void,
        project_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_get_operation_record(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        record_identifier: *const ::std::os::raw::c_char,
        record_output: *mut ::std::os::raw::c_void,
        record_output_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_assess_operation_replay(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        descriptor: *const ctex_operation_replay_assessment_descriptor,
        out_info: *mut ctex_project_operation_replay_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_change_resolution(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        descriptor: *const ctex_texture_set_resolution_change_descriptor,
        out_info: *mut ctex_texture_set_resolution_change_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_undo_resolution_change(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_texture_set_resolution_restore_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_redo_resolution_change(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_texture_set_resolution_restore_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_editable_entry_add(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        descriptor: *const ctex_editable_entry_descriptor,
        out_info: *mut ctex_editable_entry_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_editable_entry_edit(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        descriptor: *const ctex_editable_entry_descriptor,
        out_info: *mut ctex_editable_entry_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_editable_entry_inspect(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_editable_entry_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_editable_entry_plan_rasterization(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_editable_entry_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_editable_entry_undo(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_editable_entry_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_editable_entry_redo(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_editable_entry_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_editable_surface_path_resolve(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        settings: *const ctex_stroke_settings_descriptor,
        out_info: *mut ctex_resolved_stroke_info,
        stamps: *mut ctex_resolved_stamp,
        stamp_capacity: usize,
        out_stamp_count: *mut usize,
        swept_segments: *mut ctex_swept_segment,
        swept_segment_capacity: usize,
        out_swept_segment_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_upsert_editable_authoring(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        asset_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_project_container_info,
        project_output: *mut ::std::os::raw::c_void,
        project_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_project_container_restore_editable_authoring(
        project_encoded: *const ::std::os::raw::c_void,
        project_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        asset_identifier: *const ::std::os::raw::c_char,
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_editable_entry_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_get_builtin_catalogue(
        out_info: *mut ctex_material_graph_catalogue_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_create_default(
        out_info: *mut ctex_material_graph_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_inspect(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        out_info: *mut ctex_material_graph_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_compare(
        left: *const ::std::os::raw::c_void,
        left_size: usize,
        right: *const ::std::os::raw::c_void,
        right_size: usize,
        out_equal: *mut u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_add_builtin_node(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        type_id: *const ::std::os::raw::c_char,
        position: ctex_vec2f,
        out_info: *mut ctex_material_graph_info,
        out_node_id: *mut u64,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_set_input_value(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        node_id: u64,
        input_id: *const ::std::os::raw::c_char,
        value: *const ctex_smart_material_value_descriptor,
        out_info: *mut ctex_material_graph_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_set_property_value(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        node_id: u64,
        property_id: *const ::std::os::raw::c_char,
        value: *const ctex_smart_material_value_descriptor,
        out_info: *mut ctex_material_graph_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_add_link(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        link: *const ctex_material_graph_link_descriptor,
        out_info: *mut ctex_material_graph_link_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        replaced_source_socket: *mut ::std::os::raw::c_char,
        replaced_source_socket_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_validate(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        resources: *const ctex_material_graph_validation_resources_descriptor,
        out_info: *mut ctex_material_graph_validation_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_library_create_empty(
        out_info: *mut ctex_material_graph_library_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_library_inspect(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        out_info: *mut ctex_material_graph_library_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_library_add_preset(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        preset: *const ctex_material_graph_preset_descriptor,
        out_info: *mut ctex_material_graph_library_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_library_resolve_preset(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        stable_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_material_graph_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_create(
        out_workspace: *mut *mut ctex_material_graph_workspace,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_destroy(workspace: *mut ctex_material_graph_workspace);
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_get_info(
        workspace: *const ctex_material_graph_workspace,
        out_info: *mut ctex_material_graph_workspace_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_add_material(
        workspace: *mut ctex_material_graph_workspace,
        identifier: *const ::std::os::raw::c_char,
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_create_group(
        workspace: *mut ctex_material_graph_workspace,
        descriptor: *const ctex_material_graph_group_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_instantiate_group(
        workspace: *mut ctex_material_graph_workspace,
        group_identifier: *const ::std::os::raw::c_char,
        owner_kind: u32,
        owner_identifier: *const ::std::os::raw::c_char,
        position: ctex_vec2f,
        out_node_id: *mut u64,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_update_group_interface(
        workspace: *mut ctex_material_graph_workspace,
        group_identifier: *const ::std::os::raw::c_char,
        descriptor: *const ctex_material_graph_group_interface_descriptor,
        out_info: *mut ctex_material_graph_group_update_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_get_graph(
        workspace: *const ctex_material_graph_workspace,
        owner_kind: u32,
        owner_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_material_graph_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_add_builtin_node(
        workspace: *mut ctex_material_graph_workspace,
        owner_kind: u32,
        owner_identifier: *const ::std::os::raw::c_char,
        type_id: *const ::std::os::raw::c_char,
        position: ctex_vec2f,
        out_node_id: *mut u64,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_set_input_value(
        workspace: *mut ctex_material_graph_workspace,
        owner_kind: u32,
        owner_identifier: *const ::std::os::raw::c_char,
        node_id: u64,
        input_id: *const ::std::os::raw::c_char,
        value: *const ctex_smart_material_value_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_set_property_value(
        workspace: *mut ctex_material_graph_workspace,
        owner_kind: u32,
        owner_identifier: *const ::std::os::raw::c_char,
        node_id: u64,
        property_id: *const ::std::os::raw::c_char,
        value: *const ctex_smart_material_value_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_add_link(
        workspace: *mut ctex_material_graph_workspace,
        owner_kind: u32,
        owner_identifier: *const ::std::os::raw::c_char,
        link: *const ctex_material_graph_link_descriptor,
        out_info: *mut ctex_material_graph_link_info,
        replaced_source_socket: *mut ::std::os::raw::c_char,
        replaced_source_socket_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_node_registry_create(
        out_registry: *mut *mut ctex_material_graph_node_registry,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_node_registry_destroy(
        registry: *mut ctex_material_graph_node_registry,
    );
}
unsafe extern "C" {
    pub fn ctex_material_graph_node_registry_get_info(
        registry: *const ctex_material_graph_node_registry,
        out_info: *mut ctex_material_graph_node_registry_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_node_registry_register(
        registry: *mut ctex_material_graph_node_registry,
        descriptor: *const ctex_material_graph_host_node_registration_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_add_registered_node(
        registry: *const ctex_material_graph_node_registry,
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        type_id: *const ::std::os::raw::c_char,
        type_version: u32,
        position: ctex_vec2f,
        out_info: *mut ctex_material_graph_info,
        out_node_id: *mut u64,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_workspace_add_registered_node(
        workspace: *mut ctex_material_graph_workspace,
        registry: *const ctex_material_graph_node_registry,
        owner_kind: u32,
        owner_identifier: *const ::std::os::raw::c_char,
        type_id: *const ::std::os::raw::c_char,
        type_version: u32,
        position: ctex_vec2f,
        out_node_id: *mut u64,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_validate_registered(
        registry: *const ctex_material_graph_node_registry,
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        target: u32,
        resources: *const ctex_material_graph_validation_resources_descriptor,
        out_info: *mut ctex_material_graph_validation_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_material_graph_node_registry_verify_contract(
        registry: *const ctex_material_graph_node_registry,
        type_id: *const ::std::os::raw::c_char,
        type_version: u32,
        pinned_dependencies: *const *const ::std::os::raw::c_char,
        pinned_dependency_count: usize,
        out_info: *mut ctex_material_graph_host_contract_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_emit_material(
        registry: *const ctex_material_graph_node_registry,
        graph_serialized: *const ::std::os::raw::c_void,
        graph_serialized_size: usize,
        request: *const ctex_shader_material_request,
        out_info: *mut ctex_shader_material_info,
        vertex_artifact: *mut ::std::os::raw::c_void,
        vertex_artifact_size: usize,
        fragment_artifact: *mut ::std::os::raw::c_void,
        fragment_artifact_size: usize,
        pass_plan_output: *mut ::std::os::raw::c_char,
        pass_plan_output_size: usize,
        workaround_report_output: *mut ::std::os::raw::c_char,
        workaround_report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_emission_cache_create(
        out_cache: *mut *mut ctex_shader_emission_cache,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_emission_cache_destroy(cache: *mut ctex_shader_emission_cache);
}
unsafe extern "C" {
    pub fn ctex_shader_emission_cache_get_info(
        cache: *const ctex_shader_emission_cache,
        out_info: *mut ctex_shader_emission_cache_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_emission_cache_clear(cache: *mut ctex_shader_emission_cache) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_emit_material_cached(
        cache: *mut ctex_shader_emission_cache,
        registry: *const ctex_material_graph_node_registry,
        graph_serialized: *const ::std::os::raw::c_void,
        graph_serialized_size: usize,
        request: *const ctex_shader_material_request,
        out_info: *mut ctex_shader_material_info,
        vertex_artifact: *mut ::std::os::raw::c_void,
        vertex_artifact_size: usize,
        fragment_artifact: *mut ::std::os::raw::c_void,
        fragment_artifact_size: usize,
        pass_plan_output: *mut ::std::os::raw::c_char,
        pass_plan_output_size: usize,
        workaround_report_output: *mut ::std::os::raw::c_char,
        workaround_report_output_size: usize,
        out_cache_hit: *mut u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_emit_material_inspectable(
        cache: *mut ctex_shader_emission_cache,
        registry: *const ctex_material_graph_node_registry,
        source: *const ctex_shader_material_source_descriptor,
        request: *const ctex_shader_material_request,
        out_info: *mut ctex_shader_material_info,
        vertex_artifact: *mut ::std::os::raw::c_void,
        vertex_artifact_size: usize,
        fragment_artifact: *mut ::std::os::raw::c_void,
        fragment_artifact_size: usize,
        pass_plan_output: *mut ::std::os::raw::c_char,
        pass_plan_output_size: usize,
        workaround_report_output: *mut ::std::os::raw::c_char,
        workaround_report_output_size: usize,
        out_debug_info: *mut ctex_shader_material_debug_info,
        debug_metadata_output: *mut ::std::os::raw::c_char,
        debug_metadata_output_size: usize,
        out_cache_hit: *mut u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_get_backend_attribution(
        out_info: *mut ctex_shader_backend_attribution_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_emit_layer_stack(
        cache: *mut ctex_shader_emission_cache,
        request: *const ctex_shader_layer_stack_request,
        out_info: *mut ctex_shader_layer_stack_info,
        artifact_blob: *mut ::std::os::raw::c_void,
        artifact_blob_size: usize,
        artifact_report_output: *mut ::std::os::raw::c_char,
        artifact_report_output_size: usize,
        pass_plan_output: *mut ::std::os::raw::c_char,
        pass_plan_output_size: usize,
        workaround_report_output: *mut ::std::os::raw::c_char,
        workaround_report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_emit_lit_preview(
        cache: *mut ctex_shader_emission_cache,
        request: *const ctex_shader_preview_request,
        out_info: *mut ctex_shader_preview_info,
        vertex_artifact: *mut ::std::os::raw::c_void,
        vertex_artifact_size: usize,
        fragment_artifact: *mut ::std::os::raw::c_void,
        fragment_artifact_size: usize,
        pass_plan_output: *mut ::std::os::raw::c_char,
        pass_plan_output_size: usize,
        workaround_report_output: *mut ::std::os::raw::c_char,
        workaround_report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_shader_emit_channel_inspection(
        cache: *mut ctex_shader_emission_cache,
        request: *const ctex_shader_preview_request,
        semantic_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_shader_preview_info,
        vertex_artifact: *mut ::std::os::raw::c_void,
        vertex_artifact_size: usize,
        fragment_artifact: *mut ::std::os::raw::c_void,
        fragment_artifact_size: usize,
        pass_plan_output: *mut ::std::os::raw::c_char,
        pass_plan_output_size: usize,
        workaround_report_output: *mut ::std::os::raw::c_char,
        workaround_report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_smart_material_inspect(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        out_info: *mut ctex_smart_material_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_smart_material_set_parameter(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        parameter_identifier: *const ::std::os::raw::c_char,
        value: *const ctex_smart_material_value_descriptor,
        out_info: *mut ctex_smart_material_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_smart_material_set_anchor(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        entry_identifier: *const ::std::os::raw::c_char,
        marked: u32,
        out_info: *mut ctex_smart_material_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_smart_material_add_anchor_reference(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        anchor_entry_identifier: *const ::std::os::raw::c_char,
        consumer_entry_identifier: *const ::std::os::raw::c_char,
        consumer_node_id: u64,
        consumer_input_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_smart_material_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_smart_material_plan_anchor_evaluation(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        changed_anchor_identifiers: *const *const ::std::os::raw::c_char,
        changed_anchor_count: usize,
        output: *mut ::std::os::raw::c_char,
        output_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_smart_material_package(
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        resources: *const ctex_project_resource_descriptor,
        resource_count: usize,
        options: *const ctex_project_asset_export_options_descriptor,
        out_info: *mut ctex_project_container_info,
        package_output: *mut ::std::os::raw::c_void,
        package_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_smart_material_import(
        package_encoded: *const ::std::os::raw::c_void,
        package_encoded_size: usize,
        limits: *const ctex_project_container_read_limits_descriptor,
        search_paths: *const ctex_project_asset_search_paths_descriptor,
        out_info: *mut ctex_smart_material_info,
        canonical_output: *mut ::std::os::raw::c_void,
        canonical_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_preset_library_enumerate(
        library: *const ctex_preset_library_descriptor,
        out_info: *mut ctex_preset_library_info,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_preset_library_resolve(
        library: *const ctex_preset_library_descriptor,
        preset_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_project_container_info,
        package_output: *mut ::std::os::raw::c_void,
        package_output_size: usize,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_stroke_settings_init(
        out_settings: *mut ctex_stroke_settings_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_stroke_resolve(
        settings: *const ctex_stroke_settings_descriptor,
        samples: *const ctex_stroke_input_sample,
        sample_count: usize,
        out_info: *mut ctex_resolved_stroke_info,
        stamps: *mut ctex_resolved_stamp,
        stamp_capacity: usize,
        out_stamp_count: *mut usize,
        swept_segments: *mut ctex_swept_segment,
        swept_segment_capacity: usize,
        out_swept_segment_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_stroke_preset_serialize(
        name: *const ::std::os::raw::c_char,
        settings: *const ctex_stroke_settings_descriptor,
        serialized_buffer: *mut ::std::os::raw::c_char,
        serialized_buffer_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_stroke_preset_deserialize(
        serialized: *const ::std::os::raw::c_char,
        serialized_size: usize,
        out_info: *mut ctex_stroke_preset_info,
        out_settings: *mut ctex_stroke_settings_descriptor,
        buffers: *const ctex_stroke_preset_buffers_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_evaluate_tile_coverage(
        mesh: *const ctex_mesh,
        tile: *const ctex_paint_tile_coverage_descriptor,
        stroke: *const ctex_resolved_stroke_descriptor,
        coverage: *mut f64,
        coverage_capacity: usize,
        out_coverage_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_evaluate_material_coordinates(
        mesh: *const ctex_mesh,
        tile: *const ctex_paint_tile_coverage_descriptor,
        descriptor: *const ctex_paint_material_coordinate_descriptor,
        samples: *mut ctex_paint_material_coordinate_sample,
        sample_capacity: usize,
        out_sample_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_rejection_init(
        out_rejection: *mut ctex_paint_rejection_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_evaluate_rejected_coverage(
        mesh: *const ctex_mesh,
        tile: *const ctex_paint_tile_coverage_descriptor,
        stroke: *const ctex_resolved_stroke_descriptor,
        rejection: *const ctex_paint_rejection_descriptor,
        out_info: *mut ctex_paint_rejection_info,
        coverage: *mut f64,
        coverage_capacity: usize,
        out_coverage_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_work_init(out_work: *mut ctex_paint_work_descriptor) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_plan_work(
        work: *const ctex_paint_work_descriptor,
        out_info: *mut ctex_paint_work_info,
        processed_tiles: *mut ctex_paint_tile_coordinate,
        processed_tile_capacity: usize,
        out_processed_tile_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_seam_dilation_init(
        out_dilation: *mut ctex_paint_seam_dilation_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_dilate_uv_seams(
        dilation: *const ctex_paint_seam_dilation_descriptor,
        out_info: *mut ctex_paint_seam_dilation_info,
        pixels: *mut f64,
        pixel_capacity: usize,
        out_pixel_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_dilation_session_create(
        radius: u32,
        out_session: *mut *mut ctex_paint_dilation_session,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_dilation_session_destroy(session: *mut ctex_paint_dilation_session);
}
unsafe extern "C" {
    pub fn ctex_paint_dilation_session_stage_tile(
        session: *mut ctex_paint_dilation_session,
        tile: *const ctex_paint_dilation_tile_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_dilation_session_get_preview(
        session: *const ctex_paint_dilation_session,
        out_info: *mut ctex_paint_dilation_session_info,
        tiles: *mut ctex_paint_dilation_tile_info,
        tile_capacity: usize,
        out_tile_count: *mut usize,
        pixels: *mut f64,
        pixel_capacity: usize,
        out_pixel_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_dilation_session_finish(
        session: *mut ctex_paint_dilation_session,
        out_info: *mut ctex_paint_dilation_session_info,
        tiles: *mut ctex_paint_dilation_tile_info,
        tile_capacity: usize,
        out_tile_count: *mut usize,
        pixels: *mut f64,
        pixel_capacity: usize,
        out_pixel_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_filter_surface_scalar(
        filter: *const ctex_paint_surface_filter_descriptor,
        values: *const f64,
        value_count: usize,
        out_value: *mut f64,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_filter_surface_tangent_vector(
        filter: *const ctex_paint_surface_filter_descriptor,
        values: *const ctex_vec3d,
        value_count: usize,
        out_value: *mut ctex_vec3d,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_plan_island_padding(
        padding: *const ctex_paint_island_padding_descriptor,
        out_info: *mut ctex_paint_island_padding_info,
        ownership: *mut u32,
        ownership_capacity: usize,
        out_ownership_count: *mut usize,
        unsupported_mip_levels: *mut ctex_paint_unsupported_mip_level,
        unsupported_mip_level_capacity: usize,
        out_unsupported_mip_level_count: *mut usize,
        affected_islands: *mut u32,
        affected_island_capacity: usize,
        out_affected_island_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_island_padding(
        padding: *const ctex_paint_island_padding_descriptor,
        out_info: *mut ctex_paint_island_padding_info,
        pixels: *mut f64,
        pixel_capacity: usize,
        out_pixel_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_surface_map_cache_create(
        out_cache: *mut *mut ctex_paint_surface_map_cache,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_surface_map_cache_destroy(cache: *mut ctex_paint_surface_map_cache);
}
unsafe extern "C" {
    pub fn ctex_paint_surface_map_cache_clear(
        cache: *mut ctex_paint_surface_map_cache,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_surface_map_cache_get_statistics(
        cache: *const ctex_paint_surface_map_cache,
        out_statistics: *mut ctex_paint_surface_map_statistics,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_surface_map_cache_lookup(
        cache: *mut ctex_paint_surface_map_cache,
        mesh: *const ctex_mesh,
        request: *const ctex_paint_surface_map_request,
        out_info: *mut ctex_paint_surface_map_info,
        buffers: *const ctex_paint_surface_map_buffers,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_combine_masks(
        width: u32,
        height: u32,
        masks: *const ctex_paint_mask_inputs_descriptor,
        out_info: *mut ctex_paint_mask_info,
        combined_mask: *mut f64,
        combined_mask_capacity: usize,
        out_combined_mask_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_evaluate_tile_deposition(
        mesh: *const ctex_mesh,
        tile: *const ctex_paint_tile_coverage_descriptor,
        stroke: *const ctex_resolved_stroke_descriptor,
        deposition: *const ctex_paint_deposition_descriptor,
        out_info: *mut ctex_paint_deposition_info,
        samples: *mut ctex_paint_deposition_sample,
        sample_capacity: usize,
        out_sample_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_blend_snapshot(
        descriptor: *const ctex_paint_blend_descriptor,
        pixels: *mut ctex_vec4f,
        pixel_capacity: usize,
        out_pixel_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_brush(
        descriptor: *const ctex_paint_brush_descriptor,
        out_info: *mut ctex_paint_brush_info,
        output_channels: *const ctex_paint_tool_channel_output,
        output_channel_count: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_eraser(
        descriptor: *const ctex_paint_eraser_descriptor,
        out_info: *mut ctex_paint_eraser_info,
        values: *mut f64,
        value_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_fill(
        descriptor: *const ctex_paint_fill_descriptor,
        out_info: *mut ctex_paint_fill_info,
        outputs: *const ctex_paint_fill_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_clone(
        descriptor: *const ctex_paint_clone_descriptor,
        out_info: *mut ctex_paint_clone_info,
        outputs: *const ctex_paint_clone_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_blur(
        descriptor: *const ctex_paint_blur_descriptor,
        out_info: *mut ctex_paint_blur_info,
        output_channels: *const ctex_paint_tool_channel_output,
        output_channel_count: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_smear(
        descriptor: *const ctex_paint_smear_descriptor,
        out_info: *mut ctex_paint_smear_info,
        output_channels: *const ctex_paint_tool_channel_output,
        output_channel_count: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_resolve_stencil_mask(
        descriptor: *const ctex_paint_stencil_descriptor,
        out_info: *mut ctex_paint_stencil_info,
        mask_values: *mut f64,
        mask_value_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_rasterize_decal(
        descriptor: *const ctex_paint_decal_descriptor,
        out_info: *mut ctex_paint_decal_info,
        outputs: *const ctex_paint_decal_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_projection(
        descriptor: *const ctex_paint_projection_descriptor,
        out_info: *mut ctex_paint_projection_info,
        outputs: *const ctex_paint_projection_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_text(
        descriptor: *const ctex_paint_text_descriptor,
        out_info: *mut ctex_paint_text_info,
        outputs: *const ctex_paint_text_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_apply_particles(
        index: *mut ctex_pick_index,
        descriptor: *const ctex_paint_particle_descriptor,
        out_info: *mut ctex_paint_particle_info,
        outputs: *const ctex_paint_particle_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_pick_enabled_channels(
        descriptor: *const ctex_paint_picker_descriptor,
        out_info: *mut ctex_paint_picker_info,
        channels: *mut ctex_paint_picker_channel_value,
        channel_capacity: usize,
        strings: *mut ::std::os::raw::c_char,
        string_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_select_colour_id(
        descriptor: *const ctex_paint_colour_id_descriptor,
        out_info: *mut ctex_paint_colour_id_info,
        values: *mut f64,
        value_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_get_parameter_catalogue(
        parameters: *mut ctex_paint_parameter_descriptor,
        parameter_capacity: usize,
        names: *mut ::std::os::raw::c_char,
        name_capacity: usize,
        out_info: *mut ctex_paint_parameter_catalogue_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_validate_parameter(
        name: *const ::std::os::raw::c_char,
        context: u32,
        supplied: f64,
        out_info: *mut ctex_paint_parameter_validation_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_select_screen(
        index: *mut ctex_pick_index,
        descriptor: *const ctex_paint_screen_selection_descriptor,
        out_info: *mut ctex_paint_selection_info,
        outputs: *const ctex_paint_selection_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_select_polygon(
        descriptor: *const ctex_paint_polygon_selection_descriptor,
        out_info: *mut ctex_paint_selection_info,
        outputs: *const ctex_paint_selection_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_set_log_sink(descriptor: *const ctex_log_sink_descriptor) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_set_allocator(descriptor: *const ctex_allocator_descriptor) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_create(
        descriptor: *const ctex_mesh_descriptor,
        out_mesh: *mut *mut ctex_mesh,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_destroy(mesh: *mut ctex_mesh);
}
unsafe extern "C" {
    pub fn ctex_mesh_replace(
        mesh: *mut ctex_mesh,
        descriptor: *const ctex_mesh_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_get_info(mesh: *const ctex_mesh, out_info: *mut ctex_mesh_info)
        -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_get_uv_set_names(
        mesh: *const ctex_mesh,
        buffer: *mut ::std::os::raw::c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
        out_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_analyze_uv_overlaps(
        mesh: *const ctex_mesh,
        uv_set: *const ::std::os::raw::c_char,
        partition_index: u32,
        out_info: *mut ctex_mesh_uv_overlap_info,
        face_indices: *mut u32,
        face_index_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_analyze_uv_coverage(
        mesh: *const ctex_mesh,
        uv_set: *const ::std::os::raw::c_char,
        partition_index: u32,
        width: u32,
        height: u32,
        out_info: *mut ctex_mesh_uv_coverage_info,
        outside_face_indices: *mut u32,
        outside_face_index_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_replacement_plan_create(
        document: *mut ctex_document,
        mesh: *mut ctex_mesh,
        replacement: *const ctex_mesh_descriptor,
        out_plan: *mut *mut ctex_mesh_replacement_plan,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_replacement_plan_create_with_tangent_data(
        document: *mut ctex_document,
        mesh: *mut ctex_mesh,
        replacement: *const ctex_mesh_descriptor,
        tangents: *const ctex_mesh_tangent_data_descriptor,
        out_plan: *mut *mut ctex_mesh_replacement_plan,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_replacement_plan_destroy(plan: *mut ctex_mesh_replacement_plan);
}
unsafe extern "C" {
    pub fn ctex_mesh_replacement_plan_get_info(
        plan: *const ctex_mesh_replacement_plan,
        out_info: *mut ctex_mesh_replacement_plan_info,
        entries: *mut ctex_mesh_replacement_entry,
        entry_capacity: usize,
        texture_set_ids: *mut ::std::os::raw::c_char,
        texture_set_id_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_replacement_plan_apply(
        plan: *mut ctex_mesh_replacement_plan,
        decisions: *const ctex_mesh_replacement_decision,
        decision_count: usize,
        out_info: *mut ctex_mesh_replacement_apply_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_replacement_plan_preflight_reprojection(
        plan: *mut ctex_mesh_replacement_plan,
        descriptor: *const ctex_mesh_reprojection_descriptor,
        out_info: *mut ctex_mesh_reprojection_preflight_info,
        mapping_json: *mut ::std::os::raw::c_char,
        mapping_json_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_replacement_plan_commit_reprojection(
        plan: *mut ctex_mesh_replacement_plan,
        hole_policy: u32,
        ambiguity_policy: u32,
        out_info: *mut ctex_mesh_reprojection_commit_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_create_with_tangent_data(
        descriptor: *const ctex_mesh_descriptor,
        tangents: *const ctex_mesh_tangent_data_descriptor,
        out_mesh: *mut *mut ctex_mesh,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_replace_with_tangent_data(
        mesh: *mut ctex_mesh,
        descriptor: *const ctex_mesh_descriptor,
        tangents: *const ctex_mesh_tangent_data_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_get_tangent_frame(
        mesh: *const ctex_mesh,
        out_info: *mut ctex_mesh_tangent_frame_info,
        uv_set: *mut ::std::os::raw::c_char,
        uv_set_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_create(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        mesh: *const ctex_mesh,
        out_map_set: *mut *mut ctex_mesh_map_set,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_destroy(map_set: *mut ctex_mesh_map_set);
}
unsafe extern "C" {
    pub fn ctex_mesh_map_kind_get_name(
        kind: u32,
        buffer: *mut ::std::os::raw::c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_get_info(
        map_set: *const ctex_mesh_map_set,
        out_info: *mut ctex_mesh_map_set_info,
        texture_set_id: *mut ::std::os::raw::c_char,
        texture_set_id_size: usize,
        uv_set: *mut ::std::os::raw::c_char,
        uv_set_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_get_entries(
        map_set: *const ctex_mesh_map_set,
        entries: *mut ctex_mesh_map_entry_info,
        entry_capacity: usize,
        out_entry_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_import_external(
        map_set: *mut ctex_mesh_map_set,
        descriptor: *const ctex_mesh_map_import_descriptor,
        out_info: *mut ctex_mesh_map_import_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_sample(
        map_set: *const ctex_mesh_map_set,
        kind: u32,
        u: f64,
        v: f64,
        out_sample: *mut ctex_mesh_map_sample_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_check_requirements(
        map_set: *const ctex_mesh_map_set,
        consumer: *const ::std::os::raw::c_char,
        required_maps: *const u32,
        required_map_count: usize,
        missing_maps: *mut u32,
        missing_map_capacity: usize,
        stale_maps: *mut ctex_mesh_map_staleness,
        stale_map_capacity: usize,
        out_info: *mut ctex_mesh_map_requirement_info,
        message: *mut ::std::os::raw::c_char,
        message_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_synchronize_mesh(
        map_set: *mut ctex_mesh_map_set,
        mesh: *const ctex_mesh,
        stale_maps: *mut ctex_mesh_map_staleness,
        stale_map_capacity: usize,
        out_stale_map_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_release(
        map_set: *mut ctex_mesh_map_set,
        kind: u32,
        out_info: *mut ctex_mesh_map_release_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_release_all(
        map_set: *mut ctex_mesh_map_set,
        out_info: *mut ctex_mesh_map_release_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_generator_get_info(
        kind: u32,
        out_info: *mut ctex_mesh_map_generator_info,
        required_maps: *mut u32,
        required_map_capacity: usize,
        parameters: *mut ctex_mesh_map_generator_parameter_descriptor,
        parameter_capacity: usize,
        strings: *mut ::std::os::raw::c_char,
        string_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_generator_generate(
        map_set: *const ctex_mesh_map_set,
        kind: u32,
        width: u32,
        height: u32,
        parameters: *const ctex_mesh_map_generator_parameter,
        parameter_count: usize,
        out_info: *mut ctex_mesh_map_generator_result_info,
        mask_values: *mut f32,
        mask_value_capacity: usize,
        resolved_parameters: *mut ctex_mesh_map_generator_resolved_parameter,
        resolved_parameter_capacity: usize,
        parameter_clamps: *mut ctex_mesh_map_generator_parameter_clamp,
        parameter_clamp_capacity: usize,
        stale_maps: *mut ctex_mesh_map_staleness,
        stale_map_capacity: usize,
        strings: *mut ::std::os::raw::c_char,
        string_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_set_request_bake(
        map_set: *mut ctex_mesh_map_set,
        provider: *const ctex_mesh_map_bake_provider_descriptor,
        kind: u32,
        width: u32,
        height: u32,
        bake_settings_revision: u64,
        request_generation: u64,
        control: *const ctex_mesh_map_bake_control_descriptor,
        out_info: *mut ctex_mesh_map_bake_result_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_session_create(
        map_set: *mut ctex_mesh_map_set,
        initial_settings_revision: u64,
        out_session: *mut *mut ctex_mesh_map_bake_session,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_session_destroy(session: *mut ctex_mesh_map_bake_session);
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_session_get_info(
        session: *const ctex_mesh_map_bake_session,
        out_info: *mut ctex_mesh_map_bake_session_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_session_begin(
        session: *mut ctex_mesh_map_bake_session,
        kind: u32,
        width: u32,
        height: u32,
        out_token: *mut *mut ctex_mesh_map_bake_request_token,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_session_cancel(
        session: *mut ctex_mesh_map_bake_session,
        token: *const ctex_mesh_map_bake_request_token,
        out_cancelled: *mut u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_session_complete(
        session: *mut ctex_mesh_map_bake_session,
        token: *const ctex_mesh_map_bake_request_token,
        output: *const ctex_mesh_map_bake_output_descriptor,
        out_info: *mut ctex_mesh_map_bake_completion_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_session_edit_settings(
        session: *mut ctex_mesh_map_bake_session,
        revision: u64,
        out_info: *mut ctex_mesh_map_bake_settings_edit_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_session_undo_settings(
        session: *mut ctex_mesh_map_bake_session,
        out_info: *mut ctex_mesh_map_bake_settings_undo_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_request_token_destroy(token: *mut ctex_mesh_map_bake_request_token);
}
unsafe extern "C" {
    pub fn ctex_mesh_map_bake_request_token_get_info(
        token: *const ctex_mesh_map_bake_request_token,
        out_info: *mut ctex_mesh_map_bake_token_info,
        texture_set_id: *mut ::std::os::raw::c_char,
        texture_set_id_size: usize,
        uv_set: *mut ::std::os::raw::c_char,
        uv_set_size: usize,
        tangent_uv_set: *mut ::std::os::raw::c_char,
        tangent_uv_set_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_index_create(
        mesh: *mut ctex_mesh,
        out_index: *mut *mut ctex_pick_index,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_index_destroy(index: *mut ctex_pick_index);
}
unsafe extern "C" {
    pub fn ctex_pick_index_get_info(
        index: *const ctex_pick_index,
        out_info: *mut ctex_pick_index_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_uv_pick_index_create(
        mesh: *mut ctex_mesh,
        uv_set: *const ::std::os::raw::c_char,
        out_index: *mut *mut ctex_uv_pick_index,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_uv_pick_index_destroy(index: *mut ctex_uv_pick_index);
}
unsafe extern "C" {
    pub fn ctex_uv_pick_index_get_info(
        index: *const ctex_uv_pick_index,
        out_info: *mut ctex_pick_index_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_ray_from_screen(
        position: ctex_vec2f,
        view: *const ctex_pick_screen_view_descriptor,
        projection_kind: u32,
        out_ray: *mut ctex_pick_ray,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_ray_query(
        index: *mut ctex_pick_index,
        ray: ctex_pick_ray,
        options: *const ctex_pick_options_descriptor,
        texture_sets: *const ctex_pick_texture_set_binding_descriptor,
        texture_set_count: usize,
        hits: *mut ctex_pick_hit,
        hit_capacity: usize,
        texture_set_ids: *mut ::std::os::raw::c_char,
        texture_set_id_buffer_size: usize,
        out_info: *mut ctex_pick_query_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_uv_query(
        index: *mut ctex_uv_pick_index,
        coordinate: ctex_vec2f,
        texture_set: *const ctex_pick_texture_set_binding_descriptor,
        hit: *mut ctex_pick_hit,
        texture_set_id: *mut ::std::os::raw::c_char,
        texture_set_id_buffer_size: usize,
        out_info: *mut ctex_pick_query_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_snap_to_surface(
        index: *mut ctex_pick_index,
        point: ctex_vec3f,
        maximum_distance: f32,
        texture_sets: *const ctex_pick_texture_set_binding_descriptor,
        texture_set_count: usize,
        hit: *mut ctex_pick_hit,
        texture_set_id: *mut ::std::os::raw::c_char,
        texture_set_id_buffer_size: usize,
        out_info: *mut ctex_pick_query_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_query_screen_rectangle(
        index: *mut ctex_pick_index,
        minimum: ctex_vec2f,
        maximum: ctex_vec2f,
        view: *const ctex_pick_screen_view_descriptor,
        triangle_indices: *mut u32,
        triangle_capacity: usize,
        out_info: *mut ctex_pick_query_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_query_screen_lasso(
        index: *mut ctex_pick_index,
        points: *const ctex_vec2f,
        point_count: usize,
        view: *const ctex_pick_screen_view_descriptor,
        triangle_indices: *mut u32,
        triangle_capacity: usize,
        out_info: *mut ctex_pick_query_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_query_world_sphere(
        index: *mut ctex_pick_index,
        center: ctex_vec3f,
        radius: f32,
        triangle_indices: *mut u32,
        triangle_capacity: usize,
        out_info: *mut ctex_pick_query_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_query_world_box(
        index: *mut ctex_pick_index,
        minimum: ctex_vec3f,
        maximum: ctex_vec3f,
        triangle_indices: *mut u32,
        triangle_capacity: usize,
        out_info: *mut ctex_pick_query_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_pick_nearest_batch(
        index: *mut ctex_pick_index,
        rays: *const ctex_pick_ray,
        ray_count: usize,
        maximum_distance: f32,
        backface_policy: u32,
        texture_sets: *const ctex_pick_texture_set_binding_descriptor,
        texture_set_count: usize,
        control: *const ctex_pick_batch_control_descriptor,
        hits: *mut ctex_pick_hit,
        hit_capacity: usize,
        texture_set_ids: *mut ::std::os::raw::c_char,
        texture_set_id_buffer_size: usize,
        out_info: *mut ctex_pick_batch_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_document_create(out_document: *mut *mut ctex_document) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_document_destroy(document: *mut ctex_document);
}
unsafe extern "C" {
    pub fn ctex_document_create_texture_set(
        document: *mut ctex_document,
        descriptor: *const ctex_texture_set_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_document_create_texture_sets_from_mesh(
        document: *mut ctex_document,
        mesh: *const ctex_mesh,
        uv_set: *const ::std::os::raw::c_char,
        width: u32,
        height: u32,
        default_bit_depth: u8,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_document_get_texture_set_ids(
        document: *const ctex_document,
        buffer: *mut ::std::os::raw::c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
        out_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_document_create_atlas(
        document: *mut ctex_document,
        descriptor: *const ctex_atlas_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_document_get_atlas_ids(
        document: *const ctex_document,
        buffer: *mut ::std::os::raw::c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
        out_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_document_get_atlas(
        document: *const ctex_document,
        atlas_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_atlas_info,
        regions: *mut ctex_atlas_region,
        region_capacity: usize,
        display_name: *mut ::std::os::raw::c_char,
        display_name_size: usize,
        texture_set_ids: *mut ::std::os::raw::c_char,
        texture_set_id_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_get_channel_ids(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        buffer: *mut ::std::os::raw::c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
        out_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_register_channel(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        descriptor: *const ctex_channel_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_set_channel_enabled(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        enabled: u32,
        bit_depth_override: u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_get_channel_info(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_channel_info,
        export_mapping_buffer: *mut ::std::os::raw::c_char,
        export_mapping_buffer_size: usize,
        out_required_export_mapping_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_get_memory_report(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        out_report: *mut ctex_texture_set_memory_report,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_set_channel_backing_store(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        udim_tile_number: u32,
        descriptor: *const ctex_tile_backing_store_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_evict_channel_tile(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        udim_tile_number: u32,
        tile_x: u32,
        tile_y: u32,
        out_report: *mut ctex_tile_eviction_report,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_document_get_memory_report(
        document: *const ctex_document,
        out_info: *mut ctex_document_memory_info,
        texture_sets: *mut ctex_document_texture_set_memory_info,
        texture_set_capacity: usize,
        texture_set_ids: *mut ::std::os::raw::c_char,
        texture_set_id_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_append(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entries: *const ctex_layer_entry_descriptor,
        entry_count: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_inspect(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        output: *mut ::std::os::raw::c_char,
        output_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_set_state(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        display_name: *const ::std::os::raw::c_char,
        enabled: u32,
        opacity: f64,
        blend_mode: *const ::std::os::raw::c_char,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_set_layout(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        parent_identifier: *const ::std::os::raw::c_char,
        target_identifier: *const ::std::os::raw::c_char,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_set_channel(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        channel: *const ctex_layer_channel_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_record_paint(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_remove(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifiers: *const *const ::std::os::raw::c_char,
        entry_count: usize,
        source_deletion_policy: u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_evaluate_blend(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        base: ctex_vec4f,
        layer: ctex_vec4f,
        factor: f64,
        out_colour: *mut ctex_vec4f,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_get_applicable_masks(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        mask_ids: *mut ::std::os::raw::c_char,
        mask_id_size: usize,
        out_required_size: *mut usize,
        out_mask_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_get_participation(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        mask_samples: *const ctex_layer_mask_sample,
        mask_sample_count: usize,
        out_info: *mut ctex_layer_participation_info,
        mask_ids: *mut ::std::os::raw::c_char,
        mask_id_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_composite_cpu(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        width: u32,
        height: u32,
        content: *const ctex_layer_composite_raster_descriptor,
        content_count: usize,
        masks: *const ctex_layer_composite_mask_descriptor,
        mask_count: usize,
        out_info: *mut ctex_layer_composite_info,
        channels: *mut ctex_layer_composite_channel_info,
        channel_capacity: usize,
        semantic_ids: *mut ::std::os::raw::c_char,
        semantic_id_size: usize,
        pixels: *mut ctex_vec4f,
        pixel_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_layer_snapshot_create(
        width: u32,
        height: u32,
        content: *const ctex_layer_composite_raster_descriptor,
        content_count: usize,
        masks: *const ctex_layer_composite_mask_descriptor,
        mask_count: usize,
        out_snapshot: *mut *mut ctex_layer_snapshot,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_layer_snapshot_destroy(snapshot: *mut ctex_layer_snapshot);
}
unsafe extern "C" {
    pub fn ctex_layer_snapshot_read(
        snapshot: *const ctex_layer_snapshot,
        out_info: *mut ctex_layer_snapshot_info,
        content: *mut ctex_layer_snapshot_content_info,
        content_capacity: usize,
        masks: *mut ctex_layer_snapshot_mask_info,
        mask_capacity: usize,
        strings: *mut ::std::os::raw::c_char,
        string_size: usize,
        pixels: *mut ctex_vec4f,
        pixel_capacity: usize,
        coverage: *mut f32,
        coverage_capacity: usize,
        mask_values: *mut f64,
        mask_value_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_layer_composite_snapshot_cpu(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        snapshot: *const ctex_layer_snapshot,
        out_info: *mut ctex_layer_composite_info,
        channels: *mut ctex_layer_composite_channel_info,
        channel_capacity: usize,
        semantic_ids: *mut ::std::os::raw::c_char,
        semantic_id_size: usize,
        pixels: *mut ctex_vec4f,
        pixel_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_apply_layer_operation(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        snapshot: *mut ctex_layer_snapshot,
        operation: *const ctex_layer_operation_descriptor,
        out_info: *mut ctex_layer_operation_info,
        affected_ids: *mut ::std::os::raw::c_char,
        affected_id_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_begin_transaction(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        step_identifier: *const ::std::os::raw::c_char,
        targets: *const ctex_tile_history_target_descriptor,
        target_count: usize,
        snapshot: *mut ctex_layer_snapshot,
        out_transaction: *mut *mut ctex_texture_set_transaction,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_destroy(transaction: *mut ctex_texture_set_transaction);
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_write_pixel(
        transaction: *mut ctex_texture_set_transaction,
        semantic_id: *const ::std::os::raw::c_char,
        x: u32,
        y: u32,
        pixel: *const ::std::os::raw::c_void,
        pixel_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_write_region(
        transaction: *mut ctex_texture_set_transaction,
        semantic_id: *const ::std::os::raw::c_char,
        region: *const ctex_channel_region_descriptor,
        pixels: *const ::std::os::raw::c_void,
        pixel_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_apply_layer_operation(
        transaction: *mut ctex_texture_set_transaction,
        operation: *const ctex_layer_operation_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_set_layer_state(
        transaction: *mut ctex_texture_set_transaction,
        entry_identifier: *const ::std::os::raw::c_char,
        display_name: *const ::std::os::raw::c_char,
        enabled: u32,
        opacity: f64,
        blend_mode: *const ::std::os::raw::c_char,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_set_layer_layout(
        transaction: *mut ctex_texture_set_transaction,
        entry_identifier: *const ::std::os::raw::c_char,
        parent_identifier: *const ::std::os::raw::c_char,
        target_identifier: *const ::std::os::raw::c_char,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_set_layer_channel(
        transaction: *mut ctex_texture_set_transaction,
        entry_identifier: *const ::std::os::raw::c_char,
        channel: *const ctex_layer_channel_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_set_fill_graph(
        transaction: *mut ctex_texture_set_transaction,
        entry_identifier: *const ::std::os::raw::c_char,
        graph_serialized: *const ::std::os::raw::c_void,
        graph_serialized_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_commit(
        transaction: *mut ctex_texture_set_transaction,
        out_info: *mut ctex_tile_history_commit_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_transaction_cancel(transaction: *mut ctex_texture_set_transaction);
}
unsafe extern "C" {
    pub fn ctex_texture_set_configure_tile_history(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        budget_bytes: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_get_tile_history_budget(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        proposed_step_bytes: usize,
        out_report: *mut ctex_tile_history_budget_report,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_begin_tile_history(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        step_identifier: *const ::std::os::raw::c_char,
        targets: *const ctex_tile_history_target_descriptor,
        target_count: usize,
        out_capture: *mut *mut ctex_tile_history_capture,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_tile_history_capture_destroy(capture: *mut ctex_tile_history_capture);
}
unsafe extern "C" {
    pub fn ctex_tile_history_capture_commit(
        capture: *mut ctex_tile_history_capture,
        out_info: *mut ctex_tile_history_commit_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_undo_tiles(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_tile_history_restore_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_redo_tiles(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_tile_history_restore_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_get_udim_tiles(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        tile_numbers: *mut u32,
        tile_capacity: usize,
        out_tile_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_ensure_udim_tiles(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        tile_numbers: *const u32,
        tile_count: usize,
        out_allocated_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_write_udim_pixels(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        writes: *const ctex_udim_pixel_write_descriptor,
        write_count: usize,
        out_info: *mut ctex_udim_write_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_read_udim_pixel(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        tile_number: u32,
        x: u32,
        y: u32,
        pixel: *mut ::std::os::raw::c_void,
        pixel_size: usize,
        out_required_pixel_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_query_channel_delta(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        synchronized_cursor: ctex_transport_revision_cursor,
        changed_tiles: *mut ctex_transport_tile_version,
        changed_tile_capacity: usize,
        out_info: *mut ctex_transport_delta_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_reset_channel_revision_history(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        out_cursor: *mut ctex_transport_revision_cursor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_pool_create(
        budget_bytes: usize,
        out_pool: *mut *mut ctex_transport_snapshot_pool,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_pool_destroy(pool: *mut ctex_transport_snapshot_pool);
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_pool_get_memory_report(
        pool: *const ctex_transport_snapshot_pool,
        out_report: *mut ctex_transport_snapshot_memory_report,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resource_ledger_create(out_ledger: *mut *mut ctex_resource_ledger) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resource_ledger_destroy(ledger: *mut ctex_resource_ledger);
}
unsafe extern "C" {
    pub fn ctex_resource_ledger_upsert(
        ledger: *mut ctex_resource_ledger,
        descriptor: *const ctex_resource_allocation_descriptor,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resource_ledger_remove(
        ledger: *mut ctex_resource_ledger,
        allocation_identity: u64,
        out_removed: *mut u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resource_ledger_get_report(
        ledger: *const ctex_resource_ledger,
        out_report: *mut ctex_resource_accounting_report,
        categories: *mut ctex_resource_category_report,
        category_capacity: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resource_ledger_set_cache_eviction_callback(
        ledger: *mut ctex_resource_ledger,
        callback: ctex_resource_cache_eviction_callback,
        user_data: *mut ::std::os::raw::c_void,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resource_ledger_admit(
        ledger: *mut ctex_resource_ledger,
        descriptor: *const ctex_resource_admission_descriptor,
        out_reservation: *mut *mut ctex_resource_reservation,
        out_report: *mut ctex_resource_admission_report,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resource_ledger_admit_operation_recovery(
        ledger: *mut ctex_resource_ledger,
        limits: *const ctex_resource_budget_limits,
        operation_record: *const ::std::os::raw::c_void,
        operation_record_size: usize,
        checkpoint_bytes: usize,
        out_reservation: *mut *mut ctex_resource_reservation,
        out_report: *mut ctex_resource_admission_report,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resource_ledger_admit_preview_quality(
        ledger: *mut ctex_resource_ledger,
        descriptor: *const ctex_preview_quality_admission_descriptor,
        out_reservation: *mut *mut ctex_resource_reservation,
        out_report: *mut ctex_preview_quality_admission_report,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_resource_reservation_destroy(reservation: *mut ctex_resource_reservation);
}
unsafe extern "C" {
    pub fn ctex_resource_reservation_release(reservation: *mut ctex_resource_reservation);
}
unsafe extern "C" {
    pub fn ctex_resource_reservation_get_evicted_allocations(
        reservation: *const ctex_resource_reservation,
        allocation_identities: *mut u64,
        allocation_identity_capacity: usize,
        out_allocation_identity_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_query_channel_snapshot(
        pool: *mut ctex_transport_snapshot_pool,
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        synchronized_cursor: ctex_transport_revision_cursor,
        out_snapshot: *mut *mut ctex_transport_snapshot,
        out_info: *mut ctex_transport_snapshot_query_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_query_snapshot(
        pool: *mut ctex_transport_snapshot_pool,
        session: *const ctex_paint_preview_session,
        synchronized_cursor: ctex_transport_revision_cursor,
        out_snapshot: *mut *mut ctex_transport_snapshot,
        out_info: *mut ctex_transport_snapshot_query_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_destroy(snapshot: *mut ctex_transport_snapshot);
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_get_tile_versions(
        snapshot: *const ctex_transport_snapshot,
        versions: *mut ctex_transport_tile_version,
        version_capacity: usize,
        out_version_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_negotiate_format(
        snapshot: *const ctex_transport_snapshot,
        accepted_formats: *const ctex_transport_pixel_format,
        accepted_format_count: usize,
        conversion_policy: u32,
        out_selection: *mut ctex_transport_format_selection,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_get_tile_memory_layout(
        snapshot: *const ctex_transport_snapshot,
        version: ctex_transport_tile_version,
        format: *const ctex_transport_format_selection,
        out_layout: *mut ctex_transport_tile_memory_layout,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_read_tiles(
        snapshot: *const ctex_transport_snapshot,
        format: *const ctex_transport_format_selection,
        destinations: *const ctex_transport_tile_readback_destination,
        destination_count: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_begin_readback(
        snapshot: *const ctex_transport_snapshot,
        format: *const ctex_transport_format_selection,
        destinations: *const ctex_transport_tile_readback_destination,
        destination_count: usize,
        out_readback: *mut *mut ctex_transport_readback,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_snapshot_begin_host_readback(
        snapshot: *const ctex_transport_snapshot,
        format: *const ctex_transport_format_selection,
        destinations: *const ctex_transport_tile_readback_destination,
        destination_count: usize,
        out_readback: *mut *mut ctex_transport_readback,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_readback_destroy(readback: *mut ctex_transport_readback);
}
unsafe extern "C" {
    pub fn ctex_transport_readback_get_info(
        readback: *const ctex_transport_readback,
        out_info: *mut ctex_transport_readback_info,
        detail: *mut ::std::os::raw::c_char,
        detail_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_readback_complete_host(
        readback: *mut ctex_transport_readback,
        completed_tiles: *const ctex_transport_host_tile_completion,
        completed_tile_count: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_readback_cancel(readback: *mut ctex_transport_readback) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_transport_readback_fail(
        readback: *mut ctex_transport_readback,
        detail: *const ::std::os::raw::c_char,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_registry_create(
        host: *const ctex_host_executor_descriptor,
        out_registry: *mut *mut ctex_executor_registry,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_registry_destroy(registry: *mut ctex_executor_registry);
}
unsafe extern "C" {
    pub fn ctex_executor_registry_get_count(
        registry: *const ctex_executor_registry,
        out_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_registry_get_info(
        registry: *const ctex_executor_registry,
        executor_index: usize,
        out_info: *mut ctex_executor_info,
        supported_texture_formats: *mut u32,
        supported_texture_format_capacity: usize,
        identifier: *mut ::std::os::raw::c_char,
        identifier_size: usize,
        display_name: *mut ::std::os::raw::c_char,
        display_name_size: usize,
        device_name: *mut ::std::os::raw::c_char,
        device_name_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_registry_select(
        registry: *const ctex_executor_registry,
        requested_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_executor_selection_info,
        requested_identifier_output: *mut ::std::os::raw::c_char,
        requested_identifier_output_size: usize,
        message: *mut ::std::os::raw::c_char,
        message_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_registry_pin_default(
        registry: *mut ctex_executor_registry,
        identifier: *const ::std::os::raw::c_char,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_registry_clear_default(
        registry: *mut ctex_executor_registry,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_make_fallback_report(
        descriptor: *const ctex_executor_fallback_descriptor,
        out_info: *mut ctex_executor_fallback_info,
        message: *mut ::std::os::raw::c_char,
        message_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_cpu_execute_bounded(
        descriptor: *const ctex_cpu_bounded_execution_descriptor,
        out_result: *mut *mut ctex_cpu_execution_result,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_cpu_execution_result_destroy(result: *mut ctex_cpu_execution_result);
}
unsafe extern "C" {
    pub fn ctex_cpu_execution_result_get_info(
        result: *const ctex_cpu_execution_result,
        out_info: *mut ctex_cpu_execution_info,
        message: *mut ::std::os::raw::c_char,
        message_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_cpu_reference_rasterize_viewport(
        descriptor: *const ctex_cpu_viewport_raster_descriptor,
        out_info: *mut ctex_cpu_raster_info,
        outputs: *const ctex_cpu_raster_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_cpu_reference_rasterize_uv(
        descriptor: *const ctex_cpu_uv_raster_descriptor,
        out_info: *mut ctex_cpu_raster_info,
        outputs: *const ctex_cpu_raster_outputs,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_parity_get_tolerance(
        value_class: u32,
        filtered: u32,
        out_info: *mut ctex_parity_tolerance_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_compare_parity(
        reference: *const f64,
        measured: *const f64,
        value_count: usize,
        value_class: u32,
        filtered: u32,
        out_info: *mut ctex_parity_comparison_info,
        message: *mut ::std::os::raw::c_char,
        message_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_executor_run_parity_gate(
        registry: *const ctex_executor_registry,
        fixtures: *const ctex_parity_fixture_descriptor,
        fixture_count: usize,
        executors: *const ctex_parity_executor_binding_descriptor,
        executor_count: usize,
        out_result: *mut *mut ctex_parity_gate_result,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_parity_gate_result_destroy(result: *mut ctex_parity_gate_result);
}
unsafe extern "C" {
    pub fn ctex_parity_gate_result_get_info(
        result: *const ctex_parity_gate_result,
        out_info: *mut ctex_parity_gate_info,
        report: *mut ::std::os::raw::c_char,
        report_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_create(
        initial_revision: u64,
        out_session: *mut *mut ctex_host_execution_session,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_destroy(session: *mut ctex_host_execution_session);
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_get_info(
        session: *const ctex_host_execution_session,
        out_info: *mut ctex_host_execution_session_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_submit(
        session: *mut ctex_host_execution_session,
        descriptor: *const ctex_host_submission_descriptor,
        out_info: *mut ctex_host_submission_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_cancel(
        session: *mut ctex_host_execution_session,
        completion_token: u64,
        out_cancelled: *mut u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_complete(
        session: *mut ctex_host_execution_session,
        descriptor: *const ctex_host_completion_descriptor,
        out_result: *mut *mut ctex_host_completion_result,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_establish_recovery(
        session: *mut ctex_host_execution_session,
        completion_token: u64,
        recovery: *const ctex_host_recovery_descriptor,
        out_result: *mut *mut ctex_host_completion_result,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_get_committed_resource(
        session: *const ctex_host_execution_session,
        logical_id: *const ::std::os::raw::c_char,
        out_found: *mut u32,
        out_generation: *mut u64,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_resource_is_held(
        session: *const ctex_host_execution_session,
        logical_id: *const ::std::os::raw::c_char,
        generation: u64,
        out_held: *mut u32,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_execution_session_report_device_loss(
        session: *mut ctex_host_execution_session,
        out_report: *mut *mut ctex_host_recovery_report,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_completion_result_destroy(result: *mut ctex_host_completion_result);
}
unsafe extern "C" {
    pub fn ctex_host_completion_result_get_info(
        result: *const ctex_host_completion_result,
        out_info: *mut ctex_host_completion_result_info,
        released_resources: *mut ctex_host_resource_version,
        released_resource_capacity: usize,
        released_identities: *mut ::std::os::raw::c_char,
        released_identity_size: usize,
        message: *mut ::std::os::raw::c_char,
        message_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_host_recovery_report_destroy(report: *mut ctex_host_recovery_report);
}
unsafe extern "C" {
    pub fn ctex_host_recovery_report_get_info(
        report: *const ctex_host_recovery_report,
        out_info: *mut ctex_host_device_loss_info,
        cancelled_submissions: *mut u64,
        cancelled_submission_capacity: usize,
        released_resources: *mut ctex_host_resource_version,
        released_resource_capacity: usize,
        released_identities: *mut ::std::os::raw::c_char,
        released_identity_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_apply_smart_material(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        application_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_preset_application_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_apply_smart_mask(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        serialized: *const ::std::os::raw::c_void,
        serialized_size: usize,
        application_identifier: *const ::std::os::raw::c_char,
        target_entry_identifier: *const ::std::os::raw::c_char,
        out_info: *mut ctex_preset_application_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_get_preset_applications(
        document: *const ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        report_output: *mut ::std::os::raw::c_char,
        report_output_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_set_applied_entry_state(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        entry_identifier: *const ::std::os::raw::c_char,
        enabled: u32,
        opacity: f64,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_texture_set_undo_last_preset_application(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        out_info: *mut ctex_preset_undo_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_create(
        document: *mut ctex_document,
        texture_set_id: *const ::std::os::raw::c_char,
        semantic_id: *const ::std::os::raw::c_char,
        out_session: *mut *mut ctex_paint_preview_session,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_destroy(session: *mut ctex_paint_preview_session);
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_write_pixel(
        session: *mut ctex_paint_preview_session,
        x: u32,
        y: u32,
        pixel: *const ::std::os::raw::c_void,
        pixel_size: usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_get_info(
        session: *const ctex_paint_preview_session,
        out_info: *mut ctex_paint_preview_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_get_pixels(
        session: *const ctex_paint_preview_session,
        pixel_buffer: *mut ::std::os::raw::c_void,
        pixel_buffer_size: usize,
        out_required_size: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_get_changed_tiles(
        session: *const ctex_paint_preview_session,
        tiles: *mut ctex_paint_tile_coordinate,
        tile_capacity: usize,
        out_tile_count: *mut usize,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_finalize(
        session: *mut ctex_paint_preview_session,
        coverage: *const u8,
        coverage_count: usize,
        dilation_radius: u32,
        out_info: *mut ctex_paint_preview_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_commit(
        session: *mut ctex_paint_preview_session,
        out_info: *mut ctex_paint_preview_info,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_paint_preview_session_cancel(
        session: *mut ctex_paint_preview_session,
    ) -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_get_last_result() -> ctex_result;
}
unsafe extern "C" {
    pub fn ctex_get_last_diagnostic_code() -> ctex_diagnostic_code;
}
unsafe extern "C" {
    pub fn ctex_get_last_diagnostic() -> *const ::std::os::raw::c_char;
}
