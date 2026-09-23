#include <ctex/capi.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <vector>

#if defined(_WIN32)
#define CTEX_CLI_PROVIDER_EXPORT __declspec(dllexport)
#else
#define CTEX_CLI_PROVIDER_EXPORT __attribute__((visibility("default")))
#endif

namespace {

struct ProviderState {
    std::vector<std::uint8_t> pixels;
};

ProviderState state;

std::uint32_t can_produce(void*, std::uint32_t kind) {
    return kind == CTEX_MESH_MAP_AMBIENT_OCCLUSION ? 1U : 0U;
}

std::uint32_t request(void* user_data, const ctex_mesh_map_bake_request_descriptor* request,
                      const ctex_mesh_map_bake_control* control,
                      ctex_mesh_map_bake_output_descriptor* output) {
    auto& provider = *static_cast<ProviderState*>(user_data);
    if (request == nullptr || output == nullptr ||
        request->size != CTEX_MESH_MAP_BAKE_REQUEST_DESCRIPTOR_CURRENT_SIZE ||
        output->size != CTEX_MESH_MAP_BAKE_OUTPUT_DESCRIPTOR_CURRENT_SIZE ||
        request->kind != CTEX_MESH_MAP_AMBIENT_OCCLUSION || request->width == 0 ||
        request->height == 0 ||
        request->width > std::numeric_limits<std::size_t>::max() / request->height) {
        return CTEX_MESH_MAP_BAKE_PROVIDER_FAILED;
    }
    if (std::getenv("CTEX_CLI_FIXTURE_MISSING_AO") != nullptr) {
        output->detail = "required ambient-occlusion mesh map is not bound";
        return CTEX_MESH_MAP_BAKE_PROVIDER_FAILED;
    }
    if (control != nullptr && control->is_cancelled != nullptr &&
        control->is_cancelled(control->user_data) != 0) {
        return CTEX_MESH_MAP_BAKE_PROVIDER_CANCELLED;
    }
    provider.pixels.resize(static_cast<std::size_t>(request->width) * request->height);
    for (std::uint32_t y = 0; y < request->height; ++y) {
        for (std::uint32_t x = 0; x < request->width; ++x) {
            provider.pixels[static_cast<std::size_t>(y) * request->width + x] =
                static_cast<std::uint8_t>((x * 17U + y * 31U) & 0xffU);
        }
    }
    if (control != nullptr && control->report_progress != nullptr) {
        control->report_progress(control->user_data, 0.5);
    }
    *output = {};
    output->size = CTEX_MESH_MAP_BAKE_OUTPUT_DESCRIPTOR_CURRENT_SIZE;
    output->buffer = {
        .size = CTEX_MESH_MAP_PIXEL_BUFFER_DESCRIPTOR_CURRENT_SIZE,
        .width = request->width,
        .height = request->height,
        .component_type = CTEX_TRANSPORT_COMPONENT_UINT8_UNORM,
        .component_count = 1,
        .row_stride_bytes = request->width,
        .pixels = provider.pixels.data(),
        .pixel_bytes = provider.pixels.size(),
    };
    output->detail = "fixture ambient occlusion";
    return CTEX_MESH_MAP_BAKE_PROVIDER_COMPLETED;
}

}  // namespace

extern "C" CTEX_CLI_PROVIDER_EXPORT ctex_result
ctex_mesh_map_bake_provider_v1(ctex_mesh_map_bake_provider_descriptor* out_provider) {
    if (out_provider == nullptr ||
        out_provider->size != CTEX_MESH_MAP_BAKE_PROVIDER_DESCRIPTOR_CURRENT_SIZE) {
        return CTEX_RESULT_INVALID_ARGUMENT;
    }
    *out_provider = {.size = CTEX_MESH_MAP_BAKE_PROVIDER_DESCRIPTOR_CURRENT_SIZE,
                     .name = "fixture-provider",
                     .user_data = &state,
                     .can_produce = can_produce,
                     .request = request};
    return CTEX_RESULT_SUCCESS;
}
