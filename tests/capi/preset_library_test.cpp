#include <ctex/capi.h>

#include <array>
#include <ctex/io/preset_library.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ctex::io::StoredTiledImage thumbnail(std::string identifier, std::byte value) {
    const std::array clear{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{255}};
    const std::array pixel{value, value, value, std::byte{255}};
    ctex::image::TiledImage source(
        1, 1, {.channel_type = ctex::image::ChannelType::uint8_unorm, .channel_count = 4}, 1,
        clear);
    source.write_pixel(0, 0, pixel);
    return ctex::io::snapshot_tiled_image(std::move(identifier), source);
}

struct LibraryFixture {
    static constexpr std::size_t preset_count = 7;

    LibraryFixture() {
        constexpr std::array kinds{
            ctex::io::asset_kind::material,      ctex::io::asset_kind::smart_material,
            ctex::io::asset_kind::smart_mask,    ctex::io::asset_kind::brush,
            ctex::io::asset_kind::stroke_preset, ctex::io::asset_kind::generator,
            ctex::io::asset_kind::export_preset};
        for (std::size_t index = 0; index < preset_count; ++index) {
            asset_ids[index] = "presets/" + std::to_string(index);
            display_names[index] = "Preset " + std::to_string(index);
            thumbnail_ids[index] = "thumbnails/" + std::to_string(index);
            container.assets.push_back(
                {.identifier = asset_ids[index],
                 .kind = std::string(kinds[index]),
                 .format_version = ctex::io::current_preset_format_version(kinds[index]),
                 .resource_dependencies = {},
                 .tiled_image_dependencies = {},
                 .payload = {static_cast<std::byte>(index)}});
            container.tiled_images.push_back(
                thumbnail(thumbnail_ids[index], static_cast<std::byte>(index + 1)));
            entries[index] = {
                .size = CTEX_PRESET_SHELF_ENTRY_DESCRIPTOR_CURRENT_SIZE,
                .asset_identifier = asset_ids[index].c_str(),
                .display_name = display_names[index].c_str(),
                .tags = tags.data(),
                .tag_count = tags.size(),
                .thumbnail_resource_identifier = thumbnail_ids[index].c_str(),
            };
        }
        encode();
    }

    void encode() {
        bytes = ctex::io::write_project_container(container);
        shelf = {
            .size = CTEX_PRESET_SHELF_DESCRIPTOR_CURRENT_SIZE,
            .identifier = "builtin",
            .display_name = "Built in",
            .contents = bytes.data(),
            .contents_size = bytes.size(),
            .entries = entries.data(),
            .entry_count = entries.size(),
        };
        library = {
            .size = CTEX_PRESET_LIBRARY_DESCRIPTOR_CURRENT_SIZE,
            .shelves = &shelf,
            .shelf_count = 1,
            .read_limits = nullptr,
        };
    }

    ctex::io::ProjectContainer container;
    std::array<std::string, preset_count> asset_ids;
    std::array<std::string, preset_count> display_names;
    std::array<std::string, preset_count> thumbnail_ids;
    std::array<const char*, 2> tags{"featured", "metal"};
    std::array<ctex_preset_shelf_entry_descriptor, preset_count> entries{};
    std::vector<std::byte> bytes;
    ctex_preset_shelf_descriptor shelf{};
    ctex_preset_library_descriptor library{};
};

bool every_kind_enumerates_and_resolves() {
    LibraryFixture fixture;
    ctex_preset_library_info info{};
    info.size = CTEX_PRESET_LIBRARY_INFO_CURRENT_SIZE;
    if (!expect(ctex_preset_library_enumerate(&fixture.library, &info, nullptr, 0) ==
                        CTEX_RESULT_SUCCESS &&
                    info.shelf_count == 1 && info.preset_count == LibraryFixture::preset_count,
                "preset library sizing or counts failed")) {
        return false;
    }
    std::vector<char> report(info.report_size);
    if (!expect(ctex_preset_library_enumerate(&fixture.library, &info, report.data(),
                                              report.size()) == CTEX_RESULT_SUCCESS,
                "preset library enumeration failed")) {
        return false;
    }
    const std::string listing = report.data();
    bool passed =
        expect(listing.find("\"id\":\"builtin\"") != std::string::npos &&
                   listing.find("\"kind\":\"smart-material\"") != std::string::npos &&
                   listing.find("\"kind\":\"export-preset\"") != std::string::npos &&
                   listing.find("\"tags\":[\"featured\",\"metal\"]") != std::string::npos &&
                   listing.find("\"id\":\"thumbnails/6\"") != std::string::npos,
               "preset enumeration omitted shelf, kind, tags, or thumbnail metadata");

    ctex_project_container_info package_info{};
    package_info.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE;
    passed = expect(ctex_preset_library_resolve(&fixture.library, "presets/5", &package_info,
                                                nullptr, 0, nullptr, 0) == CTEX_RESULT_SUCCESS &&
                        package_info.asset_count == 1 && package_info.tiled_image_count == 1,
                    "preset resolution sizing omitted its asset or thumbnail") &&
             passed;
    std::vector<std::byte> package(package_info.canonical_size);
    std::vector<char> package_report(package_info.report_size);
    passed =
        expect(ctex_preset_library_resolve(&fixture.library, "presets/5", &package_info,
                                           package.data(), package.size(), package_report.data(),
                                           package_report.size()) == CTEX_RESULT_SUCCESS,
               "preset resolution failed") &&
        passed;
    if (passed) {
        const ctex::io::ProjectContainer resolved =
            ctex::io::read_project_container(package).container;
        passed = expect(resolved.assets.front().identifier == "presets/5" &&
                            resolved.assets.front().kind == ctex::io::asset_kind::generator &&
                            resolved.tiled_images.front().resource_id == "thumbnails/5",
                        "stable identity did not resolve the preset with its thumbnail");
    }
    return passed;
}

bool future_versions_are_refused_by_name() {
    LibraryFixture fixture;
    fixture.container.assets.front().format_version += 1;
    fixture.encode();
    ctex_preset_library_info info{};
    info.size = CTEX_PRESET_LIBRARY_INFO_CURRENT_SIZE;
    return expect(ctex_preset_library_enumerate(&fixture.library, &info, nullptr, 0) ==
                      CTEX_RESULT_UNSUPPORTED_OPERATION,
                  "future preset version was accepted") &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_PRESET_LIBRARY &&
                      std::string_view(ctex_get_last_diagnostic()).find("presets/0") !=
                          std::string_view::npos,
                  "future preset refusal did not name the preset");
}

}  // namespace

int main() {
    return every_kind_enumerates_and_resolves() && future_versions_are_refused_by_name() ? 0 : 1;
}
