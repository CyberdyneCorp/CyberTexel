#include <algorithm>
#include <array>
#include <ctex/doc/smart_mask.hpp>
#include <ctex/io/preset_library.hpp>
#include <iterator>
#include <map>
#include <set>
#include <tuple>
#include <utility>

namespace ctex::io {
namespace {

constexpr std::uint32_t current_stroke_format_version = 2;

[[noreturn]] void shelf_error(PresetLibraryErrorCode code, std::string message) {
    throw PresetLibraryError(code, std::move(message));
}

bool supported_kind(std::string_view kind) {
    constexpr std::array kinds{asset_kind::material,      asset_kind::smart_material,
                               asset_kind::smart_mask,    asset_kind::brush,
                               asset_kind::stroke_preset, asset_kind::generator,
                               asset_kind::export_preset};
    return std::find(kinds.begin(), kinds.end(), kind) != kinds.end();
}

std::uint32_t format_version_for_kind(std::string_view kind) {
    constexpr std::array versions{
        std::pair{asset_kind::material, std::uint32_t{1}},
        std::pair{asset_kind::smart_material, doc::current_smart_material_schema_version},
        std::pair{asset_kind::smart_mask, doc::current_smart_mask_schema_version},
        std::pair{asset_kind::brush, current_stroke_format_version},
        std::pair{asset_kind::stroke_preset, current_stroke_format_version},
        std::pair{asset_kind::generator, std::uint32_t{1}},
        std::pair{asset_kind::export_preset, std::uint32_t{1}},
    };
    const auto found = std::find_if(versions.begin(), versions.end(),
                                    [&](const auto& item) { return item.first == kind; });
    if (found == versions.end()) {
        shelf_error(PresetLibraryErrorCode::invalid_entry,
                    "unsupported shelf preset kind '" + std::string(kind) + "'");
    }
    return found->second;
}

const StandaloneAsset& find_asset(const PresetShelf& shelf, std::string_view identifier) {
    const auto found =
        std::find_if(shelf.contents.assets.begin(), shelf.contents.assets.end(),
                     [&](const StandaloneAsset& asset) { return asset.identifier == identifier; });
    if (found == shelf.contents.assets.end()) {
        shelf_error(PresetLibraryErrorCode::invalid_entry,
                    "shelf metadata names an absent preset '" + std::string(identifier) + "'");
    }
    return *found;
}

const StoredTiledImage& find_thumbnail(const PresetShelf& shelf, std::string_view identifier) {
    const auto found = std::find_if(
        shelf.contents.tiled_images.begin(), shelf.contents.tiled_images.end(),
        [&](const StoredTiledImage& image) { return image.resource_id == identifier; });
    if (found == shelf.contents.tiled_images.end()) {
        shelf_error(PresetLibraryErrorCode::invalid_entry,
                    "shelf preset thumbnail is absent: " + std::string(identifier));
    }
    return *found;
}

void validate_tags(const PresetShelfEntry& entry) {
    std::set<std::string_view, std::less<>> tags;
    for (const std::string& tag : entry.tags) {
        if (tag.empty() || !tags.insert(tag).second) {
            shelf_error(PresetLibraryErrorCode::invalid_entry,
                        "shelf preset tags must be non-empty and unique");
        }
    }
}

void validate_shelf(const PresetShelf& shelf, std::set<std::string_view, std::less<>>& preset_ids) {
    if (shelf.identifier.empty() || shelf.display_name.empty()) {
        shelf_error(PresetLibraryErrorCode::invalid_shelf,
                    "preset shelf requires an identity and display name");
    }
    try {
        static_cast<void>(write_project_container(shelf.contents));
    } catch (const ProjectContainerError& error) {
        shelf_error(PresetLibraryErrorCode::invalid_shelf,
                    "preset shelf contains an invalid package: " + std::string(error.what()));
    }
    if (shelf.entries.size() != shelf.contents.assets.size()) {
        shelf_error(PresetLibraryErrorCode::invalid_shelf,
                    "preset shelf requires exactly one metadata entry per asset");
    }
    std::set<std::string_view, std::less<>> shelf_entries;
    for (const PresetShelfEntry& entry : shelf.entries) {
        const StandaloneAsset& asset = find_asset(shelf, entry.asset_identifier);
        if (entry.display_name.empty() || entry.thumbnail_resource_identifier.empty() ||
            !supported_kind(asset.kind) || !shelf_entries.insert(entry.asset_identifier).second ||
            !preset_ids.insert(entry.asset_identifier).second) {
            shelf_error(PresetLibraryErrorCode::invalid_entry,
                        "shelf preset metadata or globally stable identity is invalid");
        }
        const std::uint32_t current_version = format_version_for_kind(asset.kind);
        if (asset.format_version > current_version) {
            shelf_error(PresetLibraryErrorCode::unsupported_version,
                        "preset '" + asset.identifier + "' of kind '" + asset.kind +
                            "' declares unsupported future format version " +
                            std::to_string(asset.format_version) + " (current " +
                            std::to_string(current_version) + ")");
        }
        validate_tags(entry);
        static_cast<void>(find_thumbnail(shelf, entry.thumbnail_resource_identifier));
    }
}

PresetListing listing(const PresetShelf& shelf, const PresetShelfEntry& entry) {
    const StandaloneAsset& asset = find_asset(shelf, entry.asset_identifier);
    std::vector<std::string> tags = entry.tags;
    std::sort(tags.begin(), tags.end());
    return {.shelf_identifier = shelf.identifier,
            .identifier = asset.identifier,
            .kind = asset.kind,
            .format_version = asset.format_version,
            .display_name = entry.display_name,
            .tags = std::move(tags),
            .thumbnail = find_thumbnail(shelf, entry.thumbnail_resource_identifier)};
}

std::vector<PresetListing> enumerate_validated_shelf(const PresetShelf& shelf) {
    std::vector<PresetListing> result;
    result.reserve(shelf.entries.size());
    for (const PresetShelfEntry& entry : shelf.entries) {
        result.push_back(listing(shelf, entry));
    }
    std::sort(result.begin(), result.end(),
              [](const PresetListing& left, const PresetListing& right) {
                  return left.identifier < right.identifier;
              });
    return result;
}

}  // namespace

PresetLibraryError::PresetLibraryError(PresetLibraryErrorCode code, std::string message)
    : std::invalid_argument(std::move(message)), code_(code) {}

std::uint32_t current_preset_format_version(std::string_view kind) {
    return format_version_for_kind(kind);
}

void validate_preset_library(const PresetLibrary& library) {
    std::set<std::string_view, std::less<>> shelf_ids;
    std::set<std::string_view, std::less<>> preset_ids;
    for (const PresetShelf& shelf : library.shelves) {
        if (!shelf_ids.insert(shelf.identifier).second) {
            shelf_error(PresetLibraryErrorCode::invalid_shelf,
                        "preset library repeats shelf identity '" + shelf.identifier + "'");
        }
        validate_shelf(shelf, preset_ids);
    }
}

std::vector<PresetShelfListing> enumerate_preset_shelves(const PresetLibrary& library) {
    validate_preset_library(library);
    std::vector<PresetShelfListing> result;
    result.reserve(library.shelves.size());
    for (const PresetShelf& shelf : library.shelves) {
        result.push_back({.identifier = shelf.identifier,
                          .display_name = shelf.display_name,
                          .preset_count = shelf.entries.size()});
    }
    std::sort(result.begin(), result.end(),
              [](const PresetShelfListing& left, const PresetShelfListing& right) {
                  return left.identifier < right.identifier;
              });
    return result;
}

std::vector<PresetListing> enumerate_presets(const PresetLibrary& library) {
    validate_preset_library(library);
    std::vector<PresetListing> result;
    for (const PresetShelf& shelf : library.shelves) {
        std::vector<PresetListing> shelf_entries = enumerate_validated_shelf(shelf);
        result.insert(result.end(), std::make_move_iterator(shelf_entries.begin()),
                      std::make_move_iterator(shelf_entries.end()));
    }
    std::sort(result.begin(), result.end(),
              [](const PresetListing& left, const PresetListing& right) {
                  return std::tie(left.shelf_identifier, left.identifier) <
                         std::tie(right.shelf_identifier, right.identifier);
              });
    return result;
}

std::vector<PresetListing> enumerate_preset_shelf(const PresetLibrary& library,
                                                  std::string_view shelf_identifier) {
    validate_preset_library(library);
    const auto shelf = std::find_if(
        library.shelves.begin(), library.shelves.end(),
        [&](const PresetShelf& candidate) { return candidate.identifier == shelf_identifier; });
    if (shelf == library.shelves.end()) {
        shelf_error(PresetLibraryErrorCode::unknown_shelf,
                    "preset shelf does not exist: " + std::string(shelf_identifier));
    }
    return enumerate_validated_shelf(*shelf);
}

ResolvedPreset resolve_preset(const PresetLibrary& library, std::string_view preset_identifier) {
    validate_preset_library(library);
    for (const PresetShelf& shelf : library.shelves) {
        const auto entry = std::find_if(shelf.entries.begin(), shelf.entries.end(),
                                        [&](const PresetShelfEntry& candidate) {
                                            return candidate.asset_identifier == preset_identifier;
                                        });
        if (entry != shelf.entries.end()) {
            return {.listing = listing(shelf, *entry),
                    .package = package_standalone_asset(shelf.contents, preset_identifier)};
        }
    }
    shelf_error(PresetLibraryErrorCode::unknown_preset,
                "preset does not exist: " + std::string(preset_identifier));
}

}  // namespace ctex::io
