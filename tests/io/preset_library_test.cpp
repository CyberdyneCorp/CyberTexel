#include <algorithm>
#include <array>
#include <ctex/io/preset_library.hpp>
#include <ctex/paint/stroke_preset.hpp>
#include <iostream>
#include <string_view>

namespace {

using namespace ctex;
using namespace ctex::io;

static_assert(paint::current_stroke_preset_schema_version == 2);

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, PresetLibraryErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const PresetLibraryError& error) {
        return expect(error.code() == code, message);
    } catch (...) {
    }
    return expect(false, message);
}

template <typename Callable>
bool expect_error_text(Callable&& callable, PresetLibraryErrorCode code, std::string_view text,
                       std::string_view message) {
    try {
        callable();
    } catch (const PresetLibraryError& error) {
        return expect(error.code() == code &&
                          std::string_view(error.what()).find(text) != std::string_view::npos,
                      message);
    } catch (...) {
    }
    return expect(false, message);
}

StoredTiledImage thumbnail(std::string identifier, std::byte value) {
    const std::array clear{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{255}};
    const std::array pixel{value, value, value, std::byte{255}};
    image::TiledImage source(
        1, 1, {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 4}, 1, clear);
    source.write_pixel(0, 0, pixel);
    return snapshot_tiled_image(std::move(identifier), source);
}

PresetShelf shelf(std::string identifier, std::string display_name, std::size_t first,
                  std::span<const std::string_view> kinds) {
    PresetShelf result{.identifier = std::move(identifier),
                       .display_name = std::move(display_name),
                       .contents = {},
                       .entries = {}};
    for (std::size_t offset = 0; offset < kinds.size(); ++offset) {
        const std::size_t index = first + offset;
        const std::string asset_identifier = "presets/" + std::to_string(index);
        const std::string thumbnail_identifier = "thumbnails/" + std::to_string(index);
        result.contents.assets.push_back(
            {.identifier = asset_identifier,
             .kind = std::string(kinds[offset]),
             .format_version = current_preset_format_version(kinds[offset]),
             .resource_dependencies = {},
             .tiled_image_dependencies = {},
             .payload = {static_cast<std::byte>(index)}});
        result.contents.tiled_images.push_back(
            thumbnail(thumbnail_identifier, static_cast<std::byte>(index + 1)));
        result.entries.push_back({.asset_identifier = asset_identifier,
                                  .display_name = "Preset " + std::to_string(index),
                                  .tags = {"metal", "featured"},
                                  .thumbnail_resource_identifier = thumbnail_identifier});
    }
    std::reverse(result.entries.begin(), result.entries.end());
    return result;
}

PresetLibrary complete_library() {
    constexpr std::array first_kinds{asset_kind::material, asset_kind::smart_material,
                                     asset_kind::smart_mask};
    constexpr std::array second_kinds{asset_kind::brush, asset_kind::stroke_preset,
                                      asset_kind::generator, asset_kind::export_preset};
    return {.shelves = {shelf("studio", "Studio", first_kinds.size(), second_kinds),
                        shelf("builtin", "Built in", 0, first_kinds)}};
}

bool every_kind_enumerates_with_metadata_and_thumbnail() {
    const PresetLibrary library = complete_library();
    const std::vector<PresetShelfListing> shelves = enumerate_preset_shelves(library);
    const std::vector<PresetListing> listings = enumerate_presets(library);
    const std::vector<PresetListing> studio = enumerate_preset_shelf(library, "studio");
    const std::array expected_kinds{asset_kind::material,      asset_kind::smart_material,
                                    asset_kind::smart_mask,    asset_kind::brush,
                                    asset_kind::stroke_preset, asset_kind::generator,
                                    asset_kind::export_preset};
    bool complete =
        shelves ==
            std::vector<PresetShelfListing>{
                {.identifier = "builtin", .display_name = "Built in", .preset_count = 3},
                {.identifier = "studio", .display_name = "Studio", .preset_count = 4}} &&
        listings.size() == expected_kinds.size() && studio.size() == 4;
    for (std::size_t index = 0; complete && index < listings.size(); ++index) {
        const PresetListing& listing = listings[index];
        const image::TiledImage restored = restore_tiled_image(listing.thumbnail);
        complete = listing.identifier == "presets/" + std::to_string(index) &&
                   listing.kind == expected_kinds[index] &&
                   listing.format_version == current_preset_format_version(listing.kind) &&
                   listing.display_name == "Preset " + std::to_string(index) &&
                   listing.tags == std::vector<std::string>{"featured", "metal"} &&
                   listing.thumbnail.resource_id == "thumbnails/" + std::to_string(index) &&
                   restored.read_pixel(0, 0)[0] == static_cast<std::byte>(index + 1);
    }
    return expect(complete,
                  "shelf enumeration omitted a preset kind, metadata field, or thumbnail");
}

bool future_versions_are_named_and_refused_before_resolution() {
    const PresetLibrary baseline = complete_library();
    const std::array kinds{asset_kind::material,      asset_kind::smart_material,
                           asset_kind::smart_mask,    asset_kind::brush,
                           asset_kind::stroke_preset, asset_kind::generator,
                           asset_kind::export_preset};
    bool complete = true;
    for (std::string_view kind : kinds) {
        PresetLibrary future = baseline;
        auto asset = future.shelves.front().contents.assets.end();
        for (PresetShelf& shelf : future.shelves) {
            asset = std::find_if(
                shelf.contents.assets.begin(), shelf.contents.assets.end(),
                [&](const StandaloneAsset& candidate) { return candidate.kind == kind; });
            if (asset != shelf.contents.assets.end()) {
                break;
            }
        }
        const std::string identifier = asset->identifier;
        asset->format_version = current_preset_format_version(kind) + 1;
        ResolvedPreset destination = resolve_preset(baseline, "presets/0");
        const PresetListing before = destination.listing;
        complete = complete &&
                   expect_error_text([&] { destination = resolve_preset(future, identifier); },
                                     PresetLibraryErrorCode::unsupported_version,
                                     std::to_string(asset->format_version),
                                     "future shelf preset version was not refused by name") &&
                   expect(destination.listing == before,
                          "future-version refusal partially replaced the destination preset");
    }
    return complete;
}

bool stable_identity_resolves_to_package() {
    const PresetLibrary library = complete_library();
    const ResolvedPreset resolved = resolve_preset(library, "presets/5");
    return expect(
               resolved.listing.shelf_identifier == "studio" &&
                   resolved.listing.kind == asset_kind::generator &&
                   resolved.package.assets.size() == 1 &&
                   resolved.package.assets.front().identifier == "presets/5" &&
                   resolved.package.assets.front().payload == std::vector<std::byte>{std::byte{5}},
               "stable preset identity did not resolve to its standalone package") &&
           expect_error([&] { static_cast<void>(resolve_preset(library, "missing")); },
                        PresetLibraryErrorCode::unknown_preset,
                        "unknown preset identity was not reported") &&
           expect_error([&] { static_cast<void>(enumerate_preset_shelf(library, "missing")); },
                        PresetLibraryErrorCode::unknown_shelf,
                        "unknown shelf identity was not reported");
}

bool ambiguous_or_incomplete_shelves_are_refused() {
    PresetLibrary duplicate = complete_library();
    duplicate.shelves[0].contents.assets.front().identifier = "presets/0";
    duplicate.shelves[0].entries.back().asset_identifier = "presets/0";
    const bool duplicate_refused = expect_error([&] { validate_preset_library(duplicate); },
                                                PresetLibraryErrorCode::invalid_entry,
                                                "duplicate global preset identity was accepted");

    PresetLibrary missing_thumbnail = complete_library();
    missing_thumbnail.shelves.front().entries.front().thumbnail_resource_identifier = "missing";
    const bool thumbnail_refused = expect_error([&] { validate_preset_library(missing_thumbnail); },
                                                PresetLibraryErrorCode::invalid_entry,
                                                "missing preset thumbnail was accepted");

    PresetLibrary unsupported = complete_library();
    unsupported.shelves.front().contents.assets.front().kind = std::string(asset_kind::node_group);
    return duplicate_refused && thumbnail_refused &&
           expect_error([&] { validate_preset_library(unsupported); },
                        PresetLibraryErrorCode::invalid_entry,
                        "unsupported shelf preset kind was accepted");
}

}  // namespace

int main() {
    return every_kind_enumerates_with_metadata_and_thumbnail() &&
                   stable_identity_resolves_to_package() &&
                   ambiguous_or_incomplete_shelves_are_refused() &&
                   future_versions_are_named_and_refused_before_resolution()
               ? 0
               : 1;
}
