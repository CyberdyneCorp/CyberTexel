#ifndef CTEX_CAPI_H
#define CTEX_CAPI_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(CTEX_SHARED)
#if defined(CTEX_BUILDING_LIBRARY)
#define CTEX_API __declspec(dllexport)
#else
#define CTEX_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define CTEX_API __attribute__((visibility("default")))
#else
#define CTEX_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ctex_result {
    CTEX_RESULT_SUCCESS = 0,
    CTEX_RESULT_INVALID_ARGUMENT = 1,
    CTEX_RESULT_MISSING_RESOURCE = 2,
    CTEX_RESULT_UNSUPPORTED_OPERATION = 3,
    CTEX_RESULT_OUT_OF_MEMORY = 4,
    CTEX_RESULT_OVER_BUDGET = 5,
    CTEX_RESULT_CANCELLED = 6,
    CTEX_RESULT_INTERNAL_ERROR = 7,
    CTEX_RESULT_BUFFER_TOO_SMALL = 8,
    CTEX_RESULT_NO_UNDO = 9,
    CTEX_RESULT_NO_REDO = 10,
    CTEX_RESULT_STALE_STATE = 11
} ctex_result;

typedef enum ctex_diagnostic_code {
    CTEX_DIAGNOSTIC_NONE = 0,
    CTEX_DIAGNOSTIC_NULL_ARGUMENT = 1,
    CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE = 2,
    CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE = 3,
    CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_VALUE = 4,
    CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL = 5,
    CTEX_DIAGNOSTIC_ALLOCATION_FAILED = 6,
    CTEX_DIAGNOSTIC_UNEXPECTED_EXCEPTION = 7,
    CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_DISPLAY_NAME = 8,
    CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_PARTITION_KEY = 9,
    CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_UV_SET = 10,
    CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_RESOLUTION = 11,
    CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_BIT_DEPTH = 12,
    CTEX_DIAGNOSTIC_DUPLICATE_TEXTURE_SET = 13,
    CTEX_DIAGNOSTIC_ALLOCATOR_CONTRACT_VIOLATION = 14,
    CTEX_DIAGNOSTIC_MISSING_TEXTURE_SET = 15,
    CTEX_DIAGNOSTIC_EMPTY_CHANNEL_SEMANTIC_ID = 16,
    CTEX_DIAGNOSTIC_EMPTY_CHANNEL_EXPORT_MAPPING = 17,
    CTEX_DIAGNOSTIC_INVALID_CHANNEL_COMPONENT_COUNT = 18,
    CTEX_DIAGNOSTIC_INVALID_CHANNEL_DEFAULT_VALUE_COUNT = 19,
    CTEX_DIAGNOSTIC_INVALID_CHANNEL_BIT_DEPTH = 20,
    CTEX_DIAGNOSTIC_DUPLICATE_CHANNEL = 21,
    CTEX_DIAGNOSTIC_MISSING_CHANNEL = 22,
    CTEX_DIAGNOSTIC_UNSUPPORTED_COLOR_SPACE = 23,
    CTEX_DIAGNOSTIC_UNSUPPORTED_CHANNEL_SEMANTIC = 24,
    CTEX_DIAGNOSTIC_INVALID_COLOR_COMPONENT = 25,
    CTEX_DIAGNOSTIC_INVALID_COLOR_BIT_DEPTH = 26,
    CTEX_DIAGNOSTIC_INVALID_CUBE_LUT = 27,
    CTEX_DIAGNOSTIC_MESH_LIMIT_EXCEEDED = 28,
    CTEX_DIAGNOSTIC_INVALID_MESH = 29,
    CTEX_DIAGNOSTIC_MISSING_UV_SET = 30,
    CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_FORMAT = 31,
    CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA = 32,
    CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED = 33,
    CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_COMBINATION = 34,
    CTEX_DIAGNOSTIC_IMAGE_ENCODING_FAILED = 35,
    CTEX_DIAGNOSTIC_INVALID_STROKE = 36,
    CTEX_DIAGNOSTIC_INVALID_STROKE_PRESET = 37,
    CTEX_DIAGNOSTIC_INVALID_PAINT_COVERAGE = 38,
    CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED = 39,
    CTEX_DIAGNOSTIC_INVALID_PAINT_BLEND = 40,
    CTEX_DIAGNOSTIC_INVALID_PAINT_MASK = 41,
    CTEX_DIAGNOSTIC_INVALID_PAINT_COORDINATES = 42,
    CTEX_DIAGNOSTIC_INVALID_PAINT_REJECTION = 43,
    CTEX_DIAGNOSTIC_INVALID_PAINT_WORK = 44,
    CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION = 45,
    CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER = 46,
    CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE = 47,
    CTEX_DIAGNOSTIC_INVALID_PAINT_PREVIEW = 48,
    CTEX_DIAGNOSTIC_INVALID_PICK_QUERY = 49,
    CTEX_DIAGNOSTIC_INVALID_TEXTURE_EXPORT = 50,
    CTEX_DIAGNOSTIC_INVALID_PROJECT_CONTAINER = 51,
    CTEX_DIAGNOSTIC_INVALID_SMART_MATERIAL = 52,
    CTEX_DIAGNOSTIC_INVALID_PRESET_LIBRARY = 53,
    CTEX_DIAGNOSTIC_INVALID_HOST_TRANSPORT = 54,
    CTEX_DIAGNOSTIC_INVALID_EXECUTOR = 55,
    CTEX_DIAGNOSTIC_INVALID_PAINT_TOOL = 56,
    CTEX_DIAGNOSTIC_INVALID_MATERIAL_GRAPH = 57,
    CTEX_DIAGNOSTIC_INVALID_SHADER_EMISSION = 58,
    CTEX_DIAGNOSTIC_INVALID_MESH_MAP = 59,
    CTEX_DIAGNOSTIC_INVALID_TILE_HISTORY = 60,
    CTEX_DIAGNOSTIC_INVALID_OPERATION_RECORD = 61,
    CTEX_DIAGNOSTIC_INVALID_EDITABLE_AUTHORING = 62
} ctex_diagnostic_code;

typedef enum ctex_log_severity {
    CTEX_LOG_SEVERITY_TRACE = 0,
    CTEX_LOG_SEVERITY_DEBUG = 1,
    CTEX_LOG_SEVERITY_INFO = 2,
    CTEX_LOG_SEVERITY_WARNING = 3,
    CTEX_LOG_SEVERITY_ERROR = 4,
    CTEX_LOG_SEVERITY_FATAL = 5
} ctex_log_severity;

typedef void (*ctex_log_callback)(ctex_log_severity severity, const char* category,
                                  const char* message, void* user_data);

typedef struct ctex_log_sink_descriptor {
    uint32_t size;
    ctex_log_callback callback;
    void* user_data;
    uint32_t minimum_severity;
} ctex_log_sink_descriptor;

#define CTEX_LOG_SINK_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_log_sink_descriptor))
#define CTEX_LOG_SINK_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_log_sink_descriptor))

typedef void* (*ctex_allocate_callback)(size_t size, size_t alignment, void* user_data);
typedef void (*ctex_deallocate_callback)(void* allocation, size_t size, size_t alignment,
                                         void* user_data);

typedef struct ctex_allocator_descriptor {
    uint32_t size;
    ctex_allocate_callback allocate;
    ctex_deallocate_callback deallocate;
    void* user_data;
} ctex_allocator_descriptor;

#define CTEX_ALLOCATOR_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_allocator_descriptor))
#define CTEX_ALLOCATOR_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_allocator_descriptor))

typedef struct ctex_document ctex_document;
typedef struct ctex_cube_lut ctex_cube_lut;
typedef struct ctex_mesh ctex_mesh;
typedef struct ctex_mesh_replacement_plan ctex_mesh_replacement_plan;
typedef struct ctex_paint_dilation_session ctex_paint_dilation_session;
typedef struct ctex_paint_surface_map_cache ctex_paint_surface_map_cache;
typedef struct ctex_paint_preview_session ctex_paint_preview_session;
typedef struct ctex_pick_index ctex_pick_index;
typedef struct ctex_uv_pick_index ctex_uv_pick_index;
typedef struct ctex_transport_snapshot_pool ctex_transport_snapshot_pool;
typedef struct ctex_transport_snapshot ctex_transport_snapshot;
typedef struct ctex_transport_readback ctex_transport_readback;
typedef struct ctex_resource_ledger ctex_resource_ledger;
typedef struct ctex_resource_reservation ctex_resource_reservation;
typedef struct ctex_mesh_map_set ctex_mesh_map_set;
typedef struct ctex_mesh_map_bake_session ctex_mesh_map_bake_session;
typedef struct ctex_mesh_map_bake_request_token ctex_mesh_map_bake_request_token;
typedef struct ctex_project_autosave_session ctex_project_autosave_session;
typedef struct ctex_executor_registry ctex_executor_registry;
typedef struct ctex_cpu_execution_result ctex_cpu_execution_result;
typedef struct ctex_parity_gate_result ctex_parity_gate_result;
typedef struct ctex_host_execution_session ctex_host_execution_session;
typedef struct ctex_host_completion_result ctex_host_completion_result;
typedef struct ctex_host_recovery_report ctex_host_recovery_report;
typedef struct ctex_material_graph_workspace ctex_material_graph_workspace;
typedef struct ctex_material_graph_node_registry ctex_material_graph_node_registry;
typedef struct ctex_shader_emission_cache ctex_shader_emission_cache;
typedef struct ctex_tile_history_capture ctex_tile_history_capture;
typedef struct ctex_layer_snapshot ctex_layer_snapshot;
typedef struct ctex_texture_set_transaction ctex_texture_set_transaction;

#define CTEX_MAX_MESH_VERTEX_COUNT ((size_t)100000000)
#define CTEX_MAX_MESH_TRIANGLE_COUNT ((size_t)100000000)
#define CTEX_MAX_MESH_UV_COVERAGE_SAMPLES UINT64_C(268435456)
#define CTEX_DEFAULT_TILE_SIZE ((uint32_t)64)
#define CTEX_NO_SURFACE_TRIANGLE UINT32_MAX
#define CTEX_NO_UV_ISLAND UINT32_MAX
#define CTEX_MAX_MATERIAL_GRAPH_SERIALIZED_SIZE ((size_t)67108864)
#define CTEX_MAX_MATERIAL_GRAPH_LIBRARY_SERIALIZED_SIZE ((size_t)268435456)

typedef enum ctex_partition_source_kind {
    CTEX_PARTITION_SOURCE_MATERIAL = 0,
    CTEX_PARTITION_SOURCE_OBJECT = 1,
    CTEX_PARTITION_SOURCE_SUBMESH = 2,
    CTEX_PARTITION_SOURCE_EXPLICIT_FACES = 3
} ctex_partition_source_kind;

typedef struct ctex_vec2f {
    float x;
    float y;
} ctex_vec2f;

typedef struct ctex_vec3f {
    float x;
    float y;
    float z;
} ctex_vec3f;

typedef struct ctex_vec4f {
    float x;
    float y;
    float z;
    float w;
} ctex_vec4f;

typedef struct ctex_vec2d {
    double x;
    double y;
} ctex_vec2d;

typedef struct ctex_vec3d {
    double x;
    double y;
    double z;
} ctex_vec3d;

typedef struct ctex_stroke_frame {
    ctex_vec3d tangent;
    ctex_vec3d bitangent;
    ctex_vec3d normal;
} ctex_stroke_frame;

typedef struct ctex_stroke_input_sample {
    uint32_t size;
    ctex_vec3d position;
    ctex_stroke_frame frame;
    uint64_t timestamp_nanoseconds;
    uint32_t has_pressure;
    double pressure;
    ctex_vec2d tilt;
} ctex_stroke_input_sample;

#define CTEX_STROKE_INPUT_SAMPLE_V1_SIZE ((uint32_t)sizeof(ctex_stroke_input_sample))
#define CTEX_STROKE_INPUT_SAMPLE_CURRENT_SIZE ((uint32_t)sizeof(ctex_stroke_input_sample))

typedef enum ctex_stroke_tip_mode {
    CTEX_STROKE_TIP_CONTINUOUS_SWEEP = 0,
    CTEX_STROKE_TIP_DISCRETE_ALPHA = 1
} ctex_stroke_tip_mode;

typedef struct ctex_response_curve_point {
    double input;
    double output;
} ctex_response_curve_point;

typedef struct ctex_response_mapping_descriptor {
    uint32_t size;
    uint32_t enabled;
    const ctex_response_curve_point* points;
    size_t point_count;
    double minimum_output;
    double maximum_output;
} ctex_response_mapping_descriptor;

#define CTEX_RESPONSE_MAPPING_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_response_mapping_descriptor))
#define CTEX_RESPONSE_MAPPING_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_response_mapping_descriptor))

typedef struct ctex_stroke_stabilizer_descriptor {
    uint32_t size;
    double radius;
    double time_constant_seconds;
} ctex_stroke_stabilizer_descriptor;

#define CTEX_STROKE_STABILIZER_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_stroke_stabilizer_descriptor))
#define CTEX_STROKE_STABILIZER_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_stroke_stabilizer_descriptor))

typedef struct ctex_stroke_jitter_descriptor {
    uint32_t size;
    uint64_t seed;
    double position_fraction;
    double radius_fraction;
    double rotation_radians;
    double opacity;
    double flow;
} ctex_stroke_jitter_descriptor;

#define CTEX_STROKE_JITTER_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_stroke_jitter_descriptor))
#define CTEX_STROKE_JITTER_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_stroke_jitter_descriptor))

typedef enum ctex_stroke_taper_unit {
    CTEX_STROKE_TAPER_NONE = 0,
    CTEX_STROKE_TAPER_STAMP_COUNT = 1,
    CTEX_STROKE_TAPER_DISTANCE = 2
} ctex_stroke_taper_unit;

typedef struct ctex_stroke_taper_span_descriptor {
    uint32_t size;
    uint32_t unit;
    double extent;
} ctex_stroke_taper_span_descriptor;

#define CTEX_STROKE_TAPER_SPAN_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_stroke_taper_span_descriptor))
#define CTEX_STROKE_TAPER_SPAN_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_stroke_taper_span_descriptor))

typedef struct ctex_stroke_taper_descriptor {
    uint32_t size;
    ctex_stroke_taper_span_descriptor entry;
    ctex_stroke_taper_span_descriptor exit;
    double floor;
    uint32_t affect_radius;
    uint32_t affect_opacity;
} ctex_stroke_taper_descriptor;

#define CTEX_STROKE_TAPER_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_stroke_taper_descriptor))
#define CTEX_STROKE_TAPER_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_stroke_taper_descriptor))

typedef enum ctex_stroke_constraint_mode {
    CTEX_STROKE_CONSTRAINT_NONE = 0,
    CTEX_STROKE_CONSTRAINT_STRAIGHT_LINE = 1,
    CTEX_STROKE_CONSTRAINT_DOMINANT_AXIS = 2,
    CTEX_STROKE_CONSTRAINT_GRID = 3
} ctex_stroke_constraint_mode;

typedef struct ctex_stroke_constraint_descriptor {
    uint32_t size;
    uint32_t mode;
    double grid_step;
} ctex_stroke_constraint_descriptor;

#define CTEX_STROKE_CONSTRAINT_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_stroke_constraint_descriptor))
#define CTEX_STROKE_CONSTRAINT_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_stroke_constraint_descriptor))

typedef enum ctex_stroke_symmetry_axis {
    CTEX_STROKE_SYMMETRY_AXIS_X = 0,
    CTEX_STROKE_SYMMETRY_AXIS_Y = 1,
    CTEX_STROKE_SYMMETRY_AXIS_Z = 2
} ctex_stroke_symmetry_axis;

typedef struct ctex_stroke_symmetry_descriptor {
    uint32_t size;
    uint32_t mirror_x;
    uint32_t mirror_y;
    uint32_t mirror_z;
    uint32_t radial_count;
    uint32_t radial_axis;
} ctex_stroke_symmetry_descriptor;

#define CTEX_STROKE_SYMMETRY_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_stroke_symmetry_descriptor))
#define CTEX_STROKE_SYMMETRY_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_stroke_symmetry_descriptor))

typedef struct ctex_stroke_settings_descriptor {
    uint32_t size;
    uint32_t reconstruction_version;
    uint32_t tip_mode;
    double spacing_fraction;
    double radius;
    double opacity;
    double hardness;
    double rotation_radians;
    double elongation;
    double flow;
    const char* tip_resource_identity;
    ctex_stroke_stabilizer_descriptor stabilizer;
    ctex_response_mapping_descriptor pressure_radius;
    ctex_response_mapping_descriptor pressure_opacity;
    ctex_response_mapping_descriptor pressure_hardness;
    ctex_response_mapping_descriptor pressure_flow;
    ctex_response_mapping_descriptor pressure_rotation;
    ctex_response_mapping_descriptor tilt_rotation;
    ctex_response_mapping_descriptor tilt_elongation;
    ctex_stroke_jitter_descriptor jitter;
    ctex_stroke_taper_descriptor taper;
    ctex_stroke_constraint_descriptor constraint;
    ctex_stroke_symmetry_descriptor symmetry;
} ctex_stroke_settings_descriptor;

#define CTEX_STROKE_SETTINGS_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_stroke_settings_descriptor))
#define CTEX_STROKE_SETTINGS_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_stroke_settings_descriptor))

typedef struct ctex_resolved_stamp {
    ctex_vec3d position;
    ctex_stroke_frame frame;
    double radius;
    double opacity;
    double hardness;
    double rotation_radians;
    double elongation;
    double flow;
    const char* tip_resource_identity;
    uint64_t source_ordinal;
    uint64_t symmetry_instance;
    uint64_t ordinal;
} ctex_resolved_stamp;

typedef struct ctex_swept_segment {
    uint64_t start_stamp_ordinal;
    uint64_t end_stamp_ordinal;
} ctex_swept_segment;

typedef struct ctex_resolved_stroke_info {
    uint32_t size;
    uint32_t reconstruction_version;
    uint32_t tip_mode;
    uint64_t symmetry_instance_count;
    size_t stamp_count;
    size_t swept_segment_count;
} ctex_resolved_stroke_info;

#define CTEX_RESOLVED_STROKE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_resolved_stroke_info))
#define CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_resolved_stroke_info))

typedef struct ctex_resolved_stroke_descriptor {
    uint32_t size;
    uint32_t reconstruction_version;
    uint32_t tip_mode;
    uint64_t symmetry_instance_count;
    const ctex_resolved_stamp* stamps;
    size_t stamp_count;
    const ctex_swept_segment* swept_segments;
    size_t swept_segment_count;
} ctex_resolved_stroke_descriptor;

#define CTEX_RESOLVED_STROKE_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_resolved_stroke_descriptor))
#define CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_resolved_stroke_descriptor))

#define CTEX_MAX_PAINT_TILE_TEXEL_COUNT ((size_t)1048576)

typedef struct ctex_paint_tile_coverage_descriptor {
    uint32_t size;
    const char* uv_set;
    uint32_t width;
    uint32_t height;
    ctex_vec2d tile_origin;
} ctex_paint_tile_coverage_descriptor;

#define CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_tile_coverage_descriptor))
#define CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_tile_coverage_descriptor))

typedef enum ctex_paint_deposition_mode {
    CTEX_PAINT_DEPOSITION_NON_BUILDING = 0,
    CTEX_PAINT_DEPOSITION_BUILD_UP = 1
} ctex_paint_deposition_mode;

typedef enum ctex_alpha_discard_format {
    CTEX_ALPHA_DISCARD_UNORM8 = 0,
    CTEX_ALPHA_DISCARD_UNORM16 = 1,
    CTEX_ALPHA_DISCARD_FLOATING_POINT = 2
} ctex_alpha_discard_format;

typedef struct ctex_paint_mask_view {
    const double* values;
    size_t value_count;
} ctex_paint_mask_view;

typedef struct ctex_paint_mask_inputs_descriptor {
    uint32_t size;
    const ctex_paint_mask_view* active_layer_masks;
    size_t active_layer_mask_count;
    const ctex_paint_mask_view* colour_id_selection;
    const ctex_paint_mask_view* geometry_selection;
    const ctex_paint_mask_view* screen_selection;
    const ctex_paint_mask_view* uv_island_selection;
} ctex_paint_mask_inputs_descriptor;

#define CTEX_PAINT_MASK_INPUTS_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_mask_inputs_descriptor))
#define CTEX_PAINT_MASK_INPUTS_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_mask_inputs_descriptor))

typedef struct ctex_paint_mask_info {
    uint32_t size;
    size_t active_input_count;
} ctex_paint_mask_info;

#define CTEX_PAINT_MASK_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_mask_info))
#define CTEX_PAINT_MASK_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_mask_info))

typedef enum ctex_paint_material_coordinate_mode {
    CTEX_PAINT_MATERIAL_COORDINATE_UV = 0,
    CTEX_PAINT_MATERIAL_COORDINATE_TRIPLANAR = 1,
    CTEX_PAINT_MATERIAL_COORDINATE_PLANAR = 2
} ctex_paint_material_coordinate_mode;

typedef struct ctex_paint_material_coordinate_descriptor {
    uint32_t size;
    uint32_t mode;
    ctex_vec3d planar_origin;
    ctex_vec3d planar_u_axis;
    ctex_vec3d planar_v_axis;
} ctex_paint_material_coordinate_descriptor;

#define CTEX_PAINT_MATERIAL_COORDINATE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_material_coordinate_descriptor))
#define CTEX_PAINT_MATERIAL_COORDINATE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_material_coordinate_descriptor))

typedef struct ctex_paint_material_coordinate_sample {
    uint32_t covered;
    uint32_t projection_count;
    ctex_vec2d coordinates[3];
    double weights[3];
} ctex_paint_material_coordinate_sample;

typedef enum ctex_paint_symmetry_depth_policy {
    CTEX_PAINT_SYMMETRY_DEPTH_REQUIRE_CONSISTENT = 0,
    CTEX_PAINT_SYMMETRY_DEPTH_DISABLE_DERIVED = 1
} ctex_paint_symmetry_depth_policy;

typedef enum ctex_paint_depth_disposition {
    CTEX_PAINT_DEPTH_DISABLED_BY_OPERATION = 0,
    CTEX_PAINT_DEPTH_CONSISTENT_PER_INSTANCE = 1,
    CTEX_PAINT_DEPTH_DISABLED_FOR_DERIVED_SYMMETRY = 2
} ctex_paint_depth_disposition;

typedef struct ctex_paint_depth_context_descriptor {
    uint32_t size;
    uint64_t symmetry_instance;
    uint32_t viewport_width;
    uint32_t viewport_height;
    const ctex_vec2d* screen_positions;
    const double* surface_depth;
    size_t surface_sample_count;
    const double* visible_depth;
    size_t visible_depth_count;
    uint32_t transform_consistent;
} ctex_paint_depth_context_descriptor;

#define CTEX_PAINT_DEPTH_CONTEXT_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_depth_context_descriptor))
#define CTEX_PAINT_DEPTH_CONTEXT_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_depth_context_descriptor))

typedef struct ctex_paint_rejection_descriptor {
    uint32_t size;
    uint32_t depth_enabled;
    double depth_bias;
    uint32_t symmetry_depth_policy;
    uint32_t angle_enabled;
    double minimum_normal_dot;
    uint32_t backface_enabled;
    const ctex_paint_depth_context_descriptor* depth_contexts;
    size_t depth_context_count;
    const ctex_vec3d* view_directions;
    size_t view_direction_count;
} ctex_paint_rejection_descriptor;

#define CTEX_PAINT_REJECTION_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_rejection_descriptor))
#define CTEX_PAINT_REJECTION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_rejection_descriptor))

typedef struct ctex_paint_rejection_info {
    uint32_t size;
    uint32_t depth_disposition;
    size_t depth_rejected_contributions;
    size_t angle_rejected_contributions;
    size_t backface_rejected_texels;
    double resolved_depth_bias;
    double resolved_minimum_normal_dot;
    uint32_t depth_bias_clamped;
    uint32_t minimum_normal_dot_clamped;
} ctex_paint_rejection_info;

#define CTEX_PAINT_REJECTION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_rejection_info))
#define CTEX_PAINT_REJECTION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_rejection_info))

typedef struct ctex_paint_stamp_footprint {
    uint64_t stamp_ordinal;
    uint32_t minimum_x;
    uint32_t minimum_y;
    uint32_t maximum_x;
    uint32_t maximum_y;
} ctex_paint_stamp_footprint;

typedef struct ctex_paint_tile_coordinate {
    uint32_t x;
    uint32_t y;
} ctex_paint_tile_coordinate;

typedef enum ctex_paint_preview_state {
    CTEX_PAINT_PREVIEW_PROVISIONAL = 0,
    CTEX_PAINT_PREVIEW_FINAL = 1,
    CTEX_PAINT_PREVIEW_COMMITTED = 2,
    CTEX_PAINT_PREVIEW_CANCELLED = 3
} ctex_paint_preview_state;

typedef struct ctex_paint_preview_info {
    uint32_t size;
    uint32_t state;
    uint32_t width;
    uint32_t height;
    uint32_t component_count;
    uint32_t scalar_representation;
    uint32_t bit_depth;
    size_t pixel_byte_count;
    uint32_t resolved_dilation_radius;
    uint32_t dilation_radius_clamped;
    size_t dilated_texel_count;
    size_t zero_gradient_texel_count;
    uint64_t baseline_epoch;
    uint64_t baseline_revision;
    uint64_t preview_epoch;
    uint64_t preview_revision;
    uint64_t committed_epoch;
    uint64_t committed_revision;
    size_t changed_tile_count;
    double maximum_component_error;
} ctex_paint_preview_info;

#define CTEX_PAINT_PREVIEW_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_preview_info))
#define CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_preview_info))

typedef struct ctex_paint_work_descriptor {
    uint32_t size;
    uint32_t canvas_width;
    uint32_t canvas_height;
    uint32_t tile_size;
    uint32_t dilation_radius;
    const ctex_paint_stamp_footprint* stamp_footprints;
    size_t stamp_footprint_count;
} ctex_paint_work_descriptor;

#define CTEX_PAINT_WORK_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_work_descriptor))
#define CTEX_PAINT_WORK_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_work_descriptor))

typedef struct ctex_paint_work_info {
    uint32_t size;
    uint64_t canvas_tile_count;
    size_t footprint_count;
    size_t candidate_tile_visits;
    size_t processed_tile_count;
    uint32_t resolved_dilation_radius;
    uint32_t dilation_radius_clamped;
} ctex_paint_work_info;

#define CTEX_PAINT_WORK_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_work_info))
#define CTEX_PAINT_WORK_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_work_info))

typedef struct ctex_paint_seam_dilation_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t component_count;
    uint32_t radius;
    const double* pixels;
    size_t pixel_count;
    const uint8_t* coverage;
    size_t coverage_count;
} ctex_paint_seam_dilation_descriptor;

#define CTEX_PAINT_SEAM_DILATION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_seam_dilation_descriptor))
#define CTEX_PAINT_SEAM_DILATION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_seam_dilation_descriptor))

typedef struct ctex_paint_seam_dilation_info {
    uint32_t size;
    size_t required_pixel_count;
    size_t dilated_texel_count;
    size_t zero_gradient_texel_count;
    uint32_t resolved_radius;
    uint32_t radius_clamped;
} ctex_paint_seam_dilation_info;

#define CTEX_PAINT_SEAM_DILATION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_seam_dilation_info))
#define CTEX_PAINT_SEAM_DILATION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_seam_dilation_info))

typedef enum ctex_paint_dilation_state {
    CTEX_PAINT_DILATION_PROVISIONAL = 0,
    CTEX_PAINT_DILATION_FINAL = 1
} ctex_paint_dilation_state;

typedef struct ctex_paint_dilation_tile_descriptor {
    uint32_t size;
    int32_t u;
    int32_t v;
    uint32_t width;
    uint32_t height;
    uint32_t component_count;
    const double* pixels;
    size_t pixel_count;
    const uint8_t* coverage;
    size_t coverage_count;
} ctex_paint_dilation_tile_descriptor;

#define CTEX_PAINT_DILATION_TILE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_dilation_tile_descriptor))
#define CTEX_PAINT_DILATION_TILE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_dilation_tile_descriptor))

typedef struct ctex_paint_dilation_tile_info {
    int32_t u;
    int32_t v;
    uint32_t width;
    uint32_t height;
    uint32_t component_count;
    size_t pixel_offset;
    size_t pixel_count;
    size_t dilated_texel_count;
    size_t zero_gradient_texel_count;
} ctex_paint_dilation_tile_info;

typedef struct ctex_paint_dilation_session_info {
    uint32_t size;
    uint32_t state;
    size_t tile_count;
    size_t required_pixel_count;
    size_t dilation_pass_count;
    uint32_t resolved_radius;
    uint32_t radius_clamped;
} ctex_paint_dilation_session_info;

#define CTEX_PAINT_DILATION_SESSION_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_dilation_session_info))
#define CTEX_PAINT_DILATION_SESSION_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_dilation_session_info))

typedef enum ctex_paint_surface_filter_operation {
    CTEX_PAINT_SURFACE_FILTER_BLUR = 0,
    CTEX_PAINT_SURFACE_FILTER_SMEAR = 1,
    CTEX_PAINT_SURFACE_FILTER_DERIVATIVE = 2,
    CTEX_PAINT_SURFACE_FILTER_MIP_GENERATION = 3
} ctex_paint_surface_filter_operation;

typedef struct ctex_paint_surface_filter_sample {
    size_t texel_index;
    ctex_stroke_frame tangent_frame;
    int32_t offset_x;
    int32_t offset_y;
    double weight;
} ctex_paint_surface_filter_sample;

typedef struct ctex_paint_surface_filter_descriptor {
    uint32_t size;
    uint32_t operation;
    uint32_t radius_x;
    uint32_t radius_y;
    ctex_stroke_frame output_frame;
    const ctex_paint_surface_filter_sample* samples;
    size_t sample_count;
} ctex_paint_surface_filter_descriptor;

#define CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_surface_filter_descriptor))
#define CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_surface_filter_descriptor))

typedef struct ctex_paint_island_padding_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t component_count;
    uint32_t radius_x;
    uint32_t radius_y;
    uint32_t requested_mip_levels;
    const uint32_t* island_identity;
    size_t island_identity_count;
    const double* pixels;
    size_t pixel_count;
} ctex_paint_island_padding_descriptor;

#define CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_island_padding_descriptor))
#define CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_island_padding_descriptor))

typedef struct ctex_paint_unsupported_mip_level {
    uint32_t mip_level;
    uint32_t required_gutter_radius;
    size_t affected_island_offset;
    size_t affected_island_count;
} ctex_paint_unsupported_mip_level;

typedef struct ctex_paint_island_padding_info {
    uint32_t size;
    size_t required_ownership_count;
    size_t unsupported_mip_level_count;
    size_t required_affected_island_count;
    size_t required_pixel_count;
    size_t padded_texel_count;
    uint32_t padding_radius;
} ctex_paint_island_padding_info;

#define CTEX_PAINT_ISLAND_PADDING_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_island_padding_info))
#define CTEX_PAINT_ISLAND_PADDING_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_island_padding_info))

typedef struct ctex_paint_surface_map_request {
    uint32_t size;
    size_t partition_index;
    const char* uv_set;
    uint32_t width;
    uint32_t height;
    ctex_vec2d tile_origin;
} ctex_paint_surface_map_request;

#define CTEX_PAINT_SURFACE_MAP_REQUEST_V1_SIZE ((uint32_t)sizeof(ctex_paint_surface_map_request))
#define CTEX_PAINT_SURFACE_MAP_REQUEST_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_surface_map_request))

typedef struct ctex_paint_surface_texel {
    ctex_vec3d position;
    ctex_vec3d normal;
    ctex_vec3d geometric_normal;
    ctex_vec2d uv;
    uint32_t triangle;
} ctex_paint_surface_texel;

typedef struct ctex_paint_surface_map_info {
    uint32_t size;
    uint32_t cache_hit;
    uint64_t mesh_revision;
    size_t required_texture_set_id_size;
    size_t required_uv_set_size;
    size_t required_texel_count;
} ctex_paint_surface_map_info;

#define CTEX_PAINT_SURFACE_MAP_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_surface_map_info))
#define CTEX_PAINT_SURFACE_MAP_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_surface_map_info))

typedef struct ctex_paint_surface_map_buffers {
    uint32_t size;
    char* texture_set_id;
    size_t texture_set_id_size;
    char* uv_set;
    size_t uv_set_size;
    ctex_paint_surface_texel* surface_texels;
    size_t surface_texel_capacity;
    uint8_t* coverage;
    size_t coverage_capacity;
    uint32_t* triangle_identity;
    size_t triangle_identity_capacity;
    uint32_t* uv_island_identity;
    size_t uv_island_identity_capacity;
} ctex_paint_surface_map_buffers;

#define CTEX_PAINT_SURFACE_MAP_BUFFERS_V1_SIZE ((uint32_t)sizeof(ctex_paint_surface_map_buffers))
#define CTEX_PAINT_SURFACE_MAP_BUFFERS_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_surface_map_buffers))

typedef struct ctex_paint_surface_map_statistics {
    uint32_t size;
    size_t entries;
    size_t hits;
    size_t misses;
    size_t invalidated_entries;
} ctex_paint_surface_map_statistics;

#define CTEX_PAINT_SURFACE_MAP_STATISTICS_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_surface_map_statistics))
#define CTEX_PAINT_SURFACE_MAP_STATISTICS_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_surface_map_statistics))

typedef struct ctex_paint_deposition_descriptor {
    uint32_t size;
    uint32_t mode;
    uint32_t alpha_discard_format;
    uint32_t has_custom_alpha_discard_threshold;
    double custom_alpha_discard_threshold;
    const ctex_paint_mask_inputs_descriptor* masks;
} ctex_paint_deposition_descriptor;

#define CTEX_PAINT_DEPOSITION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)offsetof(ctex_paint_deposition_descriptor, masks))
#define CTEX_PAINT_DEPOSITION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_deposition_descriptor))

typedef struct ctex_paint_deposition_info {
    uint32_t size;
    uint32_t mode;
    size_t applied_stamp_count;
    double alpha_discard_threshold;
    uint32_t alpha_discard_threshold_clamped;
} ctex_paint_deposition_info;

#define CTEX_PAINT_DEPOSITION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_deposition_info))
#define CTEX_PAINT_DEPOSITION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_deposition_info))

typedef struct ctex_paint_deposition_sample {
    double non_building_coverage;
    double build_up_deposition;
    double strength;
    double retained_strength;
    uint32_t write;
} ctex_paint_deposition_sample;

typedef struct ctex_paint_blend_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    const char* blend_mode;
    const ctex_vec4f* stroke_start_snapshot;
    const ctex_vec4f* paint;
    const ctex_paint_deposition_sample* deposition;
    size_t pixel_count;
} ctex_paint_blend_descriptor;

#define CTEX_PAINT_BLEND_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_blend_descriptor))
#define CTEX_PAINT_BLEND_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_blend_descriptor))

typedef struct ctex_paint_tool_channel_descriptor {
    uint32_t size;
    const char* semantic_id;
    uint32_t component_count;
    const ctex_vec4f* pixels;
    size_t pixel_count;
} ctex_paint_tool_channel_descriptor;

#define CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_tool_channel_descriptor))
#define CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_tool_channel_descriptor))

typedef struct ctex_paint_tool_channel_output {
    uint32_t size;
    ctex_vec4f* pixels;
    size_t pixel_capacity;
} ctex_paint_tool_channel_output;

#define CTEX_PAINT_TOOL_CHANNEL_OUTPUT_V1_SIZE ((uint32_t)sizeof(ctex_paint_tool_channel_output))
#define CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_tool_channel_output))

typedef struct ctex_paint_brush_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    const ctex_paint_tool_channel_descriptor* enabled_layer_snapshot;
    size_t enabled_layer_channel_count;
    const ctex_paint_tool_channel_descriptor* material;
    size_t material_channel_count;
    const ctex_paint_deposition_sample* deposition;
    size_t deposition_count;
    const char* blend_mode;
} ctex_paint_brush_descriptor;

#define CTEX_PAINT_BRUSH_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_brush_descriptor))
#define CTEX_PAINT_BRUSH_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_brush_descriptor))

typedef struct ctex_paint_brush_info {
    uint32_t size;
    size_t applied_channel_count;
    size_t required_pixels_per_channel;
} ctex_paint_brush_info;

#define CTEX_PAINT_BRUSH_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_brush_info))
#define CTEX_PAINT_BRUSH_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_brush_info))

typedef enum ctex_paint_eraser_target {
    CTEX_PAINT_ERASER_TARGET_LAYER_OPACITY = 0,
    CTEX_PAINT_ERASER_TARGET_MASK = 1
} ctex_paint_eraser_target;

typedef struct ctex_paint_eraser_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t target;
    const double* stroke_start_values;
    size_t value_count;
    const ctex_paint_deposition_sample* deposition;
    size_t deposition_count;
} ctex_paint_eraser_descriptor;

#define CTEX_PAINT_ERASER_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_eraser_descriptor))
#define CTEX_PAINT_ERASER_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_eraser_descriptor))

typedef struct ctex_paint_eraser_info {
    uint32_t size;
    uint32_t target;
    size_t required_value_count;
} ctex_paint_eraser_info;

#define CTEX_PAINT_ERASER_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_eraser_info))
#define CTEX_PAINT_ERASER_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_eraser_info))

typedef enum ctex_paint_fill_scope {
    CTEX_PAINT_FILL_WHOLE_SET = 0,
    CTEX_PAINT_FILL_TRIANGLE = 1,
    CTEX_PAINT_FILL_CONNECTED_BY_ANGLE = 2,
    CTEX_PAINT_FILL_UV_ISLAND = 3,
    CTEX_PAINT_FILL_UV_TILE = 4,
    CTEX_PAINT_FILL_SELECTION = 5
} ctex_paint_fill_scope;

typedef struct ctex_paint_fill_triangle_topology {
    uint32_t triangle_identity;
    ctex_vec3d geometric_normal;
    const uint32_t* adjacent_triangles;
    size_t adjacent_triangle_count;
} ctex_paint_fill_triangle_topology;

typedef struct ctex_paint_fill_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t scope;
    uint32_t has_picked_texel;
    size_t picked_texel;
    double maximum_angle_degrees;
    const ctex_paint_surface_texel* surface_texels;
    size_t surface_texel_count;
    const uint8_t* coverage;
    size_t coverage_count;
    const uint32_t* triangle_identity;
    size_t triangle_identity_count;
    const uint32_t* uv_island_identity;
    size_t uv_island_identity_count;
    const ctex_paint_fill_triangle_topology* triangle_topology;
    size_t triangle_topology_count;
    const double* selection;
    size_t selection_count;
    const ctex_paint_tool_channel_descriptor* enabled_layer_snapshot;
    size_t enabled_layer_channel_count;
    const ctex_paint_tool_channel_descriptor* material;
    size_t material_channel_count;
    const ctex_paint_mask_inputs_descriptor* masks;
    const double* rejection_acceptance;
    size_t rejection_acceptance_count;
    const char* blend_mode;
} ctex_paint_fill_descriptor;

#define CTEX_PAINT_FILL_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_fill_descriptor))
#define CTEX_PAINT_FILL_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_fill_descriptor))

typedef struct ctex_paint_fill_info {
    uint32_t size;
    uint32_t scope;
    double resolved_maximum_angle_degrees;
    uint32_t maximum_angle_clamped;
    size_t selected_texel_count;
    size_t selected_triangle_count;
    size_t applied_channel_count;
    size_t required_pixels_per_channel;
} ctex_paint_fill_info;

#define CTEX_PAINT_FILL_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_fill_info))
#define CTEX_PAINT_FILL_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_fill_info))

typedef struct ctex_paint_fill_outputs {
    uint32_t size;
    double* scope_values;
    size_t scope_value_capacity;
    uint32_t* selected_triangle_ids;
    size_t selected_triangle_capacity;
    const ctex_paint_tool_channel_output* channels;
    size_t channel_count;
} ctex_paint_fill_outputs;

#define CTEX_PAINT_FILL_OUTPUTS_V1_SIZE ((uint32_t)sizeof(ctex_paint_fill_outputs))
#define CTEX_PAINT_FILL_OUTPUTS_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_fill_outputs))

typedef enum ctex_paint_clone_mode {
    CTEX_PAINT_CLONE_ALIGNED = 0,
    CTEX_PAINT_CLONE_FIXED = 1
} ctex_paint_clone_mode;

typedef struct ctex_paint_clone_source_descriptor {
    uint32_t size;
    const char* texture_set_id;
    ctex_vec2d uv;
} ctex_paint_clone_source_descriptor;

#define CTEX_PAINT_CLONE_SOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_clone_source_descriptor))
#define CTEX_PAINT_CLONE_SOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_clone_source_descriptor))

typedef struct ctex_paint_clone_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t mode;
    const char* destination_texture_set_id;
    ctex_vec2d tile_origin;
    ctex_vec2d destination_anchor_uv;
    const ctex_paint_clone_source_descriptor* source;
    const ctex_paint_surface_texel* destination_surface_texels;
    size_t destination_surface_texel_count;
    const uint8_t* destination_coverage;
    size_t destination_coverage_count;
    const ctex_paint_tool_channel_descriptor* enabled_layer_snapshot;
    size_t enabled_layer_channel_count;
    const ctex_paint_tool_channel_descriptor* source_snapshot;
    size_t source_channel_count;
    const ctex_paint_deposition_sample* deposition;
    size_t deposition_count;
    const char* blend_mode;
} ctex_paint_clone_descriptor;

#define CTEX_PAINT_CLONE_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_clone_descriptor))
#define CTEX_PAINT_CLONE_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_clone_descriptor))

typedef struct ctex_paint_clone_info {
    uint32_t size;
    uint32_t mode;
    ctex_vec2d source_anchor_uv;
    ctex_vec2d destination_anchor_uv;
    size_t required_source_sample_count;
    size_t applied_channel_count;
    size_t required_pixels_per_channel;
} ctex_paint_clone_info;

#define CTEX_PAINT_CLONE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_clone_info))
#define CTEX_PAINT_CLONE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_clone_info))

typedef struct ctex_paint_clone_outputs {
    uint32_t size;
    size_t* source_sample_indices;
    size_t source_sample_capacity;
    const ctex_paint_tool_channel_output* channels;
    size_t channel_count;
} ctex_paint_clone_outputs;

#define CTEX_PAINT_CLONE_OUTPUTS_V1_SIZE ((uint32_t)sizeof(ctex_paint_clone_outputs))
#define CTEX_PAINT_CLONE_OUTPUTS_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_clone_outputs))
#define CTEX_PAINT_NO_CLONE_SAMPLE ((size_t)-1)

typedef struct ctex_paint_blur_neighborhood_descriptor {
    uint32_t size;
    ctex_stroke_frame output_frame;
    const ctex_paint_surface_filter_sample* horizontal_samples;
    size_t horizontal_sample_count;
    const ctex_paint_surface_filter_sample* vertical_samples;
    size_t vertical_sample_count;
} ctex_paint_blur_neighborhood_descriptor;

#define CTEX_PAINT_BLUR_NEIGHBORHOOD_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_blur_neighborhood_descriptor))
#define CTEX_PAINT_BLUR_NEIGHBORHOOD_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_blur_neighborhood_descriptor))

typedef struct ctex_paint_blur_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t radius;
    const ctex_paint_tool_channel_descriptor* stroke_start_snapshot;
    size_t channel_count;
    const ctex_paint_deposition_sample* deposition;
    size_t deposition_count;
    const char* blend_mode;
    const ctex_paint_blur_neighborhood_descriptor* neighborhoods;
    size_t neighborhood_count;
} ctex_paint_blur_descriptor;

#define CTEX_PAINT_BLUR_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_blur_descriptor))
#define CTEX_PAINT_BLUR_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_blur_descriptor))

typedef struct ctex_paint_blur_info {
    uint32_t size;
    uint32_t resolved_radius;
    uint32_t radius_clamped;
    size_t applied_channel_count;
    size_t required_pixels_per_channel;
} ctex_paint_blur_info;

#define CTEX_PAINT_BLUR_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_blur_info))
#define CTEX_PAINT_BLUR_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_blur_info))

typedef struct ctex_paint_smear_mapping_descriptor {
    uint32_t size;
    ctex_stroke_frame output_frame;
    ctex_paint_surface_filter_sample upstream_sample;
} ctex_paint_smear_mapping_descriptor;

#define CTEX_PAINT_SMEAR_MAPPING_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_smear_mapping_descriptor))
#define CTEX_PAINT_SMEAR_MAPPING_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_smear_mapping_descriptor))

typedef struct ctex_paint_smear_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    double strength;
    uint32_t footprint_radius_x;
    uint32_t footprint_radius_y;
    const ctex_paint_tool_channel_descriptor* stroke_start_snapshot;
    size_t channel_count;
    const ctex_paint_deposition_sample* deposition;
    size_t deposition_count;
    const char* blend_mode;
    const ctex_paint_smear_mapping_descriptor* mappings;
    size_t mapping_count;
} ctex_paint_smear_descriptor;

#define CTEX_PAINT_SMEAR_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_smear_descriptor))
#define CTEX_PAINT_SMEAR_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_smear_descriptor))

typedef struct ctex_paint_smear_info {
    uint32_t size;
    double resolved_strength;
    uint32_t strength_clamped;
    uint32_t resolved_footprint_radius_x;
    uint32_t resolved_footprint_radius_y;
    uint32_t footprint_radius_x_clamped;
    uint32_t footprint_radius_y_clamped;
    size_t applied_channel_count;
    size_t required_pixels_per_channel;
} ctex_paint_smear_info;

#define CTEX_PAINT_SMEAR_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_smear_info))
#define CTEX_PAINT_SMEAR_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_smear_info))

typedef struct ctex_paint_stencil_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    const ctex_vec2d* screen_positions;
    size_t screen_position_count;
    uint32_t image_width;
    uint32_t image_height;
    const double* image_opacity;
    size_t image_opacity_count;
    ctex_vec2d position;
    double rotation_radians;
    ctex_vec2d scale;
    uint32_t inverted;
} ctex_paint_stencil_descriptor;

#define CTEX_PAINT_STENCIL_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_stencil_descriptor))
#define CTEX_PAINT_STENCIL_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_stencil_descriptor))

typedef struct ctex_paint_stencil_info {
    uint32_t size;
    ctex_vec2d resolved_position;
    double resolved_rotation_radians;
    ctex_vec2d resolved_scale;
    uint32_t inverted;
    uint32_t position_x_clamped;
    uint32_t position_y_clamped;
    uint32_t rotation_clamped;
    uint32_t scale_x_clamped;
    uint32_t scale_y_clamped;
    size_t required_mask_value_count;
} ctex_paint_stencil_info;

#define CTEX_PAINT_STENCIL_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_stencil_info))
#define CTEX_PAINT_STENCIL_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_stencil_info))

typedef struct ctex_paint_decal_transform {
    double rotation_radians;
    double uniform_scale;
    ctex_vec2d axis_scale;
} ctex_paint_decal_transform;

typedef struct ctex_paint_decal_placement {
    ctex_vec3d position;
    ctex_vec3d surface_normal;
    ctex_paint_decal_transform transform;
} ctex_paint_decal_placement;

typedef struct ctex_paint_decal_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    const ctex_paint_surface_texel* surface_texels;
    size_t surface_texel_count;
    const uint8_t* coverage;
    size_t coverage_count;
    ctex_paint_decal_placement placement;
    uint32_t material_width;
    uint32_t material_height;
    const ctex_paint_tool_channel_descriptor* material;
    size_t material_channel_count;
    const double* material_opacity;
    size_t material_opacity_count;
    const ctex_paint_tool_channel_descriptor* enabled_layer_snapshot;
    size_t enabled_layer_channel_count;
    const ctex_paint_mask_inputs_descriptor* masks;
    const double* rejection_acceptance;
    size_t rejection_acceptance_count;
    const char* blend_mode;
} ctex_paint_decal_descriptor;

#define CTEX_PAINT_DECAL_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_decal_descriptor))
#define CTEX_PAINT_DECAL_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_decal_descriptor))

typedef struct ctex_paint_decal_info {
    uint32_t size;
    ctex_paint_decal_placement resolved_placement;
    ctex_vec3d frame_tangent;
    ctex_vec3d frame_bitangent;
    ctex_vec2d frame_scale;
    uint32_t rotation_clamped;
    uint32_t uniform_scale_clamped;
    uint32_t axis_scale_x_clamped;
    uint32_t axis_scale_y_clamped;
    size_t applied_channel_count;
    size_t required_pixels_per_channel;
} ctex_paint_decal_info;

#define CTEX_PAINT_DECAL_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_decal_info))
#define CTEX_PAINT_DECAL_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_decal_info))

typedef struct ctex_paint_decal_outputs {
    uint32_t size;
    size_t* source_sample_indices;
    size_t source_sample_capacity;
    double* strength;
    size_t strength_capacity;
    const ctex_paint_tool_channel_output* channels;
    size_t channel_count;
} ctex_paint_decal_outputs;

#define CTEX_PAINT_DECAL_OUTPUTS_V1_SIZE ((uint32_t)sizeof(ctex_paint_decal_outputs))
#define CTEX_PAINT_DECAL_OUTPUTS_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_decal_outputs))
#define CTEX_PAINT_NO_DECAL_SAMPLE ((size_t)-1)

typedef enum ctex_paint_projection_mode {
    CTEX_PAINT_PROJECTION_CAMERA = 0,
    CTEX_PAINT_PROJECTION_PLANAR = 1,
    CTEX_PAINT_PROJECTION_TRIPLANAR = 2
} ctex_paint_projection_mode;

typedef struct ctex_paint_projection_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    const ctex_paint_surface_texel* surface_texels;
    size_t surface_texel_count;
    const uint8_t* coverage;
    size_t coverage_count;
    uint32_t mode;
    float camera_view_projection[16];
    const double* camera_visible_surface;
    size_t camera_visible_surface_count;
    ctex_vec3d planar_origin;
    ctex_vec3d planar_u_axis;
    ctex_vec3d planar_v_axis;
    ctex_vec2d planar_extent;
    double triplanar_scale;
    ctex_vec2d triplanar_offset;
    uint32_t material_width;
    uint32_t material_height;
    const ctex_paint_tool_channel_descriptor* material;
    size_t material_channel_count;
    const double* material_opacity;
    size_t material_opacity_count;
    const ctex_paint_tool_channel_descriptor* enabled_layer_snapshot;
    size_t enabled_layer_channel_count;
    const ctex_paint_mask_inputs_descriptor* masks;
    const double* rejection_acceptance;
    size_t rejection_acceptance_count;
    const char* blend_mode;
} ctex_paint_projection_descriptor;

#define CTEX_PAINT_PROJECTION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_projection_descriptor))
#define CTEX_PAINT_PROJECTION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_projection_descriptor))

typedef struct ctex_paint_projection_sample {
    size_t source_indices[3];
    double weights[3];
    size_t count;
} ctex_paint_projection_sample;

typedef struct ctex_paint_projection_info {
    uint32_t size;
    uint32_t resolved_mode;
    ctex_vec2d resolved_planar_extent;
    double resolved_triplanar_scale;
    ctex_vec2d resolved_triplanar_offset;
    uint32_t planar_extent_x_clamped;
    uint32_t planar_extent_y_clamped;
    uint32_t triplanar_scale_clamped;
    uint32_t triplanar_offset_x_clamped;
    uint32_t triplanar_offset_y_clamped;
    size_t applied_channel_count;
    size_t required_sample_count;
    size_t required_pixels_per_channel;
} ctex_paint_projection_info;

#define CTEX_PAINT_PROJECTION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_projection_info))
#define CTEX_PAINT_PROJECTION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_projection_info))

typedef struct ctex_paint_projection_outputs {
    uint32_t size;
    ctex_paint_projection_sample* samples;
    size_t sample_capacity;
    double* strength;
    size_t strength_capacity;
    const ctex_paint_tool_channel_output* channels;
    size_t channel_count;
} ctex_paint_projection_outputs;

#define CTEX_PAINT_PROJECTION_OUTPUTS_V1_SIZE ((uint32_t)sizeof(ctex_paint_projection_outputs))
#define CTEX_PAINT_PROJECTION_OUTPUTS_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_projection_outputs))
#define CTEX_PAINT_NO_PROJECTION_SAMPLE ((size_t)-1)

typedef enum ctex_paint_text_alignment {
    CTEX_PAINT_TEXT_ALIGN_LEFT = 0,
    CTEX_PAINT_TEXT_ALIGN_CENTRE = 1,
    CTEX_PAINT_TEXT_ALIGN_RIGHT = 2
} ctex_paint_text_alignment;

typedef struct ctex_paint_font_glyph_descriptor {
    uint32_t size;
    uint32_t codepoint;
    uint32_t width;
    uint32_t height;
    double bearing_x;
    double bearing_y;
    double advance;
    const double* coverage;
    size_t coverage_count;
} ctex_paint_font_glyph_descriptor;

#define CTEX_PAINT_FONT_GLYPH_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_font_glyph_descriptor))
#define CTEX_PAINT_FONT_GLYPH_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_font_glyph_descriptor))

typedef struct ctex_paint_font_descriptor {
    uint32_t size;
    const char* identity;
    double pixels_per_em;
    double ascent;
    double descent;
    double line_gap;
    const ctex_paint_font_glyph_descriptor* glyphs;
    size_t glyph_count;
} ctex_paint_font_descriptor;

#define CTEX_PAINT_FONT_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_font_descriptor))
#define CTEX_PAINT_FONT_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_font_descriptor))

typedef struct ctex_paint_text_material_value {
    uint32_t size;
    const char* semantic_id;
    uint32_t component_count;
    ctex_vec4f value;
} ctex_paint_text_material_value;

#define CTEX_PAINT_TEXT_MATERIAL_VALUE_V1_SIZE ((uint32_t)sizeof(ctex_paint_text_material_value))
#define CTEX_PAINT_TEXT_MATERIAL_VALUE_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_text_material_value))

typedef struct ctex_paint_text_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    const ctex_paint_surface_texel* surface_texels;
    size_t surface_texel_count;
    const uint8_t* coverage;
    size_t coverage_count;
    const ctex_paint_font_descriptor* font;
    const char* utf8;
    size_t utf8_size;
    double tracking_em;
    uint32_t alignment;
    double text_size;
    ctex_paint_decal_placement placement;
    const ctex_paint_text_material_value* material;
    size_t material_channel_count;
    const ctex_paint_tool_channel_descriptor* enabled_layer_snapshot;
    size_t enabled_layer_channel_count;
    const ctex_paint_mask_inputs_descriptor* masks;
    const double* rejection_acceptance;
    size_t rejection_acceptance_count;
    const char* blend_mode;
} ctex_paint_text_descriptor;

#define CTEX_PAINT_TEXT_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_text_descriptor))
#define CTEX_PAINT_TEXT_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_text_descriptor))

typedef struct ctex_paint_text_info {
    uint32_t size;
    double resolved_tracking_em;
    uint32_t resolved_alignment;
    double resolved_text_size;
    uint32_t tracking_clamped;
    uint32_t text_size_clamped;
    uint32_t raster_width;
    uint32_t raster_height;
    size_t line_count;
    double width_em;
    double height_em;
    ctex_paint_decal_placement resolved_placement;
    ctex_vec3d frame_tangent;
    ctex_vec3d frame_bitangent;
    ctex_vec2d frame_scale;
    uint32_t rotation_clamped;
    uint32_t uniform_scale_clamped;
    uint32_t axis_scale_x_clamped;
    uint32_t axis_scale_y_clamped;
    uint64_t editable_revision;
    size_t applied_channel_count;
    size_t required_codepoint_count;
    size_t required_raster_opacity_count;
    size_t required_pixels_per_channel;
} ctex_paint_text_info;

#define CTEX_PAINT_TEXT_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_text_info))
#define CTEX_PAINT_TEXT_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_text_info))

typedef struct ctex_paint_text_outputs {
    uint32_t size;
    uint32_t* codepoints;
    size_t codepoint_capacity;
    double* raster_opacity;
    size_t raster_opacity_capacity;
    size_t* source_sample_indices;
    size_t source_sample_capacity;
    double* strength;
    size_t strength_capacity;
    const ctex_paint_tool_channel_output* channels;
    size_t channel_count;
} ctex_paint_text_outputs;

#define CTEX_PAINT_TEXT_OUTPUTS_V1_SIZE ((uint32_t)sizeof(ctex_paint_text_outputs))
#define CTEX_PAINT_TEXT_OUTPUTS_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_text_outputs))

typedef struct ctex_paint_particle_settings {
    uint32_t count;
    double lifetime_seconds;
    double initial_speed;
    double mass;
    ctex_vec3d gravity;
    double friction;
    double restitution;
    double randomness;
    uint64_t seed;
} ctex_paint_particle_settings;

typedef struct ctex_paint_particle_contact {
    uint32_t particle_ordinal;
    uint32_t collision_ordinal;
    double time_seconds;
    ctex_vec3d position;
    ctex_vec3d normal;
    ctex_vec2d uv;
    uint32_t triangle;
    double impact_speed;
    double impulse;
    double strength;
    size_t texture_set_id_offset;
    size_t texture_set_id_size;
    size_t mapped_texel;
} ctex_paint_particle_contact;

typedef struct ctex_paint_particle_state {
    ctex_vec3d position;
    ctex_vec3d velocity;
    double simulated_seconds;
    uint32_t collision_count;
    uint32_t resting;
} ctex_paint_particle_state;

typedef struct ctex_paint_particle_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    ctex_vec2d tile_origin;
    const char* texture_set_id;
    uint64_t mesh_revision;
    const ctex_paint_surface_texel* surface_texels;
    size_t surface_texel_count;
    const uint8_t* coverage;
    size_t coverage_count;
    const uint32_t* triangle_identity;
    size_t triangle_identity_count;
    const struct ctex_pick_texture_set_binding_descriptor* texture_sets;
    size_t texture_set_count;
    ctex_vec3d emitter_position;
    ctex_vec3d emitter_direction;
    ctex_paint_particle_settings simulation;
    const ctex_paint_tool_channel_descriptor* material;
    size_t material_channel_count;
    const ctex_paint_tool_channel_descriptor* enabled_layer_snapshot;
    size_t enabled_layer_channel_count;
    const ctex_paint_mask_inputs_descriptor* masks;
    const double* rejection_acceptance;
    size_t rejection_acceptance_count;
    const char* blend_mode;
} ctex_paint_particle_descriptor;

#define CTEX_PAINT_PARTICLE_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_particle_descriptor))
#define CTEX_PAINT_PARTICLE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_particle_descriptor))

typedef struct ctex_paint_particle_info {
    uint32_t size;
    ctex_paint_particle_settings resolved_settings;
    uint32_t count_clamped;
    uint32_t lifetime_clamped;
    uint32_t initial_speed_clamped;
    uint32_t mass_clamped;
    uint32_t gravity_x_clamped;
    uint32_t gravity_y_clamped;
    uint32_t gravity_z_clamped;
    uint32_t friction_clamped;
    uint32_t restitution_clamped;
    uint32_t randomness_clamped;
    uint32_t emitted_count;
    size_t mapped_contact_count;
    size_t applied_channel_count;
    size_t required_contact_count;
    size_t required_final_state_count;
    size_t required_texture_set_id_size;
    size_t required_pixels_per_channel;
} ctex_paint_particle_info;

#define CTEX_PAINT_PARTICLE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_particle_info))
#define CTEX_PAINT_PARTICLE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_particle_info))

typedef struct ctex_paint_particle_outputs {
    uint32_t size;
    ctex_paint_particle_contact* contacts;
    size_t contact_capacity;
    ctex_paint_particle_state* final_states;
    size_t final_state_capacity;
    char* texture_set_ids;
    size_t texture_set_id_size;
    double* strength;
    size_t strength_capacity;
    const ctex_paint_tool_channel_output* channels;
    size_t channel_count;
} ctex_paint_particle_outputs;

#define CTEX_PAINT_PARTICLE_OUTPUTS_V1_SIZE ((uint32_t)sizeof(ctex_paint_particle_outputs))
#define CTEX_PAINT_PARTICLE_OUTPUTS_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_particle_outputs))
#define CTEX_PAINT_NO_PARTICLE_TEXEL ((size_t)-1)

typedef struct ctex_stroke_preset_info {
    uint32_t size;
    uint32_t schema_version;
    size_t required_name_size;
    size_t required_tip_resource_identity_size;
    size_t required_curve_point_count;
} ctex_stroke_preset_info;

#define CTEX_STROKE_PRESET_INFO_V1_SIZE ((uint32_t)sizeof(ctex_stroke_preset_info))
#define CTEX_STROKE_PRESET_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_stroke_preset_info))

typedef struct ctex_stroke_preset_buffers_descriptor {
    uint32_t size;
    char* name_buffer;
    size_t name_buffer_size;
    char* tip_resource_identity_buffer;
    size_t tip_resource_identity_buffer_size;
    ctex_response_curve_point* curve_points;
    size_t curve_point_capacity;
} ctex_stroke_preset_buffers_descriptor;

#define CTEX_STROKE_PRESET_BUFFERS_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_stroke_preset_buffers_descriptor))
#define CTEX_STROKE_PRESET_BUFFERS_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_stroke_preset_buffers_descriptor))

typedef struct ctex_uv_set_descriptor {
    uint32_t size;
    const char* name;
    const ctex_vec2f* values;
    size_t value_count;
} ctex_uv_set_descriptor;

#define CTEX_UV_SET_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_uv_set_descriptor))
#define CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_uv_set_descriptor))

typedef struct ctex_mesh_partition_descriptor {
    uint32_t size;
    uint32_t kind;
    const char* stable_key;
    const char* display_name;
} ctex_mesh_partition_descriptor;

#define CTEX_MESH_PARTITION_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_mesh_partition_descriptor))
#define CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_partition_descriptor))

typedef struct ctex_mesh_descriptor {
    uint32_t size;
    const ctex_vec3f* positions;
    size_t position_count;
    const ctex_vec3f* normals;
    size_t normal_count;
    const ctex_vec4f* vertex_colors;
    size_t vertex_color_count;
    const uint32_t* triangle_indices;
    size_t triangle_index_count;
    const ctex_uv_set_descriptor* uv_sets;
    size_t uv_set_count;
    const char* default_uv_set;
    const ctex_mesh_partition_descriptor* partitions;
    size_t partition_count;
    const uint32_t* face_partition_indices;
    size_t face_partition_index_count;
    const uint32_t* face_material_ids;
    size_t face_material_id_count;
} ctex_mesh_descriptor;

#define CTEX_MESH_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_mesh_descriptor))
#define CTEX_MESH_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_descriptor))

typedef struct ctex_mesh_info {
    uint32_t size;
    size_t vertex_count;
    size_t triangle_count;
    size_t uv_set_count;
    size_t partition_count;
    uint32_t has_vertex_colors;
    uint64_t revision;
} ctex_mesh_info;

#define CTEX_MESH_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_info))
#define CTEX_MESH_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_info))

typedef struct ctex_mesh_uv_overlap_info {
    uint32_t size;
    size_t required_face_count;
    size_t overlap_pair_count;
    size_t candidate_pair_count;
} ctex_mesh_uv_overlap_info;

#define CTEX_MESH_UV_OVERLAP_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_uv_overlap_info))
#define CTEX_MESH_UV_OVERLAP_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_uv_overlap_info))

typedef struct ctex_mesh_uv_coverage_info {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    size_t selected_face_count;
    size_t covered_texel_count;
    size_t uncovered_texel_count;
    size_t tested_texel_count;
    size_t required_outside_face_count;
    double uncovered_fraction;
} ctex_mesh_uv_coverage_info;

#define CTEX_MESH_UV_COVERAGE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_uv_coverage_info))
#define CTEX_MESH_UV_COVERAGE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_uv_coverage_info))

typedef enum ctex_mesh_uv_change {
    CTEX_MESH_UV_UNCHANGED = 0,
    CTEX_MESH_UV_CHANGED = 1,
    CTEX_MESH_SOURCE_PARTITION_MISSING = 2,
    CTEX_MESH_REPLACEMENT_PARTITION_MISSING = 3,
    CTEX_MESH_UV_SET_MISSING = 4
} ctex_mesh_uv_change;

typedef enum ctex_mesh_replacement_policy {
    CTEX_MESH_REPLACEMENT_KEEP_TEXELS = 0,
    CTEX_MESH_REPLACEMENT_REQUEST_REPROJECTION = 1,
    CTEX_MESH_REPLACEMENT_CLEAR = 2
} ctex_mesh_replacement_policy;

typedef struct ctex_mesh_replacement_entry {
    uint32_t uv_change;
    uint32_t source_partition_index;
    uint32_t replacement_partition_index;
    size_t source_face_count;
    size_t replacement_face_count;
    size_t texture_set_id_offset;
    size_t texture_set_id_size;
} ctex_mesh_replacement_entry;

#define CTEX_MESH_REPLACEMENT_NO_PARTITION UINT32_MAX

typedef struct ctex_mesh_replacement_plan_info {
    uint32_t size;
    uint64_t source_mesh_revision;
    size_t texture_set_count;
    size_t changed_texture_set_count;
    size_t required_texture_set_id_size;
} ctex_mesh_replacement_plan_info;

#define CTEX_MESH_REPLACEMENT_PLAN_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_replacement_plan_info))
#define CTEX_MESH_REPLACEMENT_PLAN_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_replacement_plan_info))

typedef struct ctex_mesh_replacement_decision {
    uint32_t size;
    const char* texture_set_id;
    uint32_t policy;
} ctex_mesh_replacement_decision;

#define CTEX_MESH_REPLACEMENT_DECISION_V1_SIZE ((uint32_t)sizeof(ctex_mesh_replacement_decision))
#define CTEX_MESH_REPLACEMENT_DECISION_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_replacement_decision))

typedef struct ctex_mesh_replacement_apply_info {
    uint32_t size;
    uint32_t replacement_applied;
    size_t kept_texture_set_count;
    size_t cleared_texture_set_count;
    size_t reprojection_pending_texture_set_count;
    uint64_t replacement_mesh_revision;
} ctex_mesh_replacement_apply_info;

#define CTEX_MESH_REPLACEMENT_APPLY_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_replacement_apply_info))
#define CTEX_MESH_REPLACEMENT_APPLY_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_replacement_apply_info))

typedef enum ctex_pick_occlusion_policy {
    CTEX_PICK_OCCLUSION_NEAREST = 0,
    CTEX_PICK_OCCLUSION_ALL_HITS = 1
} ctex_pick_occlusion_policy;

typedef enum ctex_pick_backface_policy {
    CTEX_PICK_BACKFACE_ACCEPT = 0,
    CTEX_PICK_BACKFACE_REJECT = 1
} ctex_pick_backface_policy;

typedef enum ctex_pick_projection_kind {
    CTEX_PICK_PROJECTION_PERSPECTIVE = 0,
    CTEX_PICK_PROJECTION_ORTHOGRAPHIC = 1
} ctex_pick_projection_kind;

typedef enum ctex_pick_batch_status {
    CTEX_PICK_BATCH_COMPLETE = 0,
    CTEX_PICK_BATCH_CANCELLED = 1,
    CTEX_PICK_BATCH_MEMORY_CEILING_EXCEEDED = 2
} ctex_pick_batch_status;

typedef struct ctex_pick_ray {
    ctex_vec3f origin;
    ctex_vec3f direction;
} ctex_pick_ray;

typedef struct ctex_pick_texture_set_binding_descriptor {
    uint32_t size;
    uint32_t partition_index;
    const char* uv_set;
} ctex_pick_texture_set_binding_descriptor;

#define CTEX_PICK_TEXTURE_SET_BINDING_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_pick_texture_set_binding_descriptor))
#define CTEX_PICK_TEXTURE_SET_BINDING_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_pick_texture_set_binding_descriptor))

typedef struct ctex_pick_options_descriptor {
    uint32_t size;
    float maximum_distance;
    uint32_t occlusion_policy;
    uint32_t backface_policy;
} ctex_pick_options_descriptor;

#define CTEX_PICK_OPTIONS_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_pick_options_descriptor))
#define CTEX_PICK_OPTIONS_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_pick_options_descriptor))

typedef struct ctex_pick_screen_view_descriptor {
    uint32_t size;
    uint32_t viewport_width;
    uint32_t viewport_height;
    float view[16];
    float projection[16];
} ctex_pick_screen_view_descriptor;

#define CTEX_PICK_SCREEN_VIEW_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_pick_screen_view_descriptor))
#define CTEX_PICK_SCREEN_VIEW_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_pick_screen_view_descriptor))

typedef struct ctex_pick_hit {
    uint32_t has_hit;
    ctex_vec3f position;
    ctex_vec3f interpolated_normal;
    ctex_vec3f geometric_normal;
    ctex_vec2f uv;
    int32_t udim_u;
    int32_t udim_v;
    int64_t udim_number;
    uint32_t triangle_index;
    ctex_vec3f barycentric;
    uint32_t material_id;
    float distance;
    size_t texture_set_id_offset;
    size_t texture_set_id_size;
} ctex_pick_hit;

typedef struct ctex_pick_index_info {
    uint32_t size;
    uint64_t mesh_revision;
    size_t build_count;
    size_t node_count;
    size_t triangle_count;
} ctex_pick_index_info;

#define CTEX_PICK_INDEX_INFO_V1_SIZE ((uint32_t)sizeof(ctex_pick_index_info))
#define CTEX_PICK_INDEX_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_pick_index_info))

typedef struct ctex_pick_query_info {
    uint32_t size;
    size_t result_count;
    size_t required_texture_set_id_size;
    size_t visited_nodes;
    size_t tested_leaf_triangles;
    size_t index_build_count;
    uint64_t mesh_revision;
} ctex_pick_query_info;

#define CTEX_PICK_QUERY_INFO_V1_SIZE ((uint32_t)sizeof(ctex_pick_query_info))
#define CTEX_PICK_QUERY_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_pick_query_info))

typedef struct ctex_paint_picker_texture_view_descriptor {
    uint32_t size;
    const char* texture_set_id;
    ctex_vec2d tile_origin;
    uint32_t width;
    uint32_t height;
    const ctex_paint_tool_channel_descriptor* enabled_channels;
    size_t enabled_channel_count;
    const char* const* material_identities;
    size_t material_identity_count;
} ctex_paint_picker_texture_view_descriptor;

#define CTEX_PAINT_PICKER_TEXTURE_VIEW_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_picker_texture_view_descriptor))
#define CTEX_PAINT_PICKER_TEXTURE_VIEW_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_picker_texture_view_descriptor))

typedef struct ctex_paint_picker_descriptor {
    uint32_t size;
    ctex_pick_hit hit;
    const char* hit_texture_set_id;
    const ctex_paint_picker_texture_view_descriptor* texture_views;
    size_t texture_view_count;
} ctex_paint_picker_descriptor;

#define CTEX_PAINT_PICKER_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_picker_descriptor))
#define CTEX_PAINT_PICKER_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_picker_descriptor))

typedef struct ctex_paint_picker_channel_value {
    uint32_t component_count;
    ctex_vec4f value;
    size_t semantic_id_offset;
    size_t semantic_id_size;
} ctex_paint_picker_channel_value;

typedef struct ctex_paint_picker_info {
    uint32_t size;
    ctex_vec2d tile_origin;
    ctex_vec2d uv;
    size_t texel;
    uint32_t has_material_identity;
    size_t texture_set_id_offset;
    size_t texture_set_id_size;
    size_t material_identity_offset;
    size_t material_identity_size;
    size_t required_channel_count;
    size_t required_string_size;
} ctex_paint_picker_info;

#define CTEX_PAINT_PICKER_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_picker_info))
#define CTEX_PAINT_PICKER_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_picker_info))

enum { CTEX_PAINT_COLOUR_ID_SELECTION_EMPTY = 0, CTEX_PAINT_COLOUR_ID_SELECTION_MATCHED = 1 };

typedef struct ctex_paint_colour_id_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    const ctex_vec4f* pixels;
    size_t pixel_count;
    ctex_vec4f picked_colour;
    double tolerance;
} ctex_paint_colour_id_descriptor;

#define CTEX_PAINT_COLOUR_ID_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_colour_id_descriptor))
#define CTEX_PAINT_COLOUR_ID_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_colour_id_descriptor))

typedef struct ctex_paint_colour_id_info {
    uint32_t size;
    double resolved_tolerance;
    uint32_t tolerance_clamped;
    uint32_t status;
    size_t selected_texel_count;
    size_t required_value_count;
} ctex_paint_colour_id_info;

#define CTEX_PAINT_COLOUR_ID_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_colour_id_info))
#define CTEX_PAINT_COLOUR_ID_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_colour_id_info))

typedef enum ctex_paint_parameter_context {
    CTEX_PAINT_PARAMETER_CONTEXT_GENERAL = 0,
    CTEX_PAINT_PARAMETER_CONTEXT_TAPER_DISABLED = 1,
    CTEX_PAINT_PARAMETER_CONTEXT_TAPER_STAMP_COUNT = 2,
    CTEX_PAINT_PARAMETER_CONTEXT_TAPER_DISTANCE = 3,
    CTEX_PAINT_PARAMETER_CONTEXT_ALPHA_UNORM8 = 4,
    CTEX_PAINT_PARAMETER_CONTEXT_ALPHA_HIGH_PRECISION = 5
} ctex_paint_parameter_context;

typedef enum ctex_paint_parameter_value_kind {
    CTEX_PAINT_PARAMETER_CONTINUOUS = 0,
    CTEX_PAINT_PARAMETER_INTEGER = 1
} ctex_paint_parameter_value_kind;

typedef struct ctex_paint_parameter_descriptor {
    uint32_t size;
    uint32_t context;
    uint32_t value_kind;
    double default_value;
    double minimum;
    double maximum;
    size_t name_offset;
    size_t name_size;
} ctex_paint_parameter_descriptor;

#define CTEX_PAINT_PARAMETER_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_paint_parameter_descriptor))
#define CTEX_PAINT_PARAMETER_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_parameter_descriptor))

typedef struct ctex_paint_parameter_catalogue_info {
    uint32_t size;
    size_t required_parameter_count;
    size_t required_name_size;
} ctex_paint_parameter_catalogue_info;

#define CTEX_PAINT_PARAMETER_CATALOGUE_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_parameter_catalogue_info))
#define CTEX_PAINT_PARAMETER_CATALOGUE_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_parameter_catalogue_info))

typedef struct ctex_paint_parameter_validation_info {
    uint32_t size;
    double supplied;
    double resolved;
    uint32_t clamped;
} ctex_paint_parameter_validation_info;

#define CTEX_PAINT_PARAMETER_VALIDATION_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_parameter_validation_info))
#define CTEX_PAINT_PARAMETER_VALIDATION_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_parameter_validation_info))

typedef enum ctex_paint_selection_kind {
    CTEX_PAINT_SELECTION_SCREEN_RECTANGLE = 0,
    CTEX_PAINT_SELECTION_SCREEN_LASSO = 1,
    CTEX_PAINT_SELECTION_POLYGON_TRIANGLE = 2,
    CTEX_PAINT_SELECTION_POLYGON_UV_ISLAND = 3,
    CTEX_PAINT_SELECTION_POLYGON_CONNECTED_BY_ANGLE = 4
} ctex_paint_selection_kind;

typedef struct ctex_paint_selection_surface_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    ctex_vec2d tile_origin;
    const char* texture_set_id;
    const char* uv_set;
    uint64_t mesh_revision;
    const ctex_paint_surface_texel* surface_texels;
    size_t surface_texel_count;
    const uint8_t* coverage;
    size_t coverage_count;
    const uint32_t* triangle_identity;
    size_t triangle_identity_count;
    const uint32_t* uv_island_identity;
    size_t uv_island_identity_count;
} ctex_paint_selection_surface_descriptor;

#define CTEX_PAINT_SELECTION_SURFACE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_selection_surface_descriptor))
#define CTEX_PAINT_SELECTION_SURFACE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_selection_surface_descriptor))

typedef struct ctex_paint_screen_selection_descriptor {
    uint32_t size;
    uint32_t kind;
    const ctex_paint_selection_surface_descriptor* surface;
    ctex_vec2f minimum;
    ctex_vec2f maximum;
    const ctex_vec2f* lasso_points;
    size_t lasso_point_count;
    ctex_pick_screen_view_descriptor view;
} ctex_paint_screen_selection_descriptor;

#define CTEX_PAINT_SCREEN_SELECTION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_screen_selection_descriptor))
#define CTEX_PAINT_SCREEN_SELECTION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_screen_selection_descriptor))

typedef struct ctex_paint_polygon_selection_descriptor {
    uint32_t size;
    uint32_t kind;
    const ctex_paint_selection_surface_descriptor* surface;
    size_t picked_texel;
    double maximum_angle_degrees;
    const ctex_paint_fill_triangle_topology* triangle_topology;
    size_t triangle_topology_count;
} ctex_paint_polygon_selection_descriptor;

#define CTEX_PAINT_POLYGON_SELECTION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_paint_polygon_selection_descriptor))
#define CTEX_PAINT_POLYGON_SELECTION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_paint_polygon_selection_descriptor))

typedef struct ctex_paint_selection_info {
    uint32_t size;
    uint32_t kind;
    double resolved_maximum_angle_degrees;
    uint32_t maximum_angle_clamped;
    size_t selected_texel_count;
    size_t selected_triangle_count;
    size_t visited_nodes;
    size_t tested_leaf_triangles;
    size_t required_value_count;
} ctex_paint_selection_info;

#define CTEX_PAINT_SELECTION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_paint_selection_info))
#define CTEX_PAINT_SELECTION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_selection_info))

typedef struct ctex_paint_selection_outputs {
    uint32_t size;
    double* values;
    size_t value_capacity;
    uint32_t* selected_triangle_ids;
    size_t selected_triangle_capacity;
} ctex_paint_selection_outputs;

#define CTEX_PAINT_SELECTION_OUTPUTS_V1_SIZE ((uint32_t)sizeof(ctex_paint_selection_outputs))
#define CTEX_PAINT_SELECTION_OUTPUTS_CURRENT_SIZE ((uint32_t)sizeof(ctex_paint_selection_outputs))

typedef uint32_t (*ctex_pick_cancel_callback)(void* user_data);
typedef void (*ctex_pick_progress_callback)(size_t completed_rays, size_t total_rays,
                                            void* user_data);

typedef struct ctex_pick_batch_control_descriptor {
    uint32_t size;
    size_t memory_ceiling_bytes;
    size_t progress_interval;
    void* user_data;
    ctex_pick_cancel_callback is_cancelled;
    ctex_pick_progress_callback report_progress;
} ctex_pick_batch_control_descriptor;

#define CTEX_PICK_BATCH_CONTROL_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_pick_batch_control_descriptor))
#define CTEX_PICK_BATCH_CONTROL_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_pick_batch_control_descriptor))

typedef struct ctex_pick_batch_info {
    uint32_t size;
    uint32_t status;
    size_t processed_rays;
    size_t required_memory_bytes;
    size_t required_hit_count;
    size_t required_texture_set_id_size;
    size_t visited_nodes;
    size_t tested_leaf_triangles;
} ctex_pick_batch_info;

#define CTEX_PICK_BATCH_INFO_V1_SIZE ((uint32_t)sizeof(ctex_pick_batch_info))
#define CTEX_PICK_BATCH_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_pick_batch_info))

typedef struct ctex_texture_set_descriptor {
    uint32_t size;
    const char* display_name;
    uint32_t partition_kind;
    const char* partition_key;
    const char* uv_set;
    uint32_t width;
    uint32_t height;
    uint8_t default_bit_depth;
    uint32_t udim_tiling;
} ctex_texture_set_descriptor;

#define CTEX_TEXTURE_SET_DESCRIPTOR_V1_SIZE \
    ((uint32_t)offsetof(ctex_texture_set_descriptor, default_bit_depth))
#define CTEX_TEXTURE_SET_DESCRIPTOR_V2_SIZE \
    ((uint32_t)offsetof(ctex_texture_set_descriptor, udim_tiling))
#define CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_texture_set_descriptor))

typedef struct ctex_udim_pixel_write_descriptor {
    uint32_t size;
    double u;
    double v;
    const void* pixel;
    size_t pixel_size;
} ctex_udim_pixel_write_descriptor;

#define CTEX_UDIM_PIXEL_WRITE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_udim_pixel_write_descriptor))
#define CTEX_UDIM_PIXEL_WRITE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_udim_pixel_write_descriptor))

typedef struct ctex_udim_write_info {
    uint32_t size;
    size_t changed_tile_count;
    size_t allocated_tile_count;
    size_t changed_pixel_count;
} ctex_udim_write_info;

#define CTEX_UDIM_WRITE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_udim_write_info))
#define CTEX_UDIM_WRITE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_udim_write_info))

typedef struct ctex_atlas_region_descriptor {
    uint32_t size;
    const char* texture_set_id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} ctex_atlas_region_descriptor;

#define CTEX_ATLAS_REGION_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_atlas_region_descriptor))
#define CTEX_ATLAS_REGION_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_atlas_region_descriptor))

typedef struct ctex_atlas_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    uint32_t width;
    uint32_t height;
    const ctex_atlas_region_descriptor* regions;
    size_t region_count;
} ctex_atlas_descriptor;

#define CTEX_ATLAS_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_atlas_descriptor))
#define CTEX_ATLAS_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_atlas_descriptor))

typedef struct ctex_atlas_region {
    size_t texture_set_id_offset;
    size_t texture_set_id_size;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} ctex_atlas_region;

typedef struct ctex_atlas_info {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    size_t region_count;
    size_t required_display_name_size;
    size_t required_texture_set_id_size;
} ctex_atlas_info;

#define CTEX_ATLAS_INFO_V1_SIZE ((uint32_t)sizeof(ctex_atlas_info))
#define CTEX_ATLAS_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_atlas_info))

typedef enum ctex_layer_entry_kind {
    CTEX_LAYER_ENTRY_PAINT = 0,
    CTEX_LAYER_ENTRY_FILL = 1,
    CTEX_LAYER_ENTRY_GROUP = 2,
    CTEX_LAYER_ENTRY_MASK = 3,
    CTEX_LAYER_ENTRY_FILTER = 4,
    CTEX_LAYER_ENTRY_INSTANCE = 5,
    CTEX_LAYER_ENTRY_EDITABLE_DECAL = 6,
    CTEX_LAYER_ENTRY_EDITABLE_TEXT = 7,
    CTEX_LAYER_ENTRY_SURFACE_PATH = 8
} ctex_layer_entry_kind;

typedef enum ctex_layer_source_deletion_policy {
    CTEX_LAYER_SOURCE_DELETION_REFUSE = 0,
    CTEX_LAYER_SOURCE_DELETION_MAKE_INSTANCES_INDEPENDENT = 1
} ctex_layer_source_deletion_policy;

typedef struct ctex_layer_channel_descriptor {
    uint32_t size;
    const char* semantic_id;
    uint32_t enabled;
    double opacity;
} ctex_layer_channel_descriptor;

#define CTEX_LAYER_CHANNEL_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_layer_channel_descriptor))
#define CTEX_LAYER_CHANNEL_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_layer_channel_descriptor))

typedef struct ctex_layer_entry_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    uint32_t kind;
    const char* parent_identifier;
    const char* target_identifier;
    const char* source_identifier;
    uint32_t enabled;
    double opacity;
    const char* blend_mode;
    const ctex_layer_channel_descriptor* channels;
    size_t channel_count;
} ctex_layer_entry_descriptor;

#define CTEX_LAYER_ENTRY_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_layer_entry_descriptor))
#define CTEX_LAYER_ENTRY_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_layer_entry_descriptor))

typedef struct ctex_layer_mask_sample {
    uint32_t size;
    const char* mask_identifier;
    double value;
} ctex_layer_mask_sample;

#define CTEX_LAYER_MASK_SAMPLE_V1_SIZE ((uint32_t)sizeof(ctex_layer_mask_sample))
#define CTEX_LAYER_MASK_SAMPLE_CURRENT_SIZE ((uint32_t)sizeof(ctex_layer_mask_sample))

typedef struct ctex_layer_participation_info {
    uint32_t size;
    uint32_t participates;
    double effective_opacity;
    size_t mask_count;
    size_t required_mask_id_size;
} ctex_layer_participation_info;

#define CTEX_LAYER_PARTICIPATION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_layer_participation_info))
#define CTEX_LAYER_PARTICIPATION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_layer_participation_info))

typedef struct ctex_tile_history_target_descriptor {
    uint32_t size;
    const char* semantic_id;
    uint32_t tile_x;
    uint32_t tile_y;
} ctex_tile_history_target_descriptor;

#define CTEX_TILE_HISTORY_TARGET_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_tile_history_target_descriptor))
#define CTEX_TILE_HISTORY_TARGET_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_tile_history_target_descriptor))

typedef struct ctex_tile_history_budget_report {
    uint32_t size;
    size_t budget_bytes;
    size_t retained_bytes;
    size_t available_bytes;
    size_t proposed_step_bytes;
    size_t additional_steps_at_proposed_size;
    size_t undo_steps;
    size_t redo_steps;
} ctex_tile_history_budget_report;

#define CTEX_TILE_HISTORY_BUDGET_REPORT_V1_SIZE ((uint32_t)sizeof(ctex_tile_history_budget_report))
#define CTEX_TILE_HISTORY_BUDGET_REPORT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_tile_history_budget_report))

typedef struct ctex_tile_history_commit_info {
    uint32_t size;
    uint32_t committed;
    size_t tile_count;
    size_t retained_bytes;
    uint32_t layer_stack_changed;
} ctex_tile_history_commit_info;

#define CTEX_TILE_HISTORY_COMMIT_INFO_V1_SIZE ((uint32_t)sizeof(ctex_tile_history_commit_info))
#define CTEX_TILE_HISTORY_COMMIT_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_tile_history_commit_info))

typedef struct ctex_tile_history_restore_info {
    uint32_t size;
    size_t tile_count;
    size_t exchanged_storage_count;
    size_t copied_pixel_bytes;
    uint32_t layer_stack_exchanged;
} ctex_tile_history_restore_info;

#define CTEX_TILE_HISTORY_RESTORE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_tile_history_restore_info))
#define CTEX_TILE_HISTORY_RESTORE_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_tile_history_restore_info))

typedef struct ctex_layer_composite_raster_descriptor {
    uint32_t size;
    const char* entry_identifier;
    const char* semantic_id;
    uint32_t width;
    uint32_t height;
    const ctex_vec4f* pixels;
    size_t pixel_count;
    const float* coverage;
    size_t coverage_count;
} ctex_layer_composite_raster_descriptor;

#define CTEX_LAYER_COMPOSITE_RASTER_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_layer_composite_raster_descriptor))
#define CTEX_LAYER_COMPOSITE_RASTER_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_layer_composite_raster_descriptor))

typedef struct ctex_layer_composite_mask_descriptor {
    uint32_t size;
    const char* mask_identifier;
    uint32_t width;
    uint32_t height;
    const double* values;
    size_t value_count;
} ctex_layer_composite_mask_descriptor;

#define CTEX_LAYER_COMPOSITE_MASK_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_layer_composite_mask_descriptor))
#define CTEX_LAYER_COMPOSITE_MASK_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_layer_composite_mask_descriptor))

typedef struct ctex_layer_composite_info {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    size_t channel_count;
    size_t required_semantic_id_size;
    size_t required_pixel_count;
} ctex_layer_composite_info;

#define CTEX_LAYER_COMPOSITE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_layer_composite_info))
#define CTEX_LAYER_COMPOSITE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_layer_composite_info))

typedef struct ctex_layer_composite_channel_info {
    uint32_t component_count;
    size_t semantic_id_offset;
    size_t semantic_id_size;
    size_t pixel_offset;
    size_t pixel_count;
} ctex_layer_composite_channel_info;

typedef struct ctex_layer_snapshot_info {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    size_t content_count;
    size_t mask_count;
    size_t required_string_size;
    size_t required_pixel_count;
    size_t required_coverage_count;
    size_t required_mask_value_count;
} ctex_layer_snapshot_info;

#define CTEX_LAYER_SNAPSHOT_INFO_V1_SIZE ((uint32_t)sizeof(ctex_layer_snapshot_info))
#define CTEX_LAYER_SNAPSHOT_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_layer_snapshot_info))

typedef struct ctex_layer_snapshot_content_info {
    uint32_t width;
    uint32_t height;
    size_t entry_identifier_offset;
    size_t entry_identifier_size;
    size_t semantic_id_offset;
    size_t semantic_id_size;
    size_t pixel_offset;
    size_t pixel_count;
    size_t coverage_offset;
    size_t coverage_count;
} ctex_layer_snapshot_content_info;

typedef struct ctex_layer_snapshot_mask_info {
    uint32_t width;
    uint32_t height;
    size_t mask_identifier_offset;
    size_t mask_identifier_size;
    size_t value_offset;
    size_t value_count;
} ctex_layer_snapshot_mask_info;

typedef enum ctex_layer_operation_kind {
    CTEX_LAYER_OPERATION_CREATE = 0,
    CTEX_LAYER_OPERATION_DUPLICATE = 1,
    CTEX_LAYER_OPERATION_DELETE = 2,
    CTEX_LAYER_OPERATION_REORDER = 3,
    CTEX_LAYER_OPERATION_REPARENT = 4,
    CTEX_LAYER_OPERATION_CLEAR = 5,
    CTEX_LAYER_OPERATION_INVERT = 6,
    CTEX_LAYER_OPERATION_MERGE_DOWN = 7,
    CTEX_LAYER_OPERATION_MERGE_GROUP = 8,
    CTEX_LAYER_OPERATION_FLATTEN = 9,
    CTEX_LAYER_OPERATION_CONVERT = 10,
    CTEX_LAYER_OPERATION_APPLY_MASK = 11
} ctex_layer_operation_kind;

typedef struct ctex_layer_operation_descriptor {
    uint32_t size;
    uint32_t kind;
    const char* identifier;
    const char* duplicate_identifier;
    const char* parent_identifier;
    const char* before_identifier;
    const char* semantic_id;
    const ctex_layer_entry_descriptor* entry;
    const ctex_layer_composite_raster_descriptor* replacement_content;
    size_t replacement_content_count;
    const ctex_layer_composite_mask_descriptor* replacement_masks;
    size_t replacement_mask_count;
    uint32_t source_deletion_policy;
    uint32_t target_kind;
    const void* graph_serialized;
    size_t graph_serialized_size;
    size_t maximum_output_bytes;
    float appearance_tolerance;
} ctex_layer_operation_descriptor;

#define CTEX_LAYER_OPERATION_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_layer_operation_descriptor))
#define CTEX_LAYER_OPERATION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_layer_operation_descriptor))

typedef struct ctex_layer_operation_info {
    uint32_t size;
    size_t affected_count;
    size_t required_affected_id_size;
} ctex_layer_operation_info;

#define CTEX_LAYER_OPERATION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_layer_operation_info))
#define CTEX_LAYER_OPERATION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_layer_operation_info))

typedef enum ctex_scalar_representation {
    CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED = 0,
    CTEX_SCALAR_REPRESENTATION_FLOATING_POINT = 1
} ctex_scalar_representation;

typedef enum ctex_image_file_format {
    CTEX_IMAGE_FILE_FORMAT_UNKNOWN = 0,
    CTEX_IMAGE_FILE_FORMAT_PNG = 1,
    CTEX_IMAGE_FILE_FORMAT_JPEG = 2,
    CTEX_IMAGE_FILE_FORMAT_BMP = 3,
    CTEX_IMAGE_FILE_FORMAT_TIFF = 4,
    CTEX_IMAGE_FILE_FORMAT_OPENEXR = 5,
    CTEX_IMAGE_FILE_FORMAT_RADIANCE_HDR = 6,
    CTEX_IMAGE_FILE_FORMAT_PSD = 7,
    CTEX_IMAGE_FILE_FORMAT_TGA = 8
} ctex_image_file_format;

typedef enum ctex_color_space_source {
    CTEX_COLOR_SPACE_SOURCE_CALLER = 0,
    CTEX_COLOR_SPACE_SOURCE_EMBEDDED_SRGB = 1,
    CTEX_COLOR_SPACE_SOURCE_AUTOMATIC_RULE = 2
} ctex_color_space_source;

typedef struct ctex_image_decode_limits_descriptor {
    uint32_t size;
    uint32_t maximum_width;
    uint32_t maximum_height;
    size_t maximum_decoded_bytes;
} ctex_image_decode_limits_descriptor;

#define CTEX_IMAGE_DECODE_LIMITS_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_image_decode_limits_descriptor))
#define CTEX_IMAGE_DECODE_LIMITS_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_image_decode_limits_descriptor))

typedef struct ctex_decoded_image_info {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t channel_count;
    uint32_t scalar_representation;
    uint32_t bit_depth;
    uint32_t color_space;
    uint32_t detected_format;
    uint32_t extension_mismatch;
    uint32_t color_space_source;
    uint32_t uninterpretable_profile;
} ctex_decoded_image_info;

#define CTEX_DECODED_IMAGE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_decoded_image_info))
#define CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_decoded_image_info))

typedef enum ctex_image_channel_expansion_rule {
    CTEX_IMAGE_CHANNEL_EXPANSION_IDENTITY = 0,
    CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_TO_RGB = 1,
    CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_TO_RGBA = 2,
    CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_ALPHA_TO_RGBA = 3,
    CTEX_IMAGE_CHANNEL_EXPANSION_RGB_TO_RGBA = 4
} ctex_image_channel_expansion_rule;

typedef struct ctex_image_channel_expansion_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t source_channel_count;
    uint32_t scalar_representation;
    uint32_t bit_depth;
    size_t source_row_stride_bytes;
    uint32_t target_channel_count;
} ctex_image_channel_expansion_descriptor;

#define CTEX_IMAGE_CHANNEL_EXPANSION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_image_channel_expansion_descriptor))
#define CTEX_IMAGE_CHANNEL_EXPANSION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_image_channel_expansion_descriptor))

typedef struct ctex_image_channel_expansion_info {
    uint32_t size;
    uint32_t channel_count;
    uint32_t scalar_representation;
    uint32_t bit_depth;
    uint32_t rule;
    size_t required_pixel_buffer_size;
} ctex_image_channel_expansion_info;

#define CTEX_IMAGE_CHANNEL_EXPANSION_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_image_channel_expansion_info))
#define CTEX_IMAGE_CHANNEL_EXPANSION_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_image_channel_expansion_info))

typedef enum ctex_image_resample_filter {
    CTEX_IMAGE_RESAMPLE_FILTER_DEFAULT = 0,
    CTEX_IMAGE_RESAMPLE_FILTER_NEAREST = 1,
    CTEX_IMAGE_RESAMPLE_FILTER_BILINEAR = 2
} ctex_image_resample_filter;

typedef struct ctex_image_resample_descriptor {
    uint32_t size;
    uint32_t source_width;
    uint32_t source_height;
    uint32_t channel_count;
    uint32_t scalar_representation;
    uint32_t bit_depth;
    size_t source_row_stride_bytes;
    uint32_t output_width;
    uint32_t output_height;
    uint32_t filter;
    size_t maximum_output_bytes;
} ctex_image_resample_descriptor;

#define CTEX_IMAGE_RESAMPLE_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_image_resample_descriptor))
#define CTEX_IMAGE_RESAMPLE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_image_resample_descriptor))

typedef struct ctex_image_resample_info {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t channel_count;
    uint32_t scalar_representation;
    uint32_t bit_depth;
    uint32_t filter;
    size_t required_pixel_buffer_size;
} ctex_image_resample_info;

#define CTEX_IMAGE_RESAMPLE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_image_resample_info))
#define CTEX_IMAGE_RESAMPLE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_image_resample_info))

typedef struct ctex_image_encode_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t channel_count;
    uint32_t scalar_representation;
    uint32_t input_bit_depth;
    size_t row_stride_bytes;
    uint32_t color_space;
    uint32_t output_format;
    uint32_t output_bit_depth;
    uint32_t jpeg_quality;
} ctex_image_encode_descriptor;

#define CTEX_IMAGE_ENCODE_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_image_encode_descriptor))
#define CTEX_IMAGE_ENCODE_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_image_encode_descriptor))

typedef enum ctex_channel_classification {
    CTEX_CHANNEL_CLASSIFICATION_COLOR = 0,
    CTEX_CHANNEL_CLASSIFICATION_DATA = 1
} ctex_channel_classification;

typedef enum ctex_blending_policy {
    CTEX_BLENDING_POLICY_COLOR = 0,
    CTEX_BLENDING_POLICY_SCALAR = 1,
    CTEX_BLENDING_POLICY_NORMAL_VECTOR = 2,
    CTEX_BLENDING_POLICY_ADDITIVE = 3
} ctex_blending_policy;

typedef struct ctex_channel_descriptor {
    uint32_t size;
    const char* semantic_id;
    uint32_t component_count;
    uint32_t scalar_representation;
    uint32_t preferred_bit_depth;
    double default_value[4];
    uint32_t default_value_count;
    uint32_t classification;
    uint32_t blending_policy;
    const char* export_mapping;
    uint32_t evaluable;
} ctex_channel_descriptor;

#define CTEX_CHANNEL_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_channel_descriptor))
#define CTEX_CHANNEL_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_channel_descriptor))

typedef struct ctex_channel_info {
    uint32_t size;
    uint32_t component_count;
    uint32_t scalar_representation;
    uint32_t preferred_bit_depth;
    double default_value[4];
    uint32_t default_value_count;
    uint32_t classification;
    uint32_t blending_policy;
    uint32_t evaluable;
    uint32_t enabled;
    uint32_t storage_bit_depth;
} ctex_channel_info;

#define CTEX_CHANNEL_INFO_V1_SIZE ((uint32_t)sizeof(ctex_channel_info))
#define CTEX_CHANNEL_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_channel_info))

typedef struct ctex_texture_set_memory_report {
    uint32_t size;
    size_t enabled_channel_count;
    size_t channel_pixel_bytes;
    size_t mesh_map_pixel_bytes;
    size_t total_resident_bytes;
} ctex_texture_set_memory_report;

#define CTEX_TEXTURE_SET_MEMORY_REPORT_V1_SIZE ((uint32_t)sizeof(ctex_texture_set_memory_report))
#define CTEX_TEXTURE_SET_MEMORY_REPORT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_set_memory_report))

typedef struct ctex_document_texture_set_memory_info {
    size_t texture_set_id_offset;
    size_t texture_set_id_size;
    size_t channel_pixel_bytes;
    size_t history_retained_bytes;
    size_t mesh_map_pixel_bytes;
    size_t total_resident_bytes;
    size_t estimated_save_bytes;
} ctex_document_texture_set_memory_info;

typedef struct ctex_document_memory_info {
    uint32_t size;
    size_t texture_set_count;
    size_t required_texture_set_id_size;
    size_t channel_pixel_bytes;
    size_t history_retained_bytes;
    size_t mesh_map_pixel_bytes;
    size_t total_resident_bytes;
    size_t estimated_save_bytes;
} ctex_document_memory_info;

#define CTEX_DOCUMENT_MEMORY_INFO_V1_SIZE ((uint32_t)sizeof(ctex_document_memory_info))
#define CTEX_DOCUMENT_MEMORY_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_document_memory_info))

typedef struct ctex_transport_revision_cursor {
    uint64_t epoch;
    uint64_t revision;
} ctex_transport_revision_cursor;

typedef enum ctex_transport_delta_disposition {
    CTEX_TRANSPORT_DELTA_COMPLETE = 0,
    CTEX_TRANSPORT_FULL_RESYNCHRONIZATION_REQUIRED = 1
} ctex_transport_delta_disposition;

typedef enum ctex_transport_tile_residency {
    CTEX_TRANSPORT_TILE_CPU = 0,
    CTEX_TRANSPORT_TILE_HOST_DEVICE = 1
} ctex_transport_tile_residency;

typedef struct ctex_transport_tile_version {
    uint32_t x;
    uint32_t y;
    uint64_t revision;
    uint64_t generation;
    uint32_t residency;
} ctex_transport_tile_version;

typedef struct ctex_transport_delta_info {
    uint32_t size;
    uint32_t disposition;
    ctex_transport_revision_cursor synchronized_cursor;
    ctex_transport_revision_cursor current_cursor;
    size_t changed_tile_count;
    size_t indexed_tiles_visited;
} ctex_transport_delta_info;

#define CTEX_TRANSPORT_DELTA_INFO_V1_SIZE ((uint32_t)sizeof(ctex_transport_delta_info))
#define CTEX_TRANSPORT_DELTA_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_transport_delta_info))

typedef enum ctex_transport_component_type {
    CTEX_TRANSPORT_COMPONENT_UINT8_UNORM = 0,
    CTEX_TRANSPORT_COMPONENT_UINT16_UNORM = 1,
    CTEX_TRANSPORT_COMPONENT_FLOAT32 = 2
} ctex_transport_component_type;

typedef struct ctex_transport_pixel_format {
    uint32_t component_type;
    uint32_t channel_count;
} ctex_transport_pixel_format;

typedef enum ctex_transport_conversion_policy {
    CTEX_TRANSPORT_EXACT_FORMAT_ONLY = 0,
    CTEX_TRANSPORT_ALLOW_FORMAT_CONVERSION = 1
} ctex_transport_conversion_policy;

typedef enum ctex_transport_format_conversion {
    CTEX_TRANSPORT_CONVERSION_NONE = 0,
    CTEX_TRANSPORT_CONVERSION_UINT8_TO_UINT16 = 1,
    CTEX_TRANSPORT_CONVERSION_UINT8_TO_FLOAT32 = 2,
    CTEX_TRANSPORT_CONVERSION_UINT16_TO_UINT8 = 3,
    CTEX_TRANSPORT_CONVERSION_UINT16_TO_FLOAT32 = 4,
    CTEX_TRANSPORT_CONVERSION_FLOAT32_TO_UINT8 = 5,
    CTEX_TRANSPORT_CONVERSION_FLOAT32_TO_UINT16 = 6
} ctex_transport_format_conversion;

typedef struct ctex_transport_format_selection {
    uint32_t size;
    ctex_transport_pixel_format source_format;
    ctex_transport_pixel_format output_format;
    uint32_t conversion;
} ctex_transport_format_selection;

#define CTEX_TRANSPORT_FORMAT_SELECTION_V1_SIZE ((uint32_t)sizeof(ctex_transport_format_selection))
#define CTEX_TRANSPORT_FORMAT_SELECTION_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_transport_format_selection))

typedef struct ctex_transport_snapshot_query_info {
    uint32_t size;
    uint32_t disposition;
    ctex_transport_revision_cursor synchronized_cursor;
    ctex_transport_revision_cursor current_cursor;
    size_t changed_tile_count;
    size_t indexed_tiles_visited;
    size_t retained_bytes;
    size_t additional_pinned_bytes;
} ctex_transport_snapshot_query_info;

#define CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_transport_snapshot_query_info))
#define CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_transport_snapshot_query_info))

typedef struct ctex_transport_snapshot_memory_report {
    uint32_t size;
    size_t budget_bytes;
    size_t pinned_bytes;
    size_t active_snapshots;
    size_t pinned_allocations;
} ctex_transport_snapshot_memory_report;

#define CTEX_TRANSPORT_SNAPSHOT_MEMORY_REPORT_V1_SIZE \
    ((uint32_t)sizeof(ctex_transport_snapshot_memory_report))
#define CTEX_TRANSPORT_SNAPSHOT_MEMORY_REPORT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_transport_snapshot_memory_report))

typedef enum ctex_resource_category {
    CTEX_RESOURCE_DOCUMENT_STORAGE = 0,
    CTEX_RESOURCE_HISTORY = 1,
    CTEX_RESOURCE_RECOVERY_RECORD = 2,
    CTEX_RESOURCE_MESH_MAP = 3,
    CTEX_RESOURCE_COMPOSITE = 4,
    CTEX_RESOURCE_CACHE = 5,
    CTEX_RESOURCE_TEMPORARY = 6,
    CTEX_RESOURCE_CATEGORY_COUNT = 7
} ctex_resource_category;

typedef enum ctex_resource_role {
    CTEX_RESOURCE_CPU_RESIDENT = 1,
    CTEX_RESOURCE_GPU_RESIDENT = 2,
    CTEX_RESOURCE_BACKING_STORE = 4,
    CTEX_RESOURCE_PINNED = 8,
    CTEX_RESOURCE_IN_FLIGHT = 16
} ctex_resource_role;

typedef void (*ctex_resource_cache_eviction_callback)(uint64_t allocation_identity,
                                                      void* user_data);

typedef struct ctex_resource_allocation_descriptor {
    uint32_t size;
    uint64_t allocation_identity;
    uint32_t category;
    size_t physical_bytes;
    uint32_t roles;
    const char* device_backend;
    const char* device_identifier;
    const char* heap_identifier;
} ctex_resource_allocation_descriptor;

#define CTEX_RESOURCE_ALLOCATION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_resource_allocation_descriptor))
#define CTEX_RESOURCE_ALLOCATION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_resource_allocation_descriptor))

typedef struct ctex_resource_category_report {
    uint32_t size;
    uint32_t category;
    size_t allocation_count;
    size_t physical_bytes;
    size_t cpu_resident_bytes;
    size_t gpu_resident_bytes;
    size_t backing_store_bytes;
    size_t pinned_bytes;
    size_t in_flight_bytes;
} ctex_resource_category_report;

#define CTEX_RESOURCE_CATEGORY_REPORT_V1_SIZE ((uint32_t)sizeof(ctex_resource_category_report))
#define CTEX_RESOURCE_CATEGORY_REPORT_CURRENT_SIZE ((uint32_t)sizeof(ctex_resource_category_report))

typedef struct ctex_resource_accounting_report {
    uint32_t size;
    size_t allocation_count;
    size_t physical_bytes;
    size_t cpu_resident_bytes;
    size_t gpu_resident_bytes;
    size_t backing_store_bytes;
    size_t pinned_bytes;
    size_t in_flight_bytes;
    size_t category_count;
} ctex_resource_accounting_report;

#define CTEX_RESOURCE_ACCOUNTING_REPORT_V1_SIZE ((uint32_t)sizeof(ctex_resource_accounting_report))
#define CTEX_RESOURCE_ACCOUNTING_REPORT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_resource_accounting_report))

typedef struct ctex_resource_budget_limits {
    uint32_t size;
    size_t cpu_bytes;
    size_t gpu_bytes;
    size_t backing_store_bytes;
    size_t temporary_bytes;
} ctex_resource_budget_limits;

#define CTEX_RESOURCE_BUDGET_LIMITS_V1_SIZE ((uint32_t)sizeof(ctex_resource_budget_limits))
#define CTEX_RESOURCE_BUDGET_LIMITS_CURRENT_SIZE ((uint32_t)sizeof(ctex_resource_budget_limits))

typedef struct ctex_resource_requirement {
    uint32_t size;
    uint32_t category;
    size_t physical_bytes;
    uint32_t roles;
} ctex_resource_requirement;

#define CTEX_RESOURCE_REQUIREMENT_V1_SIZE ((uint32_t)sizeof(ctex_resource_requirement))
#define CTEX_RESOURCE_REQUIREMENT_CURRENT_SIZE ((uint32_t)sizeof(ctex_resource_requirement))

typedef struct ctex_resource_admission_descriptor {
    uint32_t size;
    const char* operation;
    ctex_resource_budget_limits limits;
    const ctex_resource_requirement* fixed_requirements;
    size_t fixed_requirement_count;
    ctex_resource_requirement per_work_item;
    size_t work_item_count;
} ctex_resource_admission_descriptor;

#define CTEX_RESOURCE_ADMISSION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_resource_admission_descriptor))
#define CTEX_RESOURCE_ADMISSION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_resource_admission_descriptor))

typedef enum ctex_resource_admission_status {
    CTEX_RESOURCE_ADMITTED_WHOLE = 0,
    CTEX_RESOURCE_ADMITTED_TILED = 1,
    CTEX_RESOURCE_OVER_BUDGET = 2,
    CTEX_RESOURCE_QUIESCING = 3
} ctex_resource_admission_status;

typedef struct ctex_resource_admission_report {
    uint32_t size;
    uint32_t status;
    size_t work_item_count;
    size_t admitted_work_items;
    size_t projected_cpu_bytes;
    size_t projected_gpu_bytes;
    size_t projected_backing_store_bytes;
    size_t projected_temporary_bytes;
    size_t evicted_allocation_count;
} ctex_resource_admission_report;

#define CTEX_RESOURCE_ADMISSION_REPORT_V1_SIZE ((uint32_t)sizeof(ctex_resource_admission_report))
#define CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_resource_admission_report))

typedef struct ctex_preview_quality_option {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    const ctex_resource_requirement* requirements;
    size_t requirement_count;
    uint32_t derived_work_deferred;
} ctex_preview_quality_option;

#define CTEX_PREVIEW_QUALITY_OPTION_V1_SIZE ((uint32_t)sizeof(ctex_preview_quality_option))
#define CTEX_PREVIEW_QUALITY_OPTION_CURRENT_SIZE ((uint32_t)sizeof(ctex_preview_quality_option))

typedef struct ctex_preview_quality_admission_descriptor {
    uint32_t size;
    const char* operation;
    ctex_resource_budget_limits limits;
    uint32_t full_quality_width;
    uint32_t full_quality_height;
    const ctex_preview_quality_option* options;
    size_t option_count;
} ctex_preview_quality_admission_descriptor;

#define CTEX_PREVIEW_QUALITY_ADMISSION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_preview_quality_admission_descriptor))
#define CTEX_PREVIEW_QUALITY_ADMISSION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_preview_quality_admission_descriptor))

typedef enum ctex_preview_quality_status {
    CTEX_PREVIEW_FULL_QUALITY = 0,
    CTEX_PREVIEW_REDUCED_RESOLUTION = 1,
    CTEX_PREVIEW_DEFERRED_DERIVED = 2,
    CTEX_PREVIEW_REDUCED_AND_DEFERRED = 3,
    CTEX_PREVIEW_OVER_BUDGET = 4,
    CTEX_PREVIEW_QUIESCING = 5
} ctex_preview_quality_status;

typedef struct ctex_preview_quality_admission_report {
    uint32_t size;
    uint32_t status;
    size_t selected_option;
    uint32_t full_quality_width;
    uint32_t full_quality_height;
    uint32_t selected_width;
    uint32_t selected_height;
    uint32_t derived_work_deferred;
    size_t projected_cpu_bytes;
    size_t projected_gpu_bytes;
    size_t projected_backing_store_bytes;
    size_t projected_temporary_bytes;
    size_t evicted_allocation_count;
} ctex_preview_quality_admission_report;

#define CTEX_PREVIEW_QUALITY_ADMISSION_REPORT_V1_SIZE \
    ((uint32_t)sizeof(ctex_preview_quality_admission_report))
#define CTEX_PREVIEW_QUALITY_ADMISSION_REPORT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_preview_quality_admission_report))

typedef struct ctex_tile_backing_key {
    uint64_t namespace_identity;
    uint32_t tile_x;
    uint32_t tile_y;
    uint64_t generation;
} ctex_tile_backing_key;

typedef uint32_t (*ctex_tile_backing_store_callback)(ctex_tile_backing_key key, const void* bytes,
                                                     size_t byte_count, void* user_data);
typedef uint32_t (*ctex_tile_backing_load_callback)(ctex_tile_backing_key key, void* bytes,
                                                    size_t byte_count, void* user_data);
typedef void (*ctex_tile_backing_discard_callback)(ctex_tile_backing_key key, void* user_data);
typedef void (*ctex_tile_backing_release_callback)(uint64_t namespace_identity, void* user_data);

typedef struct ctex_tile_backing_store_descriptor {
    uint32_t size;
    ctex_tile_backing_store_callback store;
    ctex_tile_backing_load_callback load;
    ctex_tile_backing_discard_callback discard;
    ctex_tile_backing_release_callback release_namespace;
    void* user_data;
} ctex_tile_backing_store_descriptor;

#define CTEX_TILE_BACKING_STORE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_tile_backing_store_descriptor))
#define CTEX_TILE_BACKING_STORE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_tile_backing_store_descriptor))

typedef enum ctex_tile_eviction_status {
    CTEX_TILE_EVICTED = 0,
    CTEX_TILE_SPARSE = 1,
    CTEX_TILE_ALREADY_EVICTED = 2,
    CTEX_TILE_PINNED = 3,
    CTEX_TILE_NO_BACKING_STORE = 4,
    CTEX_TILE_BACKING_STORE_FAILED = 5
} ctex_tile_eviction_status;

typedef struct ctex_tile_eviction_report {
    uint32_t size;
    uint32_t status;
    size_t resident_bytes_released;
    size_t backing_bytes_written;
    size_t resident_pixel_bytes;
    size_t backed_pixel_bytes;
} ctex_tile_eviction_report;

#define CTEX_TILE_EVICTION_REPORT_V1_SIZE ((uint32_t)sizeof(ctex_tile_eviction_report))
#define CTEX_TILE_EVICTION_REPORT_CURRENT_SIZE ((uint32_t)sizeof(ctex_tile_eviction_report))

typedef enum ctex_transport_channel_order {
    CTEX_TRANSPORT_CHANNEL_ORDER_R = 0,
    CTEX_TRANSPORT_CHANNEL_ORDER_RG = 1,
    CTEX_TRANSPORT_CHANNEL_ORDER_RGB = 2,
    CTEX_TRANSPORT_CHANNEL_ORDER_RGBA = 3
} ctex_transport_channel_order;

typedef enum ctex_transport_component_byte_order {
    CTEX_TRANSPORT_COMPONENT_BYTE_ORDER_NATIVE = 0
} ctex_transport_component_byte_order;

typedef enum ctex_transport_tile_contiguity {
    CTEX_TRANSPORT_SEPARATE_TILE_BUFFERS = 0
} ctex_transport_tile_contiguity;

typedef struct ctex_transport_tile_memory_layout {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    size_t row_pitch_bytes;
    size_t pixel_stride_bytes;
    uint32_t channel_order;
    uint32_t component_type;
    uint32_t component_byte_order;
    uint32_t tile_contiguity;
    size_t byte_size;
} ctex_transport_tile_memory_layout;

#define CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_V1_SIZE \
    ((uint32_t)sizeof(ctex_transport_tile_memory_layout))
#define CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_transport_tile_memory_layout))

typedef struct ctex_transport_tile_readback_destination {
    uint32_t size;
    ctex_transport_tile_version version;
    ctex_transport_tile_memory_layout layout;
    void* output;
    size_t output_size;
} ctex_transport_tile_readback_destination;

#define CTEX_TRANSPORT_TILE_READBACK_DESTINATION_V1_SIZE \
    ((uint32_t)sizeof(ctex_transport_tile_readback_destination))
#define CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_transport_tile_readback_destination))

typedef enum ctex_transport_readback_status {
    CTEX_TRANSPORT_READBACK_PENDING = 0,
    CTEX_TRANSPORT_READBACK_COMPLETE = 1,
    CTEX_TRANSPORT_READBACK_CANCELLED = 2,
    CTEX_TRANSPORT_READBACK_FAILED = 3
} ctex_transport_readback_status;

typedef struct ctex_transport_host_tile_completion {
    uint32_t size;
    ctex_transport_tile_version version;
    ctex_transport_tile_memory_layout layout;
    const void* bytes;
    size_t byte_size;
} ctex_transport_host_tile_completion;

#define CTEX_TRANSPORT_HOST_TILE_COMPLETION_V1_SIZE \
    ((uint32_t)sizeof(ctex_transport_host_tile_completion))
#define CTEX_TRANSPORT_HOST_TILE_COMPLETION_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_transport_host_tile_completion))

typedef struct ctex_transport_readback_info {
    uint32_t size;
    uint32_t status;
    uint32_t output_readable;
    size_t tile_count;
    size_t required_detail_size;
} ctex_transport_readback_info;

#define CTEX_TRANSPORT_READBACK_INFO_V1_SIZE ((uint32_t)sizeof(ctex_transport_readback_info))
#define CTEX_TRANSPORT_READBACK_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_transport_readback_info))

typedef enum ctex_mesh_map_kind {
    CTEX_MESH_MAP_TANGENT_SPACE_NORMAL = 0,
    CTEX_MESH_MAP_OBJECT_SPACE_NORMAL = 1,
    CTEX_MESH_MAP_WORLD_SPACE_DIRECTION = 2,
    CTEX_MESH_MAP_AMBIENT_OCCLUSION = 3,
    CTEX_MESH_MAP_CURVATURE = 4,
    CTEX_MESH_MAP_THICKNESS = 5,
    CTEX_MESH_MAP_POSITION = 6,
    CTEX_MESH_MAP_HEIGHT = 7,
    CTEX_MESH_MAP_BENT_NORMAL = 8,
    CTEX_MESH_MAP_MATERIAL_ID = 9,
    CTEX_MESH_MAP_OBJECT_ID = 10,
    CTEX_MESH_MAP_UV_DENSITY = 11,
    CTEX_MESH_MAP_VERTEX_COLOUR = 12
} ctex_mesh_map_kind;

typedef enum ctex_mesh_map_channel_meaning {
    CTEX_MESH_MAP_SCALAR_DATA = 0,
    CTEX_MESH_MAP_NORMAL_XYZ = 1,
    CTEX_MESH_MAP_DIRECTION_XYZ = 2,
    CTEX_MESH_MAP_POSITION_XYZ = 3,
    CTEX_MESH_MAP_IDENTIFIER = 4,
    CTEX_MESH_MAP_COLOUR_RGB = 5,
    CTEX_MESH_MAP_COLOUR_RGBA = 6
} ctex_mesh_map_channel_meaning;

typedef enum ctex_mesh_map_normal_convention {
    CTEX_MESH_MAP_NORMAL_OPENGL = 0,
    CTEX_MESH_MAP_NORMAL_DIRECTX = 1
} ctex_mesh_map_normal_convention;

typedef enum ctex_tangent_basis_algorithm {
    CTEX_TANGENT_BASIS_UV_DERIVATIVE = 0,
    CTEX_TANGENT_BASIS_LENGYEL_ORTHONORMALIZED = 1,
    CTEX_TANGENT_BASIS_MIKKTSPACE = 2
} ctex_tangent_basis_algorithm;

typedef enum ctex_tangent_normal_orientation {
    CTEX_TANGENT_NORMAL_VERTEX = 0,
    CTEX_TANGENT_NORMAL_INVERTED_VERTEX = 1
} ctex_tangent_normal_orientation;

typedef enum ctex_coordinate_handedness {
    CTEX_COORDINATE_RIGHT_HANDED = 0,
    CTEX_COORDINATE_LEFT_HANDED = 1
} ctex_coordinate_handedness;

typedef enum ctex_uv_v_axis {
    CTEX_UV_V_AXIS_UPWARD = 0,
    CTEX_UV_V_AXIS_DOWNWARD = 1
} ctex_uv_v_axis;

typedef enum ctex_tangent_handedness_encoding {
    CTEX_TANGENT_HANDEDNESS_W_SIGN = 0
} ctex_tangent_handedness_encoding;

typedef enum ctex_tangent_frame_source {
    CTEX_TANGENT_FRAME_SUPPLIED = 0,
    CTEX_TANGENT_FRAME_GENERATED = 1
} ctex_tangent_frame_source;

typedef struct ctex_tangent_frame_descriptor {
    uint32_t size;
    uint32_t algorithm;
    uint32_t algorithm_version;
    uint32_t normal_orientation;
    uint32_t coordinate_handedness;
    uint32_t uv_v_axis;
    uint32_t handedness_encoding;
    const char* uv_set;
} ctex_tangent_frame_descriptor;

#define CTEX_TANGENT_FRAME_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_tangent_frame_descriptor))
#define CTEX_TANGENT_FRAME_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_tangent_frame_descriptor))

typedef struct ctex_mesh_tangent_data_descriptor {
    uint32_t size;
    ctex_tangent_frame_descriptor frame;
    const ctex_vec4f* corner_tangents;
    size_t corner_tangent_count;
} ctex_mesh_tangent_data_descriptor;

#define CTEX_MESH_TANGENT_DATA_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_tangent_data_descriptor))
#define CTEX_MESH_TANGENT_DATA_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_tangent_data_descriptor))

typedef struct ctex_mesh_tangent_frame_info {
    uint32_t size;
    uint32_t source;
    uint32_t algorithm;
    uint32_t algorithm_version;
    uint32_t normal_orientation;
    uint32_t coordinate_handedness;
    uint32_t uv_v_axis;
    uint32_t handedness_encoding;
    size_t corner_tangent_count;
    size_t required_uv_set_size;
} ctex_mesh_tangent_frame_info;

#define CTEX_MESH_TANGENT_FRAME_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_tangent_frame_info))
#define CTEX_MESH_TANGENT_FRAME_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_tangent_frame_info))

typedef struct ctex_mesh_map_pixel_buffer_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t component_type;
    uint32_t component_count;
    size_t row_stride_bytes;
    const void* pixels;
    size_t pixel_bytes;
} ctex_mesh_map_pixel_buffer_descriptor;

#define CTEX_MESH_MAP_PIXEL_BUFFER_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_pixel_buffer_descriptor))
#define CTEX_MESH_MAP_PIXEL_BUFFER_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_pixel_buffer_descriptor))

typedef struct ctex_mesh_map_import_descriptor {
    uint32_t size;
    uint32_t kind;
    uint32_t channel_meaning;
    uint32_t color_space;
    uint32_t has_normal_convention;
    uint32_t normal_convention;
    const ctex_tangent_frame_descriptor* tangent_frame;
    ctex_mesh_map_pixel_buffer_descriptor buffer;
} ctex_mesh_map_import_descriptor;

#define CTEX_MESH_MAP_IMPORT_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_import_descriptor))
#define CTEX_MESH_MAP_IMPORT_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_import_descriptor))

typedef struct ctex_mesh_map_import_info {
    uint32_t size;
    uint32_t replaced_existing;
    uint32_t resolution_mismatch;
    uint32_t stale;
    uint32_t converted_to_working_space;
    uint32_t storage_color_space;
    uint32_t channel_meaning;
    uint32_t map_width;
    uint32_t map_height;
    uint32_t texture_set_width;
    uint32_t texture_set_height;
    uint64_t produced_mesh_revision;
    uint64_t current_mesh_revision;
} ctex_mesh_map_import_info;

#define CTEX_MESH_MAP_IMPORT_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_import_info))
#define CTEX_MESH_MAP_IMPORT_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_map_import_info))

typedef struct ctex_mesh_map_set_info {
    uint32_t size;
    uint32_t texture_set_width;
    uint32_t texture_set_height;
    uint64_t mesh_revision;
    size_t bound_map_count;
    size_t resident_pixel_bytes;
    size_t required_texture_set_id_size;
    size_t required_uv_set_size;
} ctex_mesh_map_set_info;

#define CTEX_MESH_MAP_SET_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_set_info))
#define CTEX_MESH_MAP_SET_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_map_set_info))

typedef struct ctex_mesh_map_entry_info {
    uint32_t kind;
    uint32_t width;
    uint32_t height;
    size_t resident_pixel_bytes;
    uint64_t produced_mesh_revision;
    uint32_t stale;
    uint32_t has_normal_convention;
    uint32_t normal_convention;
    uint32_t has_tangent_frame;
    uint32_t tangent_algorithm;
    uint32_t tangent_algorithm_version;
    uint32_t tangent_normal_orientation;
    uint32_t tangent_coordinate_handedness;
    uint32_t tangent_uv_v_axis;
    uint32_t tangent_handedness_encoding;
} ctex_mesh_map_entry_info;

typedef struct ctex_mesh_map_sample_info {
    uint32_t size;
    uint32_t component_count;
    double values[4];
    uint32_t stale;
    uint64_t produced_mesh_revision;
    uint64_t current_mesh_revision;
} ctex_mesh_map_sample_info;

#define CTEX_MESH_MAP_SAMPLE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_sample_info))
#define CTEX_MESH_MAP_SAMPLE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_map_sample_info))

typedef struct ctex_mesh_map_staleness {
    uint32_t kind;
    uint64_t produced_mesh_revision;
    uint64_t current_mesh_revision;
} ctex_mesh_map_staleness;

typedef struct ctex_mesh_map_requirement_info {
    uint32_t size;
    size_t required_missing_map_count;
    size_t required_stale_map_count;
    size_t required_message_size;
} ctex_mesh_map_requirement_info;

#define CTEX_MESH_MAP_REQUIREMENT_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_requirement_info))
#define CTEX_MESH_MAP_REQUIREMENT_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_requirement_info))

typedef struct ctex_mesh_map_release_info {
    uint32_t size;
    size_t released_map_count;
    size_t resident_pixel_bytes_released;
} ctex_mesh_map_release_info;

#define CTEX_MESH_MAP_RELEASE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_release_info))
#define CTEX_MESH_MAP_RELEASE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_map_release_info))

typedef enum ctex_mesh_map_generator_kind {
    CTEX_MESH_MAP_GENERATOR_AMBIENT_OCCLUSION = 0,
    CTEX_MESH_MAP_GENERATOR_CURVATURE = 1,
    CTEX_MESH_MAP_GENERATOR_THICKNESS = 2,
    CTEX_MESH_MAP_GENERATOR_POSITION_GRADIENT = 3,
    CTEX_MESH_MAP_GENERATOR_WORLD_SPACE_DIRECTION = 4,
    CTEX_MESH_MAP_GENERATOR_DIRT = 5,
    CTEX_MESH_MAP_GENERATOR_EDGE_WEAR = 6,
    CTEX_MESH_MAP_GENERATOR_SCRATCHES = 7
} ctex_mesh_map_generator_kind;

typedef struct ctex_mesh_map_generator_parameter_descriptor {
    uint32_t size;
    size_t name_offset;
    size_t name_size;
    double default_value;
    double minimum;
    double maximum;
    size_t meaning_offset;
    size_t meaning_size;
} ctex_mesh_map_generator_parameter_descriptor;

#define CTEX_MESH_MAP_GENERATOR_PARAMETER_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_generator_parameter_descriptor))
#define CTEX_MESH_MAP_GENERATOR_PARAMETER_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_generator_parameter_descriptor))

typedef struct ctex_mesh_map_generator_info {
    uint32_t size;
    uint32_t kind;
    size_t name_offset;
    size_t name_size;
    size_t required_map_count;
    size_t parameter_count;
    size_t required_string_size;
} ctex_mesh_map_generator_info;

#define CTEX_MESH_MAP_GENERATOR_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_generator_info))
#define CTEX_MESH_MAP_GENERATOR_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_map_generator_info))

typedef struct ctex_mesh_map_generator_parameter {
    uint32_t size;
    const char* name;
    double value;
} ctex_mesh_map_generator_parameter;

#define CTEX_MESH_MAP_GENERATOR_PARAMETER_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_generator_parameter))
#define CTEX_MESH_MAP_GENERATOR_PARAMETER_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_generator_parameter))

typedef struct ctex_mesh_map_generator_resolved_parameter {
    size_t name_offset;
    size_t name_size;
    double value;
} ctex_mesh_map_generator_resolved_parameter;

typedef struct ctex_mesh_map_generator_parameter_clamp {
    size_t name_offset;
    size_t name_size;
    double supplied;
    double resolved;
} ctex_mesh_map_generator_parameter_clamp;

typedef struct ctex_mesh_map_generator_result_info {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    size_t row_stride_bytes;
    size_t required_mask_value_count;
    size_t required_resolved_parameter_count;
    size_t required_parameter_clamp_count;
    size_t required_stale_map_count;
    size_t message_offset;
    size_t message_size;
    size_t required_string_size;
} ctex_mesh_map_generator_result_info;

#define CTEX_MESH_MAP_GENERATOR_RESULT_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_generator_result_info))
#define CTEX_MESH_MAP_GENERATOR_RESULT_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_generator_result_info))

typedef enum ctex_mesh_map_bake_provider_status {
    CTEX_MESH_MAP_BAKE_PROVIDER_COMPLETED = 0,
    CTEX_MESH_MAP_BAKE_PROVIDER_CANCELLED = 1,
    CTEX_MESH_MAP_BAKE_PROVIDER_FAILED = 2
} ctex_mesh_map_bake_provider_status;

typedef enum ctex_mesh_map_bake_request_status {
    CTEX_MESH_MAP_BAKE_COMPLETED = 0,
    CTEX_MESH_MAP_BAKE_CANCELLED = 1,
    CTEX_MESH_MAP_BAKE_UNSUPPORTED = 2,
    CTEX_MESH_MAP_BAKE_REQUEST_PROVIDER_FAILED = 3
} ctex_mesh_map_bake_request_status;

typedef enum ctex_mesh_map_bake_completion_disposition {
    CTEX_MESH_MAP_BAKE_BOUND = 0,
    CTEX_MESH_MAP_BAKE_STALE = 1,
    CTEX_MESH_MAP_BAKE_COMPLETION_CANCELLED = 2,
    CTEX_MESH_MAP_BAKE_INVALID_OUTPUT = 3,
    CTEX_MESH_MAP_BAKE_UNKNOWN_TOKEN = 4
} ctex_mesh_map_bake_completion_disposition;

typedef uint32_t (*ctex_mesh_map_bake_can_produce_fn)(void* user_data, uint32_t kind);
typedef uint32_t (*ctex_mesh_map_bake_is_cancelled_fn)(void* user_data);
typedef void (*ctex_mesh_map_bake_report_progress_fn)(void* user_data, double fraction);

typedef struct ctex_mesh_map_bake_request_descriptor {
    uint32_t size;
    uint32_t kind;
    const char* texture_set_id;
    const char* uv_set;
    uint64_t mesh_revision;
    uint64_t bake_settings_revision;
    uint64_t request_generation;
    const ctex_tangent_frame_descriptor* tangent_frame;
    uint32_t width;
    uint32_t height;
} ctex_mesh_map_bake_request_descriptor;

#define CTEX_MESH_MAP_BAKE_REQUEST_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_request_descriptor))
#define CTEX_MESH_MAP_BAKE_REQUEST_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_request_descriptor))

typedef struct ctex_mesh_map_bake_control {
    uint32_t size;
    void* user_data;
    ctex_mesh_map_bake_is_cancelled_fn is_cancelled;
    ctex_mesh_map_bake_report_progress_fn report_progress;
} ctex_mesh_map_bake_control;

#define CTEX_MESH_MAP_BAKE_CONTROL_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_bake_control))
#define CTEX_MESH_MAP_BAKE_CONTROL_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_map_bake_control))

typedef struct ctex_mesh_map_bake_output_descriptor {
    uint32_t size;
    ctex_mesh_map_pixel_buffer_descriptor buffer;
    uint32_t has_normal_convention;
    uint32_t normal_convention;
    const ctex_tangent_frame_descriptor* tangent_frame;
    const char* detail;
} ctex_mesh_map_bake_output_descriptor;

#define CTEX_MESH_MAP_BAKE_OUTPUT_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_output_descriptor))
#define CTEX_MESH_MAP_BAKE_OUTPUT_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_output_descriptor))

typedef uint32_t (*ctex_mesh_map_bake_request_fn)(
    void* user_data, const ctex_mesh_map_bake_request_descriptor* request,
    const ctex_mesh_map_bake_control* control, ctex_mesh_map_bake_output_descriptor* output);

typedef struct ctex_mesh_map_bake_provider_descriptor {
    uint32_t size;
    const char* name;
    void* user_data;
    ctex_mesh_map_bake_can_produce_fn can_produce;
    ctex_mesh_map_bake_request_fn request;
} ctex_mesh_map_bake_provider_descriptor;

#define CTEX_MESH_MAP_BAKE_PROVIDER_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_provider_descriptor))
#define CTEX_MESH_MAP_BAKE_PROVIDER_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_provider_descriptor))

typedef struct ctex_mesh_map_bake_control_descriptor {
    uint32_t size;
    void* user_data;
    ctex_mesh_map_bake_is_cancelled_fn is_cancelled;
    ctex_mesh_map_bake_report_progress_fn report_progress;
} ctex_mesh_map_bake_control_descriptor;

#define CTEX_MESH_MAP_BAKE_CONTROL_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_control_descriptor))
#define CTEX_MESH_MAP_BAKE_CONTROL_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_control_descriptor))

typedef struct ctex_mesh_map_bake_result_info {
    uint32_t size;
    uint32_t status;
    uint32_t has_binding;
    uint32_t replaced_existing;
    uint32_t resolution_mismatch;
    uint32_t stale;
    uint64_t produced_mesh_revision;
    uint64_t current_mesh_revision;
} ctex_mesh_map_bake_result_info;

#define CTEX_MESH_MAP_BAKE_RESULT_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_bake_result_info))
#define CTEX_MESH_MAP_BAKE_RESULT_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_result_info))

typedef struct ctex_mesh_map_bake_session_info {
    uint32_t size;
    uint64_t settings_revision;
    size_t pending_request_count;
    size_t undo_step_count;
} ctex_mesh_map_bake_session_info;

#define CTEX_MESH_MAP_BAKE_SESSION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_bake_session_info))
#define CTEX_MESH_MAP_BAKE_SESSION_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_session_info))

typedef struct ctex_mesh_map_bake_token_info {
    uint32_t size;
    uint64_t session_identity;
    uint32_t kind;
    uint64_t mesh_revision;
    uint64_t bake_settings_revision;
    uint64_t request_generation;
    uint32_t width;
    uint32_t height;
    uint32_t has_tangent_frame;
    ctex_tangent_frame_descriptor tangent_frame;
    size_t required_texture_set_id_size;
    size_t required_uv_set_size;
    size_t required_tangent_uv_set_size;
} ctex_mesh_map_bake_token_info;

#define CTEX_MESH_MAP_BAKE_TOKEN_INFO_V1_SIZE ((uint32_t)sizeof(ctex_mesh_map_bake_token_info))
#define CTEX_MESH_MAP_BAKE_TOKEN_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_mesh_map_bake_token_info))

typedef struct ctex_mesh_map_bake_completion_info {
    uint32_t size;
    uint32_t disposition;
    uint32_t has_binding;
    uint32_t replaced_existing;
    uint32_t resolution_mismatch;
    uint32_t stale;
} ctex_mesh_map_bake_completion_info;

#define CTEX_MESH_MAP_BAKE_COMPLETION_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_completion_info))
#define CTEX_MESH_MAP_BAKE_COMPLETION_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_completion_info))

typedef struct ctex_mesh_map_bake_settings_edit_info {
    uint32_t size;
    uint64_t previous_revision;
    uint64_t current_revision;
    size_t invalidated_request_count;
} ctex_mesh_map_bake_settings_edit_info;

#define CTEX_MESH_MAP_BAKE_SETTINGS_EDIT_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_settings_edit_info))
#define CTEX_MESH_MAP_BAKE_SETTINGS_EDIT_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_settings_edit_info))

typedef struct ctex_mesh_map_bake_settings_undo_info {
    uint32_t size;
    uint32_t restored;
    uint64_t previous_revision;
    uint64_t restored_revision;
    size_t restored_map_count;
    size_t invalidated_request_count;
} ctex_mesh_map_bake_settings_undo_info;

#define CTEX_MESH_MAP_BAKE_SETTINGS_UNDO_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_settings_undo_info))
#define CTEX_MESH_MAP_BAKE_SETTINGS_UNDO_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_mesh_map_bake_settings_undo_info))

typedef enum ctex_executor_route {
    CTEX_EXECUTOR_ROUTE_HOST_EXECUTED = 0,
    CTEX_EXECUTOR_ROUTE_CPU_REFERENCE = 1,
    CTEX_EXECUTOR_ROUTE_OWNED_GPU = 2
} ctex_executor_route;

typedef enum ctex_executor_availability {
    CTEX_EXECUTOR_AVAILABLE = 0,
    CTEX_EXECUTOR_DEVICE_UNAVAILABLE = 1,
    CTEX_EXECUTOR_HOST_NOT_ATTACHED = 2
} ctex_executor_availability;

typedef enum ctex_executor_texture_format {
    CTEX_EXECUTOR_TEXTURE_R8_UNORM = 0,
    CTEX_EXECUTOR_TEXTURE_RG8_UNORM = 1,
    CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM = 2,
    CTEX_EXECUTOR_TEXTURE_R16_UNORM = 3,
    CTEX_EXECUTOR_TEXTURE_RG16_UNORM = 4,
    CTEX_EXECUTOR_TEXTURE_RGBA16_UNORM = 5,
    CTEX_EXECUTOR_TEXTURE_R16_FLOAT = 6,
    CTEX_EXECUTOR_TEXTURE_RG16_FLOAT = 7,
    CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT = 8,
    CTEX_EXECUTOR_TEXTURE_R32_FLOAT = 9,
    CTEX_EXECUTOR_TEXTURE_RG32_FLOAT = 10,
    CTEX_EXECUTOR_TEXTURE_RGBA32_FLOAT = 11,
    CTEX_EXECUTOR_TEXTURE_DEPTH32_FLOAT = 12
} ctex_executor_texture_format;

typedef struct ctex_host_executor_descriptor {
    uint32_t size;
    const char* device_name;
    uint32_t binding_budget;
    uint32_t maximum_texture_dimension;
    const uint32_t* supported_texture_formats;
    size_t supported_texture_format_count;
    uint32_t floating_point_filtering;
    uint32_t compute_available;
    uint32_t attached;
} ctex_host_executor_descriptor;

#define CTEX_HOST_EXECUTOR_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_host_executor_descriptor))
#define CTEX_HOST_EXECUTOR_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_host_executor_descriptor))

typedef struct ctex_executor_info {
    uint32_t size;
    uint32_t route;
    uint32_t availability;
    uint32_t binding_budget;
    uint32_t maximum_texture_dimension;
    size_t supported_texture_format_count;
    uint32_t floating_point_filtering;
    uint32_t compute_available;
    size_t required_identifier_size;
    size_t required_display_name_size;
    size_t required_device_name_size;
} ctex_executor_info;

#define CTEX_EXECUTOR_INFO_V1_SIZE ((uint32_t)sizeof(ctex_executor_info))
#define CTEX_EXECUTOR_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_executor_info))

typedef enum ctex_executor_selection_source {
    CTEX_EXECUTOR_SELECTION_AUTOMATIC = 0,
    CTEX_EXECUTOR_SELECTION_EXPLICIT = 1,
    CTEX_EXECUTOR_SELECTION_ENVIRONMENT = 2
} ctex_executor_selection_source;

typedef struct ctex_executor_selection_info {
    uint32_t size;
    uint32_t source;
    size_t selected_executor_index;
    size_t required_requested_identifier_size;
    size_t required_message_size;
} ctex_executor_selection_info;

#define CTEX_EXECUTOR_SELECTION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_executor_selection_info))
#define CTEX_EXECUTOR_SELECTION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_executor_selection_info))

typedef enum ctex_execution_failure_code {
    CTEX_EXECUTION_FAILURE_DEVICE_UNAVAILABLE = 0,
    CTEX_EXECUTION_FAILURE_DEVICE_LOST = 1,
    CTEX_EXECUTION_FAILURE_OPERATION_FAILED = 2,
    CTEX_EXECUTION_FAILURE_CANCELLED = 3
} ctex_execution_failure_code;

typedef enum ctex_executor_fallback_disposition {
    CTEX_EXECUTOR_NO_FALLBACK = 0,
    CTEX_EXECUTOR_CPU_FALLBACK = 1,
    CTEX_EXECUTOR_RECOVERY_REQUIRED = 2
} ctex_executor_fallback_disposition;

typedef struct ctex_executor_fallback_descriptor {
    uint32_t size;
    const char* failed_executor;
    uint32_t failure;
    const char* failure_detail;
    uint32_t disposition;
    const char* fallback_executor;
    uint32_t recovery_restored;
} ctex_executor_fallback_descriptor;

#define CTEX_EXECUTOR_FALLBACK_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_executor_fallback_descriptor))
#define CTEX_EXECUTOR_FALLBACK_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_executor_fallback_descriptor))

typedef struct ctex_executor_fallback_info {
    uint32_t size;
    uint32_t disposition;
    uint32_t recovery_restored;
    size_t required_message_size;
} ctex_executor_fallback_info;

#define CTEX_EXECUTOR_FALLBACK_INFO_V1_SIZE ((uint32_t)sizeof(ctex_executor_fallback_info))
#define CTEX_EXECUTOR_FALLBACK_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_executor_fallback_info))

typedef enum ctex_cpu_execution_status {
    CTEX_CPU_EXECUTION_COMPLETED = 0,
    CTEX_CPU_EXECUTION_CANCELLED = 1,
    CTEX_CPU_EXECUTION_MEMORY_CEILING_EXCEEDED = 2
} ctex_cpu_execution_status;

typedef ctex_result (*ctex_cpu_work_item_callback)(size_t work_item, void* shared_working_memory,
                                                   size_t shared_working_memory_size,
                                                   void* worker_working_memory,
                                                   size_t worker_working_memory_size,
                                                   void* user_data);
typedef ctex_result (*ctex_cpu_commit_callback)(const void* shared_working_memory,
                                                size_t shared_working_memory_size, void* user_data);
typedef uint32_t (*ctex_cpu_cancel_callback)(void* user_data);
typedef void (*ctex_cpu_progress_callback)(size_t completed_work_items, size_t total_work_items,
                                           void* user_data);

typedef struct ctex_cpu_bounded_execution_descriptor {
    uint32_t size;
    const char* operation;
    size_t work_item_count;
    size_t shared_working_memory_bytes;
    size_t working_memory_bytes_per_worker;
    size_t maximum_workers;
    size_t memory_ceiling_bytes;
    size_t progress_interval;
    ctex_cpu_work_item_callback execute_work_item;
    ctex_cpu_commit_callback commit;
    ctex_cpu_cancel_callback is_cancelled;
    ctex_cpu_progress_callback report_progress;
    void* user_data;
} ctex_cpu_bounded_execution_descriptor;

#define CTEX_CPU_BOUNDED_EXECUTION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_cpu_bounded_execution_descriptor))
#define CTEX_CPU_BOUNDED_EXECUTION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_cpu_bounded_execution_descriptor))

typedef struct ctex_cpu_execution_info {
    uint32_t size;
    uint32_t status;
    size_t completed_work_items;
    size_t total_work_items;
    size_t required_memory_bytes;
    size_t worker_count;
    size_t required_message_size;
} ctex_cpu_execution_info;

#define CTEX_CPU_EXECUTION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_cpu_execution_info))
#define CTEX_CPU_EXECUTION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_cpu_execution_info))

typedef struct ctex_cpu_raster_mesh_descriptor {
    uint32_t size;
    const ctex_vec3f* positions;
    const ctex_vec2f* uv;
    size_t vertex_count;
    const uint32_t* triangle_indices;
    size_t triangle_index_count;
} ctex_cpu_raster_mesh_descriptor;

#define CTEX_CPU_RASTER_MESH_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_cpu_raster_mesh_descriptor))
#define CTEX_CPU_RASTER_MESH_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_cpu_raster_mesh_descriptor))

typedef struct ctex_cpu_raster_camera_descriptor {
    uint32_t size;
    /* Column-major; element at row r, column c is view_projection[c * 4 + r]. */
    float view_projection[16];
    uint32_t width;
    uint32_t height;
} ctex_cpu_raster_camera_descriptor;

#define CTEX_CPU_RASTER_CAMERA_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_cpu_raster_camera_descriptor))
#define CTEX_CPU_RASTER_CAMERA_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_cpu_raster_camera_descriptor))

typedef struct ctex_cpu_viewport_raster_descriptor {
    uint32_t size;
    const ctex_cpu_raster_mesh_descriptor* mesh;
    const ctex_cpu_raster_camera_descriptor* camera;
    size_t maximum_output_pixels;
} ctex_cpu_viewport_raster_descriptor;

#define CTEX_CPU_VIEWPORT_RASTER_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_cpu_viewport_raster_descriptor))
#define CTEX_CPU_VIEWPORT_RASTER_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_cpu_viewport_raster_descriptor))

typedef struct ctex_cpu_uv_raster_descriptor {
    uint32_t size;
    const ctex_cpu_raster_mesh_descriptor* mesh;
    const ctex_cpu_raster_camera_descriptor* camera;
    uint32_t width;
    uint32_t height;
    ctex_vec2f tile_origin;
    size_t maximum_output_pixels;
} ctex_cpu_uv_raster_descriptor;

#define CTEX_CPU_UV_RASTER_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_cpu_uv_raster_descriptor))
#define CTEX_CPU_UV_RASTER_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_cpu_uv_raster_descriptor))

typedef struct ctex_cpu_raster_info {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    size_t pixel_count;
} ctex_cpu_raster_info;

#define CTEX_CPU_RASTER_INFO_V1_SIZE ((uint32_t)sizeof(ctex_cpu_raster_info))
#define CTEX_CPU_RASTER_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_cpu_raster_info))

typedef struct ctex_cpu_raster_outputs {
    uint32_t size;
    float* depth;
    size_t depth_capacity;
    /* Viewport raster: UV. UV-space raster: projected screen position. */
    ctex_vec2f* coordinates;
    size_t coordinate_capacity;
    uint8_t* coverage;
    size_t coverage_capacity;
    uint32_t* triangle_identity;
    size_t triangle_identity_capacity;
} ctex_cpu_raster_outputs;

#define CTEX_CPU_RASTER_OUTPUTS_V1_SIZE ((uint32_t)sizeof(ctex_cpu_raster_outputs))
#define CTEX_CPU_RASTER_OUTPUTS_CURRENT_SIZE ((uint32_t)sizeof(ctex_cpu_raster_outputs))

typedef enum ctex_parity_value_class {
    CTEX_PARITY_UNORM8 = 0,
    CTEX_PARITY_UNORM16 = 1,
    CTEX_PARITY_FLOATING_POINT = 2
} ctex_parity_value_class;

typedef struct ctex_parity_tolerance_info {
    uint32_t size;
    double absolute;
    double relative;
} ctex_parity_tolerance_info;

#define CTEX_PARITY_TOLERANCE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_parity_tolerance_info))
#define CTEX_PARITY_TOLERANCE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_parity_tolerance_info))

typedef struct ctex_parity_comparison_info {
    uint32_t size;
    uint32_t matches;
    size_t compared_value_count;
    double maximum_absolute_deviation;
    uint32_t has_failure;
    size_t failure_value_index;
    double failure_reference;
    double failure_measured;
    double failure_absolute_deviation;
    double failure_allowed_deviation;
    size_t required_message_size;
} ctex_parity_comparison_info;

#define CTEX_PARITY_COMPARISON_INFO_V1_SIZE ((uint32_t)sizeof(ctex_parity_comparison_info))
#define CTEX_PARITY_COMPARISON_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_parity_comparison_info))

typedef struct ctex_parity_fixture_channel_descriptor {
    uint32_t size;
    const char* semantic;
    uint32_t value_class;
    uint32_t filtered;
    uint32_t component_count;
} ctex_parity_fixture_channel_descriptor;

#define CTEX_PARITY_FIXTURE_CHANNEL_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_parity_fixture_channel_descriptor))
#define CTEX_PARITY_FIXTURE_CHANNEL_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_parity_fixture_channel_descriptor))

typedef struct ctex_parity_fixture_descriptor {
    uint32_t size;
    const char* identifier;
    const char* document;
    const char* stroke;
    const char* camera;
    const char* material;
    const ctex_parity_fixture_channel_descriptor* channels;
    size_t channel_count;
} ctex_parity_fixture_descriptor;

#define CTEX_PARITY_FIXTURE_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_parity_fixture_descriptor))
#define CTEX_PARITY_FIXTURE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_parity_fixture_descriptor))

typedef struct ctex_parity_rendered_channel_descriptor {
    uint32_t size;
    const char* semantic;
    const double* values;
    size_t value_count;
} ctex_parity_rendered_channel_descriptor;

#define CTEX_PARITY_RENDERED_CHANNEL_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_parity_rendered_channel_descriptor))
#define CTEX_PARITY_RENDERED_CHANNEL_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_parity_rendered_channel_descriptor))

typedef struct ctex_parity_rendered_fixture_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    const ctex_parity_rendered_channel_descriptor* channels;
    size_t channel_count;
} ctex_parity_rendered_fixture_descriptor;

#define CTEX_PARITY_RENDERED_FIXTURE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_parity_rendered_fixture_descriptor))
#define CTEX_PARITY_RENDERED_FIXTURE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_parity_rendered_fixture_descriptor))

typedef ctex_result (*ctex_parity_render_callback)(
    const ctex_parity_fixture_descriptor* fixture,
    ctex_parity_rendered_fixture_descriptor* out_rendered, void* user_data);

typedef struct ctex_parity_executor_binding_descriptor {
    uint32_t size;
    size_t executor_index;
    ctex_parity_render_callback render;
    void* user_data;
} ctex_parity_executor_binding_descriptor;

#define CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_parity_executor_binding_descriptor))
#define CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_parity_executor_binding_descriptor))

typedef struct ctex_parity_gate_info {
    uint32_t size;
    uint32_t passed;
    size_t executor_count;
    size_t reference_count;
    size_t passed_count;
    size_t failed_count;
    size_t unmeasured_count;
    size_t required_report_size;
} ctex_parity_gate_info;

#define CTEX_PARITY_GATE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_parity_gate_info))
#define CTEX_PARITY_GATE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_parity_gate_info))

typedef enum ctex_host_resource_owner {
    CTEX_HOST_RESOURCE_LIBRARY = 0,
    CTEX_HOST_RESOURCE_HOST = 1
} ctex_host_resource_owner;

typedef enum ctex_host_resource_state {
    CTEX_HOST_RESOURCE_SHADER_READ = 0,
    CTEX_HOST_RESOURCE_STORAGE_READ = 1,
    CTEX_HOST_RESOURCE_STORAGE_WRITE = 2,
    CTEX_HOST_RESOURCE_RENDER_TARGET = 3,
    CTEX_HOST_RESOURCE_DEPTH_TARGET = 4
} ctex_host_resource_state;

typedef enum ctex_host_replay_semantics {
    CTEX_HOST_REPLAY_DETERMINISTIC = 0,
    CTEX_HOST_REPLAY_CHECKPOINT_ONLY = 1
} ctex_host_replay_semantics;

typedef struct ctex_host_resource_descriptor {
    uint32_t size;
    const char* logical_id;
    uint64_t generation;
    const char* role;
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    uint32_t mip_levels;
    uint32_t tile_width;
    uint32_t tile_height;
    uint32_t externally_initialized;
    uint32_t owner;
    uint32_t required_state;
    uint32_t output;
} ctex_host_resource_descriptor;

#define CTEX_HOST_RESOURCE_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_host_resource_descriptor))
#define CTEX_HOST_RESOURCE_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_host_resource_descriptor))

typedef struct ctex_host_submission_descriptor {
    uint32_t size;
    const char* operation;
    uint64_t base_revision;
    const ctex_host_resource_descriptor* resources;
    size_t resource_count;
    uint32_t replay_semantics;
} ctex_host_submission_descriptor;

#define CTEX_HOST_SUBMISSION_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_host_submission_descriptor))
#define CTEX_HOST_SUBMISSION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_host_submission_descriptor))

typedef struct ctex_host_submission_info {
    uint32_t size;
    uint64_t completion_token;
    uint64_t base_revision;
    size_t resource_count;
} ctex_host_submission_info;

#define CTEX_HOST_SUBMISSION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_host_submission_info))
#define CTEX_HOST_SUBMISSION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_host_submission_info))

typedef struct ctex_host_execution_session_info {
    uint32_t size;
    uint64_t revision;
    size_t active_submission_count;
    size_t retained_recovery_bytes;
} ctex_host_execution_session_info;

#define CTEX_HOST_EXECUTION_SESSION_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_host_execution_session_info))
#define CTEX_HOST_EXECUTION_SESSION_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_host_execution_session_info))

typedef enum ctex_host_execution_status {
    CTEX_HOST_EXECUTION_SUCCEEDED = 0,
    CTEX_HOST_EXECUTION_FAILED = 1,
    CTEX_HOST_EXECUTION_CANCELLED = 2
} ctex_host_execution_status;

typedef struct ctex_host_completed_resource_descriptor {
    uint32_t size;
    const char* logical_id;
    uint64_t generation;
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
} ctex_host_completed_resource_descriptor;

#define CTEX_HOST_COMPLETED_RESOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_host_completed_resource_descriptor))
#define CTEX_HOST_COMPLETED_RESOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_host_completed_resource_descriptor))

typedef enum ctex_host_recovery_kind {
    CTEX_HOST_RECOVERY_RESULT_CHECKPOINT = 0,
    CTEX_HOST_RECOVERY_DETERMINISTIC_RECORD = 1
} ctex_host_recovery_kind;

typedef struct ctex_host_recovery_descriptor {
    uint32_t size;
    uint32_t kind;
    uint32_t checkpoint_complete;
    uint64_t checkpoint_revision;
    const char* operation_record_version;
    uint32_t inputs_pinned;
    size_t retained_bytes;
} ctex_host_recovery_descriptor;

#define CTEX_HOST_RECOVERY_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_host_recovery_descriptor))
#define CTEX_HOST_RECOVERY_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_host_recovery_descriptor))

typedef struct ctex_host_completion_descriptor {
    uint32_t size;
    uint64_t completion_token;
    uint32_t status;
    const ctex_host_completed_resource_descriptor* outputs;
    size_t output_count;
    const ctex_host_recovery_descriptor* recovery;
    const char* detail;
} ctex_host_completion_descriptor;

#define CTEX_HOST_COMPLETION_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_host_completion_descriptor))
#define CTEX_HOST_COMPLETION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_host_completion_descriptor))

typedef enum ctex_host_completion_disposition {
    CTEX_HOST_COMPLETION_PUBLISHED = 0,
    CTEX_HOST_COMPLETION_AWAITING_RECOVERY = 1,
    CTEX_HOST_COMPLETION_STALE = 2,
    CTEX_HOST_COMPLETION_CANCELLED = 3,
    CTEX_HOST_COMPLETION_FAILED = 4,
    CTEX_HOST_COMPLETION_REJECTED = 5,
    CTEX_HOST_COMPLETION_DUPLICATE = 6,
    CTEX_HOST_COMPLETION_UNKNOWN_TOKEN = 7
} ctex_host_completion_disposition;

typedef struct ctex_host_resource_version {
    size_t logical_id_offset;
    uint64_t generation;
} ctex_host_resource_version;

typedef struct ctex_host_completion_result_info {
    uint32_t size;
    uint32_t disposition;
    uint64_t completion_token;
    uint32_t has_published_revision;
    uint64_t published_revision;
    size_t released_resource_count;
    size_t required_released_identity_size;
    size_t required_message_size;
} ctex_host_completion_result_info;

#define CTEX_HOST_COMPLETION_RESULT_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_host_completion_result_info))
#define CTEX_HOST_COMPLETION_RESULT_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_host_completion_result_info))

typedef struct ctex_host_device_loss_info {
    uint32_t size;
    uint64_t recovered_revision;
    size_t cancelled_submission_count;
    size_t released_resource_count;
    size_t required_released_identity_size;
    size_t retained_recovery_bytes;
    uint32_t restored;
} ctex_host_device_loss_info;

#define CTEX_HOST_DEVICE_LOSS_INFO_V1_SIZE ((uint32_t)sizeof(ctex_host_device_loss_info))
#define CTEX_HOST_DEVICE_LOSS_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_host_device_loss_info))

typedef enum ctex_texture_export_texture_set_selection {
    CTEX_TEXTURE_EXPORT_TEXTURE_SET_ALL = 0,
    CTEX_TEXTURE_EXPORT_TEXTURE_SET_SELECTED = 1
} ctex_texture_export_texture_set_selection;

typedef enum ctex_texture_export_spatial_scope {
    CTEX_TEXTURE_EXPORT_SCOPE_TEXTURE_SET = 0,
    CTEX_TEXTURE_EXPORT_SCOPE_UDIM = 1,
    CTEX_TEXTURE_EXPORT_SCOPE_ATLAS = 2
} ctex_texture_export_spatial_scope;

typedef enum ctex_texture_export_layer_scope {
    CTEX_TEXTURE_EXPORT_LAYER_FLATTEN_VISIBLE = 0,
    CTEX_TEXTURE_EXPORT_LAYER_FLATTEN_SELECTED = 1,
    CTEX_TEXTURE_EXPORT_LAYER_EACH_SELECTED = 2
} ctex_texture_export_layer_scope;

typedef enum ctex_texture_export_layer_kind {
    CTEX_TEXTURE_EXPORT_LAYER_CONTENT = 0,
    CTEX_TEXTURE_EXPORT_LAYER_GROUP = 1
} ctex_texture_export_layer_kind;

typedef struct ctex_texture_export_layer_source_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    const char* parent_identifier;
    uint32_t kind;
    uint32_t visible;
} ctex_texture_export_layer_source_descriptor;

#define CTEX_TEXTURE_EXPORT_LAYER_SOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_layer_source_descriptor))
#define CTEX_TEXTURE_EXPORT_LAYER_SOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_layer_source_descriptor))

typedef struct ctex_texture_export_texture_set_source_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    uint32_t width;
    uint32_t height;
    const uint32_t* occupied_udim_tiles;
    size_t occupied_udim_tile_count;
    const ctex_texture_export_layer_source_descriptor* layers;
    size_t layer_count;
} ctex_texture_export_texture_set_source_descriptor;

#define CTEX_TEXTURE_EXPORT_TEXTURE_SET_SOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_texture_set_source_descriptor))
#define CTEX_TEXTURE_EXPORT_TEXTURE_SET_SOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_texture_set_source_descriptor))

typedef struct ctex_texture_export_atlas_source_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    uint32_t width;
    uint32_t height;
    const char* const* texture_set_identifiers;
    size_t texture_set_identifier_count;
} ctex_texture_export_atlas_source_descriptor;

#define CTEX_TEXTURE_EXPORT_ATLAS_SOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_atlas_source_descriptor))
#define CTEX_TEXTURE_EXPORT_ATLAS_SOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_atlas_source_descriptor))

typedef struct ctex_texture_export_catalogue_descriptor {
    uint32_t size;
    const char* project_name;
    const ctex_texture_export_texture_set_source_descriptor* texture_sets;
    size_t texture_set_count;
    const ctex_texture_export_atlas_source_descriptor* atlases;
    size_t atlas_count;
} ctex_texture_export_catalogue_descriptor;

#define CTEX_TEXTURE_EXPORT_CATALOGUE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_catalogue_descriptor))
#define CTEX_TEXTURE_EXPORT_CATALOGUE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_catalogue_descriptor))

typedef struct ctex_texture_export_layer_selection_descriptor {
    uint32_t size;
    const char* texture_set_identifier;
    const char* const* layer_identifiers;
    size_t layer_identifier_count;
} ctex_texture_export_layer_selection_descriptor;

#define CTEX_TEXTURE_EXPORT_LAYER_SELECTION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_layer_selection_descriptor))
#define CTEX_TEXTURE_EXPORT_LAYER_SELECTION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_layer_selection_descriptor))

typedef struct ctex_texture_export_plan_descriptor {
    uint32_t size;
    uint32_t texture_set_selection;
    const char* const* selected_texture_set_identifiers;
    size_t selected_texture_set_identifier_count;
    uint32_t spatial_scope;
    uint32_t layer_scope;
    const ctex_texture_export_layer_selection_descriptor* selected_layers;
    size_t selected_layer_count;
    uint32_t output_width;
    uint32_t output_height;
    const char* filename_pattern;
} ctex_texture_export_plan_descriptor;

#define CTEX_TEXTURE_EXPORT_PLAN_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_plan_descriptor))
#define CTEX_TEXTURE_EXPORT_PLAN_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_plan_descriptor))

typedef struct ctex_texture_export_texture_descriptor {
    uint32_t size;
    const char* suffix;
    const char* channel_tokens[4];
    uint32_t color_space;
    uint32_t bit_depth;
    uint32_t format;
} ctex_texture_export_texture_descriptor;

#define CTEX_TEXTURE_EXPORT_TEXTURE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_texture_descriptor))
#define CTEX_TEXTURE_EXPORT_TEXTURE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_texture_descriptor))

/*
 * A NULL preset selects the default built-in. A descriptor with no textures
 * selects its built-in identifier. Otherwise it defines a custom preset.
 */
typedef struct ctex_texture_export_preset_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    const ctex_texture_export_texture_descriptor* textures;
    size_t texture_count;
} ctex_texture_export_preset_descriptor;

#define CTEX_TEXTURE_EXPORT_PRESET_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_preset_descriptor))
#define CTEX_TEXTURE_EXPORT_PRESET_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_preset_descriptor))

typedef struct ctex_texture_export_options_descriptor {
    uint32_t size;
    const ctex_texture_export_plan_descriptor* plan;
    uint32_t padding_radius;
    uint32_t jpeg_quality;
    uint32_t dry_run;
} ctex_texture_export_options_descriptor;

#define CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_options_descriptor))
#define CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_options_descriptor))

typedef struct ctex_texture_export_named_value {
    uint32_t size;
    const char* identifier;
    uint32_t component_count;
    double components[4];
} ctex_texture_export_named_value;

#define CTEX_TEXTURE_EXPORT_NAMED_VALUE_V1_SIZE ((uint32_t)sizeof(ctex_texture_export_named_value))
#define CTEX_TEXTURE_EXPORT_NAMED_VALUE_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_named_value))

typedef struct ctex_texture_export_sample {
    uint32_t size;
    double base_color[3];
    double opacity;
    double roughness;
    double metallic;
    double normal[3];
    double height;
    double occlusion;
    double emission[3];
    double subsurface;
    const ctex_texture_export_named_value* mesh_maps;
    size_t mesh_map_count;
    const ctex_texture_export_named_value* registered_channels;
    size_t registered_channel_count;
} ctex_texture_export_sample;

#define CTEX_TEXTURE_EXPORT_SAMPLE_V1_SIZE ((uint32_t)sizeof(ctex_texture_export_sample))
#define CTEX_TEXTURE_EXPORT_SAMPLE_CURRENT_SIZE ((uint32_t)sizeof(ctex_texture_export_sample))

typedef ctex_result (*ctex_texture_export_sample_callback)(uint32_t x, uint32_t y,
                                                           ctex_texture_export_sample* out_sample,
                                                           void* user_data);

typedef struct ctex_texture_export_pixel_source_descriptor {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    ctex_texture_export_sample_callback sample;
    void* sample_user_data;
    const uint8_t* coverage;
    size_t coverage_count;
} ctex_texture_export_pixel_source_descriptor;

#define CTEX_TEXTURE_EXPORT_PIXEL_SOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_pixel_source_descriptor))
#define CTEX_TEXTURE_EXPORT_PIXEL_SOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_pixel_source_descriptor))

typedef struct ctex_texture_export_layer_selection_view {
    uint32_t size;
    const char* texture_set_identifier;
    const char* const* layer_identifiers;
    size_t layer_identifier_count;
} ctex_texture_export_layer_selection_view;

#define CTEX_TEXTURE_EXPORT_LAYER_SELECTION_VIEW_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_layer_selection_view))
#define CTEX_TEXTURE_EXPORT_LAYER_SELECTION_VIEW_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_layer_selection_view))

typedef struct ctex_texture_export_planned_output {
    uint32_t size;
    const char* relative_path;
    const char* const* texture_set_identifiers;
    size_t texture_set_identifier_count;
    uint32_t has_udim_tile;
    uint32_t udim_tile;
    const char* atlas_identifier;
    const ctex_texture_export_layer_selection_view* layer_selections;
    size_t layer_selection_count;
    size_t preset_texture_index;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t bit_depth;
    uint32_t color_space;
} ctex_texture_export_planned_output;

#define CTEX_TEXTURE_EXPORT_PLANNED_OUTPUT_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_planned_output))
#define CTEX_TEXTURE_EXPORT_PLANNED_OUTPUT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_planned_output))

typedef struct ctex_texture_export_encoded_output {
    uint32_t size;
    size_t report_entry_index;
    const char* relative_path;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t bit_depth;
    uint32_t color_space;
    const void* bytes;
    size_t byte_count;
} ctex_texture_export_encoded_output;

#define CTEX_TEXTURE_EXPORT_ENCODED_OUTPUT_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_encoded_output))
#define CTEX_TEXTURE_EXPORT_ENCODED_OUTPUT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_encoded_output))

typedef ctex_result (*ctex_texture_export_source_callback)(
    const ctex_texture_export_planned_output* output,
    ctex_texture_export_pixel_source_descriptor* out_source, void* user_data);
typedef ctex_result (*ctex_texture_export_output_callback)(
    const ctex_texture_export_encoded_output* output, void* user_data);
typedef ctex_result (*ctex_texture_export_report_callback)(const char* json, size_t json_size,
                                                           void* user_data);
typedef void (*ctex_texture_export_progress_callback)(size_t completed_outputs,
                                                      size_t total_outputs,
                                                      const char* relative_path, void* user_data);
typedef uint32_t (*ctex_texture_export_cancel_callback)(void* user_data);

typedef struct ctex_texture_export_callbacks_descriptor {
    uint32_t size;
    ctex_texture_export_source_callback source;
    ctex_texture_export_output_callback output;
    ctex_texture_export_report_callback report;
    ctex_texture_export_progress_callback progress;
    ctex_texture_export_cancel_callback cancel;
    void* user_data;
} ctex_texture_export_callbacks_descriptor;

#define CTEX_TEXTURE_EXPORT_CALLBACKS_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_callbacks_descriptor))
#define CTEX_TEXTURE_EXPORT_CALLBACKS_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_texture_export_callbacks_descriptor))

typedef struct ctex_texture_export_info {
    uint32_t size;
    size_t planned_output_count;
    size_t encoded_output_count;
    uint32_t dry_run;
    uint32_t cancelled;
} ctex_texture_export_info;

#define CTEX_TEXTURE_EXPORT_INFO_V1_SIZE ((uint32_t)sizeof(ctex_texture_export_info))
#define CTEX_TEXTURE_EXPORT_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_texture_export_info))

typedef struct ctex_project_container_read_limits_descriptor {
    uint32_t size;
    size_t maximum_input_bytes;
    size_t maximum_total_allocation_bytes;
    size_t maximum_sections;
    size_t maximum_images;
    size_t maximum_tiles;
    size_t maximum_resources;
    size_t maximum_assets;
    size_t maximum_asset_dependencies;
    size_t maximum_string_bytes;
    size_t maximum_decoded_tile_bytes;
    size_t maximum_packed_resource_bytes;
    size_t maximum_asset_payload_bytes;
} ctex_project_container_read_limits_descriptor;

#define CTEX_PROJECT_CONTAINER_READ_LIMITS_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_project_container_read_limits_descriptor))
#define CTEX_PROJECT_CONTAINER_READ_LIMITS_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_container_read_limits_descriptor))

typedef struct ctex_project_container_version {
    uint32_t size;
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} ctex_project_container_version;

#define CTEX_PROJECT_CONTAINER_VERSION_V1_SIZE ((uint32_t)sizeof(ctex_project_container_version))
#define CTEX_PROJECT_CONTAINER_VERSION_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_container_version))

typedef struct ctex_project_container_info {
    uint32_t size;
    ctex_project_container_version source_schema;
    uint32_t newer_schema;
    size_t tiled_image_count;
    size_t resource_count;
    size_t asset_count;
    size_t opaque_section_count;
    size_t occupied_tile_count;
    size_t packed_resource_bytes;
    size_t canonical_size;
    size_t report_size;
} ctex_project_container_info;

#define CTEX_PROJECT_CONTAINER_INFO_V1_SIZE ((uint32_t)sizeof(ctex_project_container_info))
#define CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_project_container_info))

typedef struct ctex_project_autosave_config_descriptor {
    uint32_t size;
    const char* recovery_directory;
    const char* recovery_key;
    uint64_t interval_milliseconds;
} ctex_project_autosave_config_descriptor;

#define CTEX_PROJECT_AUTOSAVE_CONFIG_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_project_autosave_config_descriptor))
#define CTEX_PROJECT_AUTOSAVE_CONFIG_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_autosave_config_descriptor))

typedef enum ctex_project_autosave_submission_status {
    CTEX_PROJECT_AUTOSAVE_QUEUED = 0,
    CTEX_PROJECT_AUTOSAVE_STALE_REVISION = 1
} ctex_project_autosave_submission_status;

typedef struct ctex_project_autosave_info {
    uint32_t size;
    uint32_t has_last_saved_revision;
    uint64_t last_saved_revision;
    uint32_t has_pending_revision;
    uint64_t pending_revision;
    uint32_t has_saving_revision;
    uint64_t saving_revision;
    uint64_t successful_writes;
    size_t required_recovery_path_size;
    size_t required_last_error_size;
} ctex_project_autosave_info;

#define CTEX_PROJECT_AUTOSAVE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_project_autosave_info))
#define CTEX_PROJECT_AUTOSAVE_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_project_autosave_info))

typedef void (*ctex_project_quiesce_cancel_callback)(void* user_data);

typedef struct ctex_project_quiesce_descriptor {
    uint32_t size;
    uint64_t current_revision;
    uint64_t deadline_milliseconds;
    ctex_project_quiesce_cancel_callback request_cancel;
    void* user_data;
} ctex_project_quiesce_descriptor;

#define CTEX_PROJECT_QUIESCE_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_project_quiesce_descriptor))
#define CTEX_PROJECT_QUIESCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_quiesce_descriptor))

typedef enum ctex_project_quiesce_status {
    CTEX_PROJECT_QUIESCE_DURABLE = 0,
    CTEX_PROJECT_QUIESCE_DEADLINE_EXCEEDED = 1,
    CTEX_PROJECT_QUIESCE_CHECKPOINT_FAILED = 2
} ctex_project_quiesce_status;

typedef struct ctex_project_quiesce_report {
    uint32_t size;
    uint32_t status;
    uint32_t admissions_stopped;
    uint32_t cancellation_requested;
    uint32_t work_drained;
    size_t active_operation_count;
    uint32_t has_durable_revision;
    uint64_t durable_revision;
    uint32_t has_uncheckpointed_range;
    uint64_t uncheckpointed_first_revision;
    uint64_t uncheckpointed_last_revision;
} ctex_project_quiesce_report;

#define CTEX_PROJECT_QUIESCE_REPORT_V1_SIZE ((uint32_t)sizeof(ctex_project_quiesce_report))
#define CTEX_PROJECT_QUIESCE_REPORT_CURRENT_SIZE ((uint32_t)sizeof(ctex_project_quiesce_report))

typedef struct ctex_project_recovery_checkpoint_info {
    uint32_t size;
    uint32_t has_revision;
    uint64_t revision;
} ctex_project_recovery_checkpoint_info;

#define CTEX_PROJECT_RECOVERY_CHECKPOINT_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_project_recovery_checkpoint_info))
#define CTEX_PROJECT_RECOVERY_CHECKPOINT_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_recovery_checkpoint_info))

typedef struct ctex_project_recovery_entry {
    size_t recovery_key_offset;
    size_t recovery_key_size;
    size_t path_offset;
    size_t path_size;
    ctex_project_container_version schema;
    uint64_t file_bytes;
} ctex_project_recovery_entry;

typedef struct ctex_project_recovery_rejection {
    size_t path_offset;
    size_t path_size;
    size_t message_offset;
    size_t message_size;
} ctex_project_recovery_rejection;

typedef struct ctex_project_recovery_enumeration_info {
    uint32_t size;
    size_t required_recoverable_count;
    size_t required_rejected_count;
    size_t required_string_size;
} ctex_project_recovery_enumeration_info;

#define CTEX_PROJECT_RECOVERY_ENUMERATION_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_project_recovery_enumeration_info))
#define CTEX_PROJECT_RECOVERY_ENUMERATION_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_recovery_enumeration_info))

typedef struct ctex_project_asset_export_options_descriptor {
    uint32_t size;
    uint32_t self_contained;
    const char* source_directory;
} ctex_project_asset_export_options_descriptor;

#define CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_project_asset_export_options_descriptor))
#define CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_asset_export_options_descriptor))

typedef struct ctex_project_asset_search_paths_descriptor {
    uint32_t size;
    const char* const* paths;
    size_t path_count;
} ctex_project_asset_search_paths_descriptor;

#define CTEX_PROJECT_ASSET_SEARCH_PATHS_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_project_asset_search_paths_descriptor))
#define CTEX_PROJECT_ASSET_SEARCH_PATHS_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_asset_search_paths_descriptor))

typedef struct ctex_project_resource_descriptor {
    uint32_t size;
    const char* identifier;
    const char* kind;
    const char* relative_path;
    uint32_t packed;
    const void* packed_bytes;
    size_t packed_byte_count;
} ctex_project_resource_descriptor;

#define CTEX_PROJECT_RESOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_project_resource_descriptor))
#define CTEX_PROJECT_RESOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_resource_descriptor))

typedef enum ctex_operation_replay_class {
    CTEX_OPERATION_REPLAY_CHECKPOINT_ONLY = 0,
    CTEX_OPERATION_REPLAY_SAME_RESOLUTION = 1,
    CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT = 2
} ctex_operation_replay_class;

typedef enum ctex_operation_payload_kind {
    CTEX_OPERATION_PAYLOAD_RESOLVED_STAMPS = 0,
    CTEX_OPERATION_PAYLOAD_EDITABLE_SOURCE_PATH = 1,
    CTEX_OPERATION_PAYLOAD_OPAQUE_ALGORITHM_DATA = 2
} ctex_operation_payload_kind;

typedef enum ctex_operation_replay_disposition {
    CTEX_OPERATION_REPLAY_SAME_RESOLUTION_AVAILABLE = 0,
    CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT_AVAILABLE = 1,
    CTEX_OPERATION_REPLAY_CHECKPOINT_ONLY_AVAILABLE = 2,
    CTEX_OPERATION_REPLAY_RESAMPLE_CHECKPOINT_REQUIRED = 3,
    CTEX_OPERATION_REPLAY_UNSUPPORTED_ALGORITHM = 4
} ctex_operation_replay_disposition;

typedef struct ctex_operation_channel_descriptor {
    uint32_t size;
    const char* semantic_id;
    uint32_t component_count;
    uint32_t scalar_representation;
    uint32_t bit_depth;
    uint32_t color_space;
    const double* default_value;
    size_t default_value_count;
} ctex_operation_channel_descriptor;

#define CTEX_OPERATION_CHANNEL_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_operation_channel_descriptor))
#define CTEX_OPERATION_CHANNEL_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_operation_channel_descriptor))

typedef struct ctex_pinned_operation_resource_descriptor {
    uint32_t size;
    const char* role;
    const char* content_identity;
    const void* bytes;
    size_t byte_count;
} ctex_pinned_operation_resource_descriptor;

#define CTEX_PINNED_OPERATION_RESOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_pinned_operation_resource_descriptor))
#define CTEX_PINNED_OPERATION_RESOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_pinned_operation_resource_descriptor))

typedef struct ctex_operation_record_descriptor {
    uint32_t size;
    const char* identifier;
    const char* algorithm_identifier;
    uint32_t algorithm_version;
    const char* preset_identifier;
    uint32_t preset_version;
    uint32_t replay_class;
    uint64_t input_document_revision;
    uint64_t seed;
    const char* mesh_content_identity;
    double coordinate_frame[16];
    uint32_t payload_kind;
    uint32_t payload_version;
    const ctex_operation_channel_descriptor* channels;
    size_t channel_count;
    const ctex_pinned_operation_resource_descriptor* pinned_resources;
    size_t pinned_resource_count;
    const char* const* checkpoint_image_identifiers;
    size_t checkpoint_image_count;
    const void* payload;
    size_t payload_size;
} ctex_operation_record_descriptor;

#define CTEX_OPERATION_RECORD_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_operation_record_descriptor))
#define CTEX_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_operation_record_descriptor))

typedef struct ctex_operation_record_info {
    uint32_t size;
    uint32_t schema_version;
    uint32_t replay_class;
    uint32_t payload_kind;
    uint32_t payload_version;
    uint64_t input_document_revision;
    uint64_t seed;
    size_t channel_count;
    size_t pinned_resource_count;
    size_t checkpoint_image_count;
    size_t pinned_resource_bytes;
    size_t payload_size;
    size_t canonical_size;
    size_t report_size;
} ctex_operation_record_info;

#define CTEX_OPERATION_RECORD_INFO_V1_SIZE ((uint32_t)sizeof(ctex_operation_record_info))
#define CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_operation_record_info))

typedef struct ctex_operation_algorithm_support_descriptor {
    uint32_t size;
    const char* identifier;
    uint32_t minimum_version;
    uint32_t maximum_version;
} ctex_operation_algorithm_support_descriptor;

#define CTEX_OPERATION_ALGORITHM_SUPPORT_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_operation_algorithm_support_descriptor))
#define CTEX_OPERATION_ALGORITHM_SUPPORT_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_operation_algorithm_support_descriptor))

typedef struct ctex_operation_replay_assessment_descriptor {
    uint32_t size;
    const ctex_operation_algorithm_support_descriptor* supported_algorithms;
    size_t supported_algorithm_count;
    uint32_t target_resolution_changed;
} ctex_operation_replay_assessment_descriptor;

#define CTEX_OPERATION_REPLAY_ASSESSMENT_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_operation_replay_assessment_descriptor))
#define CTEX_OPERATION_REPLAY_ASSESSMENT_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_operation_replay_assessment_descriptor))

typedef struct ctex_operation_replay_info {
    uint32_t size;
    uint32_t disposition;
    uint32_t declared_replay_class;
    uint32_t replay_available;
    uint32_t checkpoint_available;
    uint32_t target_resolution_changed;
    size_t required_report_size;
} ctex_operation_replay_info;

#define CTEX_OPERATION_REPLAY_INFO_V1_SIZE ((uint32_t)sizeof(ctex_operation_replay_info))
#define CTEX_OPERATION_REPLAY_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_operation_replay_info))

typedef struct ctex_project_operation_replay_info {
    uint32_t size;
    size_t record_count;
    size_t replay_available_count;
    size_t checkpoint_fallback_count;
    size_t unsupported_algorithm_count;
    size_t required_report_size;
} ctex_project_operation_replay_info;

#define CTEX_PROJECT_OPERATION_REPLAY_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_project_operation_replay_info))
#define CTEX_PROJECT_OPERATION_REPLAY_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_project_operation_replay_info))

typedef enum ctex_editable_entry_kind {
    CTEX_EDITABLE_ENTRY_DECAL = 0,
    CTEX_EDITABLE_ENTRY_TEXT = 1,
    CTEX_EDITABLE_ENTRY_SURFACE_PATH = 2
} ctex_editable_entry_kind;

typedef struct ctex_editable_placement_frame {
    ctex_vec3d position;
    ctex_vec3d normal;
    double rotation_radians;
    double uniform_scale;
    ctex_vec2d axis_scale;
} ctex_editable_placement_frame;

typedef struct ctex_editable_material_parameter_descriptor {
    uint32_t size;
    const char* identifier;
    uint32_t component_count;
    double value[4];
} ctex_editable_material_parameter_descriptor;

#define CTEX_EDITABLE_MATERIAL_PARAMETER_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_editable_material_parameter_descriptor))
#define CTEX_EDITABLE_MATERIAL_PARAMETER_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_editable_material_parameter_descriptor))

typedef struct ctex_editable_tile_dependency_descriptor {
    uint32_t size;
    const char* semantic_id;
    uint32_t tile_x;
    uint32_t tile_y;
} ctex_editable_tile_dependency_descriptor;

#define CTEX_EDITABLE_TILE_DEPENDENCY_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_editable_tile_dependency_descriptor))
#define CTEX_EDITABLE_TILE_DEPENDENCY_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_editable_tile_dependency_descriptor))

typedef struct ctex_editable_surface_point_descriptor {
    uint32_t size;
    ctex_vec3d position;
    ctex_vec3d normal;
    uint32_t triangle;
    double barycentric[3];
    double width;
} ctex_editable_surface_point_descriptor;

#define CTEX_EDITABLE_SURFACE_POINT_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_editable_surface_point_descriptor))
#define CTEX_EDITABLE_SURFACE_POINT_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_editable_surface_point_descriptor))

typedef struct ctex_editable_entry_descriptor {
    uint32_t size;
    const char* identifier;
    uint32_t kind;
    uint64_t expected_revision;
    ctex_editable_placement_frame placement;
    const char* material_identity;
    const ctex_editable_material_parameter_descriptor* material_parameters;
    size_t material_parameter_count;
    const char* text;
    const char* font_identity;
    uint64_t mesh_revision;
    const ctex_editable_surface_point_descriptor* surface_points;
    size_t surface_point_count;
    const ctex_editable_tile_dependency_descriptor* dependent_tiles;
    size_t dependent_tile_count;
} ctex_editable_entry_descriptor;

#define CTEX_EDITABLE_ENTRY_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_editable_entry_descriptor))
#define CTEX_EDITABLE_ENTRY_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_editable_entry_descriptor))

typedef struct ctex_editable_entry_info {
    uint32_t size;
    uint32_t kind;
    uint32_t entry_present;
    uint64_t entry_revision;
    uint64_t document_revision;
    size_t entry_count;
    size_t material_parameter_count;
    size_t surface_point_count;
    size_t invalidated_tile_count;
    size_t undo_step_count;
    size_t redo_step_count;
    size_t required_report_size;
} ctex_editable_entry_info;

#define CTEX_EDITABLE_ENTRY_INFO_V1_SIZE ((uint32_t)sizeof(ctex_editable_entry_info))
#define CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_editable_entry_info))

typedef struct ctex_preset_shelf_entry_descriptor {
    uint32_t size;
    const char* asset_identifier;
    const char* display_name;
    const char* const* tags;
    size_t tag_count;
    const char* thumbnail_resource_identifier;
} ctex_preset_shelf_entry_descriptor;

#define CTEX_PRESET_SHELF_ENTRY_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_preset_shelf_entry_descriptor))
#define CTEX_PRESET_SHELF_ENTRY_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_preset_shelf_entry_descriptor))

typedef struct ctex_preset_shelf_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    const void* contents;
    size_t contents_size;
    const ctex_preset_shelf_entry_descriptor* entries;
    size_t entry_count;
} ctex_preset_shelf_descriptor;

#define CTEX_PRESET_SHELF_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_preset_shelf_descriptor))
#define CTEX_PRESET_SHELF_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_preset_shelf_descriptor))

typedef struct ctex_preset_library_descriptor {
    uint32_t size;
    const ctex_preset_shelf_descriptor* shelves;
    size_t shelf_count;
    const ctex_project_container_read_limits_descriptor* read_limits;
} ctex_preset_library_descriptor;

#define CTEX_PRESET_LIBRARY_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_preset_library_descriptor))
#define CTEX_PRESET_LIBRARY_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_preset_library_descriptor))

typedef struct ctex_preset_library_info {
    uint32_t size;
    size_t shelf_count;
    size_t preset_count;
    size_t report_size;
} ctex_preset_library_info;

#define CTEX_PRESET_LIBRARY_INFO_V1_SIZE ((uint32_t)sizeof(ctex_preset_library_info))
#define CTEX_PRESET_LIBRARY_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_preset_library_info))

typedef struct ctex_material_graph_catalogue_info {
    uint32_t size;
    size_t node_count;
    size_t input_node_count;
    size_t texture_node_count;
    size_t colour_filter_node_count;
    size_t vector_math_node_count;
    size_t math_operation_count;
    size_t vector_math_operation_count;
    size_t report_size;
} ctex_material_graph_catalogue_info;

#define CTEX_MATERIAL_GRAPH_CATALOGUE_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_catalogue_info))
#define CTEX_MATERIAL_GRAPH_CATALOGUE_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_catalogue_info))

typedef enum ctex_material_graph_socket_coercion {
    CTEX_MATERIAL_GRAPH_COERCION_IDENTITY = 0,
    CTEX_MATERIAL_GRAPH_COERCION_SCALAR_TO_VECTOR = 1,
    CTEX_MATERIAL_GRAPH_COERCION_VECTOR_TO_SCALAR = 2,
    CTEX_MATERIAL_GRAPH_COERCION_COLOUR_TO_VECTOR = 3,
    CTEX_MATERIAL_GRAPH_COERCION_COLOUR_TO_SCALAR = 4
} ctex_material_graph_socket_coercion;

typedef enum ctex_material_graph_diagnostic_code {
    CTEX_MATERIAL_GRAPH_UNCONNECTED_REQUIRED_INPUT = 0,
    CTEX_MATERIAL_GRAPH_MISSING_MESH_MAP = 1,
    CTEX_MATERIAL_GRAPH_MISSING_IMAGE_RESOURCE = 2,
    CTEX_MATERIAL_GRAPH_MISSING_GROUP = 3,
    CTEX_MATERIAL_GRAPH_MISSING_NODE_TYPE = 4,
    CTEX_MATERIAL_GRAPH_INCOMPATIBLE_NODE_INTERFACE = 5,
    CTEX_MATERIAL_GRAPH_UNSUPPORTED_EMISSION_TARGET = 6,
    CTEX_MATERIAL_GRAPH_UNREACHABLE_NODE = 7
} ctex_material_graph_diagnostic_code;

typedef struct ctex_material_graph_info {
    uint32_t size;
    uint64_t output_node_id;
    size_t node_count;
    size_t link_count;
    size_t output_channel_count;
    size_t canonical_size;
    size_t report_size;
} ctex_material_graph_info;

#define CTEX_MATERIAL_GRAPH_INFO_V1_SIZE ((uint32_t)sizeof(ctex_material_graph_info))
#define CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_material_graph_info))

typedef struct ctex_material_graph_link_descriptor {
    uint32_t size;
    uint64_t source_node;
    const char* source_socket;
    uint64_t target_node;
    const char* target_socket;
} ctex_material_graph_link_descriptor;

#define CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_link_descriptor))
#define CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_link_descriptor))

typedef struct ctex_material_graph_link_info {
    uint32_t size;
    ctex_material_graph_info graph;
    uint32_t coercion;
    uint32_t replaced;
    uint64_t replaced_source_node;
    size_t replaced_source_socket_size;
} ctex_material_graph_link_info;

#define CTEX_MATERIAL_GRAPH_LINK_INFO_V1_SIZE ((uint32_t)sizeof(ctex_material_graph_link_info))
#define CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_material_graph_link_info))

typedef struct ctex_material_graph_validation_resources_descriptor {
    uint32_t size;
    const char* const* image_resources;
    size_t image_resource_count;
    const char* const* mesh_maps;
    size_t mesh_map_count;
} ctex_material_graph_validation_resources_descriptor;

#define CTEX_MATERIAL_GRAPH_VALIDATION_RESOURCES_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_validation_resources_descriptor))
#define CTEX_MATERIAL_GRAPH_VALIDATION_RESOURCES_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_validation_resources_descriptor))

typedef struct ctex_material_graph_validation_info {
    uint32_t size;
    uint32_t valid;
    size_t error_count;
    size_t warning_count;
    size_t diagnostic_count;
    size_t report_size;
} ctex_material_graph_validation_info;

#define CTEX_MATERIAL_GRAPH_VALIDATION_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_validation_info))
#define CTEX_MATERIAL_GRAPH_VALIDATION_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_validation_info))

typedef struct ctex_material_graph_library_info {
    uint32_t size;
    size_t preset_count;
    size_t canonical_size;
    size_t report_size;
} ctex_material_graph_library_info;

#define CTEX_MATERIAL_GRAPH_LIBRARY_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_library_info))
#define CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_library_info))

typedef struct ctex_material_graph_preset_descriptor {
    uint32_t size;
    const char* stable_id;
    const char* name;
    const char* thumbnail_resource;
    const void* graph_serialized;
    size_t graph_serialized_size;
} ctex_material_graph_preset_descriptor;

#define CTEX_MATERIAL_GRAPH_PRESET_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_preset_descriptor))
#define CTEX_MATERIAL_GRAPH_PRESET_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_preset_descriptor))

typedef enum ctex_smart_material_value_type {
    CTEX_SMART_MATERIAL_VALUE_SCALAR = 0,
    CTEX_SMART_MATERIAL_VALUE_VECTOR = 1,
    CTEX_SMART_MATERIAL_VALUE_COLOUR = 2,
    CTEX_SMART_MATERIAL_VALUE_STRING = 3,
    CTEX_SMART_MATERIAL_VALUE_IMAGE = 4,
    CTEX_SMART_MATERIAL_VALUE_BOOLEAN = 5
} ctex_smart_material_value_type;

typedef struct ctex_smart_material_value_descriptor {
    uint32_t size;
    uint32_t type;
    double scalar;
    ctex_vec3f vector;
    ctex_vec4f colour;
    const char* text;
    uint32_t boolean;
} ctex_smart_material_value_descriptor;

#define CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_smart_material_value_descriptor))
#define CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_smart_material_value_descriptor))

typedef enum ctex_material_graph_owner_kind {
    CTEX_MATERIAL_GRAPH_OWNER_MATERIAL = 0,
    CTEX_MATERIAL_GRAPH_OWNER_GROUP = 1
} ctex_material_graph_owner_kind;

typedef struct ctex_material_graph_socket_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    uint32_t type;
    const ctex_smart_material_value_descriptor* default_value;
} ctex_material_graph_socket_descriptor;

#define CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_socket_descriptor))
#define CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_socket_descriptor))

typedef struct ctex_material_graph_group_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    const ctex_material_graph_socket_descriptor* inputs;
    size_t input_count;
    const ctex_material_graph_socket_descriptor* outputs;
    size_t output_count;
} ctex_material_graph_group_descriptor;

#define CTEX_MATERIAL_GRAPH_GROUP_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_group_descriptor))
#define CTEX_MATERIAL_GRAPH_GROUP_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_group_descriptor))

typedef struct ctex_material_graph_group_interface_descriptor {
    uint32_t size;
    const ctex_material_graph_socket_descriptor* inputs;
    size_t input_count;
    const ctex_material_graph_socket_descriptor* outputs;
    size_t output_count;
} ctex_material_graph_group_interface_descriptor;

#define CTEX_MATERIAL_GRAPH_GROUP_INTERFACE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_group_interface_descriptor))
#define CTEX_MATERIAL_GRAPH_GROUP_INTERFACE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_group_interface_descriptor))

typedef struct ctex_material_graph_workspace_info {
    uint32_t size;
    size_t material_count;
    size_t group_count;
} ctex_material_graph_workspace_info;

#define CTEX_MATERIAL_GRAPH_WORKSPACE_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_workspace_info))
#define CTEX_MATERIAL_GRAPH_WORKSPACE_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_workspace_info))

typedef struct ctex_material_graph_group_update_info {
    uint32_t size;
    uint32_t group_version;
    size_t instances_updated;
    size_t removed_link_count;
} ctex_material_graph_group_update_info;

#define CTEX_MATERIAL_GRAPH_GROUP_UPDATE_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_group_update_info))
#define CTEX_MATERIAL_GRAPH_GROUP_UPDATE_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_group_update_info))

typedef enum ctex_material_graph_emission_target {
    CTEX_MATERIAL_GRAPH_TARGET_WGSL = 0,
    CTEX_MATERIAL_GRAPH_TARGET_MSL = 1,
    CTEX_MATERIAL_GRAPH_TARGET_SPIRV = 2,
    CTEX_MATERIAL_GRAPH_TARGET_HLSL = 3
} ctex_material_graph_emission_target;

typedef enum ctex_shader_filter_mode {
    CTEX_SHADER_FILTER_NEAREST = 0,
    CTEX_SHADER_FILTER_LINEAR = 1
} ctex_shader_filter_mode;

typedef enum ctex_shader_preview_kind {
    CTEX_SHADER_PREVIEW_LIT = 0,
    CTEX_SHADER_PREVIEW_CHANNEL_INSPECTION = 1
} ctex_shader_preview_kind;

typedef struct ctex_shader_texture_descriptor {
    uint32_t size;
    const char* logical_id;
    uint64_t generation;
    const char* role;
    uint32_t format; /* ctex_executor_texture_format */
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    uint32_t mip_levels;
    uint32_t tile_width;
    uint32_t tile_height;
    uint32_t externally_initialized;
} ctex_shader_texture_descriptor;

#define CTEX_SHADER_TEXTURE_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_shader_texture_descriptor))
#define CTEX_SHADER_TEXTURE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_texture_descriptor))

typedef struct ctex_shader_material_resource_descriptor {
    uint32_t size;
    const char* identifier;
    ctex_shader_texture_descriptor texture;
} ctex_shader_material_resource_descriptor;

#define CTEX_SHADER_MATERIAL_RESOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_shader_material_resource_descriptor))
#define CTEX_SHADER_MATERIAL_RESOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_material_resource_descriptor))

typedef struct ctex_shader_device_features_descriptor {
    uint32_t size;
    uint32_t binding_budget;
    uint32_t maximum_texture_dimension;
    const uint32_t* supported_texture_formats;
    size_t supported_texture_format_count;
    uint32_t floating_point_filtering;
    uint32_t compute_available;
} ctex_shader_device_features_descriptor;

#define CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_shader_device_features_descriptor))
#define CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_device_features_descriptor))

typedef struct ctex_shader_material_request {
    uint32_t size;
    const char* stable_identity;
    uint32_t target; /* ctex_material_graph_emission_target */
    ctex_shader_device_features_descriptor features;
    const ctex_shader_material_resource_descriptor* resources;
    size_t resource_count;
    ctex_shader_texture_descriptor output;
    uint32_t requested_filter; /* ctex_shader_filter_mode */
    uint32_t vertex_count;
} ctex_shader_material_request;

#define CTEX_SHADER_MATERIAL_REQUEST_V1_SIZE ((uint32_t)sizeof(ctex_shader_material_request))
#define CTEX_SHADER_MATERIAL_REQUEST_CURRENT_SIZE ((uint32_t)sizeof(ctex_shader_material_request))

typedef struct ctex_shader_material_info {
    uint32_t size;
    uint32_t target;
    size_t vertex_artifact_size;
    size_t fragment_artifact_size;
    size_t pass_plan_size;
    size_t workaround_report_size;
    size_t pass_count;
    size_t logical_resource_count;
    size_t binding_count;
    size_t workaround_count;
} ctex_shader_material_info;

#define CTEX_SHADER_MATERIAL_INFO_V1_SIZE ((uint32_t)sizeof(ctex_shader_material_info))
#define CTEX_SHADER_MATERIAL_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_shader_material_info))

typedef enum ctex_shader_material_source_kind {
    CTEX_SHADER_MATERIAL_SOURCE_SERIALIZED_GRAPH = 0,
    CTEX_SHADER_MATERIAL_SOURCE_WORKSPACE = 1
} ctex_shader_material_source_kind;

typedef struct ctex_shader_material_source_descriptor {
    uint32_t size;
    uint32_t kind; /* ctex_shader_material_source_kind */
    const void* graph_serialized;
    size_t graph_serialized_size;
    const ctex_material_graph_workspace* workspace;
    const char* material_identifier;
} ctex_shader_material_source_descriptor;

#define CTEX_SHADER_MATERIAL_SOURCE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_shader_material_source_descriptor))
#define CTEX_SHADER_MATERIAL_SOURCE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_material_source_descriptor))

typedef struct ctex_shader_material_debug_info {
    uint32_t size;
    size_t node_attribution_count;
    size_t metadata_size;
    uint32_t binary_companion;
} ctex_shader_material_debug_info;

#define CTEX_SHADER_MATERIAL_DEBUG_INFO_V1_SIZE ((uint32_t)sizeof(ctex_shader_material_debug_info))
#define CTEX_SHADER_MATERIAL_DEBUG_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_material_debug_info))

typedef struct ctex_shader_backend_attribution_info {
    uint32_t size;
    size_t report_size;
} ctex_shader_backend_attribution_info;

#define CTEX_SHADER_BACKEND_ATTRIBUTION_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_shader_backend_attribution_info))
#define CTEX_SHADER_BACKEND_ATTRIBUTION_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_backend_attribution_info))

typedef struct ctex_shader_layer_descriptor {
    uint32_t size;
    const char* identifier;
    ctex_shader_texture_descriptor texture;
} ctex_shader_layer_descriptor;

#define CTEX_SHADER_LAYER_DESCRIPTOR_V1_SIZE ((uint32_t)sizeof(ctex_shader_layer_descriptor))
#define CTEX_SHADER_LAYER_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_shader_layer_descriptor))

typedef struct ctex_shader_layer_stack_request {
    uint32_t size;
    const char* stable_identity;
    uint32_t target; /* ctex_material_graph_emission_target */
    ctex_shader_device_features_descriptor features;
    const ctex_shader_layer_descriptor* layers;
    size_t layer_count;
    ctex_shader_texture_descriptor output;
    uint32_t requested_filter; /* ctex_shader_filter_mode */
} ctex_shader_layer_stack_request;

#define CTEX_SHADER_LAYER_STACK_REQUEST_V1_SIZE ((uint32_t)sizeof(ctex_shader_layer_stack_request))
#define CTEX_SHADER_LAYER_STACK_REQUEST_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_layer_stack_request))

typedef struct ctex_shader_layer_stack_info {
    uint32_t size;
    uint32_t target;
    size_t layer_count;
    size_t pass_count;
    size_t artifact_blob_size;
    size_t artifact_report_size;
    size_t pass_plan_size;
    size_t workaround_report_size;
    size_t workaround_count;
    uint32_t compute_used;
    uint32_t cache_hit;
} ctex_shader_layer_stack_info;

#define CTEX_SHADER_LAYER_STACK_INFO_V1_SIZE ((uint32_t)sizeof(ctex_shader_layer_stack_info))
#define CTEX_SHADER_LAYER_STACK_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_shader_layer_stack_info))

typedef struct ctex_shader_emission_cache_info {
    uint32_t size;
    size_t entry_count;
    size_t hit_count;
    size_t miss_count;
} ctex_shader_emission_cache_info;

#define CTEX_SHADER_EMISSION_CACHE_INFO_V1_SIZE ((uint32_t)sizeof(ctex_shader_emission_cache_info))
#define CTEX_SHADER_EMISSION_CACHE_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_emission_cache_info))

typedef struct ctex_shader_preview_channel_descriptor {
    uint32_t size;
    const char* semantic_id;
    uint32_t component_count;
    ctex_shader_texture_descriptor texture;
} ctex_shader_preview_channel_descriptor;

#define CTEX_SHADER_PREVIEW_CHANNEL_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_shader_preview_channel_descriptor))
#define CTEX_SHADER_PREVIEW_CHANNEL_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_preview_channel_descriptor))

typedef struct ctex_shader_preview_environment_descriptor {
    uint32_t size;
    ctex_shader_texture_descriptor radiance;
    ctex_shader_texture_descriptor diffuse_irradiance;
    ctex_shader_texture_descriptor specular_brdf_lookup;
} ctex_shader_preview_environment_descriptor;

#define CTEX_SHADER_PREVIEW_ENVIRONMENT_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_shader_preview_environment_descriptor))
#define CTEX_SHADER_PREVIEW_ENVIRONMENT_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_shader_preview_environment_descriptor))

typedef struct ctex_shader_preview_request {
    uint32_t size;
    const char* stable_identity;
    uint32_t target; /* ctex_material_graph_emission_target */
    ctex_shader_device_features_descriptor features;
    const ctex_shader_preview_channel_descriptor* channels;
    size_t channel_count;
    ctex_shader_texture_descriptor output;
    const ctex_shader_preview_environment_descriptor* environment; /* optional */
    size_t analytic_light_count;
    uint32_t vertex_count;
} ctex_shader_preview_request;

#define CTEX_SHADER_PREVIEW_REQUEST_V1_SIZE ((uint32_t)sizeof(ctex_shader_preview_request))
#define CTEX_SHADER_PREVIEW_REQUEST_CURRENT_SIZE ((uint32_t)sizeof(ctex_shader_preview_request))

typedef struct ctex_shader_preview_info {
    uint32_t size;
    uint32_t target;
    uint32_t kind; /* ctex_shader_preview_kind */
    uint32_t fallback_lighting;
    uint32_t cache_hit;
    size_t vertex_artifact_size;
    size_t fragment_artifact_size;
    size_t pass_plan_size;
    size_t workaround_report_size;
    size_t pass_count;
    size_t logical_resource_count;
    size_t binding_count;
    size_t workaround_count;
} ctex_shader_preview_info;

#define CTEX_SHADER_PREVIEW_INFO_V1_SIZE ((uint32_t)sizeof(ctex_shader_preview_info))
#define CTEX_SHADER_PREVIEW_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_shader_preview_info))

typedef struct ctex_material_graph_property_descriptor {
    uint32_t size;
    const char* identifier;
    const char* display_name;
    const ctex_smart_material_value_descriptor* default_value;
    const char* const* allowed_values;
    size_t allowed_value_count;
} ctex_material_graph_property_descriptor;

#define CTEX_MATERIAL_GRAPH_PROPERTY_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_property_descriptor))
#define CTEX_MATERIAL_GRAPH_PROPERTY_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_property_descriptor))

typedef struct ctex_material_graph_parity_fixture_descriptor {
    uint32_t size;
    const char* identifier;
    const ctex_smart_material_value_descriptor* inputs;
    size_t input_count;
    const ctex_smart_material_value_descriptor* expected_outputs;
    size_t expected_output_count;
    double tolerance;
} ctex_material_graph_parity_fixture_descriptor;

#define CTEX_MATERIAL_GRAPH_PARITY_FIXTURE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_parity_fixture_descriptor))
#define CTEX_MATERIAL_GRAPH_PARITY_FIXTURE_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_parity_fixture_descriptor))

typedef struct ctex_material_graph_host_property_value {
    const char* identifier;
    ctex_smart_material_value_descriptor value;
} ctex_material_graph_host_property_value;

typedef struct ctex_material_graph_host_evaluation_request {
    uint32_t size;
    uint64_t node_id;
    const char* type_id;
    uint32_t type_version;
    const ctex_material_graph_host_property_value* properties;
    size_t property_count;
    const ctex_smart_material_value_descriptor* inputs;
    size_t input_count;
} ctex_material_graph_host_evaluation_request;

#define CTEX_MATERIAL_GRAPH_HOST_EVALUATION_REQUEST_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_evaluation_request))
#define CTEX_MATERIAL_GRAPH_HOST_EVALUATION_REQUEST_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_evaluation_request))

typedef ctex_result (*ctex_material_graph_cpu_evaluate_callback)(
    const ctex_material_graph_host_evaluation_request* request,
    ctex_smart_material_value_descriptor* outputs, size_t output_count, void* user_data);

typedef struct ctex_material_graph_host_emission_request {
    uint32_t size;
    uint64_t node_id;
    const char* type_id;
    uint32_t type_version;
    uint32_t target;
    const ctex_material_graph_host_property_value* properties;
    size_t property_count;
    const char* const* input_expressions;
    size_t input_expression_count;
} ctex_material_graph_host_emission_request;

#define CTEX_MATERIAL_GRAPH_HOST_EMISSION_REQUEST_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_emission_request))
#define CTEX_MATERIAL_GRAPH_HOST_EMISSION_REQUEST_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_emission_request))

typedef struct ctex_material_graph_host_emission_result {
    uint32_t size;
    const char* const* output_expressions;
    size_t output_expression_count;
    const char* const* resource_identifiers;
    size_t resource_identifier_count;
} ctex_material_graph_host_emission_result;

#define CTEX_MATERIAL_GRAPH_HOST_EMISSION_RESULT_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_emission_result))
#define CTEX_MATERIAL_GRAPH_HOST_EMISSION_RESULT_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_emission_result))

typedef ctex_result (*ctex_material_graph_emit_callback)(
    const ctex_material_graph_host_emission_request* request,
    ctex_material_graph_host_emission_result* out_result, void* user_data);

/* Callback-returned string storage must remain valid until the enclosing API call returns. */

typedef struct ctex_material_graph_host_node_registration_descriptor {
    uint32_t size;
    const char* type_id;
    uint32_t type_version;
    const char* display_name;
    const ctex_material_graph_socket_descriptor* inputs;
    size_t input_count;
    const ctex_material_graph_socket_descriptor* outputs;
    size_t output_count;
    const ctex_material_graph_property_descriptor* properties;
    size_t property_count;
    ctex_material_graph_cpu_evaluate_callback cpu_evaluate;
    ctex_material_graph_emit_callback emit;
    void* user_data;
    uint32_t deterministic;
    const char* const* resource_dependencies;
    size_t resource_dependency_count;
    const uint32_t* supported_targets;
    size_t supported_target_count;
    const ctex_material_graph_parity_fixture_descriptor* parity_fixtures;
    size_t parity_fixture_count;
} ctex_material_graph_host_node_registration_descriptor;

#define CTEX_MATERIAL_GRAPH_HOST_NODE_REGISTRATION_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_node_registration_descriptor))
#define CTEX_MATERIAL_GRAPH_HOST_NODE_REGISTRATION_DESCRIPTOR_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_node_registration_descriptor))

typedef struct ctex_material_graph_node_registry_info {
    uint32_t size;
    size_t registration_count;
} ctex_material_graph_node_registry_info;

#define CTEX_MATERIAL_GRAPH_NODE_REGISTRY_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_node_registry_info))
#define CTEX_MATERIAL_GRAPH_NODE_REGISTRY_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_node_registry_info))

typedef struct ctex_material_graph_host_contract_info {
    uint32_t size;
    uint32_t parity_passed;
    size_t parity_failure_count;
    uint32_t replay_eligible;
    size_t unpinned_dependency_count;
    size_t report_size;
} ctex_material_graph_host_contract_info;

#define CTEX_MATERIAL_GRAPH_HOST_CONTRACT_INFO_V1_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_contract_info))
#define CTEX_MATERIAL_GRAPH_HOST_CONTRACT_INFO_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_material_graph_host_contract_info))

typedef struct ctex_smart_material_info {
    uint32_t size;
    uint32_t source_schema_version;
    uint32_t canonical_schema_version;
    size_t entry_count;
    size_t derived_entry_count;
    size_t model_specific_entry_count;
    size_t model_specific_pixel_bytes;
    size_t exposed_parameter_count;
    size_t anchor_count;
    size_t anchor_reference_count;
    size_t resource_reference_count;
    size_t canonical_size;
    size_t report_size;
} ctex_smart_material_info;

#define CTEX_SMART_MATERIAL_INFO_V1_SIZE ((uint32_t)sizeof(ctex_smart_material_info))
#define CTEX_SMART_MATERIAL_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_smart_material_info))

typedef enum ctex_applied_preset_kind {
    CTEX_APPLIED_PRESET_SMART_MATERIAL = 0,
    CTEX_APPLIED_PRESET_SMART_MASK = 1
} ctex_applied_preset_kind;

typedef struct ctex_preset_application_info {
    uint32_t size;
    uint32_t kind;
    uint32_t schema_version;
    size_t entry_count;
    size_t application_count;
    size_t undo_step_count;
} ctex_preset_application_info;

#define CTEX_PRESET_APPLICATION_INFO_V1_SIZE ((uint32_t)sizeof(ctex_preset_application_info))
#define CTEX_PRESET_APPLICATION_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_preset_application_info))

typedef struct ctex_preset_undo_info {
    uint32_t size;
    uint32_t removed;
    size_t removed_entry_count;
    size_t application_count;
    size_t undo_step_count;
} ctex_preset_undo_info;

#define CTEX_PRESET_UNDO_INFO_V1_SIZE ((uint32_t)sizeof(ctex_preset_undo_info))
#define CTEX_PRESET_UNDO_INFO_CURRENT_SIZE ((uint32_t)sizeof(ctex_preset_undo_info))

typedef enum ctex_color_space {
    CTEX_COLOR_SPACE_LINEAR_REC709 = 0,
    CTEX_COLOR_SPACE_SRGB_REC709 = 1
} ctex_color_space;

typedef enum ctex_input_color_space {
    CTEX_INPUT_COLOR_SPACE_AUTOMATIC = 0,
    CTEX_INPUT_COLOR_SPACE_LINEAR_REC709 = 1,
    CTEX_INPUT_COLOR_SPACE_SRGB_REC709 = 2
} ctex_input_color_space;

typedef enum ctex_channel_semantic {
    CTEX_CHANNEL_SEMANTIC_BASE_COLOR = 0,
    CTEX_CHANNEL_SEMANTIC_OPACITY = 1,
    CTEX_CHANNEL_SEMANTIC_ROUGHNESS = 2,
    CTEX_CHANNEL_SEMANTIC_METALLIC = 3,
    CTEX_CHANNEL_SEMANTIC_NORMAL = 4,
    CTEX_CHANNEL_SEMANTIC_HEIGHT = 5,
    CTEX_CHANNEL_SEMANTIC_OCCLUSION = 6,
    CTEX_CHANNEL_SEMANTIC_EMISSION = 7,
    CTEX_CHANNEL_SEMANTIC_SUBSURFACE = 8
} ctex_channel_semantic;

typedef struct ctex_rgb_color {
    double red;
    double green;
    double blue;
    uint32_t color_space;
} ctex_rgb_color;

typedef struct ctex_channel_color_policy {
    uint32_t size;
    uint32_t color_valued;
    uint32_t recommended_bit_depth;
} ctex_channel_color_policy;

#define CTEX_CHANNEL_COLOR_POLICY_V1_SIZE ((uint32_t)sizeof(ctex_channel_color_policy))
#define CTEX_CHANNEL_COLOR_POLICY_CURRENT_SIZE ((uint32_t)sizeof(ctex_channel_color_policy))

typedef struct ctex_resolved_input_color_space {
    uint32_t size;
    uint32_t color_space;
    uint32_t inferred;
} ctex_resolved_input_color_space;

#define CTEX_RESOLVED_INPUT_COLOR_SPACE_V1_SIZE ((uint32_t)sizeof(ctex_resolved_input_color_space))
#define CTEX_RESOLVED_INPUT_COLOR_SPACE_CURRENT_SIZE \
    ((uint32_t)sizeof(ctex_resolved_input_color_space))

typedef struct ctex_bit_depth_warning {
    uint32_t size;
    uint32_t warning;
    uint32_t selected_bit_depth;
    uint32_t recommended_bit_depth;
} ctex_bit_depth_warning;

#define CTEX_BIT_DEPTH_WARNING_V1_SIZE ((uint32_t)sizeof(ctex_bit_depth_warning))
#define CTEX_BIT_DEPTH_WARNING_CURRENT_SIZE ((uint32_t)sizeof(ctex_bit_depth_warning))

typedef struct ctex_version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    const char* string;
} ctex_version;

CTEX_API ctex_version ctex_get_version(void);
CTEX_API ctex_version ctex_get_abi_version(void);
CTEX_API ctex_color_space ctex_get_working_color_space(void);
CTEX_API ctex_result ctex_color_space_get_name(uint32_t color_space, char* buffer,
                                               size_t buffer_size, size_t* out_required_size);
CTEX_API ctex_result ctex_channel_get_color_policy(uint32_t channel_semantic,
                                                   ctex_channel_color_policy* out_policy);
CTEX_API ctex_result ctex_resolve_input_color_space(uint32_t declaration, uint32_t channel_semantic,
                                                    ctex_resolved_input_color_space* out_resolved);
CTEX_API ctex_result ctex_color_convert(const ctex_rgb_color* input,
                                        uint32_t destination_color_space,
                                        ctex_rgb_color* out_color);
CTEX_API ctex_result ctex_color_input_to_working(const ctex_rgb_color* input,
                                                 uint32_t channel_semantic,
                                                 ctex_rgb_color* out_color);
CTEX_API ctex_result ctex_channel_get_bit_depth_warning(uint32_t channel_semantic,
                                                        uint32_t selected_bit_depth,
                                                        ctex_bit_depth_warning* out_warning);
CTEX_API ctex_result ctex_accumulate_height(const double* contributions, size_t contribution_count,
                                            uint32_t storage_bit_depth, double* out_accumulated);
CTEX_API ctex_result ctex_quantize_unorm8(double value, uint32_t x, uint32_t y, uint32_t dither,
                                          uint8_t* out_value);
CTEX_API ctex_result ctex_cube_lut_create(const char* cube_source, size_t cube_source_size,
                                          ctex_cube_lut** out_lut);
CTEX_API void ctex_cube_lut_destroy(ctex_cube_lut* lut);
CTEX_API ctex_result ctex_cube_lut_apply_preview(const ctex_cube_lut* lut,
                                                 const ctex_rgb_color* input,
                                                 ctex_rgb_color* out_color);

/*
 * Decodes content-detected image bytes into tightly packed, row-major,
 * interleaved component bytes. Sixteen-bit components use native byte order.
 * Pass a NULL pixel_buffer with size zero to query out_required_size.
 */
CTEX_API ctex_result ctex_image_decode_memory(const void* encoded, size_t encoded_size,
                                              const char* source_name, uint32_t intended_channel,
                                              uint32_t input_color_space,
                                              const ctex_image_decode_limits_descriptor* limits,
                                              ctex_decoded_image_info* out_info, void* pixel_buffer,
                                              size_t pixel_buffer_size, size_t* out_required_size);

/*
 * Expands packed or row-strided pixels without changing component bit depth.
 * Grayscale replicates to RGB; grayscale-alpha preserves alpha when expanding
 * to RGBA; RGB gains an opaque alpha. Pass NULL output with size zero to size.
 */
CTEX_API ctex_result
ctex_image_expand_channels(const void* source_pixels, size_t source_pixel_buffer_size,
                           const ctex_image_channel_expansion_descriptor* descriptor,
                           ctex_image_channel_expansion_info* out_info, void* output_pixels,
                           size_t output_pixel_buffer_size);

/*
 * Resamples packed or row-strided pixels with nearest or pixel-centred
 * bilinear filtering. DEFAULT selects bilinear. Output is tightly packed.
 */
CTEX_API ctex_result ctex_image_resample(const void* source_pixels, size_t source_pixel_buffer_size,
                                         const ctex_image_resample_descriptor* descriptor,
                                         ctex_image_resample_info* out_info, void* output_pixels,
                                         size_t output_pixel_buffer_size);

/*
 * Encodes borrowed, row-major interleaved pixels to PNG, JPEG, TGA, TIFF or
 * OpenEXR. A zero row stride means tightly packed input. Pass a NULL output
 * buffer with size zero to query out_required_size.
 */
CTEX_API ctex_result ctex_image_encode_memory(const void* pixels, size_t pixel_buffer_size,
                                              const ctex_image_encode_descriptor* descriptor,
                                              void* encoded_buffer, size_t encoded_buffer_size,
                                              size_t* out_required_size);

/* Returns packed NUL-terminated built-in preset identifiers. */
CTEX_API ctex_result ctex_texture_export_get_built_in_preset_ids(char* buffer, size_t buffer_size,
                                                                 size_t* out_required_size,
                                                                 size_t* out_count);

/*
 * Plans and optionally encodes a complete texture export synchronously. All
 * descriptors and callback views are borrowed for the duration of the call.
 * Output and report bytes are valid only during their callback and must be
 * copied by the host. Callbacks execute on the calling thread, carry user_data,
 * and must not throw across the C boundary. A cancelled export delivers every
 * fully encoded output and a report marked cancelled, then returns CANCELLED.
 */
CTEX_API ctex_result ctex_texture_export_run(
    const ctex_texture_export_catalogue_descriptor* catalogue,
    const ctex_texture_export_preset_descriptor* preset,
    const ctex_texture_export_options_descriptor* options,
    const ctex_texture_export_callbacks_descriptor* callbacks, ctex_texture_export_info* out_info);

/*
 * Creates the canonical byte representation of an empty project container.
 * A null output with zero size queries the exact byte count.
 */
CTEX_API ctex_result ctex_project_container_create_empty(void* output, size_t output_size,
                                                         size_t* out_required_size);

/* Reads only the fixed project-container header and reports its schema version. */
CTEX_API ctex_result ctex_project_container_probe_version(
    const void* encoded, size_t encoded_size, ctex_project_container_version* out_version);

/*
 * Opens untrusted bytes under explicit limits, preserves supported and opaque
 * content, and deterministically emits canonical bytes plus a NUL-terminated
 * JSON inspection report. A null limits descriptor selects the documented
 * defaults. Required output sizes are returned in out_info; both outputs follow
 * the atomic two-call caller-buffer contract.
 */
CTEX_API ctex_result ctex_project_container_normalize(
    const void* encoded, size_t encoded_size,
    const ctex_project_container_read_limits_descriptor* limits,
    ctex_project_container_info* out_info, void* canonical_output, size_t canonical_output_size,
    char* report_output, size_t report_output_size);

/*
 * Validates and atomically publishes canonical project bytes at path. A null
 * limits descriptor selects the documented defaults.
 */
CTEX_API ctex_result
ctex_project_container_save_atomic(const void* encoded, size_t encoded_size,
                                   const ctex_project_container_read_limits_descriptor* limits,
                                   const char* path, ctex_project_container_info* out_info);

/*
 * Runs periodic project autosave on a worker thread. Submission validates and
 * snapshots canonical project bytes before returning; only the newest queued
 * revision is written. Destroy flushes no pending work, so call flush when the
 * latest queued revision must be durable before shutdown.
 */
CTEX_API ctex_result
ctex_project_autosave_session_create(const ctex_project_autosave_config_descriptor* config,
                                     ctex_project_autosave_session** out_session);
CTEX_API void ctex_project_autosave_session_destroy(ctex_project_autosave_session* session);
CTEX_API ctex_result ctex_project_autosave_session_submit(
    ctex_project_autosave_session* session, uint64_t revision, const void* encoded,
    size_t encoded_size, const ctex_project_container_read_limits_descriptor* limits,
    uint32_t* out_status);
CTEX_API ctex_result ctex_project_autosave_session_wait(ctex_project_autosave_session* session,
                                                        uint64_t timeout_milliseconds,
                                                        uint32_t* out_idle);
CTEX_API ctex_result ctex_project_autosave_session_flush(ctex_project_autosave_session* session);
CTEX_API ctex_result ctex_project_autosave_session_get_info(
    const ctex_project_autosave_session* session, ctex_project_autosave_info* out_info,
    char* recovery_path, size_t recovery_path_size, char* last_error, size_t last_error_size);
/*
 * Stops new ledger admissions, requests an immediate checkpoint, invokes the
 * optional host cancellation callback, and waits within one relative deadline.
 * Admission remains stopped after every outcome until lifecycle_resume.
 */
CTEX_API ctex_result ctex_project_lifecycle_quiesce(
    ctex_resource_ledger* ledger, ctex_project_autosave_session* autosave,
    const ctex_project_quiesce_descriptor* descriptor, ctex_project_quiesce_report* out_report);
CTEX_API ctex_result ctex_project_lifecycle_resume(ctex_resource_ledger* ledger);

/* Enumerates atomically published recovery files and names malformed candidates. */
CTEX_API ctex_result ctex_project_recovery_enumerate(
    const char* recovery_directory, ctex_project_recovery_enumeration_info* out_info,
    ctex_project_recovery_entry* recoverable, size_t recoverable_capacity,
    ctex_project_recovery_rejection* rejected, size_t rejected_capacity, char* strings,
    size_t string_capacity);

/* Opens one recovery path under project-container limits using the normal two-call contract. */
CTEX_API ctex_result ctex_project_recovery_read(
    const char* path, const ctex_project_container_read_limits_descriptor* limits,
    ctex_project_container_info* out_info, void* canonical_output, size_t canonical_output_size,
    char* report_output, size_t report_output_size);
/* Reads the same canonical recovery bytes and reports their atomic checkpoint revision. */
CTEX_API ctex_result ctex_project_recovery_resume(
    const char* path, const ctex_project_container_read_limits_descriptor* limits,
    ctex_project_recovery_checkpoint_info* out_checkpoint, ctex_project_container_info* out_info,
    void* canonical_output, size_t canonical_output_size, char* report_output,
    size_t report_output_size);

/*
 * Extracts one asset and its exact resource/image dependencies from a project
 * container. Self-contained export packs referenced resources from the source
 * directory. Outputs use the same atomic two-call contract as normalization.
 */
CTEX_API ctex_result ctex_project_asset_export(
    const void* project_encoded, size_t project_encoded_size,
    const ctex_project_container_read_limits_descriptor* limits, const char* asset_identifier,
    const ctex_project_asset_export_options_descriptor* options,
    ctex_project_container_info* out_info, void* asset_output, size_t asset_output_size,
    char* report_output, size_t report_output_size);

/*
 * Opens a standalone asset package, resolves referenced resources through the
 * supplied search paths, and atomically returns an updated library container.
 * A null search-path descriptor permits only already-packed resources.
 */
CTEX_API ctex_result ctex_project_asset_install(
    const void* library_encoded, size_t library_encoded_size, const void* asset_encoded,
    size_t asset_encoded_size, const ctex_project_container_read_limits_descriptor* limits,
    const ctex_project_asset_search_paths_descriptor* search_paths,
    ctex_project_container_info* out_info, void* library_output, size_t library_output_size,
    char* report_output, size_t report_output_size);

/* Creates or validates a canonical, self-contained editable operation record. */
CTEX_API ctex_result ctex_operation_record_create(
    const ctex_operation_record_descriptor* descriptor, ctex_operation_record_info* out_info,
    void* canonical_output, size_t canonical_output_size, char* report_output,
    size_t report_output_size);
CTEX_API ctex_result ctex_operation_record_inspect(const void* serialized, size_t serialized_size,
                                                   ctex_operation_record_info* out_info,
                                                   void* canonical_output,
                                                   size_t canonical_output_size,
                                                   char* report_output, size_t report_output_size);
CTEX_API ctex_result ctex_operation_record_assess_replay(
    const void* serialized, size_t serialized_size,
    const ctex_operation_replay_assessment_descriptor* descriptor,
    ctex_operation_replay_info* out_info, char* report_output, size_t report_output_size);

/* Atomically adds or replaces one operation-record asset in canonical project bytes. */
CTEX_API ctex_result ctex_project_container_upsert_operation_record(
    const void* project_encoded, size_t project_encoded_size,
    const ctex_project_container_read_limits_descriptor* limits, const void* record_serialized,
    size_t record_serialized_size, ctex_project_container_info* out_info, void* project_output,
    size_t project_output_size, char* report_output, size_t report_output_size);

/* Extracts one operation record from canonical project bytes. */
CTEX_API ctex_result ctex_project_container_get_operation_record(
    const void* project_encoded, size_t project_encoded_size,
    const ctex_project_container_read_limits_descriptor* limits, const char* record_identifier,
    void* record_output, size_t record_output_size, size_t* out_required_size);
CTEX_API ctex_result ctex_project_container_assess_operation_replay(
    const void* project_encoded, size_t project_encoded_size,
    const ctex_project_container_read_limits_descriptor* limits,
    const ctex_operation_replay_assessment_descriptor* descriptor,
    ctex_project_operation_replay_info* out_info, char* report_output, size_t report_output_size);

/* Retains decal, text, and surface-path source data independently of rasterization. */
CTEX_API ctex_result ctex_texture_set_editable_entry_add(
    ctex_document* document, const char* texture_set_id,
    const ctex_editable_entry_descriptor* descriptor, ctex_editable_entry_info* out_info,
    char* report_output, size_t report_output_size);
CTEX_API ctex_result ctex_texture_set_editable_entry_edit(
    ctex_document* document, const char* texture_set_id,
    const ctex_editable_entry_descriptor* descriptor, ctex_editable_entry_info* out_info,
    char* report_output, size_t report_output_size);
CTEX_API ctex_result ctex_texture_set_editable_entry_inspect(
    const ctex_document* document, const char* texture_set_id, const char* entry_identifier,
    ctex_editable_entry_info* out_info, char* report_output, size_t report_output_size);
CTEX_API ctex_result ctex_texture_set_editable_entry_plan_rasterization(
    const ctex_document* document, const char* texture_set_id, const char* entry_identifier,
    ctex_editable_entry_info* out_info, char* report_output, size_t report_output_size);
CTEX_API ctex_result ctex_texture_set_editable_entry_undo(ctex_document* document,
                                                          const char* texture_set_id,
                                                          ctex_editable_entry_info* out_info,
                                                          char* report_output,
                                                          size_t report_output_size);
CTEX_API ctex_result ctex_texture_set_editable_entry_redo(ctex_document* document,
                                                          const char* texture_set_id,
                                                          ctex_editable_entry_info* out_info,
                                                          char* report_output,
                                                          size_t report_output_size);
CTEX_API ctex_result ctex_texture_set_editable_surface_path_resolve(
    const ctex_document* document, const char* texture_set_id, const char* entry_identifier,
    const ctex_stroke_settings_descriptor* settings, ctex_resolved_stroke_info* out_info,
    ctex_resolved_stamp* stamps, size_t stamp_capacity, size_t* out_stamp_count,
    ctex_swept_segment* swept_segments, size_t swept_segment_capacity,
    size_t* out_swept_segment_count);

/* Saves and restores the editable store as one versioned project asset. */
CTEX_API ctex_result ctex_project_container_upsert_editable_authoring(
    const void* project_encoded, size_t project_encoded_size,
    const ctex_project_container_read_limits_descriptor* limits, const ctex_document* document,
    const char* texture_set_id, const char* asset_identifier, ctex_project_container_info* out_info,
    void* project_output, size_t project_output_size, char* report_output,
    size_t report_output_size);
CTEX_API ctex_result ctex_project_container_restore_editable_authoring(
    const void* project_encoded, size_t project_encoded_size,
    const ctex_project_container_read_limits_descriptor* limits, const char* asset_identifier,
    ctex_document* document, const char* texture_set_id, ctex_editable_entry_info* out_info,
    char* report_output, size_t report_output_size);

/*
 * Returns every built-in node schema and the documented scalar/vector math
 * formulas as deterministic NUL-terminated JSON. A null report with zero size
 * queries the exact byte count.
 */
CTEX_API ctex_result ctex_material_graph_get_builtin_catalogue(
    ctex_material_graph_catalogue_info* out_info, char* report_output, size_t report_output_size);

/* Creates the canonical graph containing the nine default material outputs. */
CTEX_API ctex_result ctex_material_graph_create_default(ctex_material_graph_info* out_info,
                                                        void* canonical_output,
                                                        size_t canonical_output_size,
                                                        char* report_output,
                                                        size_t report_output_size);

/* Validates, canonicalizes and inspects one serialized graph atomically. */
CTEX_API ctex_result ctex_material_graph_inspect(const void* serialized, size_t serialized_size,
                                                 ctex_material_graph_info* out_info,
                                                 void* canonical_output,
                                                 size_t canonical_output_size, char* report_output,
                                                 size_t report_output_size);

/* Compares complete canonical graph documents, including positions and values. */
CTEX_API ctex_result ctex_material_graph_compare(const void* left, size_t left_size,
                                                 const void* right, size_t right_size,
                                                 uint32_t* out_equal);

/* Adds one declared built-in node and returns its stable graph-local identity. */
CTEX_API ctex_result ctex_material_graph_add_builtin_node(
    const void* serialized, size_t serialized_size, const char* type_id, ctex_vec2f position,
    ctex_material_graph_info* out_info, uint64_t* out_node_id, void* canonical_output,
    size_t canonical_output_size);

/* Updates one typed unconnected input value without changing its declaration. */
CTEX_API ctex_result ctex_material_graph_set_input_value(
    const void* serialized, size_t serialized_size, uint64_t node_id, const char* input_id,
    const ctex_smart_material_value_descriptor* value, ctex_material_graph_info* out_info,
    void* canonical_output, size_t canonical_output_size);

/* Updates one typed node property without changing its declaration. */
CTEX_API ctex_result ctex_material_graph_set_property_value(
    const void* serialized, size_t serialized_size, uint64_t node_id, const char* property_id,
    const ctex_smart_material_value_descriptor* value, ctex_material_graph_info* out_info,
    void* canonical_output, size_t canonical_output_size);

/* Adds or replaces one input link and reports its coercion and prior source. */
CTEX_API ctex_result ctex_material_graph_add_link(
    const void* serialized, size_t serialized_size, const ctex_material_graph_link_descriptor* link,
    ctex_material_graph_link_info* out_info, void* canonical_output, size_t canonical_output_size,
    char* replaced_source_socket, size_t replaced_source_socket_size);

/* Validates a graph without emission and returns deterministic JSON diagnostics. */
CTEX_API ctex_result ctex_material_graph_validate(
    const void* serialized, size_t serialized_size,
    const ctex_material_graph_validation_resources_descriptor* resources,
    ctex_material_graph_validation_info* out_info, char* report_output, size_t report_output_size);

/* Creates the canonical empty named-material library. */
CTEX_API ctex_result ctex_material_graph_library_create_empty(
    ctex_material_graph_library_info* out_info, void* canonical_output,
    size_t canonical_output_size, char* report_output, size_t report_output_size);

/* Validates, canonicalizes and enumerates stable material preset identities. */
CTEX_API ctex_result ctex_material_graph_library_inspect(
    const void* serialized, size_t serialized_size, ctex_material_graph_library_info* out_info,
    void* canonical_output, size_t canonical_output_size, char* report_output,
    size_t report_output_size);

/* Saves one graph with a stable identity, display name and thumbnail resource. */
CTEX_API ctex_result ctex_material_graph_library_add_preset(
    const void* serialized, size_t serialized_size,
    const ctex_material_graph_preset_descriptor* preset, ctex_material_graph_library_info* out_info,
    void* canonical_output, size_t canonical_output_size, char* report_output,
    size_t report_output_size);

/* Resolves a stable preset identity to an independent canonical graph document. */
CTEX_API ctex_result ctex_material_graph_library_resolve_preset(
    const void* serialized, size_t serialized_size, const char* stable_id,
    ctex_material_graph_info* out_info, void* canonical_output, size_t canonical_output_size,
    char* report_output, size_t report_output_size);

/* Creates an allocator-owned workspace for reusable material node groups. */
CTEX_API ctex_result
ctex_material_graph_workspace_create(ctex_material_graph_workspace** out_workspace);
CTEX_API void ctex_material_graph_workspace_destroy(ctex_material_graph_workspace* workspace);

/* Returns the number of material graphs and reusable group definitions. */
CTEX_API ctex_result ctex_material_graph_workspace_get_info(
    const ctex_material_graph_workspace* workspace, ctex_material_graph_workspace_info* out_info);

/* Adds a canonical material graph under a stable workspace-local identifier. */
CTEX_API ctex_result ctex_material_graph_workspace_add_material(
    ctex_material_graph_workspace* workspace, const char* identifier, const void* serialized,
    size_t serialized_size);

/* Creates a reusable group and its editable input/output boundary nodes. */
CTEX_API ctex_result
ctex_material_graph_workspace_create_group(ctex_material_graph_workspace* workspace,
                                           const ctex_material_graph_group_descriptor* descriptor);

/* Places a group in a material or group graph; recursive placement is atomic. */
CTEX_API ctex_result ctex_material_graph_workspace_instantiate_group(
    ctex_material_graph_workspace* workspace, const char* group_identifier, uint32_t owner_kind,
    const char* owner_identifier, ctex_vec2f position, uint64_t* out_node_id);

/* Propagates an interface edit to every material and nested-group instance. */
CTEX_API ctex_result ctex_material_graph_workspace_update_group_interface(
    ctex_material_graph_workspace* workspace, const char* group_identifier,
    const ctex_material_graph_group_interface_descriptor* descriptor,
    ctex_material_graph_group_update_info* out_info);

/* Returns one material or group subgraph using the graph two-call contract. */
CTEX_API ctex_result ctex_material_graph_workspace_get_graph(
    const ctex_material_graph_workspace* workspace, uint32_t owner_kind,
    const char* owner_identifier, ctex_material_graph_info* out_info, void* canonical_output,
    size_t canonical_output_size, char* report_output, size_t report_output_size);

/* Edits a material or group subgraph without bypassing workspace ownership. */
CTEX_API ctex_result ctex_material_graph_workspace_add_builtin_node(
    ctex_material_graph_workspace* workspace, uint32_t owner_kind, const char* owner_identifier,
    const char* type_id, ctex_vec2f position, uint64_t* out_node_id);
CTEX_API ctex_result ctex_material_graph_workspace_set_input_value(
    ctex_material_graph_workspace* workspace, uint32_t owner_kind, const char* owner_identifier,
    uint64_t node_id, const char* input_id, const ctex_smart_material_value_descriptor* value);
CTEX_API ctex_result ctex_material_graph_workspace_set_property_value(
    ctex_material_graph_workspace* workspace, uint32_t owner_kind, const char* owner_identifier,
    uint64_t node_id, const char* property_id, const ctex_smart_material_value_descriptor* value);
CTEX_API ctex_result ctex_material_graph_workspace_add_link(
    ctex_material_graph_workspace* workspace, uint32_t owner_kind, const char* owner_identifier,
    const ctex_material_graph_link_descriptor* link, ctex_material_graph_link_info* out_info,
    char* replaced_source_socket, size_t replaced_source_socket_size);

/* Creates an isolated registry for host-provided material node declarations. */
CTEX_API ctex_result
ctex_material_graph_node_registry_create(ctex_material_graph_node_registry** out_registry);
CTEX_API void ctex_material_graph_node_registry_destroy(
    ctex_material_graph_node_registry* registry);
CTEX_API ctex_result
ctex_material_graph_node_registry_get_info(const ctex_material_graph_node_registry* registry,
                                           ctex_material_graph_node_registry_info* out_info);

/* Copies and validates a complete host-node reference contract before use. */
CTEX_API ctex_result ctex_material_graph_node_registry_register(
    ctex_material_graph_node_registry* registry,
    const ctex_material_graph_host_node_registration_descriptor* descriptor);

/* Adds one registered host node to a caller-owned canonical graph document. */
CTEX_API ctex_result ctex_material_graph_add_registered_node(
    const ctex_material_graph_node_registry* registry, const void* serialized,
    size_t serialized_size, const char* type_id, uint32_t type_version, ctex_vec2f position,
    ctex_material_graph_info* out_info, uint64_t* out_node_id, void* canonical_output,
    size_t canonical_output_size);

/* Adds one registered host node to a material or reusable group subgraph. */
CTEX_API ctex_result ctex_material_graph_workspace_add_registered_node(
    ctex_material_graph_workspace* workspace, const ctex_material_graph_node_registry* registry,
    uint32_t owner_kind, const char* owner_identifier, const char* type_id, uint32_t type_version,
    ctex_vec2f position, uint64_t* out_node_id);

/* Performs registry-aware validation and names opaque or stale node types. */
CTEX_API ctex_result ctex_material_graph_validate_registered(
    const ctex_material_graph_node_registry* registry, const void* serialized,
    size_t serialized_size, uint32_t target,
    const ctex_material_graph_validation_resources_descriptor* resources,
    ctex_material_graph_validation_info* out_info, char* report_output, size_t report_output_size);

/* Runs every CPU/emission parity fixture and reports replay eligibility. */
CTEX_API ctex_result ctex_material_graph_node_registry_verify_contract(
    const ctex_material_graph_node_registry* registry, const char* type_id, uint32_t type_version,
    const char* const* pinned_dependencies, size_t pinned_dependency_count,
    ctex_material_graph_host_contract_info* out_info, char* report_output,
    size_t report_output_size);

/*
 * Emits a headless material shader and its complete host pass plan. The node
 * registry is optional for built-in-only graphs. WGSL and HLSL return two
 * NUL-terminated text artifacts; MSL returns one unified NUL-terminated text
 * artifact in vertex_artifact; SPIR-V returns two raw byte modules. The JSON
 * outputs and all artifacts are published atomically with two-call sizing.
 */
CTEX_API ctex_result ctex_shader_emit_material(
    const ctex_material_graph_node_registry* registry, const void* graph_serialized,
    size_t graph_serialized_size, const ctex_shader_material_request* request,
    ctex_shader_material_info* out_info, void* vertex_artifact, size_t vertex_artifact_size,
    void* fragment_artifact, size_t fragment_artifact_size, char* pass_plan_output,
    size_t pass_plan_output_size, char* workaround_report_output,
    size_t workaround_report_output_size);

/* Creates a thread-safe cache for material, layer-stack, and preview emission results. */
CTEX_API ctex_result ctex_shader_emission_cache_create(ctex_shader_emission_cache** out_cache);
CTEX_API void ctex_shader_emission_cache_destroy(ctex_shader_emission_cache* cache);
CTEX_API ctex_result ctex_shader_emission_cache_get_info(const ctex_shader_emission_cache* cache,
                                                         ctex_shader_emission_cache_info* out_info);
CTEX_API ctex_result ctex_shader_emission_cache_clear(ctex_shader_emission_cache* cache);

/* The cached material route has the same outputs as ctex_shader_emit_material. */
CTEX_API ctex_result ctex_shader_emit_material_cached(
    ctex_shader_emission_cache* cache, const ctex_material_graph_node_registry* registry,
    const void* graph_serialized, size_t graph_serialized_size,
    const ctex_shader_material_request* request, ctex_shader_material_info* out_info,
    void* vertex_artifact, size_t vertex_artifact_size, void* fragment_artifact,
    size_t fragment_artifact_size, char* pass_plan_output, size_t pass_plan_output_size,
    char* workaround_report_output, size_t workaround_report_output_size, uint32_t* out_cache_hit);

/*
 * Emits a serialized graph or a named workspace material and publishes
 * structured node attribution beside text or binary artifacts. The cache and
 * registry are optional. SPIR-V reports binary_companion=1 because attribution
 * remains JSON metadata rather than pretending the module is source text.
 */
CTEX_API ctex_result ctex_shader_emit_material_inspectable(
    ctex_shader_emission_cache* cache, const ctex_material_graph_node_registry* registry,
    const ctex_shader_material_source_descriptor* source,
    const ctex_shader_material_request* request, ctex_shader_material_info* out_info,
    void* vertex_artifact, size_t vertex_artifact_size, void* fragment_artifact,
    size_t fragment_artifact_size, char* pass_plan_output, size_t pass_plan_output_size,
    char* workaround_report_output, size_t workaround_report_output_size,
    ctex_shader_material_debug_info* out_debug_info, char* debug_metadata_output,
    size_t debug_metadata_output_size, uint32_t* out_cache_hit);

/* Returns the pinned Kongruent backend identity, licence, source and revisions as JSON. */
CTEX_API ctex_result ctex_shader_get_backend_attribution(
    ctex_shader_backend_attribution_info* out_info, char* report_output, size_t report_output_size);

/*
 * Emits a bottom-to-top premultiplied layer stack. The packed artifact report
 * maps each pass identifier to its vertex and fragment byte ranges and states
 * whether those ranges contain NUL-terminated text or raw SPIR-V. A null cache
 * performs uncached emission. All four outputs publish atomically.
 */
CTEX_API ctex_result ctex_shader_emit_layer_stack(
    ctex_shader_emission_cache* cache, const ctex_shader_layer_stack_request* request,
    ctex_shader_layer_stack_info* out_info, void* artifact_blob, size_t artifact_blob_size,
    char* artifact_report_output, size_t artifact_report_output_size, char* pass_plan_output,
    size_t pass_plan_output_size, char* workaround_report_output,
    size_t workaround_report_output_size);

/* Emits a lit material preview with declared environment and analytic-light inputs. */
CTEX_API ctex_result ctex_shader_emit_lit_preview(
    ctex_shader_emission_cache* cache, const ctex_shader_preview_request* request,
    ctex_shader_preview_info* out_info, void* vertex_artifact, size_t vertex_artifact_size,
    void* fragment_artifact, size_t fragment_artifact_size, char* pass_plan_output,
    size_t pass_plan_output_size, char* workaround_report_output,
    size_t workaround_report_output_size);

/* Emits an unlit shader that displays exactly one requested semantic channel. */
CTEX_API ctex_result ctex_shader_emit_channel_inspection(
    ctex_shader_emission_cache* cache, const ctex_shader_preview_request* request,
    const char* semantic_id, ctex_shader_preview_info* out_info, void* vertex_artifact,
    size_t vertex_artifact_size, void* fragment_artifact, size_t fragment_artifact_size,
    char* pass_plan_output, size_t pass_plan_output_size, char* workaround_report_output,
    size_t workaround_report_output_size);

/*
 * Validates and migrates a canonical smart-material serialization. The output
 * and JSON inventory follow an atomic two-call sizing contract.
 */
CTEX_API ctex_result ctex_smart_material_inspect(const void* serialized, size_t serialized_size,
                                                 ctex_smart_material_info* out_info,
                                                 void* canonical_output,
                                                 size_t canonical_output_size, char* report_output,
                                                 size_t report_output_size);

/* Updates every graph binding driven by one typed exposed parameter. */
CTEX_API ctex_result ctex_smart_material_set_parameter(
    const void* serialized, size_t serialized_size, const char* parameter_identifier,
    const ctex_smart_material_value_descriptor* value, ctex_smart_material_info* out_info,
    void* canonical_output, size_t canonical_output_size, char* report_output,
    size_t report_output_size);

/* Marks or unmarks one stack entry as an anchor. */
CTEX_API ctex_result ctex_smart_material_set_anchor(const void* serialized, size_t serialized_size,
                                                    const char* entry_identifier, uint32_t marked,
                                                    ctex_smart_material_info* out_info,
                                                    void* canonical_output,
                                                    size_t canonical_output_size,
                                                    char* report_output, size_t report_output_size);

/* Adds one validated anchor dependency; ordering and cycles are refused atomically. */
CTEX_API ctex_result ctex_smart_material_add_anchor_reference(
    const void* serialized, size_t serialized_size, const char* anchor_entry_identifier,
    const char* consumer_entry_identifier, uint64_t consumer_node_id,
    const char* consumer_input_identifier, ctex_smart_material_info* out_info,
    void* canonical_output, size_t canonical_output_size, char* report_output,
    size_t report_output_size);

/* Returns a NUL-terminated JSON array in deterministic stack evaluation order. */
CTEX_API ctex_result ctex_smart_material_plan_anchor_evaluation(
    const void* serialized, size_t serialized_size, const char* const* changed_anchor_identifiers,
    size_t changed_anchor_count, char* output, size_t output_size, size_t* out_required_size);

/* Packages a canonical smart material with its declared portable resources. */
CTEX_API ctex_result ctex_smart_material_package(
    const void* serialized, size_t serialized_size,
    const ctex_project_resource_descriptor* resources, size_t resource_count,
    const ctex_project_asset_export_options_descriptor* options,
    ctex_project_container_info* out_info, void* package_output, size_t package_output_size,
    char* report_output, size_t report_output_size);

/* Imports a package and reports each image input as packed, referenced, or missing. */
CTEX_API ctex_result ctex_smart_material_import(
    const void* package_encoded, size_t package_encoded_size,
    const ctex_project_container_read_limits_descriptor* limits,
    const ctex_project_asset_search_paths_descriptor* search_paths,
    ctex_smart_material_info* out_info, void* canonical_output, size_t canonical_output_size,
    char* report_output, size_t report_output_size);

/* Validates and enumerates every named shelf and preset in stable order. */
CTEX_API ctex_result ctex_preset_library_enumerate(const ctex_preset_library_descriptor* library,
                                                   ctex_preset_library_info* out_info,
                                                   char* report_output, size_t report_output_size);

/* Resolves one globally stable preset identity into a standalone package. */
CTEX_API ctex_result ctex_preset_library_resolve(const ctex_preset_library_descriptor* library,
                                                 const char* preset_identifier,
                                                 ctex_project_container_info* out_info,
                                                 void* package_output, size_t package_output_size,
                                                 char* report_output, size_t report_output_size);

/*
 * Initializes the current stroke settings descriptor to the canonical defaults.
 * Response mappings use their built-in linear curve while points is NULL and
 * point_count is zero. Callers may replace any mapping with borrowed points.
 */
CTEX_API ctex_result ctex_stroke_settings_init(ctex_stroke_settings_descriptor* out_settings);

/*
 * Resolves borrowed timestamped input samples into ordered stamps and optional
 * swept segments. Pass NULL arrays with zero capacities to query the exact
 * counts. On success, each stamp's tip_resource_identity points to the string
 * borrowed from settings and remains valid for the same duration.
 */
CTEX_API ctex_result ctex_stroke_resolve(
    const ctex_stroke_settings_descriptor* settings, const ctex_stroke_input_sample* samples,
    size_t sample_count, ctex_resolved_stroke_info* out_info, ctex_resolved_stamp* stamps,
    size_t stamp_capacity, size_t* out_stamp_count, ctex_swept_segment* swept_segments,
    size_t swept_segment_capacity, size_t* out_swept_segment_count);

/* Serializes a named current-schema stroke preset into canonical bytes. */
CTEX_API ctex_result ctex_stroke_preset_serialize(const char* name,
                                                  const ctex_stroke_settings_descriptor* settings,
                                                  char* serialized_buffer,
                                                  size_t serialized_buffer_size,
                                                  size_t* out_required_size);

/*
 * Validates and migrates canonical stroke-preset bytes. Pass NULL for both
 * out_settings and buffers to query the storage requirements in out_info.
 * A filling call points the returned settings at the supplied tip and curve
 * buffers; all output storage remains owned by the caller.
 */
CTEX_API ctex_result ctex_stroke_preset_deserialize(
    const char* serialized, size_t serialized_size, ctex_stroke_preset_info* out_info,
    ctex_stroke_settings_descriptor* out_settings,
    const ctex_stroke_preset_buffers_descriptor* buffers);

/*
 * Rasterizes one bounded UV tile and evaluates geometric stroke coverage.
 * Resolved stamps are consumed exactly as supplied; spacing, taper and jitter
 * are not applied again. Use a NULL output with zero capacity to query count.
 */
CTEX_API ctex_result ctex_paint_evaluate_tile_coverage(
    const ctex_mesh* mesh, const ctex_paint_tile_coverage_descriptor* tile,
    const ctex_resolved_stroke_descriptor* stroke, double* coverage, size_t coverage_capacity,
    size_t* out_coverage_count);

/* Evaluates UV, triplanar or planar material coordinates for one UV tile. */
CTEX_API ctex_result ctex_paint_evaluate_material_coordinates(
    const ctex_mesh* mesh, const ctex_paint_tile_coverage_descriptor* tile,
    const ctex_paint_material_coordinate_descriptor* descriptor,
    ctex_paint_material_coordinate_sample* samples, size_t sample_capacity,
    size_t* out_sample_count);

/* Initializes canonical depth, angle and backface rejection settings. */
CTEX_API ctex_result ctex_paint_rejection_init(ctex_paint_rejection_descriptor* out_rejection);

/* Applies depth, angle and backface rejection to geometric tile coverage. */
CTEX_API ctex_result ctex_paint_evaluate_rejected_coverage(
    const ctex_mesh* mesh, const ctex_paint_tile_coverage_descriptor* tile,
    const ctex_resolved_stroke_descriptor* stroke, const ctex_paint_rejection_descriptor* rejection,
    ctex_paint_rejection_info* out_info, double* coverage, size_t coverage_capacity,
    size_t* out_coverage_count);

/* Initializes the canonical storage-tile size and seam-dilation radius. */
CTEX_API ctex_result ctex_paint_work_init(ctex_paint_work_descriptor* out_work);

/* Plans and reports the exact storage tiles reachable by supplied footprints. */
CTEX_API ctex_result ctex_paint_plan_work(const ctex_paint_work_descriptor* work,
                                          ctex_paint_work_info* out_info,
                                          ctex_paint_tile_coordinate* processed_tiles,
                                          size_t processed_tile_capacity,
                                          size_t* out_processed_tile_count);

/* Initializes the canonical two-texel seam-dilation radius. */
CTEX_API ctex_result
ctex_paint_seam_dilation_init(ctex_paint_seam_dilation_descriptor* out_dilation);

/* Extrapolates covered source values into a UV seam gutter. */
CTEX_API ctex_result ctex_paint_dilate_uv_seams(const ctex_paint_seam_dilation_descriptor* dilation,
                                                ctex_paint_seam_dilation_info* out_info,
                                                double* pixels, size_t pixel_capacity,
                                                size_t* out_pixel_count);

/* Creates one allocator-routed deferred seam-dilation session. */
CTEX_API ctex_result ctex_paint_dilation_session_create(uint32_t radius,
                                                        ctex_paint_dilation_session** out_session);
CTEX_API void ctex_paint_dilation_session_destroy(ctex_paint_dilation_session* session);

/* Stages or replaces the latest pixels for one dirtied UV tile. */
CTEX_API ctex_result ctex_paint_dilation_session_stage_tile(
    ctex_paint_dilation_session* session, const ctex_paint_dilation_tile_descriptor* tile);

/* Returns an undilated provisional snapshot without finishing the session. */
CTEX_API ctex_result ctex_paint_dilation_session_get_preview(
    const ctex_paint_dilation_session* session, ctex_paint_dilation_session_info* out_info,
    ctex_paint_dilation_tile_info* tiles, size_t tile_capacity, size_t* out_tile_count,
    double* pixels, size_t pixel_capacity, size_t* out_pixel_count);

/* Finalizes every staged tile once and returns the idempotent final snapshot. */
CTEX_API ctex_result ctex_paint_dilation_session_finish(
    ctex_paint_dilation_session* session, ctex_paint_dilation_session_info* out_info,
    ctex_paint_dilation_tile_info* tiles, size_t tile_capacity, size_t* out_tile_count,
    double* pixels, size_t pixel_capacity, size_t* out_pixel_count);

/* Applies explicit surface-adjacent taps to scalar or tangent-vector values. */
CTEX_API ctex_result
ctex_paint_filter_surface_scalar(const ctex_paint_surface_filter_descriptor* filter,
                                 const double* values, size_t value_count, double* out_value);
CTEX_API ctex_result ctex_paint_filter_surface_tangent_vector(
    const ctex_paint_surface_filter_descriptor* filter, const ctex_vec3d* values,
    size_t value_count, ctex_vec3d* out_value);

/* Plans owned gutters and reports unsupported mip levels through flattened arrays. */
CTEX_API ctex_result ctex_paint_plan_island_padding(
    const ctex_paint_island_padding_descriptor* padding, ctex_paint_island_padding_info* out_info,
    uint32_t* ownership, size_t ownership_capacity, size_t* out_ownership_count,
    ctex_paint_unsupported_mip_level* unsupported_mip_levels, size_t unsupported_mip_level_capacity,
    size_t* out_unsupported_mip_level_count, uint32_t* affected_islands,
    size_t affected_island_capacity, size_t* out_affected_island_count);

/* Applies the deterministic plan without overwriting valid or contested island texels. */
CTEX_API ctex_result ctex_paint_apply_island_padding(
    const ctex_paint_island_padding_descriptor* padding, ctex_paint_island_padding_info* out_info,
    double* pixels, size_t pixel_capacity, size_t* out_pixel_count);

/* Owns allocator-routed immutable surface maps shared by paint operations. */
CTEX_API ctex_result ctex_paint_surface_map_cache_create(ctex_paint_surface_map_cache** out_cache);
CTEX_API void ctex_paint_surface_map_cache_destroy(ctex_paint_surface_map_cache* cache);
CTEX_API ctex_result ctex_paint_surface_map_cache_clear(ctex_paint_surface_map_cache* cache);
CTEX_API ctex_result ctex_paint_surface_map_cache_get_statistics(
    const ctex_paint_surface_map_cache* cache, ctex_paint_surface_map_statistics* out_statistics);
CTEX_API ctex_result ctex_paint_surface_map_cache_lookup(
    ctex_paint_surface_map_cache* cache, const ctex_mesh* mesh,
    const ctex_paint_surface_map_request* request, ctex_paint_surface_map_info* out_info,
    const ctex_paint_surface_map_buffers* buffers);

/* Intersects every active normalized mask for one bounded tile. */
CTEX_API ctex_result ctex_paint_combine_masks(uint32_t width, uint32_t height,
                                              const ctex_paint_mask_inputs_descriptor* masks,
                                              ctex_paint_mask_info* out_info, double* combined_mask,
                                              size_t combined_mask_capacity,
                                              size_t* out_combined_mask_count);

/* Evaluates per-stamp deposition and alpha discard for one bounded UV tile. */
CTEX_API ctex_result ctex_paint_evaluate_tile_deposition(
    const ctex_mesh* mesh, const ctex_paint_tile_coverage_descriptor* tile,
    const ctex_resolved_stroke_descriptor* stroke,
    const ctex_paint_deposition_descriptor* deposition, ctex_paint_deposition_info* out_info,
    ctex_paint_deposition_sample* samples, size_t sample_capacity, size_t* out_sample_count);

/*
 * Blends one deposited tile against caller-owned stroke-start pixels. Samples
 * whose deposition write flag is zero preserve the corresponding snapshot
 * pixel. Pass NULL output with zero capacity to query the required count.
 */
CTEX_API ctex_result ctex_paint_blend_snapshot(const ctex_paint_blend_descriptor* descriptor,
                                               ctex_vec4f* pixels, size_t pixel_capacity,
                                               size_t* out_pixel_count);

/*
 * Applies one deposited stroke to every enabled layer channel in layer order.
 * A null output array with count zero queries the exact output shape in info.
 */
CTEX_API ctex_result ctex_paint_apply_brush(const ctex_paint_brush_descriptor* descriptor,
                                            ctex_paint_brush_info* out_info,
                                            const ctex_paint_tool_channel_output* output_channels,
                                            size_t output_channel_count);

/* Reduces layer opacity or mask values through the same deposited stroke. */
CTEX_API ctex_result ctex_paint_apply_eraser(const ctex_paint_eraser_descriptor* descriptor,
                                             ctex_paint_eraser_info* out_info, double* values,
                                             size_t value_capacity);

/* Resolves one of the six fill scopes and shades every enabled channel atomically. */
CTEX_API ctex_result ctex_paint_apply_fill(const ctex_paint_fill_descriptor* descriptor,
                                           ctex_paint_fill_info* out_info,
                                           const ctex_paint_fill_outputs* outputs);

/* Copies an immutable source snapshot through aligned or fixed UV mapping. */
CTEX_API ctex_result ctex_paint_apply_clone(const ctex_paint_clone_descriptor* descriptor,
                                            ctex_paint_clone_info* out_info,
                                            const ctex_paint_clone_outputs* outputs);

/* Filters immutable stroke-start channels with explicit surface-aware neighborhoods. */
CTEX_API ctex_result ctex_paint_apply_blur(const ctex_paint_blur_descriptor* descriptor,
                                           ctex_paint_blur_info* out_info,
                                           const ctex_paint_tool_channel_output* output_channels,
                                           size_t output_channel_count);

/* Drags immutable stroke-start channels through one upstream mapping per texel. */
CTEX_API ctex_result ctex_paint_apply_smear(const ctex_paint_smear_descriptor* descriptor,
                                            ctex_paint_smear_info* out_info,
                                            const ctex_paint_tool_channel_output* output_channels,
                                            size_t output_channel_count);

/* Resolves a screen-anchored stencil for use as a canonical paint mask. */
CTEX_API ctex_result ctex_paint_resolve_stencil_mask(
    const ctex_paint_stencil_descriptor* descriptor, ctex_paint_stencil_info* out_info,
    double* mask_values, size_t mask_value_capacity);

/* Rasterizes a retained surface placement and pinned decal material explicitly. */
CTEX_API ctex_result ctex_paint_rasterize_decal(const ctex_paint_decal_descriptor* descriptor,
                                                ctex_paint_decal_info* out_info,
                                                const ctex_paint_decal_outputs* outputs);

/* Projects a pinned material through a camera, planar frame or triplanar mapping. */
CTEX_API ctex_result ctex_paint_apply_projection(const ctex_paint_projection_descriptor* descriptor,
                                                 ctex_paint_projection_info* out_info,
                                                 const ctex_paint_projection_outputs* outputs);

/* Rasterizes length-delimited UTF-8 from a supplied font and applies it as a decal. */
CTEX_API ctex_result ctex_paint_apply_text(const ctex_paint_text_descriptor* descriptor,
                                           ctex_paint_text_info* out_info,
                                           const ctex_paint_text_outputs* outputs);

/* Simulates deterministic mesh-colliding particles and shades mapped contacts. */
CTEX_API ctex_result ctex_paint_apply_particles(ctex_pick_index* index,
                                                const ctex_paint_particle_descriptor* descriptor,
                                                ctex_paint_particle_info* out_info,
                                                const ctex_paint_particle_outputs* outputs);

/* Reads enabled channel values and optional material provenance at a surface hit. */
CTEX_API ctex_result ctex_paint_pick_enabled_channels(
    const ctex_paint_picker_descriptor* descriptor, ctex_paint_picker_info* out_info,
    ctex_paint_picker_channel_value* channels, size_t channel_capacity, char* strings,
    size_t string_capacity);

/* Selects a linear-RGB colour-ID region for paint, masking or visibility use. */
CTEX_API ctex_result ctex_paint_select_colour_id(const ctex_paint_colour_id_descriptor* descriptor,
                                                 ctex_paint_colour_id_info* out_info,
                                                 double* values, size_t value_capacity);

/* Enumerates every numeric paint parameter and its contextual finite range. */
CTEX_API ctex_result ctex_paint_get_parameter_catalogue(
    ctex_paint_parameter_descriptor* parameters, size_t parameter_capacity, char* names,
    size_t name_capacity, ctex_paint_parameter_catalogue_info* out_info);

/* Resolves one supplied parameter through the same validator used by paint tools. */
CTEX_API ctex_result ctex_paint_validate_parameter(const char* name, uint32_t context,
                                                   double supplied,
                                                   ctex_paint_parameter_validation_info* out_info);

/* Selects surface texels through a clipped screen rectangle or lasso. */
CTEX_API ctex_result ctex_paint_select_screen(
    ctex_pick_index* index, const ctex_paint_screen_selection_descriptor* descriptor,
    ctex_paint_selection_info* out_info, const ctex_paint_selection_outputs* outputs);

/* Selects a triangle, UV island or connected-by-angle polygon region. */
CTEX_API ctex_result ctex_paint_select_polygon(
    const ctex_paint_polygon_selection_descriptor* descriptor, ctex_paint_selection_info* out_info,
    const ctex_paint_selection_outputs* outputs);

/*
 * Installs one process-wide sink. Pass NULL to uninstall it. The callback can
 * be invoked concurrently from calling or library worker threads and must not
 * throw across the C boundary. The host keeps user_data valid until all calls
 * that could have observed the sink finish. CyberTexel never writes logs to a
 * stream.
 */
CTEX_API ctex_result ctex_set_log_sink(const ctex_log_sink_descriptor* descriptor);

/*
 * Sets the process-wide allocator captured by subsequently created objects.
 * Pass NULL to restore the library default. The callbacks can run on calling
 * or library worker threads and must not throw across the C boundary. The host
 * keeps user_data valid until every object created from this configuration has
 * been destroyed.
 */
CTEX_API ctex_result ctex_set_allocator(const ctex_allocator_descriptor* descriptor);

/*
 * Mesh creation copies the supplied arrays and strings. The inputs are never
 * modified and need only remain valid for the duration of the call. Replacement
 * is atomic: a rejected descriptor leaves the mesh and its revision unchanged.
 */
CTEX_API ctex_result ctex_mesh_create(const ctex_mesh_descriptor* descriptor, ctex_mesh** out_mesh);
CTEX_API void ctex_mesh_destroy(ctex_mesh* mesh);
CTEX_API ctex_result ctex_mesh_replace(ctex_mesh* mesh, const ctex_mesh_descriptor* descriptor);
CTEX_API ctex_result ctex_mesh_get_info(const ctex_mesh* mesh, ctex_mesh_info* out_info);
CTEX_API ctex_result ctex_mesh_get_uv_set_names(const ctex_mesh* mesh, char* buffer,
                                                size_t buffer_size, size_t* out_required_size,
                                                size_t* out_count);
/* Reports positive-area UV overlap within one texture-set partition. */
CTEX_API ctex_result ctex_mesh_analyze_uv_overlaps(const ctex_mesh* mesh, const char* uv_set,
                                                   uint32_t partition_index,
                                                   ctex_mesh_uv_overlap_info* out_info,
                                                   uint32_t* face_indices,
                                                   size_t face_index_capacity);
/* Samples non-UDIM unit-square coverage at texture-texel centres. */
CTEX_API ctex_result ctex_mesh_analyze_uv_coverage(const ctex_mesh* mesh, const char* uv_set,
                                                   uint32_t partition_index, uint32_t width,
                                                   uint32_t height,
                                                   ctex_mesh_uv_coverage_info* out_info,
                                                   uint32_t* outside_face_indices,
                                                   size_t outside_face_index_capacity);

/*
 * A replacement plan owns a validated copy of the proposed mesh and compares it
 * with every texture set in the document. Query it before supplying exactly one
 * policy for each entry whose uv_change is not CTEX_MESH_UV_UNCHANGED.
 *
 * Apply is atomic with mesh publication. If any set requests reprojection, no
 * channel is cleared and the mesh is not replaced; replacement_applied is zero
 * and the plan remains reusable. The document and mesh must outlive the plan.
 */
CTEX_API ctex_result ctex_mesh_replacement_plan_create(ctex_document* document, ctex_mesh* mesh,
                                                       const ctex_mesh_descriptor* replacement,
                                                       ctex_mesh_replacement_plan** out_plan);
CTEX_API ctex_result ctex_mesh_replacement_plan_create_with_tangent_data(
    ctex_document* document, ctex_mesh* mesh, const ctex_mesh_descriptor* replacement,
    const ctex_mesh_tangent_data_descriptor* tangents, ctex_mesh_replacement_plan** out_plan);
CTEX_API void ctex_mesh_replacement_plan_destroy(ctex_mesh_replacement_plan* plan);
CTEX_API ctex_result ctex_mesh_replacement_plan_get_info(const ctex_mesh_replacement_plan* plan,
                                                         ctex_mesh_replacement_plan_info* out_info,
                                                         ctex_mesh_replacement_entry* entries,
                                                         size_t entry_capacity,
                                                         char* texture_set_ids,
                                                         size_t texture_set_id_size);
CTEX_API ctex_result ctex_mesh_replacement_plan_apply(
    ctex_mesh_replacement_plan* plan, const ctex_mesh_replacement_decision* decisions,
    size_t decision_count, ctex_mesh_replacement_apply_info* out_info);

/*
 * These variants copy one tangent per triangle corner and retain the complete
 * declared frame. Ordinary mesh creation generates the pinned default frame.
 */
CTEX_API ctex_result ctex_mesh_create_with_tangent_data(
    const ctex_mesh_descriptor* descriptor, const ctex_mesh_tangent_data_descriptor* tangents,
    ctex_mesh** out_mesh);
CTEX_API ctex_result
ctex_mesh_replace_with_tangent_data(ctex_mesh* mesh, const ctex_mesh_descriptor* descriptor,
                                    const ctex_mesh_tangent_data_descriptor* tangents);
CTEX_API ctex_result ctex_mesh_get_tangent_frame(const ctex_mesh* mesh,
                                                 ctex_mesh_tangent_frame_info* out_info,
                                                 char* uv_set, size_t uv_set_size);

/*
 * Mesh-map sets are bound to one document texture set and one mesh revision.
 * The document and mesh must outlive the set. Imported pixel buffers are copied;
 * all other caller storage is borrowed only for the call.
 */
CTEX_API ctex_result ctex_mesh_map_set_create(ctex_document* document, const char* texture_set_id,
                                              const ctex_mesh* mesh,
                                              ctex_mesh_map_set** out_map_set);
CTEX_API void ctex_mesh_map_set_destroy(ctex_mesh_map_set* map_set);
CTEX_API ctex_result ctex_mesh_map_kind_get_name(uint32_t kind, char* buffer, size_t buffer_size,
                                                 size_t* out_required_size);
CTEX_API ctex_result ctex_mesh_map_set_get_info(const ctex_mesh_map_set* map_set,
                                                ctex_mesh_map_set_info* out_info,
                                                char* texture_set_id, size_t texture_set_id_size,
                                                char* uv_set, size_t uv_set_size);
CTEX_API ctex_result ctex_mesh_map_set_get_entries(const ctex_mesh_map_set* map_set,
                                                   ctex_mesh_map_entry_info* entries,
                                                   size_t entry_capacity, size_t* out_entry_count);
CTEX_API ctex_result ctex_mesh_map_set_import_external(
    ctex_mesh_map_set* map_set, const ctex_mesh_map_import_descriptor* descriptor,
    ctex_mesh_map_import_info* out_info);
CTEX_API ctex_result ctex_mesh_map_set_sample(const ctex_mesh_map_set* map_set, uint32_t kind,
                                              double u, double v,
                                              ctex_mesh_map_sample_info* out_sample);
CTEX_API ctex_result ctex_mesh_map_set_check_requirements(
    const ctex_mesh_map_set* map_set, const char* consumer, const uint32_t* required_maps,
    size_t required_map_count, uint32_t* missing_maps, size_t missing_map_capacity,
    ctex_mesh_map_staleness* stale_maps, size_t stale_map_capacity,
    ctex_mesh_map_requirement_info* out_info, char* message, size_t message_size);
CTEX_API ctex_result ctex_mesh_map_set_synchronize_mesh(ctex_mesh_map_set* map_set,
                                                        const ctex_mesh* mesh,
                                                        ctex_mesh_map_staleness* stale_maps,
                                                        size_t stale_map_capacity,
                                                        size_t* out_stale_map_count);
CTEX_API ctex_result ctex_mesh_map_set_release(ctex_mesh_map_set* map_set, uint32_t kind,
                                               ctex_mesh_map_release_info* out_info);
CTEX_API ctex_result ctex_mesh_map_set_release_all(ctex_mesh_map_set* map_set,
                                                   ctex_mesh_map_release_info* out_info);

/* Enumerates and evaluates deterministic CPU mesh-map mask generators. */
CTEX_API ctex_result ctex_mesh_map_generator_get_info(
    uint32_t kind, ctex_mesh_map_generator_info* out_info, uint32_t* required_maps,
    size_t required_map_capacity, ctex_mesh_map_generator_parameter_descriptor* parameters,
    size_t parameter_capacity, char* strings, size_t string_capacity);
CTEX_API ctex_result ctex_mesh_map_generator_generate(
    const ctex_mesh_map_set* map_set, uint32_t kind, uint32_t width, uint32_t height,
    const ctex_mesh_map_generator_parameter* parameters, size_t parameter_count,
    ctex_mesh_map_generator_result_info* out_info, float* mask_values, size_t mask_value_capacity,
    ctex_mesh_map_generator_resolved_parameter* resolved_parameters,
    size_t resolved_parameter_capacity, ctex_mesh_map_generator_parameter_clamp* parameter_clamps,
    size_t parameter_clamp_capacity, ctex_mesh_map_staleness* stale_maps, size_t stale_map_capacity,
    char* strings, size_t string_capacity);

/* Invokes a host-owned baker synchronously; CyberTexel only validates and copies its output. */
CTEX_API ctex_result ctex_mesh_map_set_request_bake(
    ctex_mesh_map_set* map_set, const ctex_mesh_map_bake_provider_descriptor* provider,
    uint32_t kind, uint32_t width, uint32_t height, uint64_t bake_settings_revision,
    uint64_t request_generation, const ctex_mesh_map_bake_control_descriptor* control,
    ctex_mesh_map_bake_result_info* out_info);

/* Versioned asynchronous requests retain no provider and publish only explicit completion data. */
CTEX_API ctex_result ctex_mesh_map_bake_session_create(ctex_mesh_map_set* map_set,
                                                       uint64_t initial_settings_revision,
                                                       ctex_mesh_map_bake_session** out_session);
CTEX_API void ctex_mesh_map_bake_session_destroy(ctex_mesh_map_bake_session* session);
CTEX_API ctex_result ctex_mesh_map_bake_session_get_info(const ctex_mesh_map_bake_session* session,
                                                         ctex_mesh_map_bake_session_info* out_info);
CTEX_API ctex_result ctex_mesh_map_bake_session_begin(ctex_mesh_map_bake_session* session,
                                                      uint32_t kind, uint32_t width,
                                                      uint32_t height,
                                                      ctex_mesh_map_bake_request_token** out_token);
CTEX_API ctex_result ctex_mesh_map_bake_session_cancel(
    ctex_mesh_map_bake_session* session, const ctex_mesh_map_bake_request_token* token,
    uint32_t* out_cancelled);
CTEX_API ctex_result ctex_mesh_map_bake_session_complete(
    ctex_mesh_map_bake_session* session, const ctex_mesh_map_bake_request_token* token,
    const ctex_mesh_map_bake_output_descriptor* output,
    ctex_mesh_map_bake_completion_info* out_info);
CTEX_API ctex_result
ctex_mesh_map_bake_session_edit_settings(ctex_mesh_map_bake_session* session, uint64_t revision,
                                         ctex_mesh_map_bake_settings_edit_info* out_info);
CTEX_API ctex_result ctex_mesh_map_bake_session_undo_settings(
    ctex_mesh_map_bake_session* session, ctex_mesh_map_bake_settings_undo_info* out_info);
CTEX_API void ctex_mesh_map_bake_request_token_destroy(ctex_mesh_map_bake_request_token* token);
CTEX_API ctex_result ctex_mesh_map_bake_request_token_get_info(
    const ctex_mesh_map_bake_request_token* token, ctex_mesh_map_bake_token_info* out_info,
    char* texture_set_id, size_t texture_set_id_size, char* uv_set, size_t uv_set_size,
    char* tangent_uv_set, size_t tangent_uv_set_size);

/*
 * Picking indexes retain a non-owning mesh reference; destroy them before the
 * mesh. Queries synchronize automatically after ctex_mesh_replace. All result
 * arrays and packed texture-set strings are caller-owned and copied atomically.
 * Counter-clockwise triangles are front-facing when viewed from the side their
 * geometric normal points toward. Shared edges resolve to the lowest triangle
 * index at an equivalent distance. out_info is required and its size field must
 * be initialized. Null result buffers query exact counts; provided short
 * buffers fail without a partial write. Texture-set identifiers are packed as
 * NUL-terminated UTF-8 strings addressed by each hit's offset and size. Batch
 * output contains one entry per ray and marks misses with has_hit == 0.
 */
CTEX_API ctex_result ctex_pick_index_create(ctex_mesh* mesh, ctex_pick_index** out_index);
CTEX_API void ctex_pick_index_destroy(ctex_pick_index* index);
CTEX_API ctex_result ctex_pick_index_get_info(const ctex_pick_index* index,
                                              ctex_pick_index_info* out_info);
CTEX_API ctex_result ctex_uv_pick_index_create(ctex_mesh* mesh, const char* uv_set,
                                               ctex_uv_pick_index** out_index);
CTEX_API void ctex_uv_pick_index_destroy(ctex_uv_pick_index* index);
CTEX_API ctex_result ctex_uv_pick_index_get_info(const ctex_uv_pick_index* index,
                                                 ctex_pick_index_info* out_info);
CTEX_API ctex_result ctex_pick_ray_from_screen(ctex_vec2f position,
                                               const ctex_pick_screen_view_descriptor* view,
                                               uint32_t projection_kind, ctex_pick_ray* out_ray);
CTEX_API ctex_result ctex_pick_ray_query(
    ctex_pick_index* index, ctex_pick_ray ray, const ctex_pick_options_descriptor* options,
    const ctex_pick_texture_set_binding_descriptor* texture_sets, size_t texture_set_count,
    ctex_pick_hit* hits, size_t hit_capacity, char* texture_set_ids,
    size_t texture_set_id_buffer_size, ctex_pick_query_info* out_info);
CTEX_API ctex_result ctex_pick_uv_query(ctex_uv_pick_index* index, ctex_vec2f coordinate,
                                        const ctex_pick_texture_set_binding_descriptor* texture_set,
                                        ctex_pick_hit* hit, char* texture_set_id,
                                        size_t texture_set_id_buffer_size,
                                        ctex_pick_query_info* out_info);
CTEX_API ctex_result
ctex_pick_snap_to_surface(ctex_pick_index* index, ctex_vec3f point, float maximum_distance,
                          const ctex_pick_texture_set_binding_descriptor* texture_sets,
                          size_t texture_set_count, ctex_pick_hit* hit, char* texture_set_id,
                          size_t texture_set_id_buffer_size, ctex_pick_query_info* out_info);
CTEX_API ctex_result ctex_pick_query_screen_rectangle(ctex_pick_index* index, ctex_vec2f minimum,
                                                      ctex_vec2f maximum,
                                                      const ctex_pick_screen_view_descriptor* view,
                                                      uint32_t* triangle_indices,
                                                      size_t triangle_capacity,
                                                      ctex_pick_query_info* out_info);
CTEX_API ctex_result ctex_pick_query_screen_lasso(ctex_pick_index* index, const ctex_vec2f* points,
                                                  size_t point_count,
                                                  const ctex_pick_screen_view_descriptor* view,
                                                  uint32_t* triangle_indices,
                                                  size_t triangle_capacity,
                                                  ctex_pick_query_info* out_info);
CTEX_API ctex_result ctex_pick_query_world_sphere(ctex_pick_index* index, ctex_vec3f center,
                                                  float radius, uint32_t* triangle_indices,
                                                  size_t triangle_capacity,
                                                  ctex_pick_query_info* out_info);
CTEX_API ctex_result ctex_pick_query_world_box(ctex_pick_index* index, ctex_vec3f minimum,
                                               ctex_vec3f maximum, uint32_t* triangle_indices,
                                               size_t triangle_capacity,
                                               ctex_pick_query_info* out_info);
CTEX_API ctex_result ctex_pick_nearest_batch(
    ctex_pick_index* index, const ctex_pick_ray* rays, size_t ray_count, float maximum_distance,
    uint32_t backface_policy, const ctex_pick_texture_set_binding_descriptor* texture_sets,
    size_t texture_set_count, const ctex_pick_batch_control_descriptor* control,
    ctex_pick_hit* hits, size_t hit_capacity, char* texture_set_ids,
    size_t texture_set_id_buffer_size, ctex_pick_batch_info* out_info);

CTEX_API ctex_result ctex_document_create(ctex_document** out_document);
CTEX_API void ctex_document_destroy(ctex_document* document);
CTEX_API ctex_result ctex_document_create_texture_set(
    ctex_document* document, const ctex_texture_set_descriptor* descriptor);
CTEX_API ctex_result ctex_document_create_texture_sets_from_mesh(ctex_document* document,
                                                                 const ctex_mesh* mesh,
                                                                 const char* uv_set, uint32_t width,
                                                                 uint32_t height,
                                                                 uint8_t default_bit_depth);
CTEX_API ctex_result ctex_document_get_texture_set_ids(const ctex_document* document, char* buffer,
                                                       size_t buffer_size,
                                                       size_t* out_required_size,
                                                       size_t* out_count);
CTEX_API ctex_result ctex_document_create_atlas(ctex_document* document,
                                                const ctex_atlas_descriptor* descriptor);
CTEX_API ctex_result ctex_document_get_atlas_ids(const ctex_document* document, char* buffer,
                                                 size_t buffer_size, size_t* out_required_size,
                                                 size_t* out_count);
CTEX_API ctex_result ctex_document_get_atlas(const ctex_document* document, const char* atlas_id,
                                             ctex_atlas_info* out_info, ctex_atlas_region* regions,
                                             size_t region_capacity, char* display_name,
                                             size_t display_name_size, char* texture_set_ids,
                                             size_t texture_set_id_size);
CTEX_API ctex_result ctex_texture_set_get_channel_ids(const ctex_document* document,
                                                      const char* texture_set_id, char* buffer,
                                                      size_t buffer_size, size_t* out_required_size,
                                                      size_t* out_count);
CTEX_API ctex_result ctex_texture_set_register_channel(ctex_document* document,
                                                       const char* texture_set_id,
                                                       const ctex_channel_descriptor* descriptor);
CTEX_API ctex_result ctex_texture_set_set_channel_enabled(ctex_document* document,
                                                          const char* texture_set_id,
                                                          const char* semantic_id, uint32_t enabled,
                                                          uint32_t bit_depth_override);
CTEX_API ctex_result ctex_texture_set_get_channel_info(
    const ctex_document* document, const char* texture_set_id, const char* semantic_id,
    ctex_channel_info* out_info, char* export_mapping_buffer, size_t export_mapping_buffer_size,
    size_t* out_required_export_mapping_size);
CTEX_API ctex_result ctex_texture_set_get_memory_report(const ctex_document* document,
                                                        const char* texture_set_id,
                                                        ctex_texture_set_memory_report* out_report);
CTEX_API ctex_result ctex_texture_set_set_channel_backing_store(
    ctex_document* document, const char* texture_set_id, const char* semantic_id,
    uint32_t udim_tile_number, const ctex_tile_backing_store_descriptor* descriptor);
CTEX_API ctex_result ctex_texture_set_evict_channel_tile(ctex_document* document,
                                                         const char* texture_set_id,
                                                         const char* semantic_id,
                                                         uint32_t udim_tile_number, uint32_t tile_x,
                                                         uint32_t tile_y,
                                                         ctex_tile_eviction_report* out_report);
CTEX_API ctex_result ctex_document_get_memory_report(
    const ctex_document* document, ctex_document_memory_info* out_info,
    ctex_document_texture_set_memory_info* texture_sets, size_t texture_set_capacity,
    char* texture_set_ids, size_t texture_set_id_size);
/* Appends a complete ordered batch after validating the resulting stack atomically. */
CTEX_API ctex_result ctex_texture_set_layer_append(ctex_document* document,
                                                   const char* texture_set_id,
                                                   const ctex_layer_entry_descriptor* entries,
                                                   size_t entry_count);
/* Returns a canonical JSON snapshot containing the stack revision and explicit entry kinds. */
CTEX_API ctex_result ctex_texture_set_layer_inspect(const ctex_document* document,
                                                    const char* texture_set_id, char* output,
                                                    size_t output_size, size_t* out_required_size);
CTEX_API ctex_result ctex_texture_set_layer_set_state(ctex_document* document,
                                                      const char* texture_set_id,
                                                      const char* entry_identifier,
                                                      const char* display_name, uint32_t enabled,
                                                      double opacity, const char* blend_mode);
CTEX_API ctex_result ctex_texture_set_layer_set_layout(ctex_document* document,
                                                       const char* texture_set_id,
                                                       const char* entry_identifier,
                                                       const char* parent_identifier,
                                                       const char* target_identifier);
CTEX_API ctex_result ctex_texture_set_layer_set_channel(
    ctex_document* document, const char* texture_set_id, const char* entry_identifier,
    const ctex_layer_channel_descriptor* channel);
CTEX_API ctex_result ctex_texture_set_layer_record_paint(ctex_document* document,
                                                         const char* texture_set_id,
                                                         const char* entry_identifier);
CTEX_API ctex_result ctex_texture_set_layer_remove(ctex_document* document,
                                                   const char* texture_set_id,
                                                   const char* const* entry_identifiers,
                                                   size_t entry_count,
                                                   uint32_t source_deletion_policy);
CTEX_API ctex_result ctex_texture_set_layer_evaluate_blend(const ctex_document* document,
                                                           const char* texture_set_id,
                                                           const char* entry_identifier,
                                                           ctex_vec4f base, ctex_vec4f layer,
                                                           double factor, ctex_vec4f* out_colour);
CTEX_API ctex_result ctex_texture_set_layer_get_applicable_masks(
    const ctex_document* document, const char* texture_set_id, const char* entry_identifier,
    char* mask_ids, size_t mask_id_size, size_t* out_required_size, size_t* out_mask_count);
CTEX_API ctex_result ctex_texture_set_layer_get_participation(
    const ctex_document* document, const char* texture_set_id, const char* entry_identifier,
    const char* semantic_id, const ctex_layer_mask_sample* mask_samples, size_t mask_sample_count,
    ctex_layer_participation_info* out_info, char* mask_ids, size_t mask_id_size);
CTEX_API ctex_result ctex_texture_set_layer_composite_cpu(
    const ctex_document* document, const char* texture_set_id, uint32_t width, uint32_t height,
    const ctex_layer_composite_raster_descriptor* content, size_t content_count,
    const ctex_layer_composite_mask_descriptor* masks, size_t mask_count,
    ctex_layer_composite_info* out_info, ctex_layer_composite_channel_info* channels,
    size_t channel_capacity, char* semantic_ids, size_t semantic_id_size, ctex_vec4f* pixels,
    size_t pixel_capacity);
CTEX_API ctex_result ctex_layer_snapshot_create(
    uint32_t width, uint32_t height, const ctex_layer_composite_raster_descriptor* content,
    size_t content_count, const ctex_layer_composite_mask_descriptor* masks, size_t mask_count,
    ctex_layer_snapshot** out_snapshot);
CTEX_API void ctex_layer_snapshot_destroy(ctex_layer_snapshot* snapshot);
CTEX_API ctex_result ctex_layer_snapshot_read(
    const ctex_layer_snapshot* snapshot, ctex_layer_snapshot_info* out_info,
    ctex_layer_snapshot_content_info* content, size_t content_capacity,
    ctex_layer_snapshot_mask_info* masks, size_t mask_capacity, char* strings, size_t string_size,
    ctex_vec4f* pixels, size_t pixel_capacity, float* coverage, size_t coverage_capacity,
    double* mask_values, size_t mask_value_capacity);
CTEX_API ctex_result ctex_texture_set_layer_composite_snapshot_cpu(
    const ctex_document* document, const char* texture_set_id, const ctex_layer_snapshot* snapshot,
    ctex_layer_composite_info* out_info, ctex_layer_composite_channel_info* channels,
    size_t channel_capacity, char* semantic_ids, size_t semantic_id_size, ctex_vec4f* pixels,
    size_t pixel_capacity);
/* A null affected_ids buffer validates and sizes the operation without committing it. */
CTEX_API ctex_result ctex_texture_set_apply_layer_operation(
    ctex_document* document, const char* texture_set_id, ctex_layer_snapshot* snapshot,
    const ctex_layer_operation_descriptor* operation, ctex_layer_operation_info* out_info,
    char* affected_ids, size_t affected_id_size);
CTEX_API ctex_result ctex_texture_set_begin_transaction(
    ctex_document* document, const char* texture_set_id, const char* step_identifier,
    const ctex_tile_history_target_descriptor* targets, size_t target_count,
    ctex_layer_snapshot* snapshot, ctex_texture_set_transaction** out_transaction);
CTEX_API void ctex_texture_set_transaction_destroy(ctex_texture_set_transaction* transaction);
CTEX_API ctex_result ctex_texture_set_transaction_write_pixel(
    ctex_texture_set_transaction* transaction, const char* semantic_id, uint32_t x, uint32_t y,
    const void* pixel, size_t pixel_size);
CTEX_API ctex_result ctex_texture_set_transaction_apply_layer_operation(
    ctex_texture_set_transaction* transaction, const ctex_layer_operation_descriptor* operation);
CTEX_API ctex_result ctex_texture_set_transaction_set_layer_state(
    ctex_texture_set_transaction* transaction, const char* entry_identifier,
    const char* display_name, uint32_t enabled, double opacity, const char* blend_mode);
CTEX_API ctex_result ctex_texture_set_transaction_set_layer_layout(
    ctex_texture_set_transaction* transaction, const char* entry_identifier,
    const char* parent_identifier, const char* target_identifier);
CTEX_API ctex_result ctex_texture_set_transaction_set_layer_channel(
    ctex_texture_set_transaction* transaction, const char* entry_identifier,
    const ctex_layer_channel_descriptor* channel);
CTEX_API ctex_result ctex_texture_set_transaction_set_fill_graph(
    ctex_texture_set_transaction* transaction, const char* entry_identifier,
    const void* graph_serialized, size_t graph_serialized_size);
/* Commit consumes the staged transaction on both success and failure. */
CTEX_API ctex_result ctex_texture_set_transaction_commit(ctex_texture_set_transaction* transaction,
                                                         ctex_tile_history_commit_info* out_info);
CTEX_API void ctex_texture_set_transaction_cancel(ctex_texture_set_transaction* transaction);
CTEX_API ctex_result ctex_texture_set_configure_tile_history(ctex_document* document,
                                                             const char* texture_set_id,
                                                             size_t budget_bytes);
CTEX_API ctex_result ctex_texture_set_get_tile_history_budget(
    const ctex_document* document, const char* texture_set_id, size_t proposed_step_bytes,
    ctex_tile_history_budget_report* out_report);
CTEX_API ctex_result ctex_texture_set_begin_tile_history(
    ctex_document* document, const char* texture_set_id, const char* step_identifier,
    const ctex_tile_history_target_descriptor* targets, size_t target_count,
    ctex_tile_history_capture** out_capture);
CTEX_API void ctex_tile_history_capture_destroy(ctex_tile_history_capture* capture);
/* Consumes the capture whether commit succeeds or fails. */
CTEX_API ctex_result ctex_tile_history_capture_commit(ctex_tile_history_capture* capture,
                                                      ctex_tile_history_commit_info* out_info);
CTEX_API ctex_result ctex_texture_set_undo_tiles(ctex_document* document,
                                                 const char* texture_set_id,
                                                 ctex_tile_history_restore_info* out_info);
CTEX_API ctex_result ctex_texture_set_redo_tiles(ctex_document* document,
                                                 const char* texture_set_id,
                                                 ctex_tile_history_restore_info* out_info);
CTEX_API ctex_result ctex_texture_set_get_udim_tiles(const ctex_document* document,
                                                     const char* texture_set_id,
                                                     uint32_t* tile_numbers, size_t tile_capacity,
                                                     size_t* out_tile_count);
CTEX_API ctex_result ctex_texture_set_ensure_udim_tiles(ctex_document* document,
                                                        const char* texture_set_id,
                                                        const uint32_t* tile_numbers,
                                                        size_t tile_count,
                                                        size_t* out_allocated_count);
CTEX_API ctex_result ctex_texture_set_write_udim_pixels(
    ctex_document* document, const char* texture_set_id, const char* semantic_id,
    const ctex_udim_pixel_write_descriptor* writes, size_t write_count,
    ctex_udim_write_info* out_info);
CTEX_API ctex_result ctex_texture_set_read_udim_pixel(const ctex_document* document,
                                                      const char* texture_set_id,
                                                      const char* semantic_id, uint32_t tile_number,
                                                      uint32_t x, uint32_t y, void* pixel,
                                                      size_t pixel_size,
                                                      size_t* out_required_pixel_size);
CTEX_API ctex_result ctex_texture_set_query_channel_delta(
    const ctex_document* document, const char* texture_set_id, const char* semantic_id,
    ctex_transport_revision_cursor synchronized_cursor, ctex_transport_tile_version* changed_tiles,
    size_t changed_tile_capacity, ctex_transport_delta_info* out_info);
CTEX_API ctex_result ctex_texture_set_reset_channel_revision_history(
    ctex_document* document, const char* texture_set_id, const char* semantic_id,
    ctex_transport_revision_cursor* out_cursor);
CTEX_API ctex_result ctex_transport_snapshot_pool_create(size_t budget_bytes,
                                                         ctex_transport_snapshot_pool** out_pool);
CTEX_API void ctex_transport_snapshot_pool_destroy(ctex_transport_snapshot_pool* pool);
CTEX_API ctex_result ctex_transport_snapshot_pool_get_memory_report(
    const ctex_transport_snapshot_pool* pool, ctex_transport_snapshot_memory_report* out_report);
CTEX_API ctex_result ctex_resource_ledger_create(ctex_resource_ledger** out_ledger);
CTEX_API void ctex_resource_ledger_destroy(ctex_resource_ledger* ledger);
CTEX_API ctex_result ctex_resource_ledger_upsert(
    ctex_resource_ledger* ledger, const ctex_resource_allocation_descriptor* descriptor);
CTEX_API ctex_result ctex_resource_ledger_remove(ctex_resource_ledger* ledger,
                                                 uint64_t allocation_identity,
                                                 uint32_t* out_removed);
CTEX_API ctex_result ctex_resource_ledger_get_report(const ctex_resource_ledger* ledger,
                                                     ctex_resource_accounting_report* out_report,
                                                     ctex_resource_category_report* categories,
                                                     size_t category_capacity);
CTEX_API ctex_result ctex_resource_ledger_set_cache_eviction_callback(
    ctex_resource_ledger* ledger, ctex_resource_cache_eviction_callback callback, void* user_data);
CTEX_API ctex_result ctex_resource_ledger_admit(
    ctex_resource_ledger* ledger, const ctex_resource_admission_descriptor* descriptor,
    ctex_resource_reservation** out_reservation, ctex_resource_admission_report* out_report);
CTEX_API ctex_result ctex_resource_ledger_admit_operation_recovery(
    ctex_resource_ledger* ledger, const ctex_resource_budget_limits* limits,
    const void* operation_record, size_t operation_record_size, size_t checkpoint_bytes,
    ctex_resource_reservation** out_reservation, ctex_resource_admission_report* out_report);
CTEX_API ctex_result ctex_resource_ledger_admit_preview_quality(
    ctex_resource_ledger* ledger, const ctex_preview_quality_admission_descriptor* descriptor,
    ctex_resource_reservation** out_reservation, ctex_preview_quality_admission_report* out_report);
CTEX_API void ctex_resource_reservation_destroy(ctex_resource_reservation* reservation);
CTEX_API void ctex_resource_reservation_release(ctex_resource_reservation* reservation);
CTEX_API ctex_result ctex_resource_reservation_get_evicted_allocations(
    const ctex_resource_reservation* reservation, uint64_t* allocation_identities,
    size_t allocation_identity_capacity, size_t* out_allocation_identity_count);
CTEX_API ctex_result ctex_texture_set_query_channel_snapshot(
    ctex_transport_snapshot_pool* pool, const ctex_document* document, const char* texture_set_id,
    const char* semantic_id, ctex_transport_revision_cursor synchronized_cursor,
    ctex_transport_snapshot** out_snapshot, ctex_transport_snapshot_query_info* out_info);
CTEX_API ctex_result ctex_paint_preview_session_query_snapshot(
    ctex_transport_snapshot_pool* pool, const ctex_paint_preview_session* session,
    ctex_transport_revision_cursor synchronized_cursor, ctex_transport_snapshot** out_snapshot,
    ctex_transport_snapshot_query_info* out_info);
CTEX_API void ctex_transport_snapshot_destroy(ctex_transport_snapshot* snapshot);
CTEX_API ctex_result ctex_transport_snapshot_get_tile_versions(
    const ctex_transport_snapshot* snapshot, ctex_transport_tile_version* versions,
    size_t version_capacity, size_t* out_version_count);
CTEX_API ctex_result ctex_transport_snapshot_negotiate_format(
    const ctex_transport_snapshot* snapshot, const ctex_transport_pixel_format* accepted_formats,
    size_t accepted_format_count, uint32_t conversion_policy,
    ctex_transport_format_selection* out_selection);
CTEX_API ctex_result ctex_transport_snapshot_get_tile_memory_layout(
    const ctex_transport_snapshot* snapshot, ctex_transport_tile_version version,
    const ctex_transport_format_selection* format, ctex_transport_tile_memory_layout* out_layout);
CTEX_API ctex_result ctex_transport_snapshot_read_tiles(
    const ctex_transport_snapshot* snapshot, const ctex_transport_format_selection* format,
    const ctex_transport_tile_readback_destination* destinations, size_t destination_count);

/*
 * Starts readback from an exact pinned snapshot. CPU-resident snapshots may
 * complete before this call returns. The host variant remains pending until
 * the host supplies one matching completion record per requested tile. Output
 * buffers must outlive the readback and are readable only when reported so.
 * A readback retains the snapshot token until the readback is destroyed.
 */
CTEX_API ctex_result ctex_transport_snapshot_begin_readback(
    const ctex_transport_snapshot* snapshot, const ctex_transport_format_selection* format,
    const ctex_transport_tile_readback_destination* destinations, size_t destination_count,
    ctex_transport_readback** out_readback);
CTEX_API ctex_result ctex_transport_snapshot_begin_host_readback(
    const ctex_transport_snapshot* snapshot, const ctex_transport_format_selection* format,
    const ctex_transport_tile_readback_destination* destinations, size_t destination_count,
    ctex_transport_readback** out_readback);
CTEX_API void ctex_transport_readback_destroy(ctex_transport_readback* readback);
CTEX_API ctex_result ctex_transport_readback_get_info(const ctex_transport_readback* readback,
                                                      ctex_transport_readback_info* out_info,
                                                      char* detail, size_t detail_size);
CTEX_API ctex_result ctex_transport_readback_complete_host(
    ctex_transport_readback* readback, const ctex_transport_host_tile_completion* completed_tiles,
    size_t completed_tile_count);
CTEX_API ctex_result ctex_transport_readback_cancel(ctex_transport_readback* readback);
CTEX_API ctex_result ctex_transport_readback_fail(ctex_transport_readback* readback,
                                                  const char* detail);

/*
 * Creates a registry containing the always-available CPU reference and
 * host-executed routes plus an owned-GPU route when that backend was compiled.
 * host is required and may declare the host route detached.
 */
CTEX_API ctex_result ctex_executor_registry_create(const ctex_host_executor_descriptor* host,
                                                   ctex_executor_registry** out_registry);
CTEX_API void ctex_executor_registry_destroy(ctex_executor_registry* registry);
CTEX_API ctex_result ctex_executor_registry_get_count(const ctex_executor_registry* registry,
                                                      size_t* out_count);

/*
 * Returns one executor and its complete device feature set by stable sorted
 * index. All arrays and strings use atomic caller-owned sizing semantics.
 */
CTEX_API ctex_result ctex_executor_registry_get_info(
    const ctex_executor_registry* registry, size_t executor_index, ctex_executor_info* out_info,
    uint32_t* supported_texture_formats, size_t supported_texture_format_capacity, char* identifier,
    size_t identifier_size, char* display_name, size_t display_name_size, char* device_name,
    size_t device_name_size);

/*
 * A null requested_identifier applies the pinned default, CTEX_EXECUTOR, then
 * automatic selection. A non-null identifier requests that compiled route.
 */
CTEX_API ctex_result ctex_executor_registry_select(const ctex_executor_registry* registry,
                                                   const char* requested_identifier,
                                                   ctex_executor_selection_info* out_info,
                                                   char* requested_identifier_output,
                                                   size_t requested_identifier_output_size,
                                                   char* message, size_t message_size);
CTEX_API ctex_result ctex_executor_registry_pin_default(ctex_executor_registry* registry,
                                                        const char* identifier);
CTEX_API ctex_result ctex_executor_registry_clear_default(ctex_executor_registry* registry);

/* Validates and formats the recovery/fallback decision for a failed executor. */
CTEX_API ctex_result ctex_executor_make_fallback_report(
    const ctex_executor_fallback_descriptor* descriptor, ctex_executor_fallback_info* out_info,
    char* message, size_t message_size);

/* Runs staged CPU work; only a completed execution invokes the commit callback. */
CTEX_API ctex_result
ctex_cpu_execute_bounded(const ctex_cpu_bounded_execution_descriptor* descriptor,
                         ctex_cpu_execution_result** out_result);
CTEX_API void ctex_cpu_execution_result_destroy(ctex_cpu_execution_result* result);
CTEX_API ctex_result ctex_cpu_execution_result_get_info(const ctex_cpu_execution_result* result,
                                                        ctex_cpu_execution_info* out_info,
                                                        char* message, size_t message_size);

/*
 * Independently rasterizes camera depth, perspective-correct UV, coverage and
 * source-triangle identity on the always-available CPU reference route.
 */
CTEX_API ctex_result ctex_cpu_reference_rasterize_viewport(
    const ctex_cpu_viewport_raster_descriptor* descriptor, ctex_cpu_raster_info* out_info,
    const ctex_cpu_raster_outputs* outputs);

/*
 * Independently rasterizes a UV tile and projects covered texels back to
 * camera depth and top-left-origin screen coordinates on the CPU reference.
 */
CTEX_API ctex_result ctex_cpu_reference_rasterize_uv(
    const ctex_cpu_uv_raster_descriptor* descriptor, ctex_cpu_raster_info* out_info,
    const ctex_cpu_raster_outputs* outputs);

/* Numeric executor agreement contract for direct and filtered channel values. */
CTEX_API ctex_result ctex_executor_parity_get_tolerance(uint32_t value_class, uint32_t filtered,
                                                        ctex_parity_tolerance_info* out_info);
CTEX_API ctex_result ctex_executor_compare_parity(const double* reference, const double* measured,
                                                  size_t value_count, uint32_t value_class,
                                                  uint32_t filtered,
                                                  ctex_parity_comparison_info* out_info,
                                                  char* message, size_t message_size);
/* Render callback output arrays remain valid until this synchronous call returns. */
CTEX_API ctex_result ctex_executor_run_parity_gate(
    const ctex_executor_registry* registry, const ctex_parity_fixture_descriptor* fixtures,
    size_t fixture_count, const ctex_parity_executor_binding_descriptor* executors,
    size_t executor_count, ctex_parity_gate_result** out_result);
CTEX_API void ctex_parity_gate_result_destroy(ctex_parity_gate_result* result);
CTEX_API ctex_result ctex_parity_gate_result_get_info(const ctex_parity_gate_result* result,
                                                      ctex_parity_gate_info* out_info, char* report,
                                                      size_t report_size);

/* Host-executed work names logical resources only; device handles stay outside the library. */
CTEX_API ctex_result ctex_host_execution_session_create(uint64_t initial_revision,
                                                        ctex_host_execution_session** out_session);
CTEX_API void ctex_host_execution_session_destroy(ctex_host_execution_session* session);
CTEX_API ctex_result ctex_host_execution_session_get_info(
    const ctex_host_execution_session* session, ctex_host_execution_session_info* out_info);
CTEX_API ctex_result ctex_host_execution_session_submit(
    ctex_host_execution_session* session, const ctex_host_submission_descriptor* descriptor,
    ctex_host_submission_info* out_info);
CTEX_API ctex_result ctex_host_execution_session_cancel(ctex_host_execution_session* session,
                                                        uint64_t completion_token,
                                                        uint32_t* out_cancelled);
CTEX_API ctex_result ctex_host_execution_session_complete(
    ctex_host_execution_session* session, const ctex_host_completion_descriptor* descriptor,
    ctex_host_completion_result** out_result);
CTEX_API ctex_result ctex_host_execution_session_establish_recovery(
    ctex_host_execution_session* session, uint64_t completion_token,
    const ctex_host_recovery_descriptor* recovery, ctex_host_completion_result** out_result);
CTEX_API ctex_result ctex_host_execution_session_get_committed_resource(
    const ctex_host_execution_session* session, const char* logical_id, uint32_t* out_found,
    uint64_t* out_generation);
CTEX_API ctex_result ctex_host_execution_session_resource_is_held(
    const ctex_host_execution_session* session, const char* logical_id, uint64_t generation,
    uint32_t* out_held);
CTEX_API ctex_result ctex_host_execution_session_report_device_loss(
    ctex_host_execution_session* session, ctex_host_recovery_report** out_report);

/* Completion and device-loss handles provide atomic two-call result readback. */
CTEX_API void ctex_host_completion_result_destroy(ctex_host_completion_result* result);
CTEX_API ctex_result ctex_host_completion_result_get_info(
    const ctex_host_completion_result* result, ctex_host_completion_result_info* out_info,
    ctex_host_resource_version* released_resources, size_t released_resource_capacity,
    char* released_identities, size_t released_identity_size, char* message, size_t message_size);
CTEX_API void ctex_host_recovery_report_destroy(ctex_host_recovery_report* report);
CTEX_API ctex_result ctex_host_recovery_report_get_info(
    const ctex_host_recovery_report* report, ctex_host_device_loss_info* out_info,
    uint64_t* cancelled_submissions, size_t cancelled_submission_capacity,
    ctex_host_resource_version* released_resources, size_t released_resource_capacity,
    char* released_identities, size_t released_identity_size);
CTEX_API ctex_result ctex_texture_set_apply_smart_material(ctex_document* document,
                                                           const char* texture_set_id,
                                                           const void* serialized,
                                                           size_t serialized_size,
                                                           const char* application_identifier,
                                                           ctex_preset_application_info* out_info);
CTEX_API ctex_result ctex_texture_set_apply_smart_mask(
    ctex_document* document, const char* texture_set_id, const void* serialized,
    size_t serialized_size, const char* application_identifier, const char* target_entry_identifier,
    ctex_preset_application_info* out_info);
CTEX_API ctex_result ctex_texture_set_get_preset_applications(const ctex_document* document,
                                                              const char* texture_set_id,
                                                              char* report_output,
                                                              size_t report_output_size,
                                                              size_t* out_required_size);
CTEX_API ctex_result ctex_texture_set_set_applied_entry_state(ctex_document* document,
                                                              const char* texture_set_id,
                                                              const char* entry_identifier,
                                                              uint32_t enabled, double opacity);
CTEX_API ctex_result ctex_texture_set_undo_last_preset_application(ctex_document* document,
                                                                   const char* texture_set_id,
                                                                   ctex_preset_undo_info* out_info);

/*
 * Creates an isolated copy-on-write preview for one enabled channel. The
 * document must outlive the session. Pixel buffers are tightly packed,
 * row-major, interleaved bytes in the channel's authored storage format.
 */
CTEX_API ctex_result ctex_paint_preview_session_create(ctex_document* document,
                                                       const char* texture_set_id,
                                                       const char* semantic_id,
                                                       ctex_paint_preview_session** out_session);
CTEX_API void ctex_paint_preview_session_destroy(ctex_paint_preview_session* session);
CTEX_API ctex_result ctex_paint_preview_session_write_pixel(ctex_paint_preview_session* session,
                                                            uint32_t x, uint32_t y,
                                                            const void* pixel, size_t pixel_size);
CTEX_API ctex_result ctex_paint_preview_session_get_info(const ctex_paint_preview_session* session,
                                                         ctex_paint_preview_info* out_info);
CTEX_API ctex_result
ctex_paint_preview_session_get_pixels(const ctex_paint_preview_session* session, void* pixel_buffer,
                                      size_t pixel_buffer_size, size_t* out_required_size);
CTEX_API ctex_result ctex_paint_preview_session_get_changed_tiles(
    const ctex_paint_preview_session* session, ctex_paint_tile_coordinate* tiles,
    size_t tile_capacity, size_t* out_tile_count);
CTEX_API ctex_result ctex_paint_preview_session_finalize(ctex_paint_preview_session* session,
                                                         const uint8_t* coverage,
                                                         size_t coverage_count,
                                                         uint32_t dilation_radius,
                                                         ctex_paint_preview_info* out_info);
CTEX_API ctex_result ctex_paint_preview_session_commit(ctex_paint_preview_session* session,
                                                       ctex_paint_preview_info* out_info);
CTEX_API ctex_result ctex_paint_preview_session_cancel(ctex_paint_preview_session* session);

/*
 * Diagnostics are local to the calling thread. The returned pointer is owned by
 * CyberTexel and remains valid until the next fallible C API call on that thread.
 * Diagnostic-code names and numeric values are stable within an ABI major.
 */
CTEX_API ctex_result ctex_get_last_result(void);
CTEX_API ctex_diagnostic_code ctex_get_last_diagnostic_code(void);
CTEX_API const char* ctex_get_last_diagnostic(void);

#ifdef __cplusplus
}
#endif

#endif
