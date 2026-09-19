#include <ctex/capi.h>

#include <barrier>
#include <cstddef>
#include <functional>
#include <string>
#include <thread>

namespace {

constexpr std::size_t texture_set_count = 256;

struct ThreadResult {
    bool completed{};
    std::size_t count{};
    ctex_result last_result{CTEX_RESULT_INTERNAL_ERROR};
    std::string diagnostic;
};

ctex_texture_set_descriptor descriptor(const std::string& name, const std::string& key) {
    return {.size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
            .display_name = name.c_str(),
            .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
            .partition_key = key.c_str(),
            .uv_set = "UV0",
            .width = 16,
            .height = 16,
            .default_bit_depth = 8};
}

void populate_document(ctex_document* document, std::string prefix, std::barrier<>& start,
                       bool finish_with_failure, ThreadResult& result) {
    start.arrive_and_wait();
    for (std::size_t index = 0; index < texture_set_count; ++index) {
        const std::string suffix = std::to_string(index);
        const std::string name = prefix + " name " + suffix;
        const std::string key = prefix + ":" + suffix;
        const ctex_texture_set_descriptor next = descriptor(name, key);
        if (ctex_document_create_texture_set(document, &next) != CTEX_RESULT_SUCCESS) {
            result.diagnostic = ctex_get_last_diagnostic();
            return;
        }
    }

    std::size_t required_size = 0;
    if (ctex_document_get_texture_set_ids(document, nullptr, 0, &required_size, &result.count) !=
        CTEX_RESULT_SUCCESS) {
        result.diagnostic = ctex_get_last_diagnostic();
        return;
    }

    if (finish_with_failure) {
        ctex_texture_set_descriptor future = descriptor("future", "future");
        future.size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE + 1;
        result.last_result = ctex_document_create_texture_set(document, &future);
        result.diagnostic = ctex_get_last_diagnostic();
    } else {
        result.last_result = ctex_get_last_result();
        result.diagnostic = ctex_get_last_diagnostic();
    }
    result.completed = true;
}

}  // namespace

int main() {
    ctex_document* first = nullptr;
    ctex_document* second = nullptr;
    if (ctex_document_create(&first) != CTEX_RESULT_SUCCESS ||
        ctex_document_create(&second) != CTEX_RESULT_SUCCESS) {
        ctex_document_destroy(first);
        ctex_document_destroy(second);
        return 1;
    }

    std::barrier<> start(3);
    ThreadResult first_result;
    ThreadResult second_result;
    std::thread first_thread(populate_document, first, "first", std::ref(start), true,
                             std::ref(first_result));
    std::thread second_thread(populate_document, second, "second", std::ref(start), false,
                              std::ref(second_result));
    start.arrive_and_wait();
    first_thread.join();
    second_thread.join();

    const bool independent_documents = first_result.completed && second_result.completed &&
                                       first_result.count == texture_set_count &&
                                       second_result.count == texture_set_count;
    const bool isolated_diagnostics =
        first_result.last_result == CTEX_RESULT_INVALID_ARGUMENT &&
        first_result.diagnostic.find("descriptor.size") != std::string::npos &&
        second_result.last_result == CTEX_RESULT_SUCCESS && second_result.diagnostic.empty();

    ctex_document_destroy(first);
    ctex_document_destroy(second);
    return independent_documents && isolated_diagnostics ? 0 : 1;
}
