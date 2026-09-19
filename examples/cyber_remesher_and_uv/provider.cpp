#include <cyber_capi.h>

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <ctex/doc/document.hpp>
#include <ctex/image/pixel_format.hpp>
#include <ctex/maps/bake_provider.hpp>
#include <ctex/maps/mesh_maps.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using ctex::maps::BakeControl;
using ctex::maps::BakeProvider;
using ctex::maps::BakeProviderOutput;
using ctex::maps::BakeProviderStatus;
using ctex::maps::BakeRequest;
using ctex::maps::MeshMapKind;

using CyberMeshPtr = std::unique_ptr<CyberMesh, decltype(&cyber_mesh_free)>;
using CyberImagePtr = std::unique_ptr<CyberImage, decltype(&cyber_image_free)>;

constexpr std::array supported_maps{
    MeshMapKind::tangent_space_normal,
    MeshMapKind::ambient_occlusion,
    MeshMapKind::curvature,
    MeshMapKind::position,
    MeshMapKind::height,
    MeshMapKind::vertex_colour,
};

std::optional<CyberBakeMap> cyber_map(MeshMapKind kind) noexcept {
    switch (kind) {
        case MeshMapKind::tangent_space_normal:
            return CYBER_BAKE_NORMAL;
        case MeshMapKind::ambient_occlusion:
            return CYBER_BAKE_AO;
        case MeshMapKind::curvature:
            return CYBER_BAKE_CURVATURE;
        case MeshMapKind::position:
            return CYBER_BAKE_POSITION;
        case MeshMapKind::height:
            return CYBER_BAKE_DISPLACEMENT;
        case MeshMapKind::vertex_colour:
            return CYBER_BAKE_COLOR;
        default:
            return std::nullopt;
    }
}

struct CyberRemesherProviderState {
    const CyberMesh* low{};
    const CyberMesh* high{};
    CyberBakeParams parameters{};
    std::vector<float> pixels;
    std::string detail;
};

bool can_produce(void*, MeshMapKind kind) noexcept { return cyber_map(kind).has_value(); }

bool is_cancelled(const BakeControl* control) noexcept {
    return control != nullptr && control->is_cancelled != nullptr &&
           control->is_cancelled(control->user_data);
}

void report_progress(const BakeControl* control, double fraction) noexcept {
    if (control != nullptr && control->report_progress != nullptr) {
        control->report_progress(control->user_data, {.fraction = fraction});
    }
}

BakeProviderStatus fail(CyberRemesherProviderState& state, std::string detail,
                        BakeProviderOutput* output) noexcept {
    state.detail = std::move(detail);
    output->detail = state.detail.c_str();
    return BakeProviderStatus::failed;
}

BakeProviderStatus produce(void* user_data, const BakeRequest* request, const BakeControl* control,
                           BakeProviderOutput* output) noexcept {
    auto& state = *static_cast<CyberRemesherProviderState*>(user_data);
    try {
        const std::optional<CyberBakeMap> map = cyber_map(request->kind);
        if (!map || state.low == nullptr || state.high == nullptr) {
            return fail(state, "unsupported map or missing source mesh", output);
        }
        if (request->width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
            request->height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
            return fail(state, "requested dimensions exceed CyberRemesherAndUV limits", output);
        }
        if (is_cancelled(control)) {
            return BakeProviderStatus::cancelled;
        }

        CyberBakeParams parameters = state.parameters;
        parameters.width = static_cast<int>(request->width);
        parameters.height = static_cast<int>(request->height);
        report_progress(control, 0.05);

        CyberImage* raw_image = nullptr;
        const CyberStatus status = cyber_bake(state.low, state.high, *map, &parameters, &raw_image);
        CyberImagePtr image(raw_image, cyber_image_free);
        if (status != CYBER_OK || image == nullptr) {
            const char* detail = cyber_last_error();
            return fail(state, detail == nullptr ? cyber_status_string(status) : detail, output);
        }
        if (is_cancelled(control)) {
            return BakeProviderStatus::cancelled;
        }

        const int width = cyber_image_width(image.get());
        const int height = cyber_image_height(image.get());
        const int channels = cyber_image_channels(image.get());
        if (width <= 0 || height <= 0 || channels < 1 || channels > 4) {
            return fail(state, "CyberRemesherAndUV returned an invalid image layout", output);
        }
        const std::size_t required = cyber_image_copy_pixels(image.get(), nullptr, 0);
        state.pixels.resize(required);
        if (cyber_image_copy_pixels(image.get(), state.pixels.data(), state.pixels.size()) !=
            required) {
            return fail(state, "CyberRemesherAndUV returned an incomplete pixel buffer", output);
        }

        output->image = {
            .width = static_cast<std::uint32_t>(width),
            .height = static_cast<std::uint32_t>(height),
            .format = {.channel_type = ctex::image::ChannelType::float32,
                       .channel_count = static_cast<std::uint8_t>(channels)},
            .row_stride_bytes = static_cast<std::size_t>(width) *
                                static_cast<std::size_t>(channels) * sizeof(float),
            .pixels = state.pixels.data(),
            .pixel_bytes = state.pixels.size() * sizeof(float),
        };
        if (request->kind == MeshMapKind::tangent_space_normal) {
            output->normal_convention = ctex::maps::NormalMapConvention::open_gl;
        }
        report_progress(control, 0.95);
        return BakeProviderStatus::completed;
    } catch (const std::exception& error) {
        return fail(state, error.what(), output);
    } catch (...) {
        return fail(state, "unexpected provider failure", output);
    }
}

BakeProvider make_provider(CyberRemesherProviderState& state) noexcept {
    return {.name = "CyberRemesherAndUV",
            .user_data = &state,
            .can_produce = can_produce,
            .request = produce};
}

std::uint32_t parse_resolution(std::string_view text) {
    std::uint32_t value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value == 0) {
        throw std::invalid_argument("resolution must be a positive integer");
    }
    return value;
}

CyberMeshPtr load_mesh(const char* path) {
    CyberMesh* raw_mesh = nullptr;
    if (cyber_mesh_load(path, &raw_mesh) != CYBER_OK) {
        const char* detail = cyber_last_error();
        throw std::runtime_error(std::string("could not load '") + path +
                                 "': " + (detail == nullptr ? "unknown error" : detail));
    }
    return CyberMeshPtr(raw_mesh, cyber_mesh_free);
}

int run(const char* low_path, const char* high_path, std::uint32_t resolution) {
    CyberMeshPtr low = load_mesh(low_path);
    CyberMeshPtr high = load_mesh(high_path);

    CyberRemesherProviderState state{
        .low = low.get(), .high = high.get(), .parameters = {}, .pixels = {}, .detail = {}};
    cyber_default_bake_params(&state.parameters);
    const BakeProvider provider = make_provider(state);

    ctex::doc::TextureDocument document;
    ctex::doc::TextureSet& texture_set =
        document.create_texture_set({.display_name = "CyberRemesherAndUV bake",
                                     .partition_kind = ctex::doc::PartitionSourceKind::object,
                                     .partition_key = "mesh",
                                     .uv_set = "uv0",
                                     .width = resolution,
                                     .height = resolution,
                                     .default_bit_depth = 16});
    ctex::maps::MeshMapSet maps(texture_set, ctex::mesh::MeshRevision{1});

    for (const MeshMapKind kind : supported_maps) {
        const ctex::maps::BakeRequestResult result =
            ctex::maps::request_bake(provider, maps, kind, resolution, resolution);
        if (result.status != ctex::maps::BakeRequestStatus::completed) {
            throw std::runtime_error(result.message);
        }
        std::cout << "bound " << ctex::maps::mesh_map_name(kind) << '\n';
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "usage: " << argv[0] << " LOW_MESH HIGH_MESH [RESOLUTION]\n";
        return 2;
    }
    try {
        return run(argv[1], argv[2], argc == 4 ? parse_resolution(argv[3]) : 512);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
