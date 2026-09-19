#include <ctex/capi.h>
#include <ctex/version.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctex/image/color_policy.hpp>
#include <ctex/io/image_io.hpp>
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

extern "C" ctex_result ctex_get_last_result(void) { return last_diagnostic.result; }

extern "C" ctex_diagnostic_code ctex_get_last_diagnostic_code(void) { return last_diagnostic.code; }

extern "C" const char* ctex_get_last_diagnostic(void) { return last_diagnostic.message; }
