#include <array>
#include <cstddef>
#include <ctex/io/standalone_asset.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::io;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, ProjectContainerErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const ProjectContainerError& error) {
        return expect(error.code() == code, "standalone asset returned the wrong error code");
    } catch (...) {
    }
    return expect(false, message);
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return {};
    }
    const std::streampos end = stream.tellg();
    std::vector<std::byte> bytes(static_cast<std::size_t>(end));
    stream.seekg(0);
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return stream ? bytes : std::vector<std::byte>{};
}

StoredTiledImage make_tiled_dependency() {
    const std::array clear{std::byte{0}};
    const std::array painted{std::byte{77}};
    image::TiledImage image(
        2, 1, {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1}, 2, clear);
    image.write_pixel(1, 0, painted);
    return snapshot_tiled_image("preview/material", image);
}

ProjectContainer make_library() {
    ProjectContainer source;
    source.resources = {
        {.identifier = "texture/noise",
         .kind = "image",
         .relative_path = "textures/noise.bin",
         .packed_bytes = std::nullopt},
        {.identifier = "font/labels",
         .kind = "font",
         .relative_path = "fonts/labels.bin",
         .packed_bytes = std::vector<std::byte>{std::byte{'f'}, std::byte{'o'}, std::byte{'n'},
                                                std::byte{'t'}}},
        {.identifier = "unrelated",
         .kind = "image",
         .relative_path = "unrelated.bin",
         .packed_bytes = std::vector<std::byte>{std::byte{'x'}}},
    };
    source.tiled_images.push_back(make_tiled_dependency());
    source.assets = {
        {.identifier = "materials/rusted-steel",
         .kind = std::string(asset_kind::material),
         .format_version = 3,
         .resource_dependencies = {"texture/noise", "font/labels"},
         .tiled_image_dependencies = {"preview/material"},
         .payload = {std::byte{'m'}, std::byte{'a'}, std::byte{'t'}}},
        {.identifier = "brushes/unrelated",
         .kind = std::string(asset_kind::brush),
         .format_version = 1,
         .resource_dependencies = {},
         .tiled_image_dependencies = {},
         .payload = {std::byte{'b'}}},
    };
    source.opaque_sections.push_back({.kind = 0x80000002U,
                                      .version = 7,
                                      .payload = {std::byte{'f'}, std::byte{'u'}, std::byte{'t'},
                                                  std::byte{'u'}, std::byte{'r'}, std::byte{'e'}}});
    return source;
}

bool every_supported_asset_kind_round_trips() {
    const std::array kinds{asset_kind::material,      asset_kind::smart_material,
                           asset_kind::smart_mask,    asset_kind::brush,
                           asset_kind::stroke_preset, asset_kind::export_preset,
                           asset_kind::node_group};
    ProjectContainer container;
    for (std::size_t index = 0; index < kinds.size(); ++index) {
        container.assets.push_back({.identifier = "asset/" + std::to_string(index),
                                    .kind = std::string(kinds[index]),
                                    .format_version = static_cast<std::uint32_t>(index + 1),
                                    .resource_dependencies = {},
                                    .tiled_image_dependencies = {},
                                    .payload = {static_cast<std::byte>(index)}});
    }
    const ProjectContainerReadResult opened =
        read_project_container(write_project_container(container));
    return expect(
        opened.container.assets == container.assets && opened.report.unknown_parts.empty(),
        "supported standalone asset kinds did not round-trip");
}

bool asset_read_limits_are_enforced() {
    ProjectContainer container;
    container.resources.push_back({.identifier = "resource",
                                   .kind = "image",
                                   .relative_path = "resource.bin",
                                   .packed_bytes = std::vector<std::byte>{std::byte{0}}});
    container.assets.push_back({.identifier = "limited",
                                .kind = std::string(asset_kind::material),
                                .format_version = 1,
                                .resource_dependencies = {"resource"},
                                .tiled_image_dependencies = {},
                                .payload = {std::byte{1}}});
    const std::vector<std::byte> encoded = write_project_container(container);
    ProjectContainerReadLimits asset_limit;
    asset_limit.maximum_assets = 0;
    ProjectContainerReadLimits dependency_limit;
    dependency_limit.maximum_asset_dependencies = 0;
    ProjectContainerReadLimits payload_limit;
    payload_limit.maximum_asset_payload_bytes = 0;
    return expect_error([&] { static_cast<void>(read_project_container(encoded, asset_limit)); },
                        ProjectContainerErrorCode::over_limit,
                        "standalone asset count limit was ignored") &&
           expect_error(
               [&] { static_cast<void>(read_project_container(encoded, dependency_limit)); },
               ProjectContainerErrorCode::over_limit,
               "standalone asset dependency limit was ignored") &&
           expect_error([&] { static_cast<void>(read_project_container(encoded, payload_limit)); },
                        ProjectContainerErrorCode::over_limit,
                        "standalone asset payload limit was ignored");
}

bool referenced_and_self_contained_packages_import() {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "ctex-standalone-asset-test";
    std::error_code filesystem_error;
    std::filesystem::remove_all(directory, filesystem_error);
    std::filesystem::create_directories(directory / "textures", filesystem_error);
    if (!expect(!filesystem_error, "could not create standalone asset test directory")) {
        return false;
    }
    const std::array external_bytes{std::byte{'n'}, std::byte{'o'}, std::byte{'i'}, std::byte{'s'},
                                    std::byte{'e'}};
    {
        std::ofstream stream(directory / "textures/noise.bin", std::ios::binary);
        stream.write(reinterpret_cast<const char*>(external_bytes.data()),
                     static_cast<std::streamsize>(external_bytes.size()));
    }
    const ProjectContainer source = make_library();
    const ProjectContainer referenced = package_standalone_asset(
        source, "materials/rusted-steel", {.self_contained = false, .source_directory = directory});
    const ProjectContainer packed = package_standalone_asset(
        source, "materials/rusted-steel", {.self_contained = true, .source_directory = directory});
    const std::filesystem::path referenced_path = directory / "referenced.ctexasset";
    const std::filesystem::path packed_path = directory / "packed.ctexasset";
    save_project_container_atomic(referenced_path, referenced);
    save_standalone_asset_atomic(packed_path, source, "materials/rusted-steel",
                                 {.self_contained = true, .source_directory = directory});

    const StandaloneAssetImportResult referenced_open =
        import_standalone_asset(read_binary_file(referenced_path), directory);
    StandaloneAssetImportResult install_open =
        import_standalone_asset(read_binary_file(referenced_path), directory);
    ProjectContainer library;
    library.resources.push_back(packed.resources[1]);
    library.tiled_images.push_back(packed.tiled_images.front());
    const StandaloneAssetInstallReport installed =
        install_standalone_asset(library, std::move(install_open));
    const std::size_t installed_asset_count = library.assets.size();
    const bool duplicate_install = expect_error(
        [&] {
            static_cast<void>(install_standalone_asset(
                library, import_standalone_asset(read_binary_file(packed_path), directory)));
        },
        ProjectContainerErrorCode::invalid_asset,
        "standalone asset installation accepted a duplicate asset identity");
    std::filesystem::remove(directory / "textures/noise.bin", filesystem_error);
    const StandaloneAssetImportResult referenced_missing =
        import_standalone_asset(read_binary_file(referenced_path), directory);
    ProjectContainer missing_library;
    const bool missing_install = expect_error(
        [&] {
            static_cast<void>(install_standalone_asset(
                missing_library,
                import_standalone_asset(read_binary_file(referenced_path), directory)));
        },
        ProjectContainerErrorCode::invalid_resource,
        "standalone asset installation accepted a missing resource");
    const StandaloneAssetImportResult packed_open =
        import_standalone_asset(read_binary_file(packed_path), directory);
    const image::TiledImage preview = restore_tiled_image(packed_open.package.tiled_images.front());
    std::filesystem::remove_all(directory, filesystem_error);

    return expect(referenced.assets.size() == 1 && referenced.resources.size() == 2 &&
                      referenced.tiled_images.size() == 1 &&
                      referenced.opaque_sections == source.opaque_sections &&
                      !referenced.resources.front().packed_bytes.has_value(),
                  "referenced export did not contain exactly the asset dependencies") &&
           expect(packed.assets == referenced.assets && packed.resources.size() == 2 &&
                      packed.resources[0].packed_bytes ==
                          std::optional<std::vector<std::byte>>(std::vector<std::byte>(
                              external_bytes.begin(), external_bytes.end())) &&
                      packed.resources[1].packed_bytes.has_value(),
                  "self-contained export did not pack every external dependency") &&
           expect(!referenced_open.self_contained && referenced_open.resources.complete() &&
                      referenced_open.read_report.unknown_parts.size() == 1 &&
                      referenced_open.resources.resources[0].status ==
                          ProjectResourceStatus::referenced,
                  "referenced standalone asset did not resolve beside its package") &&
           expect(
               installed.asset_identifier == "materials/rusted-steel" &&
                   installed.added_resources == std::vector<std::string>{"texture/noise"} &&
                   installed.reused_resources == std::vector<std::string>{"font/labels"} &&
                   installed.added_tiled_images.empty() &&
                   installed.reused_tiled_images == std::vector<std::string>{"preview/material"} &&
                   installed_asset_count == 1 && duplicate_install &&
                   library.resources.size() == 2 &&
                   library.resources.back().identifier == "texture/noise" &&
                   library.resources.back().packed_bytes.has_value(),
               "standalone asset was not atomically installed into the library") &&
           expect(!referenced_missing.resources.complete() &&
                      referenced_missing.resources.missing_identifiers ==
                          std::vector<std::string>{"texture/noise"} &&
                      missing_install && missing_library.assets.empty() &&
                      missing_library.resources.empty(),
                  "missing standalone asset dependency was not reported") &&
           expect(packed_open.self_contained && packed_open.resources.complete() &&
                      preview.read_pixel(1, 0)[0] == std::byte{77},
                  "self-contained standalone asset did not import without external files");
}

bool invalid_dependencies_are_refused_before_replacement() {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "ctex-standalone-asset-invalid-test";
    std::error_code filesystem_error;
    std::filesystem::remove_all(directory, filesystem_error);
    std::filesystem::create_directories(directory / "textures", filesystem_error);
    {
        std::ofstream stream(directory / "textures/noise.bin", std::ios::binary);
        stream << "noise";
    }
    ProjectContainer source = make_library();
    const std::filesystem::path target = directory / "asset.ctexasset";
    save_standalone_asset_atomic(target, source, "materials/rusted-steel",
                                 {.self_contained = true, .source_directory = directory});
    const std::vector<std::byte> before = read_binary_file(target);
    std::filesystem::remove(directory / "textures/noise.bin", filesystem_error);
    const bool missing_external = expect_error(
        [&] {
            save_standalone_asset_atomic(target, source, "materials/rusted-steel",
                                         {.self_contained = true, .source_directory = directory});
        },
        ProjectContainerErrorCode::invalid_resource,
        "self-contained export accepted a missing external resource");
    const bool preserved = read_binary_file(target) == before;

    source.assets.front().resource_dependencies.push_back("absent");
    const bool undeclared = expect_error(
        [&] {
            static_cast<void>(
                package_standalone_asset(source, "materials/rusted-steel",
                                         {.self_contained = false, .source_directory = directory}));
        },
        ProjectContainerErrorCode::invalid_asset,
        "standalone export accepted an absent declared dependency");

    ProjectContainer multi = make_library();
    const bool multiple_assets = expect_error(
        [&] {
            static_cast<void>(import_standalone_asset(write_project_container(multi), directory));
        },
        ProjectContainerErrorCode::invalid_asset,
        "standalone import accepted a package with several assets");
    std::filesystem::remove_all(directory, filesystem_error);
    return expect(missing_external && preserved,
                  "failed self-contained export replaced the prior asset file") &&
           undeclared && multiple_assets;
}

}  // namespace

int main() {
    return every_supported_asset_kind_round_trips() && asset_read_limits_are_enforced() &&
                   referenced_and_self_contained_packages_import() &&
                   invalid_dependencies_are_refused_before_replacement()
               ? 0
               : 1;
}
