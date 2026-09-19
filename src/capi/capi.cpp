#include <ctex/capi.h>
#include <ctex/version.h>

#include <cstdio>
#include <ctex/doc/document.hpp>
#include <exception>
#include <new>
#include <stdexcept>

struct ctex_document {
    ctex::doc::TextureDocument value;
};

namespace {

struct DiagnosticState {
    ctex_result result{CTEX_RESULT_SUCCESS};
    char message[1024]{};
};

thread_local DiagnosticState last_diagnostic;

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

extern "C" ctex_result ctex_get_last_result(void) { return last_diagnostic.result; }

extern "C" const char* ctex_get_last_diagnostic(void) { return last_diagnostic.message; }
