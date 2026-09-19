#include <ctex/capi.h>

#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "capi_internal.hpp"

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ctex::doc::TextureSetDescriptor descriptor(std::string name, std::string key) {
    return {.display_name = std::move(name),
            .partition_kind = ctex::doc::PartitionSourceKind::material,
            .partition_key = std::move(key),
            .uv_set = "UV0",
            .width = 16,
            .height = 16,
            .default_bit_depth = 8};
}

std::vector<std::string> decode_identifiers(const std::vector<char>& bytes, std::size_t count) {
    std::vector<std::string> identifiers;
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const char* identifier = bytes.data() + offset;
        identifiers.emplace_back(identifier);
        offset += identifiers.back().size() + 1;
    }
    if (identifiers.size() != count) {
        return {};
    }
    return identifiers;
}

}  // namespace

int main() {
    ctex_document* document = nullptr;
    if (!expect(ctex_document_create(&document) == CTEX_RESULT_SUCCESS && document != nullptr,
                "document creation failed")) {
        return 1;
    }
    document->value.create_texture_set(descriptor("Second", "material:b"));
    document->value.create_texture_set(descriptor("First", "material:a"));
    const std::vector<std::string> expected = document->value.texture_set_ids();

    std::size_t required_size = 0;
    std::size_t count = 0;
    bool passed = expect(ctex_document_get_texture_set_ids(document, nullptr, 0, &required_size,
                                                           &count) == CTEX_RESULT_SUCCESS,
                         "null-buffer sizing query failed");
    const std::size_t expected_size = expected[0].size() + expected[1].size() + 2;
    passed = expect(required_size == expected_size && count == expected.size(),
                    "sizing query returned the wrong byte or item count") &&
             passed;

    std::vector<char> short_buffer(required_size - 1, 'x');
    const std::vector<char> untouched = short_buffer;
    std::size_t short_required = 0;
    const ctex_result short_result = ctex_document_get_texture_set_ids(
        document, short_buffer.data(), short_buffer.size(), &short_required, nullptr);
    passed = expect(short_result == CTEX_RESULT_BUFFER_TOO_SMALL &&
                        ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL &&
                        short_required == required_size && short_buffer == untouched,
                    "short buffer was not refused atomically with its required size") &&
             passed;
    const std::string diagnostic = ctex_get_last_diagnostic();
    passed = expect(diagnostic.find(std::to_string(short_buffer.size())) != std::string::npos &&
                        diagnostic.find(std::to_string(required_size)) != std::string::npos,
                    "short-buffer diagnostic omitted supplied or required size") &&
             passed;

    std::vector<char> bytes(required_size, '\0');
    std::size_t filled_size = 0;
    std::size_t filled_count = 0;
    const ctex_result fill_result = ctex_document_get_texture_set_ids(
        document, bytes.data(), bytes.size(), &filled_size, &filled_count);
    passed = expect(fill_result == CTEX_RESULT_SUCCESS && filled_size == required_size &&
                        filled_count == count && decode_identifiers(bytes, count) == expected,
                    "right-sized caller buffer did not receive the complete ordered list") &&
             passed;
    passed = expect(ctex_get_last_result() == CTEX_RESULT_SUCCESS &&
                        ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_NONE &&
                        std::strcmp(ctex_get_last_diagnostic(), "") == 0,
                    "successful fill did not clear the prior diagnostic") &&
             passed;

    ctex_document_destroy(document);
    return passed ? 0 : 1;
}
