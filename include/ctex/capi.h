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
    CTEX_DIAGNOSTIC_MISSING_UV_SET = 30
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

#define CTEX_MAX_MESH_VERTEX_COUNT ((size_t)100000000)
#define CTEX_MAX_MESH_TRIANGLE_COUNT ((size_t)100000000)

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
