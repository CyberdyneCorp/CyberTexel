#include <ctex/capi.h>
#include <ctex/version.h>

#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "capi_internal.hpp"

namespace {

struct DiagnosticState {
    ctex_result result{CTEX_RESULT_SUCCESS};
    char message[1024]{};
};

thread_local DiagnosticState last_diagnostic;

class BoundaryError final : public std::runtime_error {
public:
    BoundaryError(ctex_result result, std::string message)
        : std::runtime_error(std::move(message)), result_(result) {}

    [[nodiscard]] ctex_result result() const noexcept { return result_; }

private:
    ctex_result result_;
};

void clear_diagnostic() noexcept { last_diagnostic = {}; }

void set_diagnostic(ctex_result result, const char* operation, const char* detail) noexcept {
    last_diagnostic.result = result;
    static_cast<void>(std::snprintf(last_diagnostic.message, sizeof(last_diagnostic.message),
                                    "%s: %s", operation, detail));
}

template <typename Operation>
ctex_result call_boundary(const char* name, Operation&& operation) noexcept {
    clear_diagnostic();
    try {
        operation();
        return CTEX_RESULT_SUCCESS;
    } catch (const BoundaryError& error) {
        set_diagnostic(error.result(), name, error.what());
        return error.result();
    } catch (const std::invalid_argument& error) {
        set_diagnostic(CTEX_RESULT_INVALID_ARGUMENT, name, error.what());
        return CTEX_RESULT_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        set_diagnostic(CTEX_RESULT_OUT_OF_MEMORY, name, "allocation failed");
        return CTEX_RESULT_OUT_OF_MEMORY;
    } catch (const std::exception& error) {
        set_diagnostic(CTEX_RESULT_INTERNAL_ERROR, name, error.what());
        return CTEX_RESULT_INTERNAL_ERROR;
    } catch (...) {
        set_diagnostic(CTEX_RESULT_INTERNAL_ERROR, name, "unknown internal failure");
        return CTEX_RESULT_INTERNAL_ERROR;
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

}  // namespace

extern "C" ctex_version ctex_get_version(void) {
    return {
        CTEX_VERSION_MAJOR,
        CTEX_VERSION_MINOR,
        CTEX_VERSION_PATCH,
        CTEX_VERSION_STRING,
    };
}

extern "C" ctex_result ctex_document_create(ctex_document** out_document) {
    return call_boundary("ctex_document_create", [out_document] {
        if (out_document == nullptr) {
            throw std::invalid_argument("out_document=null");
        }
        *out_document = nullptr;
        *out_document = new ctex_document{};
    });
}

extern "C" void ctex_document_destroy(ctex_document* document) { delete document; }

extern "C" ctex_result ctex_document_get_texture_set_ids(const ctex_document* document,
                                                         char* buffer, std::size_t buffer_size,
                                                         std::size_t* out_required_size,
                                                         std::size_t* out_count) {
    return call_boundary("ctex_document_get_texture_set_ids", [&] {
        if (document == nullptr) {
            throw std::invalid_argument("document=null");
        }
        if (out_required_size == nullptr) {
            throw std::invalid_argument("out_required_size=null");
        }
        if (buffer == nullptr && buffer_size != 0) {
            throw std::invalid_argument("buffer=null with nonzero buffer_size");
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
            throw BoundaryError(CTEX_RESULT_BUFFER_TOO_SMALL,
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

extern "C" const char* ctex_get_last_diagnostic(void) { return last_diagnostic.message; }
