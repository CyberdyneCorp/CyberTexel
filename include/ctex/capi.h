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
    CTEX_RESULT_BUFFER_TOO_SMALL = 8
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
    CTEX_DIAGNOSTIC_INVALID_PAINT_TOOL = 56
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
typedef struct ctex_paint_dilation_session ctex_paint_dilation_session;
typedef struct ctex_paint_surface_map_cache ctex_paint_surface_map_cache;
typedef struct ctex_paint_preview_session ctex_paint_preview_session;
typedef struct ctex_pick_index ctex_pick_index;
typedef struct ctex_uv_pick_index ctex_uv_pick_index;
typedef struct ctex_transport_snapshot_pool ctex_transport_snapshot_pool;
typedef struct ctex_transport_snapshot ctex_transport_snapshot;
typedef struct ctex_executor_registry ctex_executor_registry;
typedef struct ctex_cpu_execution_result ctex_cpu_execution_result;
typedef struct ctex_parity_gate_result ctex_parity_gate_result;
typedef struct ctex_host_execution_session ctex_host_execution_session;
typedef struct ctex_host_completion_result ctex_host_completion_result;
typedef struct ctex_host_recovery_report ctex_host_recovery_report;

#define CTEX_MAX_MESH_VERTEX_COUNT ((size_t)100000000)
#define CTEX_MAX_MESH_TRIANGLE_COUNT ((size_t)100000000)
#define CTEX_DEFAULT_TILE_SIZE ((uint32_t)64)
#define CTEX_NO_SURFACE_TRIANGLE UINT32_MAX
#define CTEX_NO_UV_ISLAND UINT32_MAX

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
} ctex_texture_set_descriptor;

#define CTEX_TEXTURE_SET_DESCRIPTOR_V1_SIZE \
    ((uint32_t)offsetof(ctex_texture_set_descriptor, default_bit_depth))
#define CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE ((uint32_t)sizeof(ctex_texture_set_descriptor))

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
