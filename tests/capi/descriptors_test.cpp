#include <ctex/capi.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "capi_internal.hpp"

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ctex_texture_set_descriptor descriptor(const char* name, const char* key, std::uint8_t bit_depth) {
    return {.size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
            .display_name = name,
            .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
            .partition_key = key,
            .uv_set = "UV0",
            .width = 16,
            .height = 16,
            .default_bit_depth = bit_depth};
}

}  // namespace

int main() {
    ctex_document* document = nullptr;
    if (!expect(ctex_document_create(&document) == CTEX_RESULT_SUCCESS && document != nullptr,
                "document creation failed")) {
        return 1;
    }

    ctex_texture_set_descriptor current = descriptor("Current", "current", 16);
    bool passed =
        expect(ctex_document_create_texture_set(document, &current) == CTEX_RESULT_SUCCESS,
               "current descriptor was refused");

    ctex_texture_set_descriptor older = descriptor("Older", "older", 32);
    older.size = CTEX_TEXTURE_SET_DESCRIPTOR_V1_SIZE;
    passed = expect(ctex_document_create_texture_set(document, &older) == CTEX_RESULT_SUCCESS,
                    "older descriptor prefix was refused") &&
             passed;

    const std::vector<std::string> identifiers = document->value.texture_set_ids();
    passed = expect(identifiers.size() == 2, "valid descriptors did not create two texture sets") &&
             passed;
    if (identifiers.size() == 2) {
        const auto& first = document->value.texture_set(identifiers[0]).descriptor();
        const auto& second = document->value.texture_set(identifiers[1]).descriptor();
        const auto depth_for = [&](std::string_view key) {
            return first.partition_key == key ? first.default_bit_depth : second.default_bit_depth;
        };
        passed = expect(depth_for("current") == 16, "current descriptor field was not consumed") &&
                 passed;
        passed = expect(depth_for("older") == 8,
                        "field absent from older descriptor did not take its default") &&
                 passed;
    }

    ctex_texture_set_descriptor future = descriptor("Future", "future", 8);
    future.size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE + 1;
    const std::size_t before_refusal = document->value.texture_set_count();
    const ctex_result future_result = ctex_document_create_texture_set(document, &future);
    const std::string future_diagnostic = ctex_get_last_diagnostic();
    passed =
        expect(future_result == CTEX_RESULT_INVALID_ARGUMENT &&
                   ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE &&
                   document->value.texture_set_count() == before_refusal,
               "implausibly large descriptor mutated the document") &&
        passed;
    passed = expect(future_diagnostic.find(std::to_string(future.size)) != std::string::npos &&
                        future_diagnostic.find(std::to_string(
                            CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE)) != std::string::npos,
                    "future descriptor diagnostic omitted declared or library size") &&
             passed;

    ctex_texture_set_descriptor truncated = descriptor("Truncated", "truncated", 8);
    truncated.size = CTEX_TEXTURE_SET_DESCRIPTOR_V1_SIZE - 1;
    passed =
        expect(ctex_document_create_texture_set(document, &truncated) ==
                       CTEX_RESULT_INVALID_ARGUMENT &&
                   ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE &&
                   document->value.texture_set_count() == before_refusal,
               "truncated descriptor was accepted or mutated the document") &&
        passed;

    ctex_document_destroy(document);
    return passed ? 0 : 1;
}
