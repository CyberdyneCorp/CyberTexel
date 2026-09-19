#include <ctex/capi.h>
#include <ctex/version.h>

#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <mutex>
#include <new>
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

}  // namespace

extern "C" ctex_version ctex_get_version(void) {
    return {
        CTEX_VERSION_MAJOR,
        CTEX_VERSION_MINOR,
        CTEX_VERSION_PATCH,
        CTEX_VERSION_STRING,
    };
}

extern "C" ctex_version ctex_get_abi_version(void) { return ctex_get_version(); }

extern "C" ctex_result ctex_set_log_sink(const ctex_log_sink_descriptor* descriptor) {
    return call_boundary("ctex_set_log_sink", [descriptor] { install_log_sink(descriptor); });
}

extern "C" ctex_result ctex_set_allocator(const ctex_allocator_descriptor* descriptor) {
    return call_boundary("ctex_set_allocator", [descriptor] { install_allocator(descriptor); });
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
        if (buffer_size < required_size) {
            throw_boundary(CTEX_RESULT_BUFFER_TOO_SMALL, CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL,
                           "buffer_size=" + std::to_string(buffer_size) +
                               " required_size=" + std::to_string(required_size));
        }

        std::size_t offset = 0;
        for (const std::string& identifier : identifiers) {
            const std::size_t entry_size = identifier.size() + 1;
            std::memcpy(buffer + offset, identifier.c_str(), entry_size);
            offset += entry_size;
        }
    });
}

extern "C" ctex_result ctex_get_last_result(void) { return last_diagnostic.result; }

extern "C" ctex_diagnostic_code ctex_get_last_diagnostic_code(void) { return last_diagnostic.code; }

extern "C" const char* ctex_get_last_diagnostic(void) { return last_diagnostic.message; }
