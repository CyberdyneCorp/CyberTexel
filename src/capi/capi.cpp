#include <ctex/capi.h>
#include <ctex/version.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctex/image/color_policy.hpp>
#include <ctex/io/image_io.hpp>
#include <ctex/io/texture_encode.hpp>
#include <ctex/paint/blending.hpp>
#include <ctex/paint/coverage.hpp>
#include <ctex/paint/deposition.hpp>
#include <ctex/paint/masking.hpp>
#include <ctex/paint/preview.hpp>
#include <ctex/paint/seam_dilation.hpp>
#include <ctex/paint/seam_filter.hpp>
#include <ctex/paint/stroke.hpp>
#include <ctex/paint/stroke_preset.hpp>
#include <ctex/paint/surface_cache.hpp>
#include <ctex/paint/work.hpp>
#include <exception>
#include <iterator>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "capi_internal.hpp"

struct ctex_paint_dilation_session_tile {
    ctex_paint_dilation_session_tile(std::pmr::memory_resource* resource,
                                     const ctex_paint_dilation_tile_descriptor& descriptor)
        : u(descriptor.u),
          v(descriptor.v),
          width(descriptor.width),
          height(descriptor.height),
          component_count(descriptor.component_count),
          pixels(descriptor.pixels, descriptor.pixels + descriptor.pixel_count, resource),
          coverage(descriptor.coverage, descriptor.coverage + descriptor.coverage_count, resource) {
    }

    ctex_paint_dilation_session_tile(std::pmr::memory_resource* resource,
                                     const ctex_paint_dilation_session_tile& source,
                                     const ctex::paint::SeamDilationResult& result)
        : u(source.u),
          v(source.v),
          width(source.width),
          height(source.height),
          component_count(source.component_count),
          pixels(result.raster.pixels.begin(), result.raster.pixels.end(), resource),
          coverage(source.coverage.begin(), source.coverage.end(), resource),
          dilated_texel_count(result.dilated_texel_count),
          zero_gradient_texel_count(result.zero_gradient_texel_count) {}

    std::int32_t u{};
    std::int32_t v{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t component_count{};
    std::pmr::vector<double> pixels;
    std::pmr::vector<std::uint8_t> coverage;
    std::size_t dilated_texel_count{};
    std::size_t zero_gradient_texel_count{};
};

struct ctex_paint_dilation_session {
    ctex_paint_dilation_session(ctex_allocator_state allocator_value,
                                std::uint32_t requested_radius)
        : allocator(allocator_value), memory_resource(allocator_value), tiles(&memory_resource) {
        ctex::paint::ToolParameterReport report;
        radius = ctex::paint::resolve_seam_dilation_radius(requested_radius, report);
        radius_clamped = report.clamp_for("seam_dilation.radius").has_value();
    }

    ctex_allocator_state allocator;
    ctex_host_memory_resource memory_resource;
    std::pmr::vector<ctex_paint_dilation_session_tile> tiles;
    std::uint32_t radius{};
    bool radius_clamped{};
    bool finished{};
    std::size_t dilation_pass_count{};
};

struct ctex_paint_surface_map_cache_entry {
    ctex_paint_surface_map_cache_entry(std::pmr::memory_resource* resource,
                                       ctex::mesh::PartitionKind partition_kind_value,
                                       std::string_view partition_key_value,
                                       const ctex_paint_surface_map_request& request,
                                       const ctex::paint::CachedSurfaceMaps& maps)
        : partition_kind(partition_kind_value),
          partition_key(partition_key_value, resource),
          uv_set(request.uv_set, resource),
          width(request.width),
          height(request.height),
          tile_origin(request.tile_origin),
          texture_set_id(maps.texture_set_id.data(), maps.texture_set_id.size(), resource),
          surface_texels(resource),
          coverage(maps.coverage.begin(), maps.coverage.end(), resource),
          triangle_identity(maps.triangle_identity.begin(), maps.triangle_identity.end(), resource),
          uv_island_identity(maps.uv_island_identity.begin(), maps.uv_island_identity.end(),
                             resource) {
        surface_texels.reserve(maps.surface.texels.size());
        for (const ctex::paint::SurfaceTexel& texel : maps.surface.texels) {
            surface_texels.push_back({
                .position = {texel.position.x, texel.position.y, texel.position.z},
                .normal = {texel.normal.x, texel.normal.y, texel.normal.z},
                .geometric_normal = {texel.geometric_normal.x, texel.geometric_normal.y,
                                     texel.geometric_normal.z},
                .uv = {texel.uv.x, texel.uv.y},
                .triangle = texel.triangle,
            });
        }
    }

    ctex::mesh::PartitionKind partition_kind{};
    std::pmr::string partition_key;
    std::pmr::string uv_set;
    std::uint32_t width{};
    std::uint32_t height{};
    ctex_vec2d tile_origin{};
    std::pmr::string texture_set_id;
    std::pmr::vector<ctex_paint_surface_texel> surface_texels;
    std::pmr::vector<std::uint8_t> coverage;
    std::pmr::vector<std::uint32_t> triangle_identity;
    std::pmr::vector<std::uint32_t> uv_island_identity;
};

struct ctex_paint_surface_map_cache {
    explicit ctex_paint_surface_map_cache(ctex_allocator_state allocator_value)
        : allocator(allocator_value), memory_resource(allocator_value), entries(&memory_resource) {}

    ctex_allocator_state allocator;
    ctex_host_memory_resource memory_resource;
    std::pmr::vector<ctex_paint_surface_map_cache_entry> entries;
    std::uint64_t mesh_revision{};
    std::size_t hits{};
    std::size_t misses{};
    std::size_t invalidated_entries{};
};

struct ctex_paint_preview_session {
    ctex_paint_preview_session(ctex_allocator_state allocator_value, ctex_document* document_value,
                               std::string_view texture_set_id_value,
                               std::string_view semantic_id_value,
                               ctex::doc::TextureChannels& channels)
        : allocator(allocator_value),
          memory_resource(allocator_value),
          document(document_value),
          texture_set_id(texture_set_id_value, &memory_resource),
          semantic_id(semantic_id_value, &memory_resource),
          baseline(channels.pixels(semantic_id_value).revision_cursor()),
          preview(channels, semantic_id_value, &document_value->memory_resource) {}

    ctex_allocator_state allocator;
    ctex_host_memory_resource memory_resource;
    ctex_document* document;
    std::pmr::string texture_set_id;
    std::pmr::string semantic_id;
    ctex::image::RevisionCursor baseline;
    ctex::image::RevisionCursor committed;
    double maximum_component_error{};
    ctex::paint::PaintPreviewSession preview;
};

namespace {

struct DiagnosticState {
    ctex_result result{CTEX_RESULT_SUCCESS};
    ctex_diagnostic_code code{CTEX_DIAGNOSTIC_NONE};
    char message[1024]{};
};

thread_local DiagnosticState last_diagnostic;

struct LogSinkState {
    ctex_log_callback callback{};
    void* user_data{};
    ctex_log_severity minimum_severity{CTEX_LOG_SEVERITY_INFO};
};

std::mutex log_sink_mutex;
LogSinkState log_sink;

std::mutex allocator_mutex;
ctex_allocator_state process_allocator;

class BoundaryError final : public std::runtime_error {
public:
    BoundaryError(ctex_result result, ctex_diagnostic_code code, std::string message)
        : std::runtime_error(std::move(message)), result_(result), code_(code) {}

    [[nodiscard]] ctex_result result() const noexcept { return result_; }
    [[nodiscard]] ctex_diagnostic_code code() const noexcept { return code_; }

private:
    ctex_result result_;
    ctex_diagnostic_code code_;
};

void clear_diagnostic() noexcept { last_diagnostic = {}; }

void emit_log(ctex_log_severity severity, const char* category, const char* message) noexcept {
    LogSinkState sink;
    {
        const std::scoped_lock lock(log_sink_mutex);
        sink = log_sink;
    }
    if (sink.callback == nullptr || severity < sink.minimum_severity) {
        return;
    }
    const DiagnosticState saved_diagnostic = last_diagnostic;
    try {
        sink.callback(severity, category, message, sink.user_data);
    } catch (...) {
        // A host callback cannot be allowed to throw through the C boundary.
    }
    last_diagnostic = saved_diagnostic;
}

void set_diagnostic(ctex_result result, ctex_diagnostic_code code, const char* operation,
                    const char* detail) noexcept {
    last_diagnostic.result = result;
    last_diagnostic.code = code;
    static_cast<void>(std::snprintf(last_diagnostic.message, sizeof(last_diagnostic.message),
                                    "%s: %s", operation, detail));
    emit_log(CTEX_LOG_SEVERITY_ERROR, "capi.diagnostic", last_diagnostic.message);
}

template <typename Operation>
ctex_result call_boundary(const char* name, Operation&& operation) noexcept {
    clear_diagnostic();
    try {
        operation();
        return CTEX_RESULT_SUCCESS;
    } catch (const BoundaryError& error) {
        set_diagnostic(error.result(), error.code(), name, error.what());
        return error.result();
    } catch (const std::invalid_argument& error) {
        set_diagnostic(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_VALUE, name,
                       error.what());
        return CTEX_RESULT_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        set_diagnostic(CTEX_RESULT_OUT_OF_MEMORY, CTEX_DIAGNOSTIC_ALLOCATION_FAILED, name,
                       "allocation failed");
        return CTEX_RESULT_OUT_OF_MEMORY;
    } catch (const std::exception& error) {
        set_diagnostic(CTEX_RESULT_INTERNAL_ERROR, CTEX_DIAGNOSTIC_UNEXPECTED_EXCEPTION, name,
                       error.what());
        return CTEX_RESULT_INTERNAL_ERROR;
    } catch (...) {
        set_diagnostic(CTEX_RESULT_INTERNAL_ERROR, CTEX_DIAGNOSTIC_UNEXPECTED_EXCEPTION, name,
                       "unknown internal failure");
        return CTEX_RESULT_INTERNAL_ERROR;
    }
}

[[noreturn]] void throw_boundary(ctex_result result, ctex_diagnostic_code code,
                                 std::string message) {
    throw BoundaryError(result, code, std::move(message));
}

ctex_log_severity log_severity(std::uint32_t severity) {
    if (severity > CTEX_LOG_SEVERITY_FATAL) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE,
                       "minimum_severity=" + std::to_string(severity));
    }
    return static_cast<ctex_log_severity>(severity);
}

void install_log_sink(const ctex_log_sink_descriptor* descriptor) {
    LogSinkState next;
    if (descriptor != nullptr) {
        if (descriptor->size < CTEX_LOG_SINK_DESCRIPTOR_V1_SIZE) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                           "descriptor.size=" + std::to_string(descriptor->size) +
                               " minimum_size=" + std::to_string(CTEX_LOG_SINK_DESCRIPTOR_V1_SIZE));
        }
        if (descriptor->size > CTEX_LOG_SINK_DESCRIPTOR_CURRENT_SIZE) {
            throw_boundary(
                CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                "descriptor.size=" + std::to_string(descriptor->size) +
                    " library_size=" + std::to_string(CTEX_LOG_SINK_DESCRIPTOR_CURRENT_SIZE));
        }
        if (descriptor->callback == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "descriptor.callback=null");
        }
        next = {.callback = descriptor->callback,
                .user_data = descriptor->user_data,
                .minimum_severity = log_severity(descriptor->minimum_severity)};
    }
    const std::scoped_lock lock(log_sink_mutex);
    log_sink = next;
}

void install_allocator(const ctex_allocator_descriptor* descriptor) {
    ctex_allocator_state next;
    if (descriptor != nullptr) {
        if (descriptor->size < CTEX_ALLOCATOR_DESCRIPTOR_V1_SIZE) {
            throw_boundary(
                CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                "descriptor.size=" + std::to_string(descriptor->size) +
                    " minimum_size=" + std::to_string(CTEX_ALLOCATOR_DESCRIPTOR_V1_SIZE));
        }
        if (descriptor->size > CTEX_ALLOCATOR_DESCRIPTOR_CURRENT_SIZE) {
            throw_boundary(
                CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                "descriptor.size=" + std::to_string(descriptor->size) +
                    " library_size=" + std::to_string(CTEX_ALLOCATOR_DESCRIPTOR_CURRENT_SIZE));
        }
        if (descriptor->allocate == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "descriptor.allocate=null");
        }
        if (descriptor->deallocate == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "descriptor.deallocate=null");
        }
        next = {.allocate = descriptor->allocate,
                .deallocate = descriptor->deallocate,
                .user_data = descriptor->user_data};
    }
    const std::scoped_lock lock(allocator_mutex);
    process_allocator = next;
}

ctex_allocator_state current_allocator() {
    const std::scoped_lock lock(allocator_mutex);
    return process_allocator;
}

void* allocate_storage(const ctex_allocator_state& allocator, std::size_t size,
                       std::size_t alignment) {
    void* storage = allocator.allocate == nullptr
                        ? ::operator new(size, std::align_val_t(alignment))
                        : allocator.allocate(size, alignment, allocator.user_data);
    if (storage == nullptr) {
        throw std::bad_alloc();
    }
    if (reinterpret_cast<std::uintptr_t>(storage) % alignment != 0) {
        if (allocator.deallocate != nullptr) {
            try {
                allocator.deallocate(storage, size, alignment, allocator.user_data);
            } catch (...) {
            }
        } else {
            ::operator delete(storage, std::align_val_t(alignment));
        }
        throw_boundary(
            CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_ALLOCATOR_CONTRACT_VIOLATION,
            "allocate returned storage that does not meet alignment=" + std::to_string(alignment));
    }
    return storage;
}

void deallocate_storage(const ctex_allocator_state& allocator, void* storage, std::size_t size,
                        std::size_t alignment) noexcept {
    if (allocator.deallocate == nullptr) {
        ::operator delete(storage, std::align_val_t(alignment));
        return;
    }
    try {
        allocator.deallocate(storage, size, alignment, allocator.user_data);
    } catch (...) {
        // Deallocation callbacks cannot report failure through the void destroy API.
    }
}

ctex_document* create_document(const ctex_allocator_state& allocator) {
    void* storage = allocate_storage(allocator, sizeof(ctex_document), alignof(ctex_document));
    try {
        return ::new (storage) ctex_document(allocator);
    } catch (...) {
        deallocate_storage(allocator, storage, sizeof(ctex_document), alignof(ctex_document));
        throw;
    }
}

ctex_paint_dilation_session* create_paint_dilation_session(const ctex_allocator_state& allocator,
                                                           std::uint32_t radius) {
    void* storage = allocate_storage(allocator, sizeof(ctex_paint_dilation_session),
                                     alignof(ctex_paint_dilation_session));
    try {
        return ::new (storage) ctex_paint_dilation_session(allocator, radius);
    } catch (...) {
        deallocate_storage(allocator, storage, sizeof(ctex_paint_dilation_session),
                           alignof(ctex_paint_dilation_session));
        throw;
    }
}

ctex_paint_surface_map_cache* create_paint_surface_map_cache(
    const ctex_allocator_state& allocator) {
    void* storage = allocate_storage(allocator, sizeof(ctex_paint_surface_map_cache),
                                     alignof(ctex_paint_surface_map_cache));
    try {
        return ::new (storage) ctex_paint_surface_map_cache(allocator);
    } catch (...) {
        deallocate_storage(allocator, storage, sizeof(ctex_paint_surface_map_cache),
                           alignof(ctex_paint_surface_map_cache));
        throw;
    }
}

ctex_paint_preview_session* create_paint_preview_session(const ctex_allocator_state& allocator,
                                                         ctex_document* document,
                                                         std::string_view texture_set_id,
                                                         std::string_view semantic_id,
                                                         ctex::doc::TextureChannels& channels) {
    void* storage = allocate_storage(allocator, sizeof(ctex_paint_preview_session),
                                     alignof(ctex_paint_preview_session));
    try {
        return ::new (storage)
            ctex_paint_preview_session(allocator, document, texture_set_id, semantic_id, channels);
    } catch (...) {
        deallocate_storage(allocator, storage, sizeof(ctex_paint_preview_session),
                           alignof(ctex_paint_preview_session));
        throw;
    }
}

std::size_t texture_set_id_buffer_size(const std::vector<std::string>& identifiers) {
    std::size_t required_size = 0;
    for (const std::string& identifier : identifiers) {
        if (identifier.size() == std::numeric_limits<std::size_t>::max()) {
            throw std::overflow_error("texture-set ID buffer size overflow");
        }
        const std::size_t entry_size = identifier.size() + 1;
        if (entry_size > std::numeric_limits<std::size_t>::max() - required_size) {
            throw std::overflow_error("texture-set ID buffer size overflow");
        }
        required_size += entry_size;
    }
    return required_size;
}

void validate_string_buffer(char* buffer, std::size_t buffer_size, std::size_t required_size) {
    if (buffer == nullptr && buffer_size != 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "buffer=null with nonzero buffer_size");
    }
    if (buffer != nullptr && buffer_size < required_size) {
        throw_boundary(CTEX_RESULT_BUFFER_TOO_SMALL, CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL,
                       "buffer_size=" + std::to_string(buffer_size) +
                           " required_size=" + std::to_string(required_size));
    }
}

void copy_packed_strings(const std::vector<std::string>& values, char* buffer) {
    std::size_t offset = 0;
    for (const std::string& value : values) {
        const std::size_t entry_size = value.size() + 1;
        std::memcpy(buffer + offset, value.c_str(), entry_size);
        offset += entry_size;
    }
}

template <typename Document>
decltype(auto) require_texture_set(Document& document, const char* texture_set_id) {
    if (texture_set_id == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "texture_set_id=null");
    }
    if (texture_set_id[0] == '\0') {
        throw_boundary(CTEX_RESULT_MISSING_RESOURCE, CTEX_DIAGNOSTIC_MISSING_TEXTURE_SET,
                       "texture_set_id is empty");
    }
    if (!document.value.contains_texture_set(texture_set_id)) {
        throw_boundary(CTEX_RESULT_MISSING_RESOURCE, CTEX_DIAGNOSTIC_MISSING_TEXTURE_SET,
                       "texture-set identity is not present: " + std::string(texture_set_id));
    }
    return document.value.texture_set(texture_set_id);
}

void require_semantic_id(const char* semantic_id) {
    if (semantic_id == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "semantic_id=null");
    }
    if (semantic_id[0] == '\0') {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_EMPTY_CHANNEL_SEMANTIC_ID,
                       "semantic_id is empty");
    }
}

bool channel_exists(const ctex::doc::TextureChannels& channels, std::string_view semantic_id) {
    const std::vector<std::string> identifiers = channels.semantic_ids();
    return std::find(identifiers.begin(), identifiers.end(), semantic_id) != identifiers.end();
}

const ctex::doc::ChannelDescriptor& require_channel(const ctex::doc::TextureChannels& channels,
                                                    const char* semantic_id) {
    require_semantic_id(semantic_id);
    if (!channel_exists(channels, semantic_id)) {
        throw_boundary(
            CTEX_RESULT_MISSING_RESOURCE, CTEX_DIAGNOSTIC_MISSING_CHANNEL,
            "channel semantic identifier is not registered: " + std::string(semantic_id));
    }
    return channels.descriptor(semantic_id);
}

ctex::doc::ScalarRepresentation scalar_representation(std::uint32_t value) {
    switch (value) {
        case CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED:
            return ctex::doc::ScalarRepresentation::unsigned_normalized;
        case CTEX_SCALAR_REPRESENTATION_FLOATING_POINT:
            return ctex::doc::ScalarRepresentation::floating_point;
    }
    throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE,
                   "scalar_representation=" + std::to_string(value));
}

ctex::doc::ChannelClassification channel_classification(std::uint32_t value) {
    switch (value) {
        case CTEX_CHANNEL_CLASSIFICATION_COLOR:
            return ctex::doc::ChannelClassification::color;
        case CTEX_CHANNEL_CLASSIFICATION_DATA:
            return ctex::doc::ChannelClassification::data;
    }
    throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE,
                   "classification=" + std::to_string(value));
}

ctex::doc::BlendingPolicy blending_policy(std::uint32_t value) {
    switch (value) {
        case CTEX_BLENDING_POLICY_COLOR:
            return ctex::doc::BlendingPolicy::color;
        case CTEX_BLENDING_POLICY_SCALAR:
            return ctex::doc::BlendingPolicy::scalar;
        case CTEX_BLENDING_POLICY_NORMAL_VECTOR:
            return ctex::doc::BlendingPolicy::normal_vector;
        case CTEX_BLENDING_POLICY_ADDITIVE:
            return ctex::doc::BlendingPolicy::additive;
    }
    throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE,
                   "blending_policy=" + std::to_string(value));
}

bool valid_channel_bit_depth(std::uint32_t value) noexcept {
    return value == 8 || value == 16 || value == 32;
}

void validate_channel_descriptor(const ctex_channel_descriptor& descriptor) {
    if (descriptor.size < CTEX_CHANNEL_DESCRIPTOR_V1_SIZE ||
        descriptor.size > CTEX_CHANNEL_DESCRIPTOR_CURRENT_SIZE) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                       "descriptor.size=" + std::to_string(descriptor.size) + " expected_size=" +
                           std::to_string(CTEX_CHANNEL_DESCRIPTOR_CURRENT_SIZE));
    }
    require_semantic_id(descriptor.semantic_id);
    if (descriptor.export_mapping == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "descriptor.export_mapping=null");
    }
    if (descriptor.export_mapping[0] == '\0') {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_EMPTY_CHANNEL_EXPORT_MAPPING,
                       "descriptor.export_mapping is empty");
    }
    if (descriptor.component_count == 0 || descriptor.component_count > 4) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT,
                       CTEX_DIAGNOSTIC_INVALID_CHANNEL_COMPONENT_COUNT,
                       "descriptor.component_count=" + std::to_string(descriptor.component_count));
    }
    if (descriptor.default_value_count != descriptor.component_count) {
        throw_boundary(
            CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_CHANNEL_DEFAULT_VALUE_COUNT,
            "descriptor.default_value_count=" + std::to_string(descriptor.default_value_count) +
                " component_count=" + std::to_string(descriptor.component_count));
    }
    if (!valid_channel_bit_depth(descriptor.preferred_bit_depth) ||
        (descriptor.scalar_representation == CTEX_SCALAR_REPRESENTATION_FLOATING_POINT &&
         descriptor.preferred_bit_depth != 32)) {
        throw_boundary(
            CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_CHANNEL_BIT_DEPTH,
            "descriptor.preferred_bit_depth=" + std::to_string(descriptor.preferred_bit_depth));
    }
    if (descriptor.evaluable > 1) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_VALUE,
                       "descriptor.evaluable=" + std::to_string(descriptor.evaluable));
    }
}

ctex::doc::ChannelDescriptor channel_descriptor(const ctex_channel_descriptor& descriptor) {
    validate_channel_descriptor(descriptor);
    return {
        .semantic_id = descriptor.semantic_id,
        .component_count = static_cast<std::uint8_t>(descriptor.component_count),
        .scalar_representation = scalar_representation(descriptor.scalar_representation),
        .preferred_bit_depth = static_cast<std::uint8_t>(descriptor.preferred_bit_depth),
        .default_value = std::pmr::vector<double>(
            descriptor.default_value, descriptor.default_value + descriptor.default_value_count),
        .classification = channel_classification(descriptor.classification),
        .blending_policy = blending_policy(descriptor.blending_policy),
        .export_mapping = descriptor.export_mapping,
        .evaluable = descriptor.evaluable != 0,
    };
}

std::uint32_t storage_bit_depth(const ctex::doc::TextureChannels& channels,
                                std::string_view semantic_id) {
    if (!channels.is_enabled(semantic_id)) {
        return 0;
    }
    switch (channels.pixels(semantic_id).format().channel_type) {
        case ctex::image::ChannelType::uint8_unorm:
            return 8;
        case ctex::image::ChannelType::uint16_unorm:
            return 16;
        case ctex::image::ChannelType::float32:
            return 32;
    }
    throw std::logic_error("channel has an unknown storage type");
}

std::vector<ctex::image::TileCoordinate> paint_preview_changed_tiles(
    const ctex_paint_preview_session& session) {
    const ctex::image::TiledImage& pixels = session.preview.preview_pixels();
    if (pixels.revision_cursor().epoch == session.baseline.epoch) {
        return pixels.changed_tiles_after(session.baseline.revision).coordinates;
    }
    std::vector<ctex::image::TileCoordinate> result;
    result.reserve(static_cast<std::size_t>(pixels.tile_columns()) * pixels.tile_rows());
    for (std::uint32_t y = 0; y < pixels.tile_rows(); ++y) {
        for (std::uint32_t x = 0; x < pixels.tile_columns(); ++x) {
            result.push_back({x, y});
        }
    }
    return result;
}

ctex_paint_preview_info paint_preview_info(const ctex_paint_preview_session& session) {
    const ctex::image::TiledImage& pixels = session.preview.preview_pixels();
    const ctex::image::PixelFormat format = pixels.format();
    const ctex::image::RevisionCursor preview = pixels.revision_cursor();
    const std::size_t pixel_byte_count =
        static_cast<std::size_t>(pixels.width()) * pixels.height() * pixels.pixel_bytes();
    return {
        .size = CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE,
        .state = static_cast<std::uint32_t>(session.preview.state()),
        .width = pixels.width(),
        .height = pixels.height(),
        .component_count = format.channel_count,
        .scalar_representation = format.channel_type == ctex::image::ChannelType::float32
                                     ? CTEX_SCALAR_REPRESENTATION_FLOATING_POINT
                                     : CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
        .bit_depth = static_cast<std::uint32_t>(format.bytes_per_channel() * 8),
        .pixel_byte_count = pixel_byte_count,
        .resolved_dilation_radius = session.preview.dilation_radius(),
        .dilation_radius_clamped =
            session.preview.parameter_report().clamp_for("seam_dilation.radius").has_value(),
        .dilated_texel_count = session.preview.dilated_texel_count(),
        .zero_gradient_texel_count = session.preview.zero_gradient_texel_count(),
        .baseline_epoch = session.baseline.epoch,
        .baseline_revision = session.baseline.revision,
        .preview_epoch = preview.epoch,
        .preview_revision = preview.revision,
        .committed_epoch = session.committed.epoch,
        .committed_revision = session.committed.revision,
        .changed_tile_count = paint_preview_changed_tiles(session).size(),
        .maximum_component_error = session.maximum_component_error,
    };
}

void validate_paint_preview_info(ctex_paint_preview_info* out_info) {
    if (out_info == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "out_info=null");
    }
    if (out_info->size < CTEX_PAINT_PREVIEW_INFO_V1_SIZE ||
        out_info->size > CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                       "out_info.size=" + std::to_string(out_info->size));
    }
}

[[noreturn]] void throw_invalid_paint_preview(const std::exception& error) {
    throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_PREVIEW,
                   error.what());
}

ctex::image::ColorSpace color_space(std::uint32_t value) {
    switch (value) {
        case CTEX_COLOR_SPACE_LINEAR_REC709:
            return ctex::image::ColorSpace::linear_rec709;
        case CTEX_COLOR_SPACE_SRGB_REC709:
            return ctex::image::ColorSpace::srgb_rec709;
    }
    throw_boundary(CTEX_RESULT_UNSUPPORTED_OPERATION, CTEX_DIAGNOSTIC_UNSUPPORTED_COLOR_SPACE,
                   "color_space=" + std::to_string(value));
}

ctex::image::InputColorSpace input_color_space(std::uint32_t value) {
    switch (value) {
        case CTEX_INPUT_COLOR_SPACE_AUTOMATIC:
            return ctex::image::InputColorSpace::automatic;
        case CTEX_INPUT_COLOR_SPACE_LINEAR_REC709:
            return ctex::image::InputColorSpace::linear_rec709;
        case CTEX_INPUT_COLOR_SPACE_SRGB_REC709:
            return ctex::image::InputColorSpace::srgb_rec709;
    }
    throw_boundary(CTEX_RESULT_UNSUPPORTED_OPERATION, CTEX_DIAGNOSTIC_UNSUPPORTED_COLOR_SPACE,
                   "input_color_space=" + std::to_string(value));
}

ctex::image::InputColorSpace declared_input_color_space(ctex::image::ColorSpace value) noexcept {
    return value == ctex::image::ColorSpace::linear_rec709
               ? ctex::image::InputColorSpace::linear_rec709
               : ctex::image::InputColorSpace::srgb_rec709;
}

ctex::image::ChannelSemantic channel_semantic(std::uint32_t value) {
    switch (value) {
        case CTEX_CHANNEL_SEMANTIC_BASE_COLOR:
            return ctex::image::ChannelSemantic::base_color;
        case CTEX_CHANNEL_SEMANTIC_OPACITY:
            return ctex::image::ChannelSemantic::opacity;
        case CTEX_CHANNEL_SEMANTIC_ROUGHNESS:
            return ctex::image::ChannelSemantic::roughness;
        case CTEX_CHANNEL_SEMANTIC_METALLIC:
            return ctex::image::ChannelSemantic::metallic;
        case CTEX_CHANNEL_SEMANTIC_NORMAL:
            return ctex::image::ChannelSemantic::normal;
        case CTEX_CHANNEL_SEMANTIC_HEIGHT:
            return ctex::image::ChannelSemantic::height;
        case CTEX_CHANNEL_SEMANTIC_OCCLUSION:
            return ctex::image::ChannelSemantic::occlusion;
        case CTEX_CHANNEL_SEMANTIC_EMISSION:
            return ctex::image::ChannelSemantic::emission;
        case CTEX_CHANNEL_SEMANTIC_SUBSURFACE:
            return ctex::image::ChannelSemantic::subsurface;
    }
    throw_boundary(CTEX_RESULT_UNSUPPORTED_OPERATION, CTEX_DIAGNOSTIC_UNSUPPORTED_CHANNEL_SEMANTIC,
                   "channel_semantic=" + std::to_string(value));
}

void validate_structure_size(std::uint32_t provided, std::uint32_t minimum, std::uint32_t current,
                             std::string_view field) {
    if (provided < minimum || provided > current) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                       std::string(field) + "=" + std::to_string(provided) +
                           " expected_size=" + std::to_string(current));
    }
}

ctex::image::RgbColor rgb_color(const ctex_rgb_color& color) {
    if (!std::isfinite(color.red) || !std::isfinite(color.green) || !std::isfinite(color.blue)) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_COLOR_COMPONENT,
                       "RGB components must be finite");
    }
    static_cast<void>(color_space(color.color_space));
    return {color.red, color.green, color.blue};
}

void validate_color_bit_depth(std::uint32_t value) {
    if (!valid_channel_bit_depth(value)) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_COLOR_BIT_DEPTH,
                       "storage_bit_depth=" + std::to_string(value));
    }
}

ctex::io::DecodeLimits image_decode_limits(const ctex_image_decode_limits_descriptor* descriptor) {
    if (descriptor == nullptr) {
        return {};
    }
    validate_structure_size(descriptor->size, CTEX_IMAGE_DECODE_LIMITS_DESCRIPTOR_V1_SIZE,
                            CTEX_IMAGE_DECODE_LIMITS_DESCRIPTOR_CURRENT_SIZE, "limits.size");
    return {
        .maximum_width = descriptor->maximum_width,
        .maximum_height = descriptor->maximum_height,
        .maximum_decoded_bytes = descriptor->maximum_decoded_bytes,
    };
}

std::uint32_t image_file_format(ctex::io::ImageFileFormat format) noexcept {
    switch (format) {
        case ctex::io::ImageFileFormat::unknown:
            return CTEX_IMAGE_FILE_FORMAT_UNKNOWN;
        case ctex::io::ImageFileFormat::png:
            return CTEX_IMAGE_FILE_FORMAT_PNG;
        case ctex::io::ImageFileFormat::jpeg:
            return CTEX_IMAGE_FILE_FORMAT_JPEG;
        case ctex::io::ImageFileFormat::bmp:
            return CTEX_IMAGE_FILE_FORMAT_BMP;
        case ctex::io::ImageFileFormat::tiff:
            return CTEX_IMAGE_FILE_FORMAT_TIFF;
        case ctex::io::ImageFileFormat::openexr:
            return CTEX_IMAGE_FILE_FORMAT_OPENEXR;
        case ctex::io::ImageFileFormat::radiance_hdr:
            return CTEX_IMAGE_FILE_FORMAT_RADIANCE_HDR;
        case ctex::io::ImageFileFormat::psd:
            return CTEX_IMAGE_FILE_FORMAT_PSD;
    }
    return CTEX_IMAGE_FILE_FORMAT_UNKNOWN;
}

std::uint32_t color_space_source(ctex::io::ColorSpaceSource source) noexcept {
    switch (source) {
        case ctex::io::ColorSpaceSource::caller:
            return CTEX_COLOR_SPACE_SOURCE_CALLER;
        case ctex::io::ColorSpaceSource::embedded_srgb:
            return CTEX_COLOR_SPACE_SOURCE_EMBEDDED_SRGB;
        case ctex::io::ColorSpaceSource::automatic_rule:
            return CTEX_COLOR_SPACE_SOURCE_AUTOMATIC_RULE;
    }
    return CTEX_COLOR_SPACE_SOURCE_AUTOMATIC_RULE;
}

[[noreturn]] void throw_image_io_error(const ctex::io::ImageIoError& error) {
    switch (error.code()) {
        case ctex::io::ImageIoErrorCode::unsupported_format:
        case ctex::io::ImageIoErrorCode::unsupported_pixel_format:
            throw_boundary(CTEX_RESULT_UNSUPPORTED_OPERATION,
                           CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_FORMAT, error.what());
        case ctex::io::ImageIoErrorCode::over_limit:
            throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED,
                           error.what());
        case ctex::io::ImageIoErrorCode::malformed_input:
        case ctex::io::ImageIoErrorCode::decode_failed:
        case ctex::io::ImageIoErrorCode::encode_failed:
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA,
                           error.what());
    }
    throw_boundary(CTEX_RESULT_INTERNAL_ERROR, CTEX_DIAGNOSTIC_UNEXPECTED_EXCEPTION,
                   "unknown image I/O failure");
}

bool has_uninterpretable_profile(const ctex::io::DecodeReport& report) {
    return std::ranges::any_of(report.diagnostics, [](const std::string& diagnostic) {
        return diagnostic.find("profile is not interpreted") != std::string::npos;
    });
}

std::size_t decoded_image_size(const ctex::image::TiledImage& pixels) {
    const std::size_t row_bytes =
        static_cast<std::size_t>(pixels.width()) * pixels.format().bytes_per_pixel();
    if (pixels.height() != 0 &&
        row_bytes > std::numeric_limits<std::size_t>::max() / pixels.height()) {
        throw std::overflow_error("decoded image byte count overflow");
    }
    return row_bytes * pixels.height();
}

void copy_decoded_pixels(const ctex::image::TiledImage& pixels, void* buffer) {
    auto* destination = static_cast<std::byte*>(buffer);
    std::size_t offset = 0;
    for (std::uint32_t y = 0; y < pixels.height(); ++y) {
        for (std::uint32_t x = 0; x < pixels.width(); ++x) {
            const std::span<const std::byte> pixel = pixels.read_pixel(x, y);
            std::memcpy(destination + offset, pixel.data(), pixel.size());
            offset += pixel.size();
        }
    }
}

ctex::image::PixelFormat encoded_input_format(const ctex_image_encode_descriptor& descriptor) {
    if (descriptor.channel_count < 1 || descriptor.channel_count > 4) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA,
                       "channel_count=" + std::to_string(descriptor.channel_count));
    }
    ctex::image::ChannelType type;
    if (descriptor.scalar_representation == CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED &&
        descriptor.input_bit_depth == 8) {
        type = ctex::image::ChannelType::uint8_unorm;
    } else if (descriptor.scalar_representation == CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED &&
               descriptor.input_bit_depth == 16) {
        type = ctex::image::ChannelType::uint16_unorm;
    } else if (descriptor.scalar_representation == CTEX_SCALAR_REPRESENTATION_FLOATING_POINT &&
               descriptor.input_bit_depth == 32) {
        type = ctex::image::ChannelType::float32;
    } else {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA,
                       "input scalar representation and bit depth are incompatible");
    }
    return {type, static_cast<std::uint8_t>(descriptor.channel_count)};
}

ctex::io::ExportImageFormat encoded_output_format(std::uint32_t value) {
    switch (value) {
        case CTEX_IMAGE_FILE_FORMAT_PNG:
            return ctex::io::ExportImageFormat::png;
        case CTEX_IMAGE_FILE_FORMAT_JPEG:
            return ctex::io::ExportImageFormat::jpeg;
        case CTEX_IMAGE_FILE_FORMAT_TGA:
            return ctex::io::ExportImageFormat::tga;
        case CTEX_IMAGE_FILE_FORMAT_TIFF:
            return ctex::io::ExportImageFormat::tiff;
        case CTEX_IMAGE_FILE_FORMAT_OPENEXR:
            return ctex::io::ExportImageFormat::openexr;
    }
    throw_boundary(CTEX_RESULT_UNSUPPORTED_OPERATION, CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_FORMAT,
                   "output_format=" + std::to_string(value));
}

ctex::io::ExportBitDepth encoded_output_depth(std::uint32_t value) {
    switch (value) {
        case 8:
            return ctex::io::ExportBitDepth::bits_8;
        case 16:
            return ctex::io::ExportBitDepth::bits_16;
        case 32:
            return ctex::io::ExportBitDepth::bits_32;
    }
    throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA,
                   "output_bit_depth=" + std::to_string(value));
}

std::size_t encoded_input_size(const ctex_image_encode_descriptor& descriptor,
                               std::size_t row_bytes) {
    if (descriptor.height == 0 || descriptor.width == 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA,
                       "image dimensions must be nonzero");
    }
    const std::size_t stride =
        descriptor.row_stride_bytes == 0 ? row_bytes : descriptor.row_stride_bytes;
    if (stride < row_bytes) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA,
                       "row_stride_bytes is smaller than one pixel row");
    }
    const std::size_t preceding_rows = descriptor.height - 1;
    if (preceding_rows != 0 &&
        stride > (std::numeric_limits<std::size_t>::max() - row_bytes) / preceding_rows) {
        throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED,
                       "encoded input byte count overflows the platform size type");
    }
    return preceding_rows * stride + row_bytes;
}

ctex::image::TiledImage copy_encoded_input(const void* pixels,
                                           const ctex_image_encode_descriptor& descriptor,
                                           ctex::image::PixelFormat format) {
    ctex::image::TiledImage image(descriptor.width, descriptor.height, format);
    const auto* source = static_cast<const std::byte*>(pixels);
    const std::size_t row_bytes =
        static_cast<std::size_t>(descriptor.width) * format.bytes_per_pixel();
    const std::size_t stride =
        descriptor.row_stride_bytes == 0 ? row_bytes : descriptor.row_stride_bytes;
    for (std::uint32_t y = 0; y < descriptor.height; ++y) {
        for (std::uint32_t x = 0; x < descriptor.width; ++x) {
            const std::size_t offset = static_cast<std::size_t>(y) * stride +
                                       static_cast<std::size_t>(x) * format.bytes_per_pixel();
            image.write_pixel(x, y, std::span(source + offset, format.bytes_per_pixel()));
        }
    }
    return image;
}

[[noreturn]] void throw_texture_encode_error(const ctex::io::TextureEncodeError& error) {
    switch (error.code()) {
        case ctex::io::TextureEncodeErrorCode::unsupported_combination:
            throw_boundary(CTEX_RESULT_UNSUPPORTED_OPERATION,
                           CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_COMBINATION, error.what());
        case ctex::io::TextureEncodeErrorCode::invalid_option:
        case ctex::io::TextureEncodeErrorCode::invalid_pixels:
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA,
                           error.what());
        case ctex::io::TextureEncodeErrorCode::over_limit:
            throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED,
                           error.what());
        case ctex::io::TextureEncodeErrorCode::encode_failed:
            throw_boundary(CTEX_RESULT_INTERNAL_ERROR, CTEX_DIAGNOSTIC_IMAGE_ENCODING_FAILED,
                           error.what());
    }
    throw_boundary(CTEX_RESULT_INTERNAL_ERROR, CTEX_DIAGNOSTIC_UNEXPECTED_EXCEPTION,
                   "unknown texture encoding failure");
}

struct PreparedImageEncode {
    ctex::image::PixelFormat input_format;
    ctex::io::TextureEncodeOptions options;
};

PreparedImageEncode prepare_image_encode(const void* pixels, std::size_t pixel_buffer_size,
                                         const ctex_image_encode_descriptor& descriptor) {
    const ctex::image::PixelFormat input_format = encoded_input_format(descriptor);
    const std::size_t row_bytes =
        static_cast<std::size_t>(descriptor.width) * input_format.bytes_per_pixel();
    const std::size_t required_input_size = encoded_input_size(descriptor, row_bytes);
    if (pixels == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT, "pixels=null");
    }
    if (pixel_buffer_size < required_input_size) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA,
                       "pixel_buffer_size=" + std::to_string(pixel_buffer_size) +
                           " required_size=" + std::to_string(required_input_size));
    }
    const ctex::io::ExportImageFormat output_format =
        encoded_output_format(descriptor.output_format);
    if (output_format == ctex::io::ExportImageFormat::jpeg &&
        (descriptor.jpeg_quality < 1 || descriptor.jpeg_quality > 100)) {
        throw_boundary(
            CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA,
            "jpeg_quality=" + std::to_string(descriptor.jpeg_quality) + " expected_range=1..100");
    }
    return {
        .input_format = input_format,
        .options =
            {
                .format = output_format,
                .bit_depth = encoded_output_depth(descriptor.output_bit_depth),
                .color_space = color_space(descriptor.color_space),
                .jpeg_quality = static_cast<int>(descriptor.jpeg_quality),
            },
    };
}

std::vector<std::byte> encode_capi_image(const ctex::image::TiledImage& image,
                                         const ctex::io::TextureEncodeOptions& options) {
    try {
        return ctex::io::encode_texture_memory(image, options);
    } catch (const ctex::io::TextureEncodeError& error) {
        throw_texture_encode_error(error);
    }
}

ctex_cube_lut* create_cube_lut(const ctex_allocator_state& allocator, std::string_view source) {
    void* storage = allocate_storage(allocator, sizeof(ctex_cube_lut), alignof(ctex_cube_lut));
    try {
        return ::new (storage) ctex_cube_lut(allocator, source);
    } catch (...) {
        deallocate_storage(allocator, storage, sizeof(ctex_cube_lut), alignof(ctex_cube_lut));
        throw;
    }
}

ctex::doc::PartitionSourceKind partition_source_kind(std::uint32_t kind) {
    switch (kind) {
        case CTEX_PARTITION_SOURCE_MATERIAL:
            return ctex::doc::PartitionSourceKind::material;
        case CTEX_PARTITION_SOURCE_OBJECT:
            return ctex::doc::PartitionSourceKind::object;
        case CTEX_PARTITION_SOURCE_SUBMESH:
            return ctex::doc::PartitionSourceKind::submesh;
        case CTEX_PARTITION_SOURCE_EXPLICIT_FACES:
            return ctex::doc::PartitionSourceKind::explicit_faces;
    }
    throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE,
                   "partition_kind=" + std::to_string(kind));
}

ctex::mesh::PartitionKind mesh_partition_kind(std::uint32_t kind) {
    switch (kind) {
        case CTEX_PARTITION_SOURCE_MATERIAL:
            return ctex::mesh::PartitionKind::material;
        case CTEX_PARTITION_SOURCE_OBJECT:
            return ctex::mesh::PartitionKind::object;
        case CTEX_PARTITION_SOURCE_SUBMESH:
            return ctex::mesh::PartitionKind::submesh;
        case CTEX_PARTITION_SOURCE_EXPLICIT_FACES:
            return ctex::mesh::PartitionKind::explicit_faces;
    }
    throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_ENUM_VALUE,
                   "partition.kind=" + std::to_string(kind));
}

template <typename Value>
void require_mesh_array(const Value* values, std::size_t count, std::string_view field) {
    if (values == nullptr && count != 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       std::string(field) + "=null with count=" + std::to_string(count));
    }
}

void validate_mesh_limit(std::size_t supplied, std::size_t maximum, std::string_view name) {
    if (supplied > maximum) {
        throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_MESH_LIMIT_EXCEEDED,
                       std::string(name) + " supplied=" + std::to_string(supplied) +
                           " maximum=" + std::to_string(maximum));
    }
}

void validate_mesh_array_pointers(const ctex_mesh_descriptor& descriptor) {
    require_mesh_array(descriptor.positions, descriptor.position_count, "descriptor.positions");
    require_mesh_array(descriptor.normals, descriptor.normal_count, "descriptor.normals");
    require_mesh_array(descriptor.vertex_colors, descriptor.vertex_color_count,
                       "descriptor.vertex_colors");
    require_mesh_array(descriptor.triangle_indices, descriptor.triangle_index_count,
                       "descriptor.triangle_indices");
    require_mesh_array(descriptor.uv_sets, descriptor.uv_set_count, "descriptor.uv_sets");
    require_mesh_array(descriptor.partitions, descriptor.partition_count, "descriptor.partitions");
    require_mesh_array(descriptor.face_partition_indices, descriptor.face_partition_index_count,
                       "descriptor.face_partition_indices");
    require_mesh_array(descriptor.face_material_ids, descriptor.face_material_id_count,
                       "descriptor.face_material_ids");
    if (descriptor.default_uv_set == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "descriptor.default_uv_set=null");
    }
}

void validate_mesh_counts(const ctex_mesh_descriptor& descriptor) {
    validate_mesh_limit(descriptor.position_count, CTEX_MAX_MESH_VERTEX_COUNT, "vertex_count");
    const std::size_t triangle_count = descriptor.triangle_index_count / 3;
    validate_mesh_limit(triangle_count, CTEX_MAX_MESH_TRIANGLE_COUNT, "triangle_count");
    if (descriptor.position_count == 0 || descriptor.normal_count != descriptor.position_count ||
        (descriptor.vertex_color_count != 0 &&
         descriptor.vertex_color_count != descriptor.position_count)) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_MESH,
                       "vertex attributes have inconsistent counts");
    }
    if (descriptor.triangle_index_count == 0 || descriptor.triangle_index_count % 3 != 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_MESH,
                       "triangle_index_count must describe complete triangles");
    }
    if (descriptor.uv_set_count == 0 || descriptor.partition_count == 0 ||
        descriptor.face_partition_index_count != triangle_count ||
        descriptor.face_material_id_count != triangle_count) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_MESH,
                       "UV sets and total face partition metadata are required");
    }
}

void validate_mesh_uv_sets(const ctex_mesh_descriptor& descriptor) {
    for (std::size_t index = 0; index < descriptor.uv_set_count; ++index) {
        const ctex_uv_set_descriptor& uv_set = descriptor.uv_sets[index];
        validate_structure_size(uv_set.size, CTEX_UV_SET_DESCRIPTOR_V1_SIZE,
                                CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, "uv_set.size");
        if (uv_set.name == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "uv_set.name=null at index=" + std::to_string(index));
        }
        require_mesh_array(uv_set.values, uv_set.value_count, "uv_set.values");
        if (uv_set.value_count != descriptor.position_count) {
            throw_boundary(
                CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_MESH,
                "uv_set.value_count does not match vertex_count at index=" + std::to_string(index));
        }
    }
}

void validate_mesh_partitions(const ctex_mesh_descriptor& descriptor) {
    for (std::size_t index = 0; index < descriptor.partition_count; ++index) {
        const ctex_mesh_partition_descriptor& partition = descriptor.partitions[index];
        validate_structure_size(partition.size, CTEX_MESH_PARTITION_DESCRIPTOR_V1_SIZE,
                                CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE, "partition.size");
        if (partition.stable_key == nullptr || partition.display_name == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "partition strings are required at index=" + std::to_string(index));
        }
        static_cast<void>(mesh_partition_kind(partition.kind));
    }
}

void validate_mesh_descriptor(const ctex_mesh_descriptor& descriptor) {
    validate_structure_size(descriptor.size, CTEX_MESH_DESCRIPTOR_V1_SIZE,
                            CTEX_MESH_DESCRIPTOR_CURRENT_SIZE, "descriptor.size");
    validate_mesh_array_pointers(descriptor);
    validate_mesh_counts(descriptor);
    validate_mesh_uv_sets(descriptor);
    validate_mesh_partitions(descriptor);
}

ctex::mesh::Vec2f mesh_vec(ctex_vec2f value) { return {value.x, value.y}; }
ctex::mesh::Vec3f mesh_vec(ctex_vec3f value) { return {value.x, value.y, value.z}; }
ctex::mesh::Vec4f mesh_vec(ctex_vec4f value) { return {value.x, value.y, value.z, value.w}; }

ctex_mesh* create_mesh(const ctex_allocator_state& allocator,
                       const ctex_mesh_descriptor& descriptor) {
    void* storage = allocate_storage(allocator, sizeof(ctex_mesh), alignof(ctex_mesh));
    try {
        return ::new (storage) ctex_mesh(allocator, descriptor);
    } catch (...) {
        deallocate_storage(allocator, storage, sizeof(ctex_mesh), alignof(ctex_mesh));
        throw;
    }
}

ctex_mesh_state* create_mesh_state(ctex_mesh& mesh, const ctex_mesh_descriptor& descriptor) {
    void* storage =
        mesh.memory_resource.allocate(sizeof(ctex_mesh_state), alignof(ctex_mesh_state));
    try {
        return ::new (storage) ctex_mesh_state(descriptor, &mesh.memory_resource);
    } catch (...) {
        mesh.memory_resource.deallocate(storage, sizeof(ctex_mesh_state), alignof(ctex_mesh_state));
        throw;
    }
}

void destroy_mesh_state(ctex_mesh& mesh, ctex_mesh_state* state) noexcept {
    state->~ctex_mesh_state();
    mesh.memory_resource.deallocate(state, sizeof(ctex_mesh_state), alignof(ctex_mesh_state));
}

void validate_descriptor_size(const ctex_texture_set_descriptor& descriptor) {
    if (descriptor.size < CTEX_TEXTURE_SET_DESCRIPTOR_V1_SIZE) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                       "descriptor.size=" + std::to_string(descriptor.size) +
                           " minimum_size=" + std::to_string(CTEX_TEXTURE_SET_DESCRIPTOR_V1_SIZE));
    }
    if (descriptor.size > CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                       "descriptor.size=" + std::to_string(descriptor.size) + " library_size=" +
                           std::to_string(CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE));
    }
}

ctex::doc::TextureSetDescriptor texture_set_descriptor(
    const ctex_texture_set_descriptor& descriptor) {
    validate_descriptor_size(descriptor);
    if (descriptor.display_name == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "descriptor.display_name=null");
    }
    if (descriptor.partition_key == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "descriptor.partition_key=null");
    }
    if (descriptor.uv_set == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "descriptor.uv_set=null");
    }
    if (descriptor.display_name[0] == '\0') {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_DISPLAY_NAME,
                       "descriptor.display_name is empty");
    }
    if (descriptor.partition_key[0] == '\0') {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT,
                       CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_PARTITION_KEY,
                       "descriptor.partition_key is empty");
    }
    if (descriptor.uv_set[0] == '\0') {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_EMPTY_TEXTURE_SET_UV_SET,
                       "descriptor.uv_set is empty");
    }
    if (descriptor.width == 0 || descriptor.height == 0) {
        throw_boundary(
            CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_RESOLUTION,
            "descriptor resolution must be non-zero: width=" + std::to_string(descriptor.width) +
                " height=" + std::to_string(descriptor.height));
    }
    std::uint8_t bit_depth = 8;
    constexpr std::size_t bit_depth_end = offsetof(ctex_texture_set_descriptor, default_bit_depth) +
                                          sizeof(ctex_texture_set_descriptor::default_bit_depth);
    if (descriptor.size >= bit_depth_end) {
        bit_depth = descriptor.default_bit_depth;
    }
    if (bit_depth != 8 && bit_depth != 16 && bit_depth != 32) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_BIT_DEPTH,
                       "descriptor.default_bit_depth=" + std::to_string(bit_depth));
    }
    return {.display_name = descriptor.display_name,
            .partition_kind = partition_source_kind(descriptor.partition_kind),
            .partition_key = descriptor.partition_key,
            .uv_set = descriptor.uv_set,
            .width = descriptor.width,
            .height = descriptor.height,
            .default_bit_depth = bit_depth};
}

void create_texture_sets_from_mesh(ctex_document& document, const ctex_mesh& mesh,
                                   const char* uv_set, std::uint32_t width, std::uint32_t height,
                                   std::uint8_t default_bit_depth) {
    if (uv_set == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT, "uv_set=null");
    }
    const auto matching_uv = std::ranges::find(mesh.state->uv_names, std::string_view(uv_set));
    if (matching_uv == mesh.state->uv_names.end()) {
        throw_boundary(CTEX_RESULT_MISSING_RESOURCE, CTEX_DIAGNOSTIC_MISSING_UV_SET,
                       "UV set is not present: " + std::string(uv_set));
    }
    if (width == 0 || height == 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_RESOLUTION,
                       "texture-set resolution must be non-zero");
    }
    if (!valid_channel_bit_depth(default_bit_depth)) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_TEXTURE_SET_BIT_DEPTH,
                       "default_bit_depth=" + std::to_string(default_bit_depth));
    }
    for (const ctex::mesh::MeshPartition& partition : mesh.state->partition_views) {
        const std::string stable_id =
            ctex::mesh::texture_set_stable_id(partition.kind, partition.stable_key, uv_set);
        if (document.value.contains_texture_set(stable_id)) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_DUPLICATE_TEXTURE_SET,
                           "texture-set identity is already present: " + stable_id);
        }
    }
    const ctex::mesh::MeshView view(mesh.state->descriptor());
    static_cast<void>(document.value.create_texture_sets_from_mesh(view, uv_set, width, height,
                                                                   default_bit_depth));
}

void validate_flag(std::uint32_t value, std::string_view name) {
    if (value > 1) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_STROKE,
                       std::string(name) + "=" + std::to_string(value));
    }
}

ctex::paint::Vec3d stroke_vec(ctex_vec3d value) { return {value.x, value.y, value.z}; }

ctex::paint::StrokeFrame stroke_frame(ctex_stroke_frame value) {
    return {
        .tangent = stroke_vec(value.tangent),
        .bitangent = stroke_vec(value.bitangent),
        .normal = stroke_vec(value.normal),
    };
}

ctex::paint::ResponseMapping stroke_mapping(const ctex_response_mapping_descriptor& descriptor,
                                            std::string_view name) {
    validate_structure_size(descriptor.size, CTEX_RESPONSE_MAPPING_DESCRIPTOR_V1_SIZE,
                            CTEX_RESPONSE_MAPPING_DESCRIPTOR_CURRENT_SIZE,
                            std::string(name) + ".size");
    validate_flag(descriptor.enabled, std::string(name) + ".enabled");
    if (descriptor.points == nullptr && descriptor.point_count != 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       std::string(name) + ".points=null with nonzero point_count");
    }
    ctex::paint::ResponseCurve curve;
    if (descriptor.point_count != 0) {
        curve.points.clear();
        curve.points.reserve(descriptor.point_count);
        for (std::size_t index = 0; index < descriptor.point_count; ++index) {
            curve.points.push_back(
                {descriptor.points[index].input, descriptor.points[index].output});
        }
    }
    return {
        .enabled = descriptor.enabled != 0,
        .curve = std::move(curve),
        .minimum_output = descriptor.minimum_output,
        .maximum_output = descriptor.maximum_output,
    };
}

ctex::paint::TaperSpan stroke_taper_span(const ctex_stroke_taper_span_descriptor& descriptor,
                                         std::string_view name) {
    validate_structure_size(descriptor.size, CTEX_STROKE_TAPER_SPAN_DESCRIPTOR_V1_SIZE,
                            CTEX_STROKE_TAPER_SPAN_DESCRIPTOR_CURRENT_SIZE,
                            std::string(name) + ".size");
    return {
        .unit = static_cast<ctex::paint::TaperUnit>(descriptor.unit),
        .extent = descriptor.extent,
    };
}

ctex::paint::StrokeSettings stroke_settings(const ctex_stroke_settings_descriptor& descriptor) {
    validate_structure_size(descriptor.size, CTEX_STROKE_SETTINGS_DESCRIPTOR_V1_SIZE,
                            CTEX_STROKE_SETTINGS_DESCRIPTOR_CURRENT_SIZE, "settings.size");
    if (descriptor.tip_resource_identity == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "settings.tip_resource_identity=null");
    }
    validate_structure_size(descriptor.stabilizer.size, CTEX_STROKE_STABILIZER_DESCRIPTOR_V1_SIZE,
                            CTEX_STROKE_STABILIZER_DESCRIPTOR_CURRENT_SIZE,
                            "settings.stabilizer.size");
    validate_structure_size(descriptor.jitter.size, CTEX_STROKE_JITTER_DESCRIPTOR_V1_SIZE,
                            CTEX_STROKE_JITTER_DESCRIPTOR_CURRENT_SIZE, "settings.jitter.size");
    validate_structure_size(descriptor.taper.size, CTEX_STROKE_TAPER_DESCRIPTOR_V1_SIZE,
                            CTEX_STROKE_TAPER_DESCRIPTOR_CURRENT_SIZE, "settings.taper.size");
    validate_structure_size(descriptor.constraint.size, CTEX_STROKE_CONSTRAINT_DESCRIPTOR_V1_SIZE,
                            CTEX_STROKE_CONSTRAINT_DESCRIPTOR_CURRENT_SIZE,
                            "settings.constraint.size");
    validate_structure_size(descriptor.symmetry.size, CTEX_STROKE_SYMMETRY_DESCRIPTOR_V1_SIZE,
                            CTEX_STROKE_SYMMETRY_DESCRIPTOR_CURRENT_SIZE, "settings.symmetry.size");
    validate_flag(descriptor.taper.affect_radius, "settings.taper.affect_radius");
    validate_flag(descriptor.taper.affect_opacity, "settings.taper.affect_opacity");
    validate_flag(descriptor.symmetry.mirror_x, "settings.symmetry.mirror_x");
    validate_flag(descriptor.symmetry.mirror_y, "settings.symmetry.mirror_y");
    validate_flag(descriptor.symmetry.mirror_z, "settings.symmetry.mirror_z");
    return {
        .reconstruction_version = descriptor.reconstruction_version,
        .tip_mode = static_cast<ctex::paint::TipMode>(descriptor.tip_mode),
        .spacing_fraction = descriptor.spacing_fraction,
        .radius = descriptor.radius,
        .opacity = descriptor.opacity,
        .hardness = descriptor.hardness,
        .rotation_radians = descriptor.rotation_radians,
        .elongation = descriptor.elongation,
        .flow = descriptor.flow,
        .tip_resource_identity = descriptor.tip_resource_identity,
        .stabilizer = {.radius = descriptor.stabilizer.radius,
                       .time_constant_seconds = descriptor.stabilizer.time_constant_seconds},
        .input_mapping =
            {
                .pressure_radius = stroke_mapping(descriptor.pressure_radius, "pressure_radius"),
                .pressure_opacity = stroke_mapping(descriptor.pressure_opacity, "pressure_opacity"),
                .pressure_hardness =
                    stroke_mapping(descriptor.pressure_hardness, "pressure_hardness"),
                .pressure_flow = stroke_mapping(descriptor.pressure_flow, "pressure_flow"),
                .pressure_rotation =
                    stroke_mapping(descriptor.pressure_rotation, "pressure_rotation"),
                .tilt_rotation = stroke_mapping(descriptor.tilt_rotation, "tilt_rotation"),
                .tilt_elongation = stroke_mapping(descriptor.tilt_elongation, "tilt_elongation"),
            },
        .jitter = {.seed = descriptor.jitter.seed,
                   .position_fraction = descriptor.jitter.position_fraction,
                   .radius_fraction = descriptor.jitter.radius_fraction,
                   .rotation_radians = descriptor.jitter.rotation_radians,
                   .opacity = descriptor.jitter.opacity,
                   .flow = descriptor.jitter.flow},
        .taper = {.entry = stroke_taper_span(descriptor.taper.entry, "taper.entry"),
                  .exit = stroke_taper_span(descriptor.taper.exit, "taper.exit"),
                  .floor = descriptor.taper.floor,
                  .affect_radius = descriptor.taper.affect_radius != 0,
                  .affect_opacity = descriptor.taper.affect_opacity != 0},
        .constraint =
            {
                .mode = static_cast<ctex::paint::ConstraintMode>(descriptor.constraint.mode),
                .grid_step = descriptor.constraint.grid_step,
            },
        .symmetry =
            {
                .mirror_x = descriptor.symmetry.mirror_x != 0,
                .mirror_y = descriptor.symmetry.mirror_y != 0,
                .mirror_z = descriptor.symmetry.mirror_z != 0,
                .radial_count = descriptor.symmetry.radial_count,
                .radial_axis =
                    static_cast<ctex::paint::SymmetryAxis>(descriptor.symmetry.radial_axis),
            },
    };
}

std::vector<ctex::paint::StrokeInputSample> stroke_samples(const ctex_stroke_input_sample* samples,
                                                           std::size_t sample_count) {
    if (samples == nullptr && sample_count != 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "samples=null with nonzero sample_count");
    }
    std::vector<ctex::paint::StrokeInputSample> converted;
    converted.reserve(sample_count);
    for (std::size_t index = 0; index < sample_count; ++index) {
        validate_structure_size(samples[index].size, CTEX_STROKE_INPUT_SAMPLE_V1_SIZE,
                                CTEX_STROKE_INPUT_SAMPLE_CURRENT_SIZE,
                                "samples[" + std::to_string(index) + "].size");
        validate_flag(samples[index].has_pressure,
                      "samples[" + std::to_string(index) + "].has_pressure");
        converted.push_back({
            .position = stroke_vec(samples[index].position),
            .frame = stroke_frame(samples[index].frame),
            .timestamp_nanoseconds = samples[index].timestamp_nanoseconds,
            .pressure = samples[index].has_pressure != 0
                            ? std::optional<double>(samples[index].pressure)
                            : std::nullopt,
            .tilt = {samples[index].tilt.x, samples[index].tilt.y},
        });
    }
    return converted;
}

void validate_output_array(const void* output, std::size_t capacity, std::size_t required,
                           std::string_view name) {
    if (output == nullptr && capacity != 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       std::string(name) + "=null with nonzero capacity");
    }
    if (output != nullptr && capacity < required) {
        throw_boundary(CTEX_RESULT_BUFFER_TOO_SMALL, CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL,
                       std::string(name) + " capacity=" + std::to_string(capacity) +
                           " required=" + std::to_string(required));
    }
}

ctex_vec3d capi_vec(ctex::paint::Vec3d value) { return {value.x, value.y, value.z}; }

ctex_stroke_frame capi_frame(ctex::paint::StrokeFrame value) {
    return {
        .tangent = capi_vec(value.tangent),
        .bitangent = capi_vec(value.bitangent),
        .normal = capi_vec(value.normal),
    };
}

ctex_resolved_stamp capi_stamp(const ctex::paint::Stamp& stamp, const char* tip_identity) {
    return {
        .position = capi_vec(stamp.position),
        .frame = capi_frame(stamp.frame),
        .radius = stamp.radius,
        .opacity = stamp.opacity,
        .hardness = stamp.hardness,
        .rotation_radians = stamp.rotation_radians,
        .elongation = stamp.elongation,
        .flow = stamp.flow,
        .tip_resource_identity = tip_identity,
        .source_ordinal = stamp.source_ordinal,
        .symmetry_instance = stamp.symmetry_instance,
        .ordinal = stamp.ordinal,
    };
}

ctex_response_mapping_descriptor capi_mapping(const ctex::paint::ResponseMapping& mapping) {
    return {
        .size = CTEX_RESPONSE_MAPPING_DESCRIPTOR_CURRENT_SIZE,
        .enabled = mapping.enabled ? 1U : 0U,
        .points = nullptr,
        .point_count = 0,
        .minimum_output = mapping.minimum_output,
        .maximum_output = mapping.maximum_output,
    };
}

ctex_stroke_taper_span_descriptor capi_taper_span(ctex::paint::TaperSpan span) {
    return {
        .size = CTEX_STROKE_TAPER_SPAN_DESCRIPTOR_CURRENT_SIZE,
        .unit = static_cast<std::uint32_t>(span.unit),
        .extent = span.extent,
    };
}

ctex_stroke_settings_descriptor default_stroke_settings() {
    const ctex::paint::StrokeSettings settings;
    return {
        .size = CTEX_STROKE_SETTINGS_DESCRIPTOR_CURRENT_SIZE,
        .reconstruction_version = settings.reconstruction_version,
        .tip_mode = static_cast<std::uint32_t>(settings.tip_mode),
        .spacing_fraction = settings.spacing_fraction,
        .radius = settings.radius,
        .opacity = settings.opacity,
        .hardness = settings.hardness,
        .rotation_radians = settings.rotation_radians,
        .elongation = settings.elongation,
        .flow = settings.flow,
        .tip_resource_identity = "builtin.circle",
        .stabilizer = {.size = CTEX_STROKE_STABILIZER_DESCRIPTOR_CURRENT_SIZE,
                       .radius = settings.stabilizer.radius,
                       .time_constant_seconds = settings.stabilizer.time_constant_seconds},
        .pressure_radius = capi_mapping(settings.input_mapping.pressure_radius),
        .pressure_opacity = capi_mapping(settings.input_mapping.pressure_opacity),
        .pressure_hardness = capi_mapping(settings.input_mapping.pressure_hardness),
        .pressure_flow = capi_mapping(settings.input_mapping.pressure_flow),
        .pressure_rotation = capi_mapping(settings.input_mapping.pressure_rotation),
        .tilt_rotation = capi_mapping(settings.input_mapping.tilt_rotation),
        .tilt_elongation = capi_mapping(settings.input_mapping.tilt_elongation),
        .jitter = {.size = CTEX_STROKE_JITTER_DESCRIPTOR_CURRENT_SIZE,
                   .seed = settings.jitter.seed,
                   .position_fraction = settings.jitter.position_fraction,
                   .radius_fraction = settings.jitter.radius_fraction,
                   .rotation_radians = settings.jitter.rotation_radians,
                   .opacity = settings.jitter.opacity,
                   .flow = settings.jitter.flow},
        .taper = {.size = CTEX_STROKE_TAPER_DESCRIPTOR_CURRENT_SIZE,
                  .entry = capi_taper_span(settings.taper.entry),
                  .exit = capi_taper_span(settings.taper.exit),
                  .floor = settings.taper.floor,
                  .affect_radius = settings.taper.affect_radius ? 1U : 0U,
                  .affect_opacity = settings.taper.affect_opacity ? 1U : 0U},
        .constraint = {.size = CTEX_STROKE_CONSTRAINT_DESCRIPTOR_CURRENT_SIZE,
                       .mode = static_cast<std::uint32_t>(settings.constraint.mode),
                       .grid_step = settings.constraint.grid_step},
        .symmetry = {.size = CTEX_STROKE_SYMMETRY_DESCRIPTOR_CURRENT_SIZE,
                     .mirror_x = settings.symmetry.mirror_x ? 1U : 0U,
                     .mirror_y = settings.symmetry.mirror_y ? 1U : 0U,
                     .mirror_z = settings.symmetry.mirror_z ? 1U : 0U,
                     .radial_count = settings.symmetry.radial_count,
                     .radial_axis = static_cast<std::uint32_t>(settings.symmetry.radial_axis)},
    };
}

std::array<const ctex::paint::ResponseMapping*, 7> stroke_mappings(
    const ctex::paint::StrokeSettings& settings) {
    return {
        &settings.input_mapping.pressure_radius,   &settings.input_mapping.pressure_opacity,
        &settings.input_mapping.pressure_hardness, &settings.input_mapping.pressure_flow,
        &settings.input_mapping.pressure_rotation, &settings.input_mapping.tilt_rotation,
        &settings.input_mapping.tilt_elongation,
    };
}

std::size_t stroke_curve_point_count(const ctex::paint::StrokeSettings& settings) {
    std::size_t result = 0;
    for (const ctex::paint::ResponseMapping* mapping : stroke_mappings(settings)) {
        if (mapping->curve.points.size() > std::numeric_limits<std::size_t>::max() - result) {
            throw std::overflow_error("stroke preset curve-point count overflow");
        }
        result += mapping->curve.points.size();
    }
    return result;
}

ctex_response_mapping_descriptor copy_capi_mapping(const ctex::paint::ResponseMapping& mapping,
                                                   ctex_response_curve_point*& destination) {
    ctex_response_mapping_descriptor result = capi_mapping(mapping);
    result.points = destination;
    result.point_count = mapping.curve.points.size();
    for (const ctex::paint::ResponseCurvePoint point : mapping.curve.points) {
        *destination = {point.input, point.output};
        ++destination;
    }
    return result;
}

ctex_stroke_settings_descriptor copy_capi_stroke_settings(
    const ctex::paint::StrokeSettings& settings, const char* tip_resource_identity,
    ctex_response_curve_point* curve_points) {
    ctex_stroke_settings_descriptor result = default_stroke_settings();
    result.reconstruction_version = settings.reconstruction_version;
    result.tip_mode = static_cast<std::uint32_t>(settings.tip_mode);
    result.spacing_fraction = settings.spacing_fraction;
    result.radius = settings.radius;
    result.opacity = settings.opacity;
    result.hardness = settings.hardness;
    result.rotation_radians = settings.rotation_radians;
    result.elongation = settings.elongation;
    result.flow = settings.flow;
    result.tip_resource_identity = tip_resource_identity;
    result.stabilizer.radius = settings.stabilizer.radius;
    result.stabilizer.time_constant_seconds = settings.stabilizer.time_constant_seconds;
    result.pressure_radius =
        copy_capi_mapping(settings.input_mapping.pressure_radius, curve_points);
    result.pressure_opacity =
        copy_capi_mapping(settings.input_mapping.pressure_opacity, curve_points);
    result.pressure_hardness =
        copy_capi_mapping(settings.input_mapping.pressure_hardness, curve_points);
    result.pressure_flow = copy_capi_mapping(settings.input_mapping.pressure_flow, curve_points);
    result.pressure_rotation =
        copy_capi_mapping(settings.input_mapping.pressure_rotation, curve_points);
    result.tilt_rotation = copy_capi_mapping(settings.input_mapping.tilt_rotation, curve_points);
    result.tilt_elongation =
        copy_capi_mapping(settings.input_mapping.tilt_elongation, curve_points);
    result.jitter = {
        .size = CTEX_STROKE_JITTER_DESCRIPTOR_CURRENT_SIZE,
        .seed = settings.jitter.seed,
        .position_fraction = settings.jitter.position_fraction,
        .radius_fraction = settings.jitter.radius_fraction,
        .rotation_radians = settings.jitter.rotation_radians,
        .opacity = settings.jitter.opacity,
        .flow = settings.jitter.flow,
    };
    result.taper = {
        .size = CTEX_STROKE_TAPER_DESCRIPTOR_CURRENT_SIZE,
        .entry = capi_taper_span(settings.taper.entry),
        .exit = capi_taper_span(settings.taper.exit),
        .floor = settings.taper.floor,
        .affect_radius = settings.taper.affect_radius ? 1U : 0U,
        .affect_opacity = settings.taper.affect_opacity ? 1U : 0U,
    };
    result.constraint = {
        .size = CTEX_STROKE_CONSTRAINT_DESCRIPTOR_CURRENT_SIZE,
        .mode = static_cast<std::uint32_t>(settings.constraint.mode),
        .grid_step = settings.constraint.grid_step,
    };
    result.symmetry = {
        .size = CTEX_STROKE_SYMMETRY_DESCRIPTOR_CURRENT_SIZE,
        .mirror_x = settings.symmetry.mirror_x ? 1U : 0U,
        .mirror_y = settings.symmetry.mirror_y ? 1U : 0U,
        .mirror_z = settings.symmetry.mirror_z ? 1U : 0U,
        .radial_count = settings.symmetry.radial_count,
        .radial_axis = static_cast<std::uint32_t>(settings.symmetry.radial_axis),
    };
    return result;
}

void validate_preset_buffers(const ctex_stroke_preset_buffers_descriptor& buffers,
                             const ctex_stroke_preset_info& info) {
    validate_structure_size(buffers.size, CTEX_STROKE_PRESET_BUFFERS_DESCRIPTOR_V1_SIZE,
                            CTEX_STROKE_PRESET_BUFFERS_DESCRIPTOR_CURRENT_SIZE, "buffers.size");
    if (buffers.name_buffer == nullptr || buffers.tip_resource_identity_buffer == nullptr ||
        buffers.curve_points == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "preset output buffers are required");
    }
    validate_output_array(buffers.name_buffer, buffers.name_buffer_size, info.required_name_size,
                          "name_buffer");
    validate_output_array(buffers.tip_resource_identity_buffer,
                          buffers.tip_resource_identity_buffer_size,
                          info.required_tip_resource_identity_size, "tip_resource_identity_buffer");
    validate_output_array(buffers.curve_points, buffers.curve_point_capacity,
                          info.required_curve_point_count, "curve_points");
}

struct PaintMeshData {
    std::vector<ctex::paint::Vec3d> positions;
    std::vector<ctex::paint::Vec3d> normals;
    std::vector<ctex::paint::Vec2d> uv;
    std::span<const std::uint32_t> triangle_indices;

    [[nodiscard]] ctex::paint::TextureSpaceMeshView view() const noexcept {
        return {positions, normals, uv, triangle_indices};
    }
};

PaintMeshData paint_mesh_data(const ctex_mesh& mesh, const char* uv_set) {
    if (uv_set == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "tile.uv_set=null");
    }
    const auto found = std::ranges::find(mesh.state->uv_views, std::string_view(uv_set),
                                         &ctex::mesh::UvSetView::name);
    if (found == mesh.state->uv_views.end()) {
        throw_boundary(CTEX_RESULT_MISSING_RESOURCE, CTEX_DIAGNOSTIC_MISSING_UV_SET,
                       "UV set is not present: " + std::string(uv_set));
    }
    PaintMeshData result;
    result.positions.reserve(mesh.state->positions.size());
    result.normals.reserve(mesh.state->normals.size());
    result.uv.reserve(found->values.size());
    std::transform(mesh.state->positions.begin(), mesh.state->positions.end(),
                   std::back_inserter(result.positions), [](ctex::mesh::Vec3f value) {
                       return ctex::paint::Vec3d{value.x, value.y, value.z};
                   });
    std::transform(
        mesh.state->normals.begin(), mesh.state->normals.end(), std::back_inserter(result.normals),
        [](ctex::mesh::Vec3f value) { return ctex::paint::Vec3d{value.x, value.y, value.z}; });
    std::transform(found->values.begin(), found->values.end(), std::back_inserter(result.uv),
                   [](ctex::mesh::Vec2f value) { return ctex::paint::Vec2d{value.x, value.y}; });
    result.triangle_indices = mesh.state->triangle_indices;
    return result;
}

ctex::paint::Stamp paint_stamp(const ctex_resolved_stamp& stamp) {
    if (stamp.tip_resource_identity == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "stroke stamp tip_resource_identity=null");
    }
    return {
        .position = stroke_vec(stamp.position),
        .frame = stroke_frame(stamp.frame),
        .radius = stamp.radius,
        .opacity = stamp.opacity,
        .hardness = stamp.hardness,
        .rotation_radians = stamp.rotation_radians,
        .elongation = stamp.elongation,
        .flow = stamp.flow,
        .tip_resource_identity = stamp.tip_resource_identity,
        .source_ordinal = stamp.source_ordinal,
        .symmetry_instance = stamp.symmetry_instance,
        .ordinal = stamp.ordinal,
    };
}

ctex::paint::ResolvedStroke paint_stroke(const ctex_resolved_stroke_descriptor& descriptor) {
    validate_structure_size(descriptor.size, CTEX_RESOLVED_STROKE_DESCRIPTOR_V1_SIZE,
                            CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE, "stroke.size");
    if ((descriptor.stamps == nullptr && descriptor.stamp_count != 0) ||
        (descriptor.swept_segments == nullptr && descriptor.swept_segment_count != 0)) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "stroke arrays are null with nonzero counts");
    }
    ctex::paint::ResolvedStroke result{
        .reconstruction_version = descriptor.reconstruction_version,
        .tip_mode = static_cast<ctex::paint::TipMode>(descriptor.tip_mode),
        .symmetry_instance_count = descriptor.symmetry_instance_count,
        .stamps = {},
        .swept_segments = {},
    };
    result.stamps.reserve(descriptor.stamp_count);
    for (std::size_t index = 0; index < descriptor.stamp_count; ++index) {
        result.stamps.push_back(paint_stamp(descriptor.stamps[index]));
    }
    result.swept_segments.reserve(descriptor.swept_segment_count);
    for (std::size_t index = 0; index < descriptor.swept_segment_count; ++index) {
        result.swept_segments.push_back({
            descriptor.swept_segments[index].start_stamp_ordinal,
            descriptor.swept_segments[index].end_stamp_ordinal,
        });
    }
    return result;
}

std::size_t bounded_paint_pixel_count(std::uint32_t width, std::uint32_t height,
                                      ctex_diagnostic_code invalid_dimensions_code) {
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / height) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, invalid_dimensions_code,
                       "paint dimensions are invalid");
    }
    const std::size_t count = static_cast<std::size_t>(width) * height;
    if (count > CTEX_MAX_PAINT_TILE_TEXEL_COUNT) {
        throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED,
                       "paint pixel_count=" + std::to_string(count) +
                           " maximum=" + std::to_string(CTEX_MAX_PAINT_TILE_TEXEL_COUNT));
    }
    return count;
}

std::size_t paint_tile_texel_count(const ctex_paint_tile_coverage_descriptor& tile) {
    validate_structure_size(tile.size, CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_V1_SIZE,
                            CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE, "tile.size");
    return bounded_paint_pixel_count(tile.width, tile.height,
                                     CTEX_DIAGNOSTIC_INVALID_PAINT_COVERAGE);
}

ctex::paint::TextureSpaceRaster paint_tile_surface(
    const ctex_mesh& mesh, const ctex_paint_tile_coverage_descriptor& tile) {
    const PaintMeshData converted_mesh = paint_mesh_data(mesh, tile.uv_set);
    return ctex::paint::rasterize_texture_space(
        converted_mesh.view(), {.width = tile.width,
                                .height = tile.height,
                                .tile_origin = {tile.tile_origin.x, tile.tile_origin.y}});
}

ctex::paint::MaterialCoordinateRequest paint_material_coordinate_request(
    const ctex_paint_material_coordinate_descriptor& descriptor) {
    ctex::paint::MaterialCoordinateMode mode;
    switch (descriptor.mode) {
        case CTEX_PAINT_MATERIAL_COORDINATE_UV:
            mode = ctex::paint::MaterialCoordinateMode::uv;
            break;
        case CTEX_PAINT_MATERIAL_COORDINATE_TRIPLANAR:
            mode = ctex::paint::MaterialCoordinateMode::triplanar;
            break;
        case CTEX_PAINT_MATERIAL_COORDINATE_PLANAR:
            mode = ctex::paint::MaterialCoordinateMode::planar;
            break;
        default:
            throw std::invalid_argument("paint material coordinate mode is invalid");
    }
    return {
        .mode = mode,
        .planar =
            {
                .origin = stroke_vec(descriptor.planar_origin),
                .u_axis = stroke_vec(descriptor.planar_u_axis),
                .v_axis = stroke_vec(descriptor.planar_v_axis),
            },
    };
}

void validate_material_coordinate_request(const ctex::paint::MaterialCoordinateRequest& request) {
    const ctex::paint::SurfaceTexel validation_texel{
        .position = {},
        .normal = {0.0, 0.0, 1.0},
        .geometric_normal = {0.0, 0.0, 1.0},
        .uv = {},
        .triangle = 0,
    };
    static_cast<void>(ctex::paint::material_coordinates(validation_texel, request));
}

ctex_paint_material_coordinate_sample capi_material_coordinates(
    const ctex::paint::MaterialCoordinates& coordinates) {
    ctex_paint_material_coordinate_sample result{
        .covered = 1,
        .projection_count = static_cast<std::uint32_t>(coordinates.count),
        .coordinates = {},
        .weights = {},
    };
    for (std::size_t index = 0; index < coordinates.count; ++index) {
        result.coordinates[index] = {coordinates.projections[index].coordinate.x,
                                     coordinates.projections[index].coordinate.y};
        result.weights[index] = coordinates.projections[index].weight;
    }
    return result;
}

bool paint_flag(std::uint32_t value, std::string_view name) {
    if (value > 1) {
        throw std::invalid_argument(std::string(name) + " flag is invalid");
    }
    return value != 0;
}

ctex::paint::SymmetryDepthPolicy paint_symmetry_depth_policy(std::uint32_t value) {
    switch (value) {
        case CTEX_PAINT_SYMMETRY_DEPTH_REQUIRE_CONSISTENT:
            return ctex::paint::SymmetryDepthPolicy::require_consistent_per_instance;
        case CTEX_PAINT_SYMMETRY_DEPTH_DISABLE_DERIVED:
            return ctex::paint::SymmetryDepthPolicy::disable_for_derived_symmetry;
        default:
            throw std::invalid_argument("paint symmetry depth policy is invalid");
    }
}

std::span<const double> paint_double_view(const double* values, std::size_t count,
                                          std::string_view name) {
    if (values == nullptr && count != 0) {
        throw std::invalid_argument(std::string(name) + " is null with nonzero count");
    }
    return count == 0 ? std::span<const double>{} : std::span<const double>(values, count);
}

struct PaintRejectionStorage {
    ctex::paint::RejectionSettings settings;
    std::vector<std::vector<ctex::paint::Vec2d>> screen_positions;
    std::vector<ctex::paint::DepthProjectionContext> depth_contexts;
    std::vector<ctex::paint::Vec3d> view_directions;

    [[nodiscard]] ctex::paint::RejectionInput input() const {
        return {.depth_contexts = depth_contexts, .view_directions = view_directions};
    }
};

void validate_paint_rejection_arrays(const ctex_paint_rejection_descriptor& descriptor,
                                     std::size_t texel_count,
                                     std::uint64_t symmetry_instance_count) {
    if ((descriptor.depth_contexts == nullptr && descriptor.depth_context_count != 0) ||
        (descriptor.view_directions == nullptr && descriptor.view_direction_count != 0)) {
        throw std::invalid_argument("paint rejection arrays are null with nonzero counts");
    }
    if (descriptor.depth_context_count > symmetry_instance_count ||
        (descriptor.view_direction_count != 0 && descriptor.view_direction_count != texel_count)) {
        throw std::invalid_argument("paint rejection array count is inconsistent");
    }
}

void append_paint_depth_context(PaintRejectionStorage& storage,
                                const ctex_paint_depth_context_descriptor& context,
                                std::size_t texel_count) {
    validate_structure_size(context.size, CTEX_PAINT_DEPTH_CONTEXT_DESCRIPTOR_V1_SIZE,
                            CTEX_PAINT_DEPTH_CONTEXT_DESCRIPTOR_CURRENT_SIZE, "depth_context.size");
    if (context.surface_sample_count != texel_count) {
        throw std::invalid_argument("depth context surface sample count is inconsistent");
    }
    if (context.screen_positions == nullptr && context.surface_sample_count != 0) {
        throw std::invalid_argument("depth context screen_positions is null with nonzero count");
    }
    std::vector<ctex::paint::Vec2d>& positions = storage.screen_positions.emplace_back();
    positions.reserve(context.surface_sample_count);
    for (std::size_t sample = 0; sample < context.surface_sample_count; ++sample) {
        positions.push_back(
            {context.screen_positions[sample].x, context.screen_positions[sample].y});
    }
    storage.depth_contexts.push_back({
        .symmetry_instance = context.symmetry_instance,
        .viewport_width = context.viewport_width,
        .viewport_height = context.viewport_height,
        .screen_positions = positions,
        .surface_depth =
            paint_double_view(context.surface_depth, context.surface_sample_count, "surface_depth"),
        .visible_depth =
            paint_double_view(context.visible_depth, context.visible_depth_count, "visible_depth"),
        .transform_consistent = paint_flag(context.transform_consistent, "transform_consistent"),
    });
}

PaintRejectionStorage paint_rejection_storage(const ctex_paint_rejection_descriptor& descriptor,
                                              std::size_t texel_count,
                                              std::uint64_t symmetry_instance_count) {
    validate_paint_rejection_arrays(descriptor, texel_count, symmetry_instance_count);
    PaintRejectionStorage storage{
        .settings =
            {
                .depth_enabled = paint_flag(descriptor.depth_enabled, "depth_enabled"),
                .depth_bias = descriptor.depth_bias,
                .symmetry_depth_policy =
                    paint_symmetry_depth_policy(descriptor.symmetry_depth_policy),
                .angle_enabled = paint_flag(descriptor.angle_enabled, "angle_enabled"),
                .minimum_normal_dot = descriptor.minimum_normal_dot,
                .backface_enabled = paint_flag(descriptor.backface_enabled, "backface_enabled"),
            },
        .screen_positions = {},
        .depth_contexts = {},
        .view_directions = {},
    };
    storage.screen_positions.reserve(descriptor.depth_context_count);
    storage.depth_contexts.reserve(descriptor.depth_context_count);
    for (std::size_t index = 0; index < descriptor.depth_context_count; ++index) {
        append_paint_depth_context(storage, descriptor.depth_contexts[index], texel_count);
    }
    storage.view_directions.reserve(descriptor.view_direction_count);
    for (std::size_t index = 0; index < descriptor.view_direction_count; ++index) {
        storage.view_directions.push_back(stroke_vec(descriptor.view_directions[index]));
    }
    return storage;
}

std::uint32_t capi_depth_disposition(ctex::paint::DepthRejectionDisposition disposition) {
    switch (disposition) {
        case ctex::paint::DepthRejectionDisposition::disabled_by_operation:
            return CTEX_PAINT_DEPTH_DISABLED_BY_OPERATION;
        case ctex::paint::DepthRejectionDisposition::consistent_per_instance:
            return CTEX_PAINT_DEPTH_CONSISTENT_PER_INSTANCE;
        case ctex::paint::DepthRejectionDisposition::disabled_for_derived_symmetry:
            return CTEX_PAINT_DEPTH_DISABLED_FOR_DERIVED_SYMMETRY;
    }
    throw std::invalid_argument("paint depth disposition is invalid");
}

ctex_paint_rejection_info capi_rejection_info(const ctex::paint::RejectionReport& report) {
    return {
        .size = CTEX_PAINT_REJECTION_INFO_CURRENT_SIZE,
        .depth_disposition = capi_depth_disposition(report.depth_disposition),
        .depth_rejected_contributions = report.depth_rejected_contributions,
        .angle_rejected_contributions = report.angle_rejected_contributions,
        .backface_rejected_texels = report.backface_rejected_texels,
        .resolved_depth_bias = report.resolved_settings.depth_bias,
        .resolved_minimum_normal_dot = report.resolved_settings.minimum_normal_dot,
        .depth_bias_clamped =
            report.parameter_report.clamp_for("rejection.depth_bias").has_value() ? 1U : 0U,
        .minimum_normal_dot_clamped =
            report.parameter_report.clamp_for("rejection.minimum_normal_dot").has_value() ? 1U : 0U,
    };
}

std::vector<ctex::paint::StampTexelFootprint> paint_work_footprints(
    const ctex_paint_work_descriptor& work) {
    std::vector<ctex::paint::StampTexelFootprint> footprints;
    footprints.reserve(work.stamp_footprint_count);
    for (std::size_t index = 0; index < work.stamp_footprint_count; ++index) {
        const ctex_paint_stamp_footprint& footprint = work.stamp_footprints[index];
        footprints.push_back({
            .stamp_ordinal = footprint.stamp_ordinal,
            .minimum_x = footprint.minimum_x,
            .minimum_y = footprint.minimum_y,
            .maximum_x = footprint.maximum_x,
            .maximum_y = footprint.maximum_y,
        });
    }
    return footprints;
}

ctex_paint_work_info capi_paint_work_info(const ctex::paint::PaintWorkReport& report) {
    return {
        .size = CTEX_PAINT_WORK_INFO_CURRENT_SIZE,
        .canvas_tile_count = report.canvas_tile_count,
        .footprint_count = report.footprint_count,
        .candidate_tile_visits = report.candidate_tile_visits,
        .processed_tile_count = report.processed_tiles.size(),
        .resolved_dilation_radius = report.dilation_radius,
        .dilation_radius_clamped =
            report.parameter_report.clamp_for("seam_dilation.radius").has_value() ? 1U : 0U,
    };
}

void copy_paint_work_tiles(const ctex::paint::PaintWorkReport& report,
                           ctex_paint_tile_coordinate* processed_tiles) {
    if (processed_tiles == nullptr) {
        return;
    }
    for (std::size_t index = 0; index < report.processed_tiles.size(); ++index) {
        processed_tiles[index] = {
            .x = report.processed_tiles[index].x,
            .y = report.processed_tiles[index].y,
        };
    }
}

std::size_t validate_seam_dilation_arrays(std::uint32_t width, std::uint32_t height,
                                          std::uint32_t component_count, const double* pixels,
                                          std::size_t pixel_count, const std::uint8_t* coverage,
                                          std::size_t coverage_count) {
    if (component_count == 0 || component_count > 4) {
        throw std::invalid_argument("seam-dilation component_count must be between one and four");
    }
    const std::size_t texel_count =
        bounded_paint_pixel_count(width, height, CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION);
    const std::size_t required_pixel_count = texel_count * component_count;
    if (pixel_count != required_pixel_count || coverage_count != texel_count) {
        throw std::invalid_argument("seam-dilation input counts are inconsistent");
    }
    if (pixels == nullptr || coverage == nullptr) {
        throw std::invalid_argument("seam-dilation input arrays are required");
    }
    return required_pixel_count;
}

std::size_t validate_capi_seam_dilation(const ctex_paint_seam_dilation_descriptor& dilation) {
    return validate_seam_dilation_arrays(dilation.width, dilation.height, dilation.component_count,
                                         dilation.pixels, dilation.pixel_count, dilation.coverage,
                                         dilation.coverage_count);
}

std::size_t validate_capi_dilation_tile(const ctex_paint_dilation_tile_descriptor& tile) {
    return validate_seam_dilation_arrays(tile.width, tile.height, tile.component_count, tile.pixels,
                                         tile.pixel_count, tile.coverage, tile.coverage_count);
}

ctex_paint_seam_dilation_info capi_seam_dilation_info(
    const ctex::paint::SeamDilationResult& result) {
    return {
        .size = CTEX_PAINT_SEAM_DILATION_INFO_CURRENT_SIZE,
        .required_pixel_count = result.raster.pixels.size(),
        .dilated_texel_count = result.dilated_texel_count,
        .zero_gradient_texel_count = result.zero_gradient_texel_count,
        .resolved_radius = result.radius,
        .radius_clamped =
            result.parameter_report.clamp_for("seam_dilation.radius").has_value() ? 1U : 0U,
    };
}

bool dilation_tile_before(const ctex_paint_dilation_session_tile& tile, std::int32_t u,
                          std::int32_t v) {
    return tile.u < u || (tile.u == u && tile.v < v);
}

void stage_capi_dilation_tile(ctex_paint_dilation_session& session,
                              const ctex_paint_dilation_tile_descriptor& descriptor) {
    static_cast<void>(validate_capi_dilation_tile(descriptor));
    if (session.finished) {
        throw std::logic_error("cannot stage a UV tile after stroke dilation finished");
    }
    ctex_paint_dilation_session_tile staged(&session.memory_resource, descriptor);
    const auto existing = std::lower_bound(session.tiles.begin(), session.tiles.end(), descriptor,
                                           [](const ctex_paint_dilation_session_tile& tile,
                                              const ctex_paint_dilation_tile_descriptor& value) {
                                               return dilation_tile_before(tile, value.u, value.v);
                                           });
    if (existing == session.tiles.end()) {
        session.tiles.push_back(std::move(staged));
    } else if (existing->u != descriptor.u || existing->v != descriptor.v) {
        session.tiles.insert(existing, std::move(staged));
    } else {
        *existing = std::move(staged);
    }
}

ctex::paint::SeamDilationRaster capi_dilation_raster(const ctex_paint_dilation_session_tile& tile) {
    return {
        .width = tile.width,
        .height = tile.height,
        .component_count = static_cast<std::uint8_t>(tile.component_count),
        .pixels = std::vector<double>(tile.pixels.begin(), tile.pixels.end()),
    };
}

void finish_capi_dilation_session(ctex_paint_dilation_session& session) {
    if (session.finished) {
        return;
    }
    std::pmr::vector<ctex_paint_dilation_session_tile> completed(&session.memory_resource);
    completed.reserve(session.tiles.size());
    for (const ctex_paint_dilation_session_tile& tile : session.tiles) {
        const ctex::paint::SeamDilationResult result =
            ctex::paint::dilate_uv_seams(capi_dilation_raster(tile), tile.coverage, session.radius);
        completed.emplace_back(&session.memory_resource, tile, result);
    }
    session.tiles = std::move(completed);
    session.dilation_pass_count = session.radius == 0 ? 0 : session.tiles.size();
    session.finished = true;
}

std::size_t capi_dilation_session_pixel_count(const ctex_paint_dilation_session& session) {
    std::size_t count = 0;
    for (const ctex_paint_dilation_session_tile& tile : session.tiles) {
        if (tile.pixels.size() > std::numeric_limits<std::size_t>::max() - count) {
            throw std::length_error("deferred seam-dilation pixel count exceeds address space");
        }
        count += tile.pixels.size();
    }
    return count;
}

ctex_paint_dilation_session_info capi_dilation_session_info(
    const ctex_paint_dilation_session& session, std::size_t pixel_count) {
    return {
        .size = CTEX_PAINT_DILATION_SESSION_INFO_CURRENT_SIZE,
        .state = session.finished ? CTEX_PAINT_DILATION_FINAL : CTEX_PAINT_DILATION_PROVISIONAL,
        .tile_count = session.tiles.size(),
        .required_pixel_count = pixel_count,
        .dilation_pass_count = session.dilation_pass_count,
        .resolved_radius = session.radius,
        .radius_clamped = session.radius_clamped ? 1U : 0U,
    };
}

void copy_capi_dilation_session_output(const ctex_paint_dilation_session& session,
                                       ctex_paint_dilation_tile_info* tile_info, double* pixels) {
    std::size_t offset = 0;
    for (std::size_t index = 0; index < session.tiles.size(); ++index) {
        const ctex_paint_dilation_session_tile& tile = session.tiles[index];
        if (tile_info != nullptr) {
            tile_info[index] = {
                .u = tile.u,
                .v = tile.v,
                .width = tile.width,
                .height = tile.height,
                .component_count = tile.component_count,
                .pixel_offset = offset,
                .pixel_count = tile.pixels.size(),
                .dilated_texel_count = tile.dilated_texel_count,
                .zero_gradient_texel_count = tile.zero_gradient_texel_count,
            };
        }
        if (pixels != nullptr) {
            std::copy(tile.pixels.begin(), tile.pixels.end(), pixels + offset);
        }
        offset += tile.pixels.size();
    }
}

void write_capi_dilation_session_output(const ctex_paint_dilation_session& session,
                                        ctex_paint_dilation_session_info& out_info,
                                        ctex_paint_dilation_tile_info* tiles,
                                        std::size_t tile_capacity, std::size_t& out_tile_count,
                                        double* pixels, std::size_t pixel_capacity,
                                        std::size_t& out_pixel_count) {
    const std::size_t pixel_count = capi_dilation_session_pixel_count(session);
    out_tile_count = session.tiles.size();
    out_pixel_count = pixel_count;
    validate_output_array(tiles, tile_capacity, session.tiles.size(), "tiles");
    validate_output_array(pixels, pixel_capacity, pixel_count, "pixels");
    out_info = capi_dilation_session_info(session, pixel_count);
    copy_capi_dilation_session_output(session, tiles, pixels);
}

ctex::paint::SurfaceFilterOperation capi_surface_filter_operation(std::uint32_t operation) {
    switch (operation) {
        case CTEX_PAINT_SURFACE_FILTER_BLUR:
            return ctex::paint::SurfaceFilterOperation::blur;
        case CTEX_PAINT_SURFACE_FILTER_SMEAR:
            return ctex::paint::SurfaceFilterOperation::smear;
        case CTEX_PAINT_SURFACE_FILTER_DERIVATIVE:
            return ctex::paint::SurfaceFilterOperation::derivative;
        case CTEX_PAINT_SURFACE_FILTER_MIP_GENERATION:
            return ctex::paint::SurfaceFilterOperation::mip_generation;
        default:
            throw std::invalid_argument("surface-filter operation is invalid");
    }
}

std::vector<ctex::paint::SurfaceAdjacentSample> capi_surface_filter_samples(
    const ctex_paint_surface_filter_descriptor& descriptor) {
    if (descriptor.samples == nullptr && descriptor.sample_count != 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "filter.samples=null with nonzero count");
    }
    if (descriptor.sample_count > CTEX_MAX_PAINT_TILE_TEXEL_COUNT) {
        throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED,
                       "surface-filter sample count exceeds the paint tile limit");
    }
    std::vector<ctex::paint::SurfaceAdjacentSample> samples;
    samples.reserve(descriptor.sample_count);
    for (std::size_t index = 0; index < descriptor.sample_count; ++index) {
        const ctex_paint_surface_filter_sample& sample = descriptor.samples[index];
        samples.push_back({
            .texel_index = sample.texel_index,
            .tangent_frame = stroke_frame(sample.tangent_frame),
            .offset_x = sample.offset_x,
            .offset_y = sample.offset_y,
            .weight = sample.weight,
        });
    }
    return samples;
}

ctex::paint::SurfaceFilterRequest capi_surface_filter_request(
    const ctex_paint_surface_filter_descriptor& descriptor,
    std::span<const ctex::paint::SurfaceAdjacentSample> samples) {
    return {
        .operation = capi_surface_filter_operation(descriptor.operation),
        .footprint = {.radius_x = descriptor.radius_x, .radius_y = descriptor.radius_y},
        .output_frame = stroke_frame(descriptor.output_frame),
        .samples = samples,
    };
}

struct CapiIslandPaddingInput {
    std::size_t texel_count{};
    std::size_t pixel_count{};
};

void validate_capi_surface_filter_value_count(std::size_t value_count) {
    if (value_count == 0) {
        throw std::invalid_argument("surface-filter values must not be empty");
    }
    if (value_count > CTEX_MAX_PAINT_TILE_TEXEL_COUNT) {
        throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED,
                       "surface-filter value count exceeds the paint tile limit");
    }
}

CapiIslandPaddingInput capi_island_padding_input(
    const ctex_paint_island_padding_descriptor& descriptor) {
    const std::size_t texel_count = bounded_paint_pixel_count(descriptor.width, descriptor.height,
                                                              CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER);
    if (descriptor.component_count == 0 || descriptor.component_count > 4) {
        throw std::invalid_argument("island-padding component_count must be between one and four");
    }
    const std::size_t pixel_count = texel_count * descriptor.component_count;
    if (descriptor.island_identity_count != texel_count || descriptor.pixel_count != pixel_count) {
        throw std::invalid_argument("island-padding input counts are inconsistent");
    }
    if (descriptor.island_identity == nullptr || descriptor.pixels == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       descriptor.island_identity == nullptr ? "padding.island_identity is required"
                                                             : "padding.pixels is required");
    }
    return {
        .texel_count = texel_count,
        .pixel_count = pixel_count,
    };
}

ctex::paint::SeamDilationRaster capi_island_padding_raster(
    const ctex_paint_island_padding_descriptor& descriptor) {
    return {
        .width = descriptor.width,
        .height = descriptor.height,
        .component_count = static_cast<std::uint8_t>(descriptor.component_count),
        .pixels =
            std::vector<double>(descriptor.pixels, descriptor.pixels + descriptor.pixel_count),
    };
}

ctex::paint::IslandPaddingPlan capi_island_padding_plan(
    const ctex_paint_island_padding_descriptor& descriptor) {
    return ctex::paint::plan_island_padding(
        descriptor.width, descriptor.height,
        {descriptor.island_identity, descriptor.island_identity_count},
        {.radius_x = descriptor.radius_x, .radius_y = descriptor.radius_y},
        descriptor.requested_mip_levels);
}

std::size_t capi_affected_island_count(const ctex::paint::IslandPaddingPlan& plan) {
    std::size_t count = 0;
    for (const ctex::paint::UnsupportedMipLevel& level : plan.unsupported_mip_levels) {
        if (level.affected_islands.size() > std::numeric_limits<std::size_t>::max() - count) {
            throw std::length_error("island-padding report exceeds address space");
        }
        count += level.affected_islands.size();
    }
    return count;
}

ctex_paint_island_padding_info capi_island_padding_info(const CapiIslandPaddingInput& input,
                                                        const ctex::paint::IslandPaddingPlan& plan,
                                                        std::size_t affected_island_count,
                                                        std::size_t padded_texel_count) {
    return {
        .size = CTEX_PAINT_ISLAND_PADDING_INFO_CURRENT_SIZE,
        .required_ownership_count = input.texel_count,
        .unsupported_mip_level_count = plan.unsupported_mip_levels.size(),
        .required_affected_island_count = affected_island_count,
        .required_pixel_count = input.pixel_count,
        .padded_texel_count = padded_texel_count,
        .padding_radius = plan.padding_radius,
    };
}

void copy_capi_island_padding_plan(const ctex::paint::IslandPaddingPlan& plan,
                                   std::uint32_t* ownership,
                                   ctex_paint_unsupported_mip_level* unsupported_mip_levels,
                                   std::uint32_t* affected_islands) {
    if (ownership != nullptr) {
        std::copy(plan.ownership.begin(), plan.ownership.end(), ownership);
    }
    std::size_t offset = 0;
    for (std::size_t index = 0; index < plan.unsupported_mip_levels.size(); ++index) {
        const ctex::paint::UnsupportedMipLevel& level = plan.unsupported_mip_levels[index];
        if (unsupported_mip_levels != nullptr) {
            unsupported_mip_levels[index] = {
                .mip_level = level.mip_level,
                .required_gutter_radius = level.required_gutter_radius,
                .affected_island_offset = offset,
                .affected_island_count = level.affected_islands.size(),
            };
        }
        if (affected_islands != nullptr) {
            std::copy(level.affected_islands.begin(), level.affected_islands.end(),
                      affected_islands + offset);
        }
        offset += level.affected_islands.size();
    }
}

ctex::paint::SurfaceMapRequest capi_surface_map_request(
    const ctex_mesh& mesh, const ctex_paint_surface_map_request& request) {
    if (request.uv_set == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "request.uv_set is required");
    }
    if (request.partition_index >= mesh.state->partition_views.size()) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE,
                       "surface-map partition index is outside the mesh partitions");
    }
    if (std::ranges::find(mesh.state->uv_names, std::string_view(request.uv_set)) ==
        mesh.state->uv_names.end()) {
        throw_boundary(CTEX_RESULT_MISSING_RESOURCE, CTEX_DIAGNOSTIC_MISSING_UV_SET,
                       "surface-map UV set is not present: " + std::string(request.uv_set));
    }
    static_cast<void>(bounded_paint_pixel_count(request.width, request.height,
                                                CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE));
    if (!std::isfinite(request.tile_origin.x) || !std::isfinite(request.tile_origin.y)) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE,
                       "surface-map tile origin must be finite");
    }
    if (std::ranges::find(mesh.state->face_partition_indices,
                          static_cast<std::uint32_t>(request.partition_index)) ==
        mesh.state->face_partition_indices.end()) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE,
                       "surface-map texture set contains no faces");
    }
    return {
        .partition_index = request.partition_index,
        .uv_set = request.uv_set,
        .raster = {.width = request.width,
                   .height = request.height,
                   .tile_origin = {request.tile_origin.x, request.tile_origin.y}},
    };
}

bool capi_surface_map_key_matches(const ctex_paint_surface_map_cache_entry& entry,
                                  ctex::mesh::PartitionKind partition_kind,
                                  std::string_view partition_key,
                                  const ctex_paint_surface_map_request& request) {
    return entry.partition_kind == partition_kind && entry.partition_key == partition_key &&
           entry.uv_set == request.uv_set && entry.width == request.width &&
           entry.height == request.height && entry.tile_origin.x == request.tile_origin.x &&
           entry.tile_origin.y == request.tile_origin.y;
}

bool capi_surface_map_same_partition(const ctex_paint_surface_map_cache_entry& entry,
                                     ctex::mesh::PartitionKind partition_kind,
                                     std::string_view partition_key) {
    return entry.partition_kind == partition_kind && entry.partition_key == partition_key;
}

struct CapiSurfaceMapLookup {
    const ctex_paint_surface_map_cache_entry* entry{};
    bool cache_hit{};
};

CapiSurfaceMapLookup lookup_capi_surface_maps(ctex_paint_surface_map_cache& cache,
                                              const ctex_mesh& mesh,
                                              const ctex_paint_surface_map_request& request) {
    const ctex::paint::SurfaceMapRequest converted_request =
        capi_surface_map_request(mesh, request);
    const ctex::mesh::MeshPartition& partition =
        mesh.state->partition_views[request.partition_index];
    if (cache.mesh_revision == mesh.state->revision) {
        const auto found = std::ranges::find_if(cache.entries, [&](const auto& entry) {
            return capi_surface_map_key_matches(entry, partition.kind, partition.stable_key,
                                                request);
        });
        if (found != cache.entries.end()) {
            ++cache.hits;
            return {.entry = &*found, .cache_hit = true};
        }
    }

    const ctex::mesh::MeshView mesh_view(mesh.state->descriptor());
    const ctex::paint::CachedSurfaceMaps built =
        ctex::paint::build_surface_maps(mesh_view, mesh.state->revision, converted_request);
    ctex_paint_surface_map_cache_entry prepared(&cache.memory_resource, partition.kind,
                                                partition.stable_key, request, built);
    cache.entries.reserve(cache.entries.size() + 1);

    if (cache.mesh_revision != 0 && cache.mesh_revision != mesh.state->revision) {
        cache.invalidated_entries += cache.entries.size();
        cache.entries.clear();
    } else {
        for (auto entry = cache.entries.begin(); entry != cache.entries.end();) {
            if (capi_surface_map_same_partition(*entry, partition.kind, partition.stable_key) &&
                entry->uv_set != request.uv_set) {
                entry = cache.entries.erase(entry);
                ++cache.invalidated_entries;
            } else {
                ++entry;
            }
        }
    }
    cache.mesh_revision = mesh.state->revision;
    cache.entries.push_back(std::move(prepared));
    ++cache.misses;
    return {.entry = &cache.entries.back(), .cache_hit = false};
}

ctex_paint_surface_map_info capi_surface_map_info(const ctex_paint_surface_map_cache_entry& entry,
                                                  bool cache_hit, std::uint64_t mesh_revision) {
    return {
        .size = CTEX_PAINT_SURFACE_MAP_INFO_CURRENT_SIZE,
        .cache_hit = cache_hit ? 1U : 0U,
        .mesh_revision = mesh_revision,
        .required_texture_set_id_size = entry.texture_set_id.size() + 1,
        .required_uv_set_size = entry.uv_set.size() + 1,
        .required_texel_count = entry.surface_texels.size(),
    };
}

void validate_capi_surface_map_buffers(const ctex_paint_surface_map_buffers& buffers,
                                       const ctex_paint_surface_map_info& info) {
    validate_structure_size(buffers.size, CTEX_PAINT_SURFACE_MAP_BUFFERS_V1_SIZE,
                            CTEX_PAINT_SURFACE_MAP_BUFFERS_CURRENT_SIZE, "buffers.size");
    validate_output_array(buffers.texture_set_id, buffers.texture_set_id_size,
                          info.required_texture_set_id_size, "buffers.texture_set_id");
    validate_output_array(buffers.uv_set, buffers.uv_set_size, info.required_uv_set_size,
                          "buffers.uv_set");
    validate_output_array(buffers.surface_texels, buffers.surface_texel_capacity,
                          info.required_texel_count, "buffers.surface_texels");
    validate_output_array(buffers.coverage, buffers.coverage_capacity, info.required_texel_count,
                          "buffers.coverage");
    validate_output_array(buffers.triangle_identity, buffers.triangle_identity_capacity,
                          info.required_texel_count, "buffers.triangle_identity");
    validate_output_array(buffers.uv_island_identity, buffers.uv_island_identity_capacity,
                          info.required_texel_count, "buffers.uv_island_identity");
}

void copy_capi_surface_map_buffers(const ctex_paint_surface_map_cache_entry& entry,
                                   const ctex_paint_surface_map_buffers& buffers) {
    if (buffers.texture_set_id != nullptr) {
        std::memcpy(buffers.texture_set_id, entry.texture_set_id.c_str(),
                    entry.texture_set_id.size() + 1);
    }
    if (buffers.uv_set != nullptr) {
        std::memcpy(buffers.uv_set, entry.uv_set.c_str(), entry.uv_set.size() + 1);
    }
    if (buffers.surface_texels != nullptr) {
        std::copy(entry.surface_texels.begin(), entry.surface_texels.end(), buffers.surface_texels);
    }
    if (buffers.coverage != nullptr) {
        std::copy(entry.coverage.begin(), entry.coverage.end(), buffers.coverage);
    }
    if (buffers.triangle_identity != nullptr) {
        std::copy(entry.triangle_identity.begin(), entry.triangle_identity.end(),
                  buffers.triangle_identity);
    }
    if (buffers.uv_island_identity != nullptr) {
        std::copy(entry.uv_island_identity.begin(), entry.uv_island_identity.end(),
                  buffers.uv_island_identity);
    }
}

ctex::paint::RejectionSettings accept_all_rejection_settings() {
    return {
        .depth_enabled = false,
        .depth_bias = ctex::paint::default_depth_rejection_bias,
        .symmetry_depth_policy = ctex::paint::SymmetryDepthPolicy::require_consistent_per_instance,
        .angle_enabled = false,
        .minimum_normal_dot = ctex::paint::default_angle_rejection_dot,
        .backface_enabled = false,
    };
}

ctex::paint::DepositionMode paint_deposition_mode(std::uint32_t value) {
    switch (value) {
        case CTEX_PAINT_DEPOSITION_NON_BUILDING:
            return ctex::paint::DepositionMode::non_building;
        case CTEX_PAINT_DEPOSITION_BUILD_UP:
            return ctex::paint::DepositionMode::build_up;
        default:
            throw std::invalid_argument("paint deposition mode is invalid");
    }
}

ctex::paint::AlphaDiscardFormat alpha_discard_format(std::uint32_t value) {
    switch (value) {
        case CTEX_ALPHA_DISCARD_UNORM8:
            return ctex::paint::AlphaDiscardFormat::unorm8;
        case CTEX_ALPHA_DISCARD_UNORM16:
            return ctex::paint::AlphaDiscardFormat::unorm16;
        case CTEX_ALPHA_DISCARD_FLOATING_POINT:
            return ctex::paint::AlphaDiscardFormat::floating_point;
        default:
            throw std::invalid_argument("alpha discard format is invalid");
    }
}

ctex::paint::AlphaDiscardSettings alpha_discard_settings(
    const ctex_paint_deposition_descriptor& descriptor) {
    if (descriptor.has_custom_alpha_discard_threshold > 1) {
        throw std::invalid_argument("custom alpha discard threshold flag is invalid");
    }
    return {
        .format = alpha_discard_format(descriptor.alpha_discard_format),
        .threshold = descriptor.has_custom_alpha_discard_threshold != 0
                         ? std::optional<double>(descriptor.custom_alpha_discard_threshold)
                         : std::nullopt,
    };
}

void copy_deposition_samples(const ctex::paint::DepositionRaster& deposition,
                             const ctex::paint::AlphaDiscardResult& discarded,
                             ctex_paint_deposition_sample* samples) {
    for (std::size_t index = 0; index < deposition.strength.size(); ++index) {
        samples[index] = {
            .non_building_coverage = deposition.non_building_coverage[index],
            .build_up_deposition = deposition.build_up_deposition[index],
            .strength = deposition.strength[index],
            .retained_strength = discarded.retained_strength[index],
            .write = discarded.write_mask[index],
        };
    }
}

ctex::paint::PaintMaskView paint_mask_view(const ctex_paint_mask_view& mask,
                                           std::string_view name) {
    if (mask.values == nullptr && mask.value_count != 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       std::string(name) + ".values=null with nonzero value_count");
    }
    return {mask.value_count == 0 ? std::span<const double>{}
                                  : std::span<const double>(mask.values, mask.value_count)};
}

std::optional<ctex::paint::PaintMaskView> optional_paint_mask(const ctex_paint_mask_view* mask,
                                                              std::string_view name) {
    return mask == nullptr
               ? std::nullopt
               : std::optional<ctex::paint::PaintMaskView>(paint_mask_view(*mask, name));
}

struct PaintMaskStorage {
    std::vector<ctex::paint::PaintMaskView> active_layer_masks;
    std::optional<ctex::paint::PaintMaskView> colour_id_selection;
    std::optional<ctex::paint::PaintMaskView> geometry_selection;
    std::optional<ctex::paint::PaintMaskView> screen_selection;
    std::optional<ctex::paint::PaintMaskView> uv_island_selection;

    [[nodiscard]] ctex::paint::PaintMaskInputs inputs() const {
        return {
            .active_layer_masks = active_layer_masks,
            .colour_id_selection = colour_id_selection,
            .geometry_selection = geometry_selection,
            .screen_selection = screen_selection,
            .uv_island_selection = uv_island_selection,
        };
    }
};

PaintMaskStorage paint_mask_storage(const ctex_paint_mask_inputs_descriptor* descriptor) {
    PaintMaskStorage result;
    if (descriptor == nullptr) {
        return result;
    }
    validate_structure_size(descriptor->size, CTEX_PAINT_MASK_INPUTS_DESCRIPTOR_V1_SIZE,
                            CTEX_PAINT_MASK_INPUTS_DESCRIPTOR_CURRENT_SIZE, "masks.size");
    if (descriptor->active_layer_masks == nullptr && descriptor->active_layer_mask_count != 0) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "masks.active_layer_masks=null with nonzero active_layer_mask_count");
    }
    result.active_layer_masks.reserve(descriptor->active_layer_mask_count);
    for (std::size_t index = 0; index < descriptor->active_layer_mask_count; ++index) {
        result.active_layer_masks.push_back(
            paint_mask_view(descriptor->active_layer_masks[index],
                            "masks.active_layer_masks[" + std::to_string(index) + "]"));
    }
    result.colour_id_selection =
        optional_paint_mask(descriptor->colour_id_selection, "masks.colour_id_selection");
    result.geometry_selection =
        optional_paint_mask(descriptor->geometry_selection, "masks.geometry_selection");
    result.screen_selection =
        optional_paint_mask(descriptor->screen_selection, "masks.screen_selection");
    result.uv_island_selection =
        optional_paint_mask(descriptor->uv_island_selection, "masks.uv_island_selection");
    return result;
}

const ctex_paint_mask_inputs_descriptor* deposition_masks(
    const ctex_paint_deposition_descriptor& descriptor) {
    constexpr std::size_t masks_end = offsetof(ctex_paint_deposition_descriptor, masks) +
                                      sizeof(ctex_paint_deposition_descriptor::masks);
    return descriptor.size >= masks_end ? descriptor.masks : nullptr;
}

ctex::paint::RejectedCoverageRaster apply_capi_paint_masks(
    const ctex::paint::RejectedCoverageRaster& rejected,
    const ctex_paint_mask_inputs_descriptor* descriptor) {
    if (descriptor == nullptr) {
        return rejected;
    }
    const PaintMaskStorage storage = paint_mask_storage(descriptor);
    try {
        return ctex::paint::apply_paint_masks(rejected, storage.inputs());
    } catch (const std::invalid_argument& error) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_MASK,
                       error.what());
    }
}

ctex::graph::ColourValue paint_colour(ctex_vec4f value) {
    return {value.x, value.y, value.z, value.w};
}

ctex_vec4f capi_colour(ctex::graph::ColourValue value) {
    return {value.r, value.g, value.b, value.a};
}

struct PaintBlendInputs {
    std::vector<ctex::graph::ColourValue> snapshot;
    std::vector<ctex::graph::ColourValue> paint;
    std::vector<double> strength;
};

PaintBlendInputs paint_blend_inputs(const ctex_paint_blend_descriptor& descriptor,
                                    std::size_t expected_count) {
    if (descriptor.blend_mode == nullptr || descriptor.stroke_start_snapshot == nullptr ||
        descriptor.paint == nullptr || descriptor.deposition == nullptr) {
        throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                       "blend_mode, stroke_start_snapshot, paint and deposition are required");
    }
    if (descriptor.pixel_count != expected_count) {
        throw std::invalid_argument("paint blend pixel count does not match its dimensions");
    }
    PaintBlendInputs result;
    result.snapshot.reserve(expected_count);
    result.paint.reserve(expected_count);
    result.strength.reserve(expected_count);
    for (std::size_t index = 0; index < expected_count; ++index) {
        if (descriptor.deposition[index].write > 1) {
            throw std::invalid_argument("paint blend deposition write flag is invalid");
        }
        result.snapshot.push_back(paint_colour(descriptor.stroke_start_snapshot[index]));
        result.paint.push_back(paint_colour(descriptor.paint[index]));
        result.strength.push_back(
            descriptor.deposition[index].write != 0 ? descriptor.deposition[index].strength : 0.0);
    }
    return result;
}

}  // namespace

void* ctex_host_memory_resource::do_allocate(std::size_t bytes, std::size_t alignment) {
    return allocate_storage(allocator_, bytes, alignment);
}

void ctex_host_memory_resource::do_deallocate(void* allocation, std::size_t bytes,
                                              std::size_t alignment) {
    deallocate_storage(allocator_, allocation, bytes, alignment);
}

bool ctex_host_memory_resource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
    return this == &other;
}

ctex_document::ctex_document(ctex_allocator_state allocator_value)
    : allocator(allocator_value), memory_resource(allocator_value), value(&memory_resource) {}

ctex_cube_lut::ctex_cube_lut(ctex_allocator_state allocator_value, std::string_view source)
    : allocator(allocator_value),
      memory_resource(allocator_value),
      value(ctex::image::CubeLut::from_cube(source, &memory_resource)) {}

ctex_mesh_state::ctex_mesh_state(const ctex_mesh_descriptor& source,
                                 std::pmr::memory_resource* memory_resource)
    : positions(memory_resource),
      normals(memory_resource),
      vertex_colors(memory_resource),
      triangle_indices(memory_resource),
      uv_values(memory_resource),
      uv_names(memory_resource),
      default_uv_set(source.default_uv_set, memory_resource),
      uv_views(memory_resource),
      partition_keys(memory_resource),
      partition_names(memory_resource),
      partition_views(memory_resource),
      face_partition_indices(memory_resource),
      face_material_ids(memory_resource) {
    positions.reserve(source.position_count);
    std::transform(source.positions, source.positions + source.position_count,
                   std::back_inserter(positions), [](ctex_vec3f value) { return mesh_vec(value); });
    normals.reserve(source.normal_count);
    std::transform(source.normals, source.normals + source.normal_count,
                   std::back_inserter(normals), [](ctex_vec3f value) { return mesh_vec(value); });
    if (source.vertex_color_count != 0) {
        vertex_colors.reserve(source.vertex_color_count);
        std::transform(source.vertex_colors, source.vertex_colors + source.vertex_color_count,
                       std::back_inserter(vertex_colors),
                       [](ctex_vec4f value) { return mesh_vec(value); });
    }
    triangle_indices.assign(source.triangle_indices,
                            source.triangle_indices + source.triangle_index_count);
    uv_names.reserve(source.uv_set_count);
    for (std::size_t index = 0; index < source.uv_set_count; ++index) {
        uv_names.emplace_back(source.uv_sets[index].name);
        const ctex_uv_set_descriptor& uv_set = source.uv_sets[index];
        std::transform(uv_set.values, uv_set.values + uv_set.value_count,
                       std::back_inserter(uv_values),
                       [](ctex_vec2f value) { return mesh_vec(value); });
    }
    uv_views.reserve(source.uv_set_count);
    std::size_t uv_offset = 0;
    for (std::size_t index = 0; index < source.uv_set_count; ++index) {
        const std::size_t count = source.uv_sets[index].value_count;
        uv_views.push_back({uv_names[index], std::span(uv_values).subspan(uv_offset, count)});
        uv_offset += count;
    }
    partition_keys.reserve(source.partition_count);
    partition_names.reserve(source.partition_count);
    for (std::size_t index = 0; index < source.partition_count; ++index) {
        partition_keys.emplace_back(source.partitions[index].stable_key);
        partition_names.emplace_back(source.partitions[index].display_name);
    }
    partition_views.reserve(source.partition_count);
    for (std::size_t index = 0; index < source.partition_count; ++index) {
        partition_views.push_back({mesh_partition_kind(source.partitions[index].kind),
                                   partition_keys[index], partition_names[index]});
    }
    face_partition_indices.assign(
        source.face_partition_indices,
        source.face_partition_indices + source.face_partition_index_count);
    face_material_ids.assign(source.face_material_ids,
                             source.face_material_ids + source.face_material_id_count);
    const ctex::mesh::MeshBinding validated(descriptor());
    revision = validated.revision();
}

ctex::mesh::MeshDescriptor ctex_mesh_state::descriptor() const noexcept {
    return {
        .positions = positions,
        .normals = normals,
        .vertex_colors = vertex_colors,
        .triangle_indices = triangle_indices,
        .uv_sets = uv_views,
        .default_uv_set = default_uv_set,
        .partitions = partition_views,
        .face_partition_indices = face_partition_indices,
        .face_material_ids = face_material_ids,
    };
}

ctex_mesh::ctex_mesh(ctex_allocator_state allocator_value, const ctex_mesh_descriptor& descriptor)
    : allocator(allocator_value), memory_resource(allocator_value), state(nullptr) {
    void* storage = memory_resource.allocate(sizeof(ctex_mesh_state), alignof(ctex_mesh_state));
    try {
        state = ::new (storage) ctex_mesh_state(descriptor, &memory_resource);
    } catch (...) {
        memory_resource.deallocate(storage, sizeof(ctex_mesh_state), alignof(ctex_mesh_state));
        throw;
    }
}

ctex_mesh::~ctex_mesh() {
    if (state != nullptr) {
        state->~ctex_mesh_state();
        memory_resource.deallocate(state, sizeof(ctex_mesh_state), alignof(ctex_mesh_state));
    }
}

extern "C" ctex_version ctex_get_version(void) {
    return {
        CTEX_VERSION_MAJOR,
        CTEX_VERSION_MINOR,
        CTEX_VERSION_PATCH,
        CTEX_VERSION_STRING,
    };
}

extern "C" ctex_version ctex_get_abi_version(void) { return ctex_get_version(); }

extern "C" ctex_color_space ctex_get_working_color_space(void) {
    return CTEX_COLOR_SPACE_LINEAR_REC709;
}

extern "C" ctex_result ctex_color_space_get_name(std::uint32_t value, char* buffer,
                                                 std::size_t buffer_size,
                                                 std::size_t* out_required_size) {
    return call_boundary("ctex_color_space_get_name", [&] {
        if (out_required_size == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_required_size=null");
        }
        const std::string_view name = ctex::image::color_space_name(color_space(value));
        const std::size_t required_size = name.size() + 1;
        *out_required_size = required_size;
        validate_string_buffer(buffer, buffer_size, required_size);
        if (buffer != nullptr) {
            std::memcpy(buffer, name.data(), name.size());
            buffer[name.size()] = '\0';
        }
    });
}

extern "C" ctex_result ctex_channel_get_color_policy(std::uint32_t semantic_value,
                                                     ctex_channel_color_policy* out_policy) {
    return call_boundary("ctex_channel_get_color_policy", [&] {
        if (out_policy == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_policy=null");
        }
        validate_structure_size(out_policy->size, CTEX_CHANNEL_COLOR_POLICY_V1_SIZE,
                                CTEX_CHANNEL_COLOR_POLICY_CURRENT_SIZE, "out_policy.size");
        const ctex::image::ChannelSemantic semantic = channel_semantic(semantic_value);
        *out_policy = {
            .size = CTEX_CHANNEL_COLOR_POLICY_CURRENT_SIZE,
            .color_valued = ctex::image::is_color_valued(semantic) ? 1U : 0U,
            .recommended_bit_depth = ctex::image::recommended_bit_depth(semantic),
        };
    });
}

extern "C" ctex_result ctex_resolve_input_color_space(
    std::uint32_t declaration_value, std::uint32_t semantic_value,
    ctex_resolved_input_color_space* out_resolved) {
    return call_boundary("ctex_resolve_input_color_space", [&] {
        if (out_resolved == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_resolved=null");
        }
        validate_structure_size(out_resolved->size, CTEX_RESOLVED_INPUT_COLOR_SPACE_V1_SIZE,
                                CTEX_RESOLVED_INPUT_COLOR_SPACE_CURRENT_SIZE, "out_resolved.size");
        const ctex::image::ResolvedInputSpace resolved = ctex::image::resolve_input_space(
            input_color_space(declaration_value), channel_semantic(semantic_value));
        *out_resolved = {
            .size = CTEX_RESOLVED_INPUT_COLOR_SPACE_CURRENT_SIZE,
            .color_space = static_cast<std::uint32_t>(resolved.color_space),
            .inferred = resolved.inferred ? 1U : 0U,
        };
    });
}

extern "C" ctex_result ctex_color_convert(const ctex_rgb_color* input,
                                          std::uint32_t destination_color_space,
                                          ctex_rgb_color* out_color) {
    return call_boundary("ctex_color_convert", [&] {
        if (input == nullptr || out_color == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           input == nullptr ? "input=null" : "out_color=null");
        }
        const ctex::image::ColorSpace source = color_space(input->color_space);
        const ctex::image::ColorSpace destination = color_space(destination_color_space);
        const ctex::image::RgbColor converted =
            ctex::image::convert_color(rgb_color(*input), source, destination);
        *out_color = {
            .red = converted.red,
            .green = converted.green,
            .blue = converted.blue,
            .color_space = destination_color_space,
        };
    });
}

extern "C" ctex_result ctex_color_input_to_working(const ctex_rgb_color* input,
                                                   std::uint32_t semantic_value,
                                                   ctex_rgb_color* out_color) {
    return call_boundary("ctex_color_input_to_working", [&] {
        if (input == nullptr || out_color == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           input == nullptr ? "input=null" : "out_color=null");
        }
        const ctex::image::ColorSpace source = color_space(input->color_space);
        const ctex::image::RgbColor converted = ctex::image::input_to_working_space(
            rgb_color(*input), declared_input_color_space(source),
            channel_semantic(semantic_value));
        *out_color = {
            .red = converted.red,
            .green = converted.green,
            .blue = converted.blue,
            .color_space = CTEX_COLOR_SPACE_LINEAR_REC709,
        };
    });
}

extern "C" ctex_result ctex_channel_get_bit_depth_warning(std::uint32_t semantic_value,
                                                          std::uint32_t selected_bit_depth,
                                                          ctex_bit_depth_warning* out_warning) {
    return call_boundary("ctex_channel_get_bit_depth_warning", [&] {
        if (out_warning == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_warning=null");
        }
        validate_structure_size(out_warning->size, CTEX_BIT_DEPTH_WARNING_V1_SIZE,
                                CTEX_BIT_DEPTH_WARNING_CURRENT_SIZE, "out_warning.size");
        validate_color_bit_depth(selected_bit_depth);
        const ctex::image::ChannelSemantic semantic = channel_semantic(semantic_value);
        const std::optional<ctex::image::BitDepthWarning> warning =
            ctex::image::bit_depth_warning(semantic, static_cast<std::uint8_t>(selected_bit_depth));
        *out_warning = {
            .size = CTEX_BIT_DEPTH_WARNING_CURRENT_SIZE,
            .warning = warning.has_value() ? 1U : 0U,
            .selected_bit_depth = selected_bit_depth,
            .recommended_bit_depth = ctex::image::recommended_bit_depth(semantic),
        };
    });
}

extern "C" ctex_result ctex_accumulate_height(const double* contributions,
                                              std::size_t contribution_count,
                                              std::uint32_t storage_bit_depth,
                                              double* out_accumulated) {
    return call_boundary("ctex_accumulate_height", [&] {
        if (out_accumulated == nullptr || (contributions == nullptr && contribution_count != 0)) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           out_accumulated == nullptr
                               ? "out_accumulated=null"
                               : "contributions=null with nonzero contribution_count");
        }
        validate_color_bit_depth(storage_bit_depth);
        for (std::size_t index = 0; index < contribution_count; ++index) {
            if (!std::isfinite(contributions[index])) {
                throw_boundary(CTEX_RESULT_INVALID_ARGUMENT,
                               CTEX_DIAGNOSTIC_INVALID_COLOR_COMPONENT,
                               "contribution is not finite at index=" + std::to_string(index));
            }
        }
        const std::span<const double> values =
            contribution_count == 0 ? std::span<const double>{}
                                    : std::span<const double>(contributions, contribution_count);
        const double accumulated =
            ctex::image::accumulate_height(values, static_cast<std::uint8_t>(storage_bit_depth));
        if (!std::isfinite(accumulated)) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_COLOR_COMPONENT,
                           "height accumulation is not finite");
        }
        *out_accumulated = accumulated;
    });
}

extern "C" ctex_result ctex_quantize_unorm8(double value, std::uint32_t x, std::uint32_t y,
                                            std::uint32_t dither, std::uint8_t* out_value) {
    return call_boundary("ctex_quantize_unorm8", [&] {
        if (out_value == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_value=null");
        }
        if (!std::isfinite(value)) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_COLOR_COMPONENT,
                           "value is not finite");
        }
        if (dither > 1) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_VALUE,
                           "dither=" + std::to_string(dither));
        }
        *out_value = ctex::image::quantize_unorm8(value, x, y, dither != 0);
    });
}

extern "C" ctex_result ctex_cube_lut_create(const char* cube_source, std::size_t cube_source_size,
                                            ctex_cube_lut** out_lut) {
    return call_boundary("ctex_cube_lut_create", [&] {
        if (out_lut == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_lut=null");
        }
        *out_lut = nullptr;
        if (cube_source == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "cube_source=null");
        }
        try {
            *out_lut = create_cube_lut(current_allocator(),
                                       std::string_view(cube_source, cube_source_size));
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_CUBE_LUT,
                           error.what());
        }
    });
}

extern "C" void ctex_cube_lut_destroy(ctex_cube_lut* lut) {
    if (lut == nullptr) {
        return;
    }
    const ctex_allocator_state allocator = lut->allocator;
    lut->~ctex_cube_lut();
    deallocate_storage(allocator, lut, sizeof(ctex_cube_lut), alignof(ctex_cube_lut));
}

extern "C" ctex_result ctex_cube_lut_apply_preview(const ctex_cube_lut* lut,
                                                   const ctex_rgb_color* input,
                                                   ctex_rgb_color* out_color) {
    return call_boundary("ctex_cube_lut_apply_preview", [&] {
        if (lut == nullptr || input == nullptr || out_color == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "lut, input and out_color are required");
        }
        const ctex::image::RgbColor working = ctex::image::convert_color(
            rgb_color(*input), color_space(input->color_space), ctex::image::working_color_space());
        const ctex::image::RgbColor preview = lut->value.apply(working);
        *out_color = {
            .red = preview.red,
            .green = preview.green,
            .blue = preview.blue,
            .color_space = CTEX_COLOR_SPACE_LINEAR_REC709,
        };
    });
}

extern "C" ctex_result ctex_image_decode_memory(
    const void* encoded, std::size_t encoded_size, const char* source_name,
    std::uint32_t intended_channel, std::uint32_t input_color_space_value,
    const ctex_image_decode_limits_descriptor* limits, ctex_decoded_image_info* out_info,
    void* pixel_buffer, std::size_t pixel_buffer_size, std::size_t* out_required_size) {
    return call_boundary("ctex_image_decode_memory", [&] {
        if (encoded == nullptr && encoded_size != 0) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "encoded=null with nonzero encoded_size");
        }
        if (source_name == nullptr || out_info == nullptr || out_required_size == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "source_name, out_info and out_required_size are required");
        }
        validate_structure_size(out_info->size, CTEX_DECODED_IMAGE_INFO_V1_SIZE,
                                CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE, "out_info.size");
        const std::span<const std::byte> bytes =
            encoded_size == 0
                ? std::span<const std::byte>{}
                : std::span<const std::byte>(static_cast<const std::byte*>(encoded), encoded_size);
        ctex::io::DecodedImage decoded = [&] {
            try {
                return ctex::io::decode_image_memory({
                    .bytes = bytes,
                    .source_name = source_name,
                    .intended_channel = channel_semantic(intended_channel),
                    .color_space = input_color_space(input_color_space_value),
                    .limits = image_decode_limits(limits),
                });
            } catch (const ctex::io::ImageIoError& error) {
                throw_image_io_error(error);
            }
        }();
        const ctex::image::PixelFormat format = decoded.pixels.format();
        const std::size_t required_size = decoded_image_size(decoded.pixels);
        *out_required_size = required_size;
        *out_info = {
            .size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE,
            .width = decoded.pixels.width(),
            .height = decoded.pixels.height(),
            .channel_count = format.channel_count,
            .scalar_representation = format.channel_type == ctex::image::ChannelType::float32
                                         ? CTEX_SCALAR_REPRESENTATION_FLOATING_POINT
                                         : CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
            .bit_depth = static_cast<std::uint32_t>(format.bytes_per_channel() * 8),
            .color_space = static_cast<std::uint32_t>(decoded.source_color_space),
            .detected_format = image_file_format(decoded.report.detected_format),
            .extension_mismatch = decoded.report.extension_mismatch ? 1U : 0U,
            .color_space_source = color_space_source(decoded.report.color_space_source),
            .uninterpretable_profile = has_uninterpretable_profile(decoded.report) ? 1U : 0U,
        };
        validate_string_buffer(static_cast<char*>(pixel_buffer), pixel_buffer_size, required_size);
        if (pixel_buffer != nullptr) {
            copy_decoded_pixels(decoded.pixels, pixel_buffer);
        }
    });
}

extern "C" ctex_result ctex_image_encode_memory(const void* pixels, std::size_t pixel_buffer_size,
                                                const ctex_image_encode_descriptor* descriptor,
                                                void* encoded_buffer,
                                                std::size_t encoded_buffer_size,
                                                std::size_t* out_required_size) {
    return call_boundary("ctex_image_encode_memory", [&] {
        if (descriptor == nullptr || out_required_size == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           descriptor == nullptr ? "descriptor=null" : "out_required_size=null");
        }
        validate_structure_size(descriptor->size, CTEX_IMAGE_ENCODE_DESCRIPTOR_V1_SIZE,
                                CTEX_IMAGE_ENCODE_DESCRIPTOR_CURRENT_SIZE, "descriptor.size");
        const PreparedImageEncode prepared =
            prepare_image_encode(pixels, pixel_buffer_size, *descriptor);
        const ctex::image::TiledImage image =
            copy_encoded_input(pixels, *descriptor, prepared.input_format);
        const std::vector<std::byte> encoded = encode_capi_image(image, prepared.options);
        *out_required_size = encoded.size();
        validate_string_buffer(static_cast<char*>(encoded_buffer), encoded_buffer_size,
                               encoded.size());
        if (encoded_buffer != nullptr) {
            std::memcpy(encoded_buffer, encoded.data(), encoded.size());
        }
    });
}

extern "C" ctex_result ctex_stroke_settings_init(ctex_stroke_settings_descriptor* out_settings) {
    return call_boundary("ctex_stroke_settings_init", [&] {
        if (out_settings == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_settings=null");
        }
        *out_settings = default_stroke_settings();
    });
}

extern "C" ctex_result ctex_stroke_resolve(
    const ctex_stroke_settings_descriptor* settings, const ctex_stroke_input_sample* samples,
    std::size_t sample_count, ctex_resolved_stroke_info* out_info, ctex_resolved_stamp* stamps,
    std::size_t stamp_capacity, std::size_t* out_stamp_count, ctex_swept_segment* swept_segments,
    std::size_t swept_segment_capacity, std::size_t* out_swept_segment_count) {
    return call_boundary("ctex_stroke_resolve", [&] {
        if (settings == nullptr || out_info == nullptr || out_stamp_count == nullptr ||
            out_swept_segment_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "settings, out_info and output counts are required");
        }
        validate_structure_size(out_info->size, CTEX_RESOLVED_STROKE_INFO_V1_SIZE,
                                CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE, "out_info.size");
        try {
            ctex::paint::StrokeResolver resolver(stroke_settings(*settings));
            const std::vector<ctex::paint::StrokeInputSample> converted =
                stroke_samples(samples, sample_count);
            resolver.append_samples(converted);
            const ctex::paint::ResolvedStroke resolved = resolver.resolve();

            *out_stamp_count = resolved.stamps.size();
            *out_swept_segment_count = resolved.swept_segments.size();
            *out_info = {
                .size = CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE,
                .reconstruction_version = resolved.reconstruction_version,
                .tip_mode = static_cast<std::uint32_t>(resolved.tip_mode),
                .symmetry_instance_count = resolved.symmetry_instance_count,
                .stamp_count = resolved.stamps.size(),
                .swept_segment_count = resolved.swept_segments.size(),
            };
            validate_output_array(stamps, stamp_capacity, resolved.stamps.size(), "stamps");
            validate_output_array(swept_segments, swept_segment_capacity,
                                  resolved.swept_segments.size(), "swept_segments");
            if (stamps != nullptr) {
                std::transform(resolved.stamps.begin(), resolved.stamps.end(), stamps,
                               [&](const ctex::paint::Stamp& stamp) {
                                   return capi_stamp(stamp, settings->tip_resource_identity);
                               });
            }
            if (swept_segments != nullptr) {
                std::transform(resolved.swept_segments.begin(), resolved.swept_segments.end(),
                               swept_segments, [](ctex::paint::SweptSegment segment) {
                                   return ctex_swept_segment{segment.start_stamp_ordinal,
                                                             segment.end_stamp_ordinal};
                               });
            }
        } catch (const ctex::paint::StrokeResolutionError& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_STROKE,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_stroke_preset_serialize(const char* name,
                                                    const ctex_stroke_settings_descriptor* settings,
                                                    char* serialized_buffer,
                                                    std::size_t serialized_buffer_size,
                                                    std::size_t* out_required_size) {
    return call_boundary("ctex_stroke_preset_serialize", [&] {
        if (name == nullptr || settings == nullptr || out_required_size == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "name, settings and out_required_size are required");
        }
        try {
            const ctex::paint::StrokePreset preset{
                .schema_version = ctex::paint::current_stroke_preset_schema_version,
                .name = name,
                .settings = stroke_settings(*settings),
            };
            const std::string serialized = ctex::paint::serialize_stroke_preset(preset);
            *out_required_size = serialized.size();
            validate_string_buffer(serialized_buffer, serialized_buffer_size, serialized.size());
            if (serialized_buffer != nullptr) {
                std::memcpy(serialized_buffer, serialized.data(), serialized.size());
            }
        } catch (const ctex::paint::StrokePresetError& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_STROKE_PRESET,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_stroke_preset_deserialize(
    const char* serialized, std::size_t serialized_size, ctex_stroke_preset_info* out_info,
    ctex_stroke_settings_descriptor* out_settings,
    const ctex_stroke_preset_buffers_descriptor* buffers) {
    return call_boundary("ctex_stroke_preset_deserialize", [&] {
        if ((serialized == nullptr && serialized_size != 0) || out_info == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           out_info == nullptr ? "out_info=null"
                                               : "serialized=null with nonzero serialized_size");
        }
        validate_structure_size(out_info->size, CTEX_STROKE_PRESET_INFO_V1_SIZE,
                                CTEX_STROKE_PRESET_INFO_CURRENT_SIZE, "out_info.size");
        try {
            const std::string_view bytes = serialized_size == 0
                                               ? std::string_view{}
                                               : std::string_view(serialized, serialized_size);
            const ctex::paint::StrokePreset preset = ctex::paint::deserialize_stroke_preset(bytes);
            const ctex_stroke_preset_info info{
                .size = CTEX_STROKE_PRESET_INFO_CURRENT_SIZE,
                .schema_version = preset.schema_version,
                .required_name_size = preset.name.size() + 1,
                .required_tip_resource_identity_size =
                    preset.settings.tip_resource_identity.size() + 1,
                .required_curve_point_count = stroke_curve_point_count(preset.settings),
            };
            *out_info = info;
            if (out_settings == nullptr && buffers == nullptr) {
                return;
            }
            if (out_settings == nullptr || buffers == nullptr) {
                throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                               "out_settings and buffers must be supplied together");
            }
            validate_structure_size(out_settings->size, CTEX_STROKE_SETTINGS_DESCRIPTOR_V1_SIZE,
                                    CTEX_STROKE_SETTINGS_DESCRIPTOR_CURRENT_SIZE,
                                    "out_settings.size");
            validate_preset_buffers(*buffers, info);

            std::memcpy(buffers->name_buffer, preset.name.c_str(), info.required_name_size);
            std::memcpy(buffers->tip_resource_identity_buffer,
                        preset.settings.tip_resource_identity.c_str(),
                        info.required_tip_resource_identity_size);
            const ctex_stroke_settings_descriptor converted = copy_capi_stroke_settings(
                preset.settings, buffers->tip_resource_identity_buffer, buffers->curve_points);
            *out_settings = converted;
        } catch (const ctex::paint::StrokePresetError& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_STROKE_PRESET,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_evaluate_tile_coverage(
    const ctex_mesh* mesh, const ctex_paint_tile_coverage_descriptor* tile,
    const ctex_resolved_stroke_descriptor* stroke, double* coverage, std::size_t coverage_capacity,
    std::size_t* out_coverage_count) {
    return call_boundary("ctex_paint_evaluate_tile_coverage", [&] {
        if (mesh == nullptr || tile == nullptr || stroke == nullptr ||
            out_coverage_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "mesh, tile, stroke and out_coverage_count are required");
        }
        const std::size_t texel_count = paint_tile_texel_count(*tile);
        *out_coverage_count = texel_count;
        try {
            const ctex::paint::ResolvedStroke converted_stroke = paint_stroke(*stroke);
            const ctex::paint::TextureSpaceRaster surface = paint_tile_surface(*mesh, *tile);
            const ctex::paint::CoverageRaster result =
                ctex::paint::evaluate_stroke_coverage(surface, converted_stroke);
            validate_output_array(coverage, coverage_capacity, result.values.size(), "coverage");
            if (coverage != nullptr) {
                std::copy(result.values.begin(), result.values.end(), coverage);
            }
        } catch (const ctex::paint::StrokeResolutionError& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_STROKE,
                           error.what());
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_COVERAGE,
                           error.what());
        } catch (const std::out_of_range& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_COVERAGE,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_evaluate_material_coordinates(
    const ctex_mesh* mesh, const ctex_paint_tile_coverage_descriptor* tile,
    const ctex_paint_material_coordinate_descriptor* descriptor,
    ctex_paint_material_coordinate_sample* samples, std::size_t sample_capacity,
    std::size_t* out_sample_count) {
    return call_boundary("ctex_paint_evaluate_material_coordinates", [&] {
        if (mesh == nullptr || tile == nullptr || descriptor == nullptr ||
            out_sample_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "mesh, tile, descriptor and out_sample_count are required");
        }
        validate_structure_size(descriptor->size, CTEX_PAINT_MATERIAL_COORDINATE_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_MATERIAL_COORDINATE_DESCRIPTOR_CURRENT_SIZE,
                                "descriptor.size");
        const std::size_t sample_count = paint_tile_texel_count(*tile);
        *out_sample_count = sample_count;
        validate_output_array(samples, sample_capacity, sample_count, "samples");
        try {
            const ctex::paint::MaterialCoordinateRequest request =
                paint_material_coordinate_request(*descriptor);
            validate_material_coordinate_request(request);
            const ctex::paint::TextureSpaceRaster surface = paint_tile_surface(*mesh, *tile);
            std::vector<ctex_paint_material_coordinate_sample> staged(sample_count);
            for (std::size_t index = 0; index < sample_count; ++index) {
                if (surface.covered(index)) {
                    staged[index] = capi_material_coordinates(
                        ctex::paint::material_coordinates(surface.texels[index], request));
                }
            }
            if (samples != nullptr) {
                std::copy(staged.begin(), staged.end(), samples);
            }
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_COORDINATES,
                           error.what());
        } catch (const std::out_of_range& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_COORDINATES,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_rejection_init(ctex_paint_rejection_descriptor* out_rejection) {
    return call_boundary("ctex_paint_rejection_init", [&] {
        if (out_rejection == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_rejection is required");
        }
        *out_rejection = {
            .size = CTEX_PAINT_REJECTION_DESCRIPTOR_CURRENT_SIZE,
            .depth_enabled = 1,
            .depth_bias = ctex::paint::default_depth_rejection_bias,
            .symmetry_depth_policy = CTEX_PAINT_SYMMETRY_DEPTH_REQUIRE_CONSISTENT,
            .angle_enabled = 1,
            .minimum_normal_dot = ctex::paint::default_angle_rejection_dot,
            .backface_enabled = 0,
            .depth_contexts = nullptr,
            .depth_context_count = 0,
            .view_directions = nullptr,
            .view_direction_count = 0,
        };
    });
}

extern "C" ctex_result ctex_paint_evaluate_rejected_coverage(
    const ctex_mesh* mesh, const ctex_paint_tile_coverage_descriptor* tile,
    const ctex_resolved_stroke_descriptor* stroke, const ctex_paint_rejection_descriptor* rejection,
    ctex_paint_rejection_info* out_info, double* coverage, std::size_t coverage_capacity,
    std::size_t* out_coverage_count) {
    return call_boundary("ctex_paint_evaluate_rejected_coverage", [&] {
        if (mesh == nullptr || tile == nullptr || stroke == nullptr || rejection == nullptr ||
            out_info == nullptr || out_coverage_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "mesh, tile, stroke, rejection, out_info and out_coverage_count are "
                           "required");
        }
        validate_structure_size(rejection->size, CTEX_PAINT_REJECTION_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_REJECTION_DESCRIPTOR_CURRENT_SIZE, "rejection.size");
        validate_structure_size(out_info->size, CTEX_PAINT_REJECTION_INFO_V1_SIZE,
                                CTEX_PAINT_REJECTION_INFO_CURRENT_SIZE, "out_info.size");
        const std::size_t texel_count = paint_tile_texel_count(*tile);
        *out_coverage_count = texel_count;
        validate_output_array(coverage, coverage_capacity, texel_count, "coverage");
        try {
            const ctex::paint::ResolvedStroke converted_stroke = paint_stroke(*stroke);
            const ctex::paint::TextureSpaceRaster surface = paint_tile_surface(*mesh, *tile);
            const PaintRejectionStorage storage = paint_rejection_storage(
                *rejection, texel_count, converted_stroke.symmetry_instance_count);
            const ctex::paint::RejectedCoverageRaster result =
                ctex::paint::evaluate_rejected_coverage(surface, converted_stroke, storage.settings,
                                                        storage.input());
            const ctex_paint_rejection_info staged_info = capi_rejection_info(result.report);
            *out_info = staged_info;
            if (coverage != nullptr) {
                std::copy(result.coverage.values.begin(), result.coverage.values.end(), coverage);
            }
        } catch (const ctex::paint::StrokeResolutionError& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_STROKE,
                           error.what());
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_REJECTION,
                           error.what());
        } catch (const std::out_of_range& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_REJECTION,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_work_init(ctex_paint_work_descriptor* out_work) {
    return call_boundary("ctex_paint_work_init", [&] {
        if (out_work == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_work is required");
        }
        *out_work = {
            .size = CTEX_PAINT_WORK_DESCRIPTOR_CURRENT_SIZE,
            .canvas_width = 0,
            .canvas_height = 0,
            .tile_size = ctex::image::default_tile_size,
            .dilation_radius = ctex::paint::default_seam_dilation_radius,
            .stamp_footprints = nullptr,
            .stamp_footprint_count = 0,
        };
    });
}

extern "C" ctex_result ctex_paint_plan_work(const ctex_paint_work_descriptor* work,
                                            ctex_paint_work_info* out_info,
                                            ctex_paint_tile_coordinate* processed_tiles,
                                            std::size_t processed_tile_capacity,
                                            std::size_t* out_processed_tile_count) {
    return call_boundary("ctex_paint_plan_work", [&] {
        if (work == nullptr || out_info == nullptr || out_processed_tile_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "work, out_info and out_processed_tile_count are required");
        }
        validate_structure_size(work->size, CTEX_PAINT_WORK_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_WORK_DESCRIPTOR_CURRENT_SIZE, "work.size");
        validate_structure_size(out_info->size, CTEX_PAINT_WORK_INFO_V1_SIZE,
                                CTEX_PAINT_WORK_INFO_CURRENT_SIZE, "out_info.size");
        if (work->stamp_footprints == nullptr && work->stamp_footprint_count != 0) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "stamp_footprints is null with nonzero count");
        }
        try {
            const std::vector<ctex::paint::StampTexelFootprint> footprints =
                paint_work_footprints(*work);
            const ctex::paint::PaintWorkReport report = ctex::paint::plan_paint_work({
                .canvas_width = work->canvas_width,
                .canvas_height = work->canvas_height,
                .tile_size = work->tile_size,
                .dilation_radius = work->dilation_radius,
                .stamp_footprints = footprints,
            });
            *out_processed_tile_count = report.processed_tiles.size();
            validate_output_array(processed_tiles, processed_tile_capacity,
                                  report.processed_tiles.size(), "processed_tiles");
            *out_info = capi_paint_work_info(report);
            copy_paint_work_tiles(report, processed_tiles);
        } catch (const std::length_error& error) {
            throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED,
                           error.what());
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_WORK,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_seam_dilation_init(
    ctex_paint_seam_dilation_descriptor* out_dilation) {
    return call_boundary("ctex_paint_seam_dilation_init", [&] {
        if (out_dilation == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_dilation is required");
        }
        *out_dilation = {
            .size = CTEX_PAINT_SEAM_DILATION_DESCRIPTOR_CURRENT_SIZE,
            .width = 0,
            .height = 0,
            .component_count = 0,
            .radius = ctex::paint::default_seam_dilation_radius,
            .pixels = nullptr,
            .pixel_count = 0,
            .coverage = nullptr,
            .coverage_count = 0,
        };
    });
}

extern "C" ctex_result ctex_paint_dilate_uv_seams(
    const ctex_paint_seam_dilation_descriptor* dilation, ctex_paint_seam_dilation_info* out_info,
    double* pixels, std::size_t pixel_capacity, std::size_t* out_pixel_count) {
    return call_boundary("ctex_paint_dilate_uv_seams", [&] {
        if (dilation == nullptr || out_info == nullptr || out_pixel_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "dilation, out_info and out_pixel_count are required");
        }
        validate_structure_size(dilation->size, CTEX_PAINT_SEAM_DILATION_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_SEAM_DILATION_DESCRIPTOR_CURRENT_SIZE, "dilation.size");
        validate_structure_size(out_info->size, CTEX_PAINT_SEAM_DILATION_INFO_V1_SIZE,
                                CTEX_PAINT_SEAM_DILATION_INFO_CURRENT_SIZE, "out_info.size");
        try {
            const std::size_t required_pixel_count = validate_capi_seam_dilation(*dilation);
            *out_pixel_count = required_pixel_count;
            validate_output_array(pixels, pixel_capacity, required_pixel_count, "pixels");
            const ctex::paint::SeamDilationRaster source{
                .width = dilation->width,
                .height = dilation->height,
                .component_count = static_cast<std::uint8_t>(dilation->component_count),
                .pixels =
                    std::vector<double>(dilation->pixels, dilation->pixels + dilation->pixel_count),
            };
            const ctex::paint::SeamDilationResult result = ctex::paint::dilate_uv_seams(
                source, std::span(dilation->coverage, dilation->coverage_count), dilation->radius);
            *out_info = capi_seam_dilation_info(result);
            if (pixels != nullptr) {
                std::copy(result.raster.pixels.begin(), result.raster.pixels.end(), pixels);
            }
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_dilation_session_create(
    std::uint32_t radius, ctex_paint_dilation_session** out_session) {
    return call_boundary("ctex_paint_dilation_session_create", [&] {
        if (out_session == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_session is required");
        }
        *out_session = nullptr;
        *out_session = create_paint_dilation_session(current_allocator(), radius);
    });
}

extern "C" void ctex_paint_dilation_session_destroy(ctex_paint_dilation_session* session) {
    if (session == nullptr) {
        return;
    }
    const ctex_allocator_state allocator = session->allocator;
    session->~ctex_paint_dilation_session();
    deallocate_storage(allocator, session, sizeof(ctex_paint_dilation_session),
                       alignof(ctex_paint_dilation_session));
}

extern "C" ctex_result ctex_paint_dilation_session_stage_tile(
    ctex_paint_dilation_session* session, const ctex_paint_dilation_tile_descriptor* tile) {
    return call_boundary("ctex_paint_dilation_session_stage_tile", [&] {
        if (session == nullptr || tile == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           session == nullptr ? "session is required" : "tile is required");
        }
        validate_structure_size(tile->size, CTEX_PAINT_DILATION_TILE_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_DILATION_TILE_DESCRIPTOR_CURRENT_SIZE, "tile.size");
        try {
            stage_capi_dilation_tile(*session, *tile);
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION,
                           error.what());
        } catch (const std::logic_error& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_dilation_session_get_preview(
    const ctex_paint_dilation_session* session, ctex_paint_dilation_session_info* out_info,
    ctex_paint_dilation_tile_info* tiles, std::size_t tile_capacity, std::size_t* out_tile_count,
    double* pixels, std::size_t pixel_capacity, std::size_t* out_pixel_count) {
    return call_boundary("ctex_paint_dilation_session_get_preview", [&] {
        if (session == nullptr || out_info == nullptr || out_tile_count == nullptr ||
            out_pixel_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "session, out_info, out_tile_count and out_pixel_count are required");
        }
        validate_structure_size(out_info->size, CTEX_PAINT_DILATION_SESSION_INFO_V1_SIZE,
                                CTEX_PAINT_DILATION_SESSION_INFO_CURRENT_SIZE, "out_info.size");
        try {
            write_capi_dilation_session_output(*session, *out_info, tiles, tile_capacity,
                                               *out_tile_count, pixels, pixel_capacity,
                                               *out_pixel_count);
        } catch (const std::length_error& error) {
            throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_dilation_session_finish(
    ctex_paint_dilation_session* session, ctex_paint_dilation_session_info* out_info,
    ctex_paint_dilation_tile_info* tiles, std::size_t tile_capacity, std::size_t* out_tile_count,
    double* pixels, std::size_t pixel_capacity, std::size_t* out_pixel_count) {
    return call_boundary("ctex_paint_dilation_session_finish", [&] {
        if (session == nullptr || out_info == nullptr || out_tile_count == nullptr ||
            out_pixel_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "session, out_info, out_tile_count and out_pixel_count are required");
        }
        validate_structure_size(out_info->size, CTEX_PAINT_DILATION_SESSION_INFO_V1_SIZE,
                                CTEX_PAINT_DILATION_SESSION_INFO_CURRENT_SIZE, "out_info.size");
        try {
            const std::size_t current_pixel_count = capi_dilation_session_pixel_count(*session);
            *out_tile_count = session->tiles.size();
            *out_pixel_count = current_pixel_count;
            validate_output_array(tiles, tile_capacity, session->tiles.size(), "tiles");
            validate_output_array(pixels, pixel_capacity, current_pixel_count, "pixels");
            finish_capi_dilation_session(*session);
            write_capi_dilation_session_output(*session, *out_info, tiles, tile_capacity,
                                               *out_tile_count, pixels, pixel_capacity,
                                               *out_pixel_count);
        } catch (const std::length_error& error) {
            throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED,
                           error.what());
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_DILATION,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_filter_surface_scalar(
    const ctex_paint_surface_filter_descriptor* filter, const double* values,
    std::size_t value_count, double* out_value) {
    return call_boundary("ctex_paint_filter_surface_scalar", [&] {
        if (filter == nullptr || values == nullptr || out_value == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "filter, values and out_value are required");
        }
        validate_structure_size(filter->size, CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_CURRENT_SIZE, "filter.size");
        try {
            validate_capi_surface_filter_value_count(value_count);
            const std::vector<ctex::paint::SurfaceAdjacentSample> samples =
                capi_surface_filter_samples(*filter);
            const ctex::paint::SurfaceFilterRequest request =
                capi_surface_filter_request(*filter, samples);
            const double result =
                ctex::paint::filter_surface_scalar({values, value_count}, request);
            *out_value = result;
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER,
                           error.what());
        } catch (const std::overflow_error& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_filter_surface_tangent_vector(
    const ctex_paint_surface_filter_descriptor* filter, const ctex_vec3d* values,
    std::size_t value_count, ctex_vec3d* out_value) {
    return call_boundary("ctex_paint_filter_surface_tangent_vector", [&] {
        if (filter == nullptr || values == nullptr || out_value == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "filter, values and out_value are required");
        }
        validate_structure_size(filter->size, CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_SURFACE_FILTER_DESCRIPTOR_CURRENT_SIZE, "filter.size");
        try {
            validate_capi_surface_filter_value_count(value_count);
            const std::vector<ctex::paint::SurfaceAdjacentSample> samples =
                capi_surface_filter_samples(*filter);
            const ctex::paint::SurfaceFilterRequest request =
                capi_surface_filter_request(*filter, samples);
            std::vector<ctex::paint::Vec3d> converted_values;
            converted_values.reserve(value_count);
            std::transform(values, values + value_count, std::back_inserter(converted_values),
                           [](ctex_vec3d value) { return stroke_vec(value); });
            const ctex::paint::Vec3d result =
                ctex::paint::filter_surface_tangent_vector(converted_values, request);
            *out_value = capi_vec(result);
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_plan_island_padding(
    const ctex_paint_island_padding_descriptor* padding, ctex_paint_island_padding_info* out_info,
    std::uint32_t* ownership, std::size_t ownership_capacity, std::size_t* out_ownership_count,
    ctex_paint_unsupported_mip_level* unsupported_mip_levels,
    std::size_t unsupported_mip_level_capacity, std::size_t* out_unsupported_mip_level_count,
    std::uint32_t* affected_islands, std::size_t affected_island_capacity,
    std::size_t* out_affected_island_count) {
    return call_boundary("ctex_paint_plan_island_padding", [&] {
        if (padding == nullptr || out_info == nullptr || out_ownership_count == nullptr ||
            out_unsupported_mip_level_count == nullptr || out_affected_island_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "padding, out_info and output counts are required");
        }
        validate_structure_size(padding->size, CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_CURRENT_SIZE, "padding.size");
        validate_structure_size(out_info->size, CTEX_PAINT_ISLAND_PADDING_INFO_V1_SIZE,
                                CTEX_PAINT_ISLAND_PADDING_INFO_CURRENT_SIZE, "out_info.size");
        try {
            const CapiIslandPaddingInput input = capi_island_padding_input(*padding);
            const ctex::paint::IslandPaddingPlan plan = capi_island_padding_plan(*padding);
            const std::size_t affected_island_count = capi_affected_island_count(plan);
            *out_ownership_count = plan.ownership.size();
            *out_unsupported_mip_level_count = plan.unsupported_mip_levels.size();
            *out_affected_island_count = affected_island_count;
            validate_output_array(ownership, ownership_capacity, plan.ownership.size(),
                                  "ownership");
            validate_output_array(unsupported_mip_levels, unsupported_mip_level_capacity,
                                  plan.unsupported_mip_levels.size(), "unsupported_mip_levels");
            validate_output_array(affected_islands, affected_island_capacity, affected_island_count,
                                  "affected_islands");
            *out_info = capi_island_padding_info(input, plan, affected_island_count, 0);
            copy_capi_island_padding_plan(plan, ownership, unsupported_mip_levels,
                                          affected_islands);
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER,
                           error.what());
        } catch (const std::length_error& error) {
            throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_apply_island_padding(
    const ctex_paint_island_padding_descriptor* padding, ctex_paint_island_padding_info* out_info,
    double* pixels, std::size_t pixel_capacity, std::size_t* out_pixel_count) {
    return call_boundary("ctex_paint_apply_island_padding", [&] {
        if (padding == nullptr || out_info == nullptr || out_pixel_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "padding, out_info and out_pixel_count are required");
        }
        validate_structure_size(padding->size, CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_ISLAND_PADDING_DESCRIPTOR_CURRENT_SIZE, "padding.size");
        validate_structure_size(out_info->size, CTEX_PAINT_ISLAND_PADDING_INFO_V1_SIZE,
                                CTEX_PAINT_ISLAND_PADDING_INFO_CURRENT_SIZE, "out_info.size");
        try {
            const CapiIslandPaddingInput input = capi_island_padding_input(*padding);
            const ctex::paint::IslandPaddingPlan plan = capi_island_padding_plan(*padding);
            const ctex::paint::SeamDilationRaster raster = capi_island_padding_raster(*padding);
            const ctex::paint::IslandPaddingResult result = ctex::paint::apply_island_padding(
                raster, {padding->island_identity, padding->island_identity_count}, plan);
            const std::size_t affected_island_count = capi_affected_island_count(plan);
            *out_pixel_count = result.raster.pixels.size();
            validate_output_array(pixels, pixel_capacity, result.raster.pixels.size(), "pixels");
            *out_info = capi_island_padding_info(input, plan, affected_island_count,
                                                 result.padded_texel_count);
            if (pixels != nullptr) {
                std::copy(result.raster.pixels.begin(), result.raster.pixels.end(), pixels);
            }
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_FILTER,
                           error.what());
        } catch (const std::length_error& error) {
            throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_surface_map_cache_create(
    ctex_paint_surface_map_cache** out_cache) {
    return call_boundary("ctex_paint_surface_map_cache_create", [&] {
        if (out_cache == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_cache is required");
        }
        *out_cache = nullptr;
        *out_cache = create_paint_surface_map_cache(current_allocator());
    });
}

extern "C" void ctex_paint_surface_map_cache_destroy(ctex_paint_surface_map_cache* cache) {
    if (cache == nullptr) {
        return;
    }
    const ctex_allocator_state allocator = cache->allocator;
    cache->~ctex_paint_surface_map_cache();
    deallocate_storage(allocator, cache, sizeof(ctex_paint_surface_map_cache),
                       alignof(ctex_paint_surface_map_cache));
}

extern "C" ctex_result ctex_paint_surface_map_cache_clear(ctex_paint_surface_map_cache* cache) {
    return call_boundary("ctex_paint_surface_map_cache_clear", [&] {
        if (cache == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "cache is required");
        }
        cache->entries.clear();
        cache->mesh_revision = 0;
        cache->hits = 0;
        cache->misses = 0;
        cache->invalidated_entries = 0;
    });
}

extern "C" ctex_result ctex_paint_surface_map_cache_get_statistics(
    const ctex_paint_surface_map_cache* cache, ctex_paint_surface_map_statistics* out_statistics) {
    return call_boundary("ctex_paint_surface_map_cache_get_statistics", [&] {
        if (cache == nullptr || out_statistics == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "cache and out_statistics are required");
        }
        validate_structure_size(out_statistics->size, CTEX_PAINT_SURFACE_MAP_STATISTICS_V1_SIZE,
                                CTEX_PAINT_SURFACE_MAP_STATISTICS_CURRENT_SIZE,
                                "out_statistics.size");
        *out_statistics = {
            .size = CTEX_PAINT_SURFACE_MAP_STATISTICS_CURRENT_SIZE,
            .entries = cache->entries.size(),
            .hits = cache->hits,
            .misses = cache->misses,
            .invalidated_entries = cache->invalidated_entries,
        };
    });
}

extern "C" ctex_result ctex_paint_surface_map_cache_lookup(
    ctex_paint_surface_map_cache* cache, const ctex_mesh* mesh,
    const ctex_paint_surface_map_request* request, ctex_paint_surface_map_info* out_info,
    const ctex_paint_surface_map_buffers* buffers) {
    return call_boundary("ctex_paint_surface_map_cache_lookup", [&] {
        if (cache == nullptr || mesh == nullptr || request == nullptr || out_info == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "cache, mesh, request and out_info are required");
        }
        validate_structure_size(request->size, CTEX_PAINT_SURFACE_MAP_REQUEST_V1_SIZE,
                                CTEX_PAINT_SURFACE_MAP_REQUEST_CURRENT_SIZE, "request.size");
        validate_structure_size(out_info->size, CTEX_PAINT_SURFACE_MAP_INFO_V1_SIZE,
                                CTEX_PAINT_SURFACE_MAP_INFO_CURRENT_SIZE, "out_info.size");
        try {
            const CapiSurfaceMapLookup lookup = lookup_capi_surface_maps(*cache, *mesh, *request);
            const ctex_paint_surface_map_info info =
                capi_surface_map_info(*lookup.entry, lookup.cache_hit, mesh->state->revision);
            *out_info = info;
            if (buffers != nullptr) {
                validate_capi_surface_map_buffers(*buffers, info);
                copy_capi_surface_map_buffers(*lookup.entry, *buffers);
            }
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT,
                           CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE, error.what());
        } catch (const std::out_of_range& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT,
                           CTEX_DIAGNOSTIC_INVALID_PAINT_SURFACE_CACHE, error.what());
        } catch (const std::length_error& error) {
            throw_boundary(CTEX_RESULT_OVER_BUDGET, CTEX_DIAGNOSTIC_PAINT_LIMIT_EXCEEDED,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_combine_masks(std::uint32_t width, std::uint32_t height,
                                                const ctex_paint_mask_inputs_descriptor* masks,
                                                ctex_paint_mask_info* out_info,
                                                double* combined_mask,
                                                std::size_t combined_mask_capacity,
                                                std::size_t* out_combined_mask_count) {
    return call_boundary("ctex_paint_combine_masks", [&] {
        if (out_info == nullptr || out_combined_mask_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           out_info == nullptr ? "out_info=null" : "out_combined_mask_count=null");
        }
        validate_structure_size(out_info->size, CTEX_PAINT_MASK_INFO_V1_SIZE,
                                CTEX_PAINT_MASK_INFO_CURRENT_SIZE, "out_info.size");
        const std::size_t pixel_count =
            bounded_paint_pixel_count(width, height, CTEX_DIAGNOSTIC_INVALID_PAINT_MASK);
        *out_combined_mask_count = pixel_count;
        validate_output_array(combined_mask, combined_mask_capacity, pixel_count, "combined_mask");
        const PaintMaskStorage storage = paint_mask_storage(masks);
        try {
            const ctex::paint::CombinedPaintMask result =
                ctex::paint::combine_paint_masks(width, height, storage.inputs());
            *out_info = {
                .size = CTEX_PAINT_MASK_INFO_CURRENT_SIZE,
                .active_input_count = result.active_input_count,
            };
            if (combined_mask != nullptr) {
                std::copy(result.values.begin(), result.values.end(), combined_mask);
            }
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_MASK,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_evaluate_tile_deposition(
    const ctex_mesh* mesh, const ctex_paint_tile_coverage_descriptor* tile,
    const ctex_resolved_stroke_descriptor* stroke,
    const ctex_paint_deposition_descriptor* descriptor, ctex_paint_deposition_info* out_info,
    ctex_paint_deposition_sample* samples, std::size_t sample_capacity,
    std::size_t* out_sample_count) {
    return call_boundary("ctex_paint_evaluate_tile_deposition", [&] {
        if (mesh == nullptr || tile == nullptr || stroke == nullptr || descriptor == nullptr ||
            out_info == nullptr || out_sample_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "mesh, tile, stroke, deposition, out_info and out_sample_count are "
                           "required");
        }
        validate_structure_size(descriptor->size, CTEX_PAINT_DEPOSITION_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_DEPOSITION_DESCRIPTOR_CURRENT_SIZE, "deposition.size");
        validate_structure_size(out_info->size, CTEX_PAINT_DEPOSITION_INFO_V1_SIZE,
                                CTEX_PAINT_DEPOSITION_INFO_CURRENT_SIZE, "out_info.size");
        const std::size_t texel_count = paint_tile_texel_count(*tile);
        *out_sample_count = texel_count;
        try {
            const ctex::paint::DepositionMode mode = paint_deposition_mode(descriptor->mode);
            const ctex::paint::AlphaDiscardSettings discard_settings =
                alpha_discard_settings(*descriptor);
            const ctex::paint::ResolvedStroke converted_stroke = paint_stroke(*stroke);
            const ctex::paint::TextureSpaceRaster surface = paint_tile_surface(*mesh, *tile);
            const ctex::paint::RejectedCoverageRaster rejected =
                ctex::paint::evaluate_rejected_coverage(surface, converted_stroke,
                                                        accept_all_rejection_settings());
            const ctex::paint::RejectedCoverageRaster masked =
                apply_capi_paint_masks(rejected, deposition_masks(*descriptor));
            const ctex::paint::DepositionRaster deposited =
                ctex::paint::evaluate_deposition(converted_stroke, masked, mode);
            const ctex::paint::AlphaDiscardResult discarded =
                ctex::paint::apply_alpha_discard(deposited.strength, discard_settings);
            validate_output_array(samples, sample_capacity, texel_count, "samples");
            *out_info = {
                .size = CTEX_PAINT_DEPOSITION_INFO_CURRENT_SIZE,
                .mode = descriptor->mode,
                .applied_stamp_count = deposited.applied_stamp_count,
                .alpha_discard_threshold = discarded.threshold,
                .alpha_discard_threshold_clamped =
                    discarded.parameter_report.clamp_for("alpha_discard.threshold").has_value()
                        ? 1U
                        : 0U,
            };
            if (samples != nullptr) {
                copy_deposition_samples(deposited, discarded, samples);
            }
        } catch (const ctex::paint::StrokeResolutionError& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_STROKE,
                           error.what());
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_COVERAGE,
                           error.what());
        } catch (const std::out_of_range& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_COVERAGE,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_paint_blend_snapshot(const ctex_paint_blend_descriptor* descriptor,
                                                 ctex_vec4f* pixels, std::size_t pixel_capacity,
                                                 std::size_t* out_pixel_count) {
    return call_boundary("ctex_paint_blend_snapshot", [&] {
        if (descriptor == nullptr || out_pixel_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           descriptor == nullptr ? "descriptor=null" : "out_pixel_count=null");
        }
        validate_structure_size(descriptor->size, CTEX_PAINT_BLEND_DESCRIPTOR_V1_SIZE,
                                CTEX_PAINT_BLEND_DESCRIPTOR_CURRENT_SIZE, "descriptor.size");
        const std::size_t pixel_count = bounded_paint_pixel_count(
            descriptor->width, descriptor->height, CTEX_DIAGNOSTIC_INVALID_PAINT_BLEND);
        *out_pixel_count = pixel_count;
        validate_output_array(pixels, pixel_capacity, pixel_count, "pixels");
        try {
            const PaintBlendInputs inputs = paint_blend_inputs(*descriptor, pixel_count);
            ctex::paint::StrokeSnapshotBlender blender(descriptor->width, descriptor->height,
                                                       inputs.snapshot, descriptor->blend_mode);
            blender.shade(inputs.paint, inputs.strength);
            if (pixels != nullptr) {
                std::transform(blender.result().pixels.begin(), blender.result().pixels.end(),
                               pixels, capi_colour);
            }
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_BLEND,
                           error.what());
        }
    });
}

extern "C" ctex_result ctex_set_log_sink(const ctex_log_sink_descriptor* descriptor) {
    return call_boundary("ctex_set_log_sink", [descriptor] { install_log_sink(descriptor); });
}

extern "C" ctex_result ctex_set_allocator(const ctex_allocator_descriptor* descriptor) {
    return call_boundary("ctex_set_allocator", [descriptor] { install_allocator(descriptor); });
}

extern "C" ctex_result ctex_mesh_create(const ctex_mesh_descriptor* descriptor,
                                        ctex_mesh** out_mesh) {
    return call_boundary("ctex_mesh_create", [&] {
        if (out_mesh == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_mesh=null");
        }
        *out_mesh = nullptr;
        if (descriptor == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "descriptor=null");
        }
        validate_mesh_descriptor(*descriptor);
        try {
            *out_mesh = create_mesh(current_allocator(), *descriptor);
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_MESH,
                           error.what());
        }
    });
}

extern "C" void ctex_mesh_destroy(ctex_mesh* mesh) {
    if (mesh == nullptr) {
        return;
    }
    const ctex_allocator_state allocator = mesh->allocator;
    mesh->~ctex_mesh();
    deallocate_storage(allocator, mesh, sizeof(ctex_mesh), alignof(ctex_mesh));
}

extern "C" ctex_result ctex_mesh_replace(ctex_mesh* mesh, const ctex_mesh_descriptor* descriptor) {
    return call_boundary("ctex_mesh_replace", [&] {
        if (mesh == nullptr || descriptor == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           mesh == nullptr ? "mesh=null" : "descriptor=null");
        }
        validate_mesh_descriptor(*descriptor);
        ctex_mesh_state* replacement = nullptr;
        try {
            replacement = create_mesh_state(*mesh, *descriptor);
        } catch (const std::invalid_argument& error) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_MESH,
                           error.what());
        }
        ctex_mesh_state* previous = std::exchange(mesh->state, replacement);
        destroy_mesh_state(*mesh, previous);
    });
}

extern "C" ctex_result ctex_mesh_get_info(const ctex_mesh* mesh, ctex_mesh_info* out_info) {
    return call_boundary("ctex_mesh_get_info", [&] {
        if (mesh == nullptr || out_info == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           mesh == nullptr ? "mesh=null" : "out_info=null");
        }
        validate_structure_size(out_info->size, CTEX_MESH_INFO_V1_SIZE, CTEX_MESH_INFO_CURRENT_SIZE,
                                "out_info.size");
        const ctex::mesh::MeshDescriptor descriptor = mesh->state->descriptor();
        *out_info = {
            .size = CTEX_MESH_INFO_CURRENT_SIZE,
            .vertex_count = descriptor.positions.size(),
            .triangle_count = descriptor.triangle_indices.size() / 3,
            .uv_set_count = descriptor.uv_sets.size(),
            .partition_count = descriptor.partitions.size(),
            .has_vertex_colors = descriptor.vertex_colors.empty() ? 0U : 1U,
            .revision = mesh->state->revision,
        };
    });
}

extern "C" ctex_result ctex_mesh_get_uv_set_names(const ctex_mesh* mesh, char* buffer,
                                                  std::size_t buffer_size,
                                                  std::size_t* out_required_size,
                                                  std::size_t* out_count) {
    return call_boundary("ctex_mesh_get_uv_set_names", [&] {
        if (mesh == nullptr || out_required_size == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           mesh == nullptr ? "mesh=null" : "out_required_size=null");
        }
        std::size_t required_size = 0;
        for (const std::pmr::string& name : mesh->state->uv_names) {
            if (name.size() + 1 > std::numeric_limits<std::size_t>::max() - required_size) {
                throw std::overflow_error("UV-set name buffer size overflow");
            }
            required_size += name.size() + 1;
        }
        *out_required_size = required_size;
        if (out_count != nullptr) {
            *out_count = mesh->state->uv_names.size();
        }
        validate_string_buffer(buffer, buffer_size, required_size);
        if (buffer != nullptr) {
            std::size_t offset = 0;
            for (const std::pmr::string& name : mesh->state->uv_names) {
                std::memcpy(buffer + offset, name.c_str(), name.size() + 1);
                offset += name.size() + 1;
            }
        }
    });
}

extern "C" ctex_result ctex_document_create(ctex_document** out_document) {
    return call_boundary("ctex_document_create", [out_document] {
        if (out_document == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_document=null");
        }
        *out_document = nullptr;
        *out_document = create_document(current_allocator());
    });
}

extern "C" void ctex_document_destroy(ctex_document* document) {
    if (document == nullptr) {
        return;
    }
    const ctex_allocator_state allocator = document->allocator;
    document->~ctex_document();
    deallocate_storage(allocator, document, sizeof(ctex_document), alignof(ctex_document));
}

extern "C" ctex_result ctex_document_create_texture_set(
    ctex_document* document, const ctex_texture_set_descriptor* descriptor) {
    return call_boundary("ctex_document_create_texture_set", [&] {
        if (document == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "document=null");
        }
        if (descriptor == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "descriptor=null");
        }
        ctex::doc::TextureSetDescriptor converted = texture_set_descriptor(*descriptor);
        const std::string stable_id = ctex::doc::texture_set_stable_id(converted);
        if (document->value.contains_texture_set(stable_id)) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_DUPLICATE_TEXTURE_SET,
                           "texture-set identity is already present: " + stable_id);
        }
        static_cast<void>(document->value.create_texture_set(std::move(converted)));
    });
}

extern "C" ctex_result ctex_document_create_texture_sets_from_mesh(
    ctex_document* document, const ctex_mesh* mesh, const char* uv_set, std::uint32_t width,
    std::uint32_t height, std::uint8_t default_bit_depth) {
    return call_boundary("ctex_document_create_texture_sets_from_mesh", [&] {
        if (document == nullptr || mesh == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           document == nullptr ? "document=null" : "mesh=null");
        }
        create_texture_sets_from_mesh(*document, *mesh, uv_set, width, height, default_bit_depth);
    });
}

extern "C" ctex_result ctex_document_get_texture_set_ids(const ctex_document* document,
                                                         char* buffer, std::size_t buffer_size,
                                                         std::size_t* out_required_size,
                                                         std::size_t* out_count) {
    return call_boundary("ctex_document_get_texture_set_ids", [&] {
        if (document == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "document=null");
        }
        if (out_required_size == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_required_size=null");
        }
        if (buffer == nullptr && buffer_size != 0) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "buffer=null with nonzero buffer_size");
        }

        const std::vector<std::string> identifiers = document->value.texture_set_ids();
        const std::size_t required_size = texture_set_id_buffer_size(identifiers);
        *out_required_size = required_size;
        if (out_count != nullptr) {
            *out_count = identifiers.size();
        }
        if (buffer == nullptr) {
            return;
        }
        validate_string_buffer(buffer, buffer_size, required_size);
        copy_packed_strings(identifiers, buffer);
    });
}

extern "C" ctex_result ctex_texture_set_get_channel_ids(const ctex_document* document,
                                                        const char* texture_set_id, char* buffer,
                                                        std::size_t buffer_size,
                                                        std::size_t* out_required_size,
                                                        std::size_t* out_count) {
    return call_boundary("ctex_texture_set_get_channel_ids", [&] {
        if (document == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "document=null");
        }
        if (out_required_size == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_required_size=null");
        }
        const ctex::doc::TextureSet& texture_set = require_texture_set(*document, texture_set_id);
        const std::vector<std::string> identifiers = texture_set.channels().semantic_ids();
        const std::size_t required_size = texture_set_id_buffer_size(identifiers);
        *out_required_size = required_size;
        if (out_count != nullptr) {
            *out_count = identifiers.size();
        }
        validate_string_buffer(buffer, buffer_size, required_size);
        if (buffer != nullptr) {
            copy_packed_strings(identifiers, buffer);
        }
    });
}

extern "C" ctex_result ctex_texture_set_register_channel(
    ctex_document* document, const char* texture_set_id,
    const ctex_channel_descriptor* descriptor) {
    return call_boundary("ctex_texture_set_register_channel", [&] {
        if (document == nullptr || descriptor == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           document == nullptr ? "document=null" : "descriptor=null");
        }
        ctex::doc::TextureSet& texture_set = require_texture_set(*document, texture_set_id);
        ctex::doc::ChannelDescriptor converted = channel_descriptor(*descriptor);
        if (channel_exists(texture_set.channels(), converted.semantic_id)) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_DUPLICATE_CHANNEL,
                           "channel semantic identifier is already registered: " +
                               std::string(converted.semantic_id));
        }
        texture_set.channels().register_descriptor(std::move(converted));
    });
}

extern "C" ctex_result ctex_texture_set_set_channel_enabled(ctex_document* document,
                                                            const char* texture_set_id,
                                                            const char* semantic_id,
                                                            std::uint32_t enabled,
                                                            std::uint32_t bit_depth_override) {
    return call_boundary("ctex_texture_set_set_channel_enabled", [&] {
        if (document == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "document=null");
        }
        if (enabled > 1) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_VALUE,
                           "enabled=" + std::to_string(enabled));
        }
        if (bit_depth_override != 0 && !valid_channel_bit_depth(bit_depth_override)) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_CHANNEL_BIT_DEPTH,
                           "bit_depth_override=" + std::to_string(bit_depth_override));
        }
        ctex::doc::TextureSet& texture_set = require_texture_set(*document, texture_set_id);
        const ctex::doc::ChannelDescriptor& descriptor =
            require_channel(texture_set.channels(), semantic_id);
        if (enabled == 0) {
            texture_set.channels().disable(semantic_id);
            return;
        }
        if (descriptor.scalar_representation == ctex::doc::ScalarRepresentation::floating_point &&
            bit_depth_override != 0 && bit_depth_override != 32) {
            throw_boundary(
                CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_CHANNEL_BIT_DEPTH,
                "floating-point channel bit_depth_override=" + std::to_string(bit_depth_override));
        }
        const std::optional<std::uint8_t> override =
            bit_depth_override == 0
                ? std::nullopt
                : std::optional<std::uint8_t>(static_cast<std::uint8_t>(bit_depth_override));
        texture_set.channels().enable(semantic_id, override);
    });
}

extern "C" ctex_result ctex_texture_set_get_channel_info(
    const ctex_document* document, const char* texture_set_id, const char* semantic_id,
    ctex_channel_info* out_info, char* export_mapping_buffer,
    std::size_t export_mapping_buffer_size, std::size_t* out_required_export_mapping_size) {
    return call_boundary("ctex_texture_set_get_channel_info", [&] {
        if (document == nullptr || out_info == nullptr ||
            out_required_export_mapping_size == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "document, out_info and out_required_export_mapping_size are required");
        }
        if (out_info->size < CTEX_CHANNEL_INFO_V1_SIZE ||
            out_info->size > CTEX_CHANNEL_INFO_CURRENT_SIZE) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                           "out_info.size=" + std::to_string(out_info->size) +
                               " expected_size=" + std::to_string(CTEX_CHANNEL_INFO_CURRENT_SIZE));
        }
        const ctex::doc::TextureSet& texture_set = require_texture_set(*document, texture_set_id);
        const ctex::doc::TextureChannels& channels = texture_set.channels();
        const ctex::doc::ChannelDescriptor& descriptor = require_channel(channels, semantic_id);
        const std::size_t required_size = descriptor.export_mapping.size() + 1;
        *out_required_export_mapping_size = required_size;
        validate_string_buffer(export_mapping_buffer, export_mapping_buffer_size, required_size);
        if (export_mapping_buffer != nullptr) {
            std::memcpy(export_mapping_buffer, descriptor.export_mapping.c_str(), required_size);
        }
        ctex_channel_info result{};
        result.size = CTEX_CHANNEL_INFO_CURRENT_SIZE;
        result.component_count = descriptor.component_count;
        result.scalar_representation = static_cast<std::uint32_t>(descriptor.scalar_representation);
        result.preferred_bit_depth = descriptor.preferred_bit_depth;
        std::copy(descriptor.default_value.begin(), descriptor.default_value.end(),
                  result.default_value);
        result.default_value_count = descriptor.default_value.size();
        result.classification = static_cast<std::uint32_t>(descriptor.classification);
        result.blending_policy = static_cast<std::uint32_t>(descriptor.blending_policy);
        result.evaluable = descriptor.evaluable ? 1U : 0U;
        result.enabled = channels.is_enabled(semantic_id) ? 1U : 0U;
        result.storage_bit_depth = storage_bit_depth(channels, semantic_id);
        *out_info = result;
    });
}

extern "C" ctex_result ctex_texture_set_get_memory_report(
    const ctex_document* document, const char* texture_set_id,
    ctex_texture_set_memory_report* out_report) {
    return call_boundary("ctex_texture_set_get_memory_report", [&] {
        if (document == nullptr || out_report == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           document == nullptr ? "document=null" : "out_report=null");
        }
        if (out_report->size < CTEX_TEXTURE_SET_MEMORY_REPORT_V1_SIZE ||
            out_report->size > CTEX_TEXTURE_SET_MEMORY_REPORT_CURRENT_SIZE) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                           "out_report.size=" + std::to_string(out_report->size) +
                               " expected_size=" +
                               std::to_string(CTEX_TEXTURE_SET_MEMORY_REPORT_CURRENT_SIZE));
        }
        const ctex::doc::TextureSet& texture_set = require_texture_set(*document, texture_set_id);
        const ctex::doc::TextureSetMemoryReport report = texture_set.memory_report();
        *out_report = {
            .size = CTEX_TEXTURE_SET_MEMORY_REPORT_CURRENT_SIZE,
            .enabled_channel_count = texture_set.channels().enabled_channel_count(),
            .channel_pixel_bytes = report.channel_pixel_bytes,
            .mesh_map_pixel_bytes = report.mesh_map_pixel_bytes,
            .total_resident_bytes = report.total_resident_bytes,
        };
    });
}

extern "C" ctex_result ctex_paint_preview_session_create(ctex_document* document,
                                                         const char* texture_set_id,
                                                         const char* semantic_id,
                                                         ctex_paint_preview_session** out_session) {
    return call_boundary("ctex_paint_preview_session_create", [&] {
        if (out_session == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "out_session=null");
        }
        *out_session = nullptr;
        if (document == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "document=null");
        }
        ctex::doc::TextureSet& texture_set = require_texture_set(*document, texture_set_id);
        ctex::doc::TextureChannels& channels = texture_set.channels();
        static_cast<void>(require_channel(channels, semantic_id));
        if (!channels.is_enabled(semantic_id)) {
            throw_boundary(CTEX_RESULT_MISSING_RESOURCE, CTEX_DIAGNOSTIC_MISSING_CHANNEL,
                           "paint preview channel is not enabled");
        }
        *out_session = create_paint_preview_session(document->allocator, document, texture_set_id,
                                                    semantic_id, channels);
    });
}

extern "C" void ctex_paint_preview_session_destroy(ctex_paint_preview_session* session) {
    if (session == nullptr) {
        return;
    }
    const ctex_allocator_state allocator = session->allocator;
    session->~ctex_paint_preview_session();
    deallocate_storage(allocator, session, sizeof(ctex_paint_preview_session),
                       alignof(ctex_paint_preview_session));
}

extern "C" ctex_result ctex_paint_preview_session_write_pixel(ctex_paint_preview_session* session,
                                                              std::uint32_t x, std::uint32_t y,
                                                              const void* pixel,
                                                              std::size_t pixel_size) {
    return call_boundary("ctex_paint_preview_session_write_pixel", [&] {
        if (session == nullptr || pixel == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           session == nullptr ? "session=null" : "pixel=null");
        }
        if (pixel_size != session->preview.preview_pixels().pixel_bytes()) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_INVALID_PAINT_PREVIEW,
                           "pixel size does not match the preview channel format");
        }
        try {
            session->preview.write_pixel(x, y, {static_cast<const std::byte*>(pixel), pixel_size});
        } catch (const std::invalid_argument& error) {
            throw_invalid_paint_preview(error);
        } catch (const std::logic_error& error) {
            throw_invalid_paint_preview(error);
        }
    });
}

extern "C" ctex_result ctex_paint_preview_session_get_info(
    const ctex_paint_preview_session* session, ctex_paint_preview_info* out_info) {
    return call_boundary("ctex_paint_preview_session_get_info", [&] {
        if (session == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "session=null");
        }
        validate_paint_preview_info(out_info);
        *out_info = paint_preview_info(*session);
    });
}

extern "C" ctex_result ctex_paint_preview_session_get_pixels(
    const ctex_paint_preview_session* session, void* pixel_buffer, std::size_t pixel_buffer_size,
    std::size_t* out_required_size) {
    return call_boundary("ctex_paint_preview_session_get_pixels", [&] {
        if (session == nullptr || out_required_size == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           session == nullptr ? "session=null" : "out_required_size=null");
        }
        const ctex::image::TiledImage& pixels = session->preview.preview_pixels();
        const std::size_t required_size =
            static_cast<std::size_t>(pixels.width()) * pixels.height() * pixels.pixel_bytes();
        *out_required_size = required_size;
        validate_string_buffer(static_cast<char*>(pixel_buffer), pixel_buffer_size, required_size);
        if (pixel_buffer == nullptr) {
            return;
        }
        auto* destination = static_cast<std::byte*>(pixel_buffer);
        for (std::uint32_t y = 0; y < pixels.height(); ++y) {
            for (std::uint32_t x = 0; x < pixels.width(); ++x) {
                const std::span<const std::byte> pixel = pixels.read_pixel(x, y);
                std::memcpy(destination, pixel.data(), pixel.size());
                destination += pixel.size();
            }
        }
    });
}

extern "C" ctex_result ctex_paint_preview_session_get_changed_tiles(
    const ctex_paint_preview_session* session, ctex_paint_tile_coordinate* tiles,
    std::size_t tile_capacity, std::size_t* out_tile_count) {
    return call_boundary("ctex_paint_preview_session_get_changed_tiles", [&] {
        if (session == nullptr || out_tile_count == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           session == nullptr ? "session=null" : "out_tile_count=null");
        }
        const std::vector<ctex::image::TileCoordinate> changed =
            paint_preview_changed_tiles(*session);
        *out_tile_count = changed.size();
        validate_output_array(tiles, tile_capacity, changed.size(), "tiles");
        for (std::size_t index = 0; index < changed.size(); ++index) {
            tiles[index] = {changed[index].x, changed[index].y};
        }
    });
}

extern "C" ctex_result ctex_paint_preview_session_finalize(ctex_paint_preview_session* session,
                                                           const std::uint8_t* coverage,
                                                           std::size_t coverage_count,
                                                           std::uint32_t dilation_radius,
                                                           ctex_paint_preview_info* out_info) {
    return call_boundary("ctex_paint_preview_session_finalize", [&] {
        if (session == nullptr || coverage == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           session == nullptr ? "session=null" : "coverage=null");
        }
        validate_paint_preview_info(out_info);
        try {
            static_cast<void>(
                session->preview.finalize({coverage, coverage_count}, dilation_radius));
        } catch (const std::invalid_argument& error) {
            throw_invalid_paint_preview(error);
        } catch (const std::logic_error& error) {
            throw_invalid_paint_preview(error);
        }
        *out_info = paint_preview_info(*session);
    });
}

extern "C" ctex_result ctex_paint_preview_session_commit(ctex_paint_preview_session* session,
                                                         ctex_paint_preview_info* out_info) {
    return call_boundary("ctex_paint_preview_session_commit", [&] {
        if (session == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "session=null");
        }
        validate_paint_preview_info(out_info);
        try {
            ctex::doc::TextureChannels& channels =
                session->document->value.texture_set(session->texture_set_id).channels();
            const ctex::paint::PaintPreviewCommitReport report = session->preview.commit(channels);
            session->committed = report.committed;
            session->maximum_component_error = report.maximum_component_error;
        } catch (const std::invalid_argument& error) {
            throw_invalid_paint_preview(error);
        } catch (const std::logic_error& error) {
            throw_invalid_paint_preview(error);
        }
        *out_info = paint_preview_info(*session);
    });
}

extern "C" ctex_result ctex_paint_preview_session_cancel(ctex_paint_preview_session* session) {
    return call_boundary("ctex_paint_preview_session_cancel", [&] {
        if (session == nullptr) {
            throw_boundary(CTEX_RESULT_INVALID_ARGUMENT, CTEX_DIAGNOSTIC_NULL_ARGUMENT,
                           "session=null");
        }
        try {
            session->preview.cancel();
        } catch (const std::logic_error& error) {
            throw_invalid_paint_preview(error);
        }
    });
}

extern "C" ctex_result ctex_get_last_result(void) { return last_diagnostic.result; }

extern "C" ctex_diagnostic_code ctex_get_last_diagnostic_code(void) { return last_diagnostic.code; }

extern "C" const char* ctex_get_last_diagnostic(void) { return last_diagnostic.message; }
