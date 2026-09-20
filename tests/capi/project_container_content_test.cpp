#include <ctex/capi.h>

#include <array>
#include <cstddef>
#include <ctex/image/tiled_image.hpp>
#include <ctex/io/project_container.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ctex::io::ProjectContainer populated_container() {
    const std::array clear{std::byte{0}};
    const std::array painted{std::byte{231}};
    ctex::image::TiledImage pixels(
        16'384, 16'384, {.channel_type = ctex::image::ChannelType::uint8_unorm, .channel_count = 1},
        ctex::image::default_tile_size, clear);
    pixels.write_pixel(16'383, 16'383, painted);

    ctex::io::ProjectContainer container;
    container.tiled_images.push_back(
        ctex::io::snapshot_tiled_image("layers/paint/base-color", pixels));
    container.resources.push_back({.identifier = "mesh/source",
                                   .kind = "mesh",
                                   .relative_path = "meshes/source.glb",
                                   .packed_bytes = std::nullopt});
    container.resources.push_back(
        {.identifier = "fonts/label",
         .kind = "font",
         .relative_path = "fonts/label.ttf",
         .packed_bytes = std::vector<std::byte>{std::byte{'o'}, std::byte{'t'}, std::byte{'f'}}});
    container.assets.push_back({.identifier = "materials/example",
                                .kind = "material",
                                .format_version = 1,
                                .resource_dependencies = {"fonts/label"},
                                .tiled_image_dependencies = {"layers/paint/base-color"},
                                .payload = {std::byte{'m'}, std::byte{'a'}, std::byte{'t'}}});
    return container;
}

}  // namespace

int main() {
    const std::vector<std::byte> encoded = ctex::io::write_project_container(populated_container());
    ctex_project_container_info info{};
    info.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE;
    if (!expect(ctex_project_container_normalize(encoded.data(), encoded.size(), nullptr, &info,
                                                 nullptr, 0, nullptr, 0) == CTEX_RESULT_SUCCESS,
                "populated container sizing query failed")) {
        return 1;
    }
    std::vector<std::byte> canonical(info.canonical_size);
    std::vector<char> report(info.report_size);
    const bool normalized =
        expect(ctex_project_container_normalize(encoded.data(), encoded.size(), nullptr, &info,
                                                canonical.data(), canonical.size(), report.data(),
                                                report.size()) == CTEX_RESULT_SUCCESS,
               "populated container normalization failed");
    const std::string json(report.data());
    const bool metadata = expect(info.tiled_image_count == 1 && info.occupied_tile_count == 1 &&
                                     info.resource_count == 2 && info.packed_resource_bytes == 3 &&
                                     info.asset_count == 1,
                                 "populated container metrics changed at the C boundary");
    const bool bytes =
        expect(canonical == encoded, "populated container did not normalize byte-identically");
    const bool inventory =
        expect(json.find("\"id\":\"layers/paint/base-color\"") != std::string::npos &&
                   json.find("\"id\":\"mesh/source\"") != std::string::npos &&
                   json.find("\"storage\":\"referenced\"") != std::string::npos &&
                   json.find("\"id\":\"fonts/label\"") != std::string::npos &&
                   json.find("\"storage\":\"packed\"") != std::string::npos &&
                   json.find("\"id\":\"materials/example\"") != std::string::npos,
               "populated container inventory omitted preserved content");
    return normalized && metadata && bytes && inventory ? 0 : 1;
}
