#include <ctex/capi.h>

#include <array>
#include <cstddef>
#include <ctex/image/tiled_image.hpp>
#include <ctex/io/project_container.hpp>
#include <filesystem>
#include <fstream>
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
                                .resource_dependencies = {"mesh/source", "fonts/label"},
                                .tiled_image_dependencies = {"layers/paint/base-color"},
                                .payload = {std::byte{'m'}, std::byte{'a'}, std::byte{'t'}}});
    container.resources.push_back({.identifier = "unused/resource",
                                   .kind = "data",
                                   .relative_path = "unused.bin",
                                   .packed_bytes = std::vector<std::byte>{std::byte{42}}});
    container.assets.push_back({.identifier = "materials/unused",
                                .kind = "material",
                                .format_version = 1,
                                .resource_dependencies = {},
                                .tiled_image_dependencies = {},
                                .payload = {std::byte{'x'}}});
    return container;
}

struct EncodedResult {
    EncodedResult() { info.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE; }

    ctex_project_container_info info{};
    std::vector<std::byte> bytes;
    std::string report;
};

bool export_asset(const std::vector<std::byte>& project, const char* identifier,
                  const ctex_project_asset_export_options_descriptor* options,
                  EncodedResult& result) {
    if (!expect(
            ctex_project_asset_export(project.data(), project.size(), nullptr, identifier, options,
                                      &result.info, nullptr, 0, nullptr, 0) == CTEX_RESULT_SUCCESS,
            "standalone asset export sizing failed")) {
        return false;
    }
    result.bytes.assign(result.info.canonical_size, std::byte{0xa5});
    std::vector<std::byte> untouched = result.bytes;
    char short_report = 'x';
    if (!expect(
            ctex_project_asset_export(project.data(), project.size(), nullptr, identifier, options,
                                      &result.info, result.bytes.data(), result.bytes.size(),
                                      &short_report, 1) == CTEX_RESULT_BUFFER_TOO_SMALL,
            "standalone asset export accepted a short report") ||
        !expect(result.bytes == untouched && short_report == 'x',
                "standalone asset export wrote a partial result")) {
        return false;
    }
    std::vector<char> report(result.info.report_size);
    if (!expect(
            ctex_project_asset_export(project.data(), project.size(), nullptr, identifier, options,
                                      &result.info, result.bytes.data(), result.bytes.size(),
                                      report.data(), report.size()) == CTEX_RESULT_SUCCESS,
            "standalone asset export failed")) {
        return false;
    }
    result.report = report.data();
    return true;
}

bool install_asset(const std::vector<std::byte>& library, const std::vector<std::byte>& asset,
                   const ctex_project_asset_search_paths_descriptor* search_paths,
                   EncodedResult& result) {
    if (!expect(ctex_project_asset_install(library.data(), library.size(), asset.data(),
                                           asset.size(), nullptr, search_paths, &result.info,
                                           nullptr, 0, nullptr, 0) == CTEX_RESULT_SUCCESS,
                "standalone asset install sizing failed")) {
        return false;
    }
    result.bytes.resize(result.info.canonical_size);
    std::vector<char> report(result.info.report_size);
    if (!expect(ctex_project_asset_install(library.data(), library.size(), asset.data(),
                                           asset.size(), nullptr, search_paths, &result.info,
                                           result.bytes.data(), result.bytes.size(), report.data(),
                                           report.size()) == CTEX_RESULT_SUCCESS,
                "standalone asset install failed")) {
        return false;
    }
    result.report = report.data();
    return true;
}

bool self_contained_round_trip(const std::vector<std::byte>& project,
                               const std::filesystem::path& source_directory) {
    const std::string source = source_directory.string();
    const ctex_project_asset_export_options_descriptor options{
        .size = CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE,
        .self_contained = 1,
        .source_directory = source.c_str(),
    };
    EncodedResult exported;
    if (!export_asset(project, "materials/example", &options, exported)) {
        return false;
    }
    const ctex::io::ProjectContainerReadResult package =
        ctex::io::read_project_container(exported.bytes);
    const bool package_is_exact =
        expect(package.container.assets.size() == 1 && package.container.resources.size() == 2 &&
                   package.container.tiled_images.size() == 1,
               "standalone export did not select exact dependencies") &&
        expect(package.container.resources[0].packed_bytes.has_value() &&
                   package.container.resources[0].packed_bytes->size() == 4 &&
                   package.container.resources[1].packed_bytes.has_value(),
               "self-contained export did not pack every resource") &&
        expect(exported.info.asset_count == 1 && exported.info.resource_count == 2 &&
                   exported.info.tiled_image_count == 1 &&
                   exported.report.find("\"id\":\"materials/example\"") != std::string::npos,
               "standalone export metadata is incomplete");

    const std::vector<std::byte> empty =
        ctex::io::write_project_container(ctex::io::ProjectContainer{});
    EncodedResult installed;
    if (!package_is_exact || !install_asset(empty, exported.bytes, nullptr, installed)) {
        return false;
    }
    const ctex::io::ProjectContainerReadResult library =
        ctex::io::read_project_container(installed.bytes);
    std::vector<std::byte> untouched(installed.info.canonical_size, std::byte{0xa5});
    const ctex_result duplicate =
        ctex_project_asset_install(installed.bytes.data(), installed.bytes.size(),
                                   exported.bytes.data(), exported.bytes.size(), nullptr, nullptr,
                                   &installed.info, untouched.data(), untouched.size(), nullptr, 0);
    return expect(library.container.assets.size() == 1 && library.container.resources.size() == 2 &&
                      library.container.tiled_images.size() == 1,
                  "standalone asset install omitted package content") &&
           expect(duplicate == CTEX_RESULT_INVALID_ARGUMENT &&
                      ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PROJECT_CONTAINER,
                  "duplicate standalone asset install was accepted") &&
           expect(untouched.front() == std::byte{0xa5} && untouched.back() == std::byte{0xa5},
                  "failed standalone asset install changed caller output");
}

bool referenced_round_trip(const std::vector<std::byte>& project,
                           const std::filesystem::path& source_directory) {
    const ctex_project_asset_export_options_descriptor options{
        .size = CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE,
        .self_contained = 0,
        .source_directory = nullptr,
    };
    EncodedResult exported;
    if (!export_asset(project, "materials/example", &options, exported)) {
        return false;
    }
    const ctex::io::ProjectContainerReadResult package =
        ctex::io::read_project_container(exported.bytes);
    const std::string search_path = source_directory.string();
    const char* paths[] = {search_path.c_str()};
    const ctex_project_asset_search_paths_descriptor search_paths{
        .size = CTEX_PROJECT_ASSET_SEARCH_PATHS_DESCRIPTOR_CURRENT_SIZE,
        .paths = paths,
        .path_count = 1,
    };
    const std::vector<std::byte> empty =
        ctex::io::write_project_container(ctex::io::ProjectContainer{});
    EncodedResult installed;
    if (!expect(!package.container.resources[0].packed_bytes.has_value(),
                "referenced export unexpectedly packed its external resource") ||
        !install_asset(empty, exported.bytes, &search_paths, installed)) {
        return false;
    }
    const ctex::io::ProjectContainerReadResult library =
        ctex::io::read_project_container(installed.bytes);
    return expect(library.container.resources[0].packed_bytes.has_value() &&
                      library.container.resources[0].packed_bytes->size() == 4,
                  "search-path install did not resolve and pack the external resource");
}

bool invalid_descriptors_are_rejected(const std::vector<std::byte>& project) {
    ctex_project_container_info info{};
    info.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE;
    ctex_project_asset_export_options_descriptor options{
        .size = CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE + 1,
        .self_contained = 0,
        .source_directory = nullptr,
    };
    const ctex_result invalid_options =
        ctex_project_asset_export(project.data(), project.size(), nullptr, "materials/example",
                                  &options, &info, nullptr, 0, nullptr, 0);
    const ctex_project_asset_search_paths_descriptor paths{
        .size = CTEX_PROJECT_ASSET_SEARCH_PATHS_DESCRIPTOR_CURRENT_SIZE,
        .paths = nullptr,
        .path_count = 1,
    };
    const std::vector<std::byte> empty =
        ctex::io::write_project_container(ctex::io::ProjectContainer{});
    EncodedResult exported;
    if (!expect(invalid_options == CTEX_RESULT_INVALID_ARGUMENT &&
                    ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_DESCRIPTOR_SIZE,
                "future standalone export options layout was accepted") ||
        !export_asset(project, "materials/example", nullptr, exported)) {
        return false;
    }
    return expect(ctex_project_asset_install(empty.data(), empty.size(), exported.bytes.data(),
                                             exported.bytes.size(), nullptr, &paths, &info, nullptr,
                                             0, nullptr, 0) == CTEX_RESULT_INVALID_ARGUMENT,
                  "null standalone asset search-path array was accepted");
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
                                     info.resource_count == 3 && info.packed_resource_bytes == 4 &&
                                     info.asset_count == 2,
                                 "populated container metrics changed at the C boundary");
    const bool bytes =
        expect(canonical == encoded, "populated container did not normalize byte-identically");
    const bool inventory =
        expect(json.find("\"id\":\"layers/paint/base-color\"") != std::string::npos &&
                   json.find("\"id\":\"mesh/source\"") != std::string::npos &&
                   json.find("\"storage\":\"referenced\"") != std::string::npos &&
                   json.find("\"id\":\"fonts/label\"") != std::string::npos &&
                   json.find("\"storage\":\"packed\"") != std::string::npos &&
                   json.find("\"id\":\"materials/example\"") != std::string::npos &&
                   json.find("\"id\":\"materials/unused\"") != std::string::npos,
               "populated container inventory omitted preserved content");
    const std::filesystem::path source_directory = "ctex-capi-project-assets-test";
    std::error_code ignored;
    std::filesystem::remove_all(source_directory, ignored);
    std::filesystem::create_directories(source_directory / "meshes");
    {
        std::ofstream mesh(source_directory / "meshes/source.glb", std::ios::binary);
        mesh.write("glb!", 4);
    }
    const bool assets = self_contained_round_trip(encoded, source_directory) &&
                        referenced_round_trip(encoded, source_directory) &&
                        invalid_descriptors_are_rejected(encoded);
    std::filesystem::remove_all(source_directory, ignored);
    return normalized && metadata && bytes && inventory && assets ? 0 : 1;
}
