#ifndef CTEX_IO_PRESET_LIBRARY_HPP
#define CTEX_IO_PRESET_LIBRARY_HPP

#include <ctex/io/standalone_asset.hpp>
#include <stdexcept>

namespace ctex::io {

struct PresetShelfEntry {
    std::string asset_identifier;
    std::string display_name;
    std::vector<std::string> tags;
    std::string thumbnail_resource_identifier;
    friend bool operator==(const PresetShelfEntry&, const PresetShelfEntry&) = default;
};

struct PresetShelf {
    std::string identifier;
    std::string display_name;
    ProjectContainer contents;
    std::vector<PresetShelfEntry> entries;
};

struct PresetLibrary {
    std::vector<PresetShelf> shelves;
};

struct PresetShelfListing {
    std::string identifier;
    std::string display_name;
    std::size_t preset_count{};
    friend bool operator==(const PresetShelfListing&, const PresetShelfListing&) = default;
};

struct PresetListing {
    std::string shelf_identifier;
    std::string identifier;
    std::string kind;
    std::uint32_t format_version{};
    std::string display_name;
    std::vector<std::string> tags;
    StoredTiledImage thumbnail;
    friend bool operator==(const PresetListing&, const PresetListing&) = default;
};

struct ResolvedPreset {
    PresetListing listing;
    ProjectContainer package;
};

enum class PresetLibraryErrorCode : std::uint8_t {
    invalid_shelf,
    invalid_entry,
    unknown_shelf,
    unknown_preset,
};

class PresetLibraryError final : public std::invalid_argument {
public:
    PresetLibraryError(PresetLibraryErrorCode code, std::string message);
    [[nodiscard]] PresetLibraryErrorCode code() const noexcept { return code_; }

private:
    PresetLibraryErrorCode code_;
};

void validate_preset_library(const PresetLibrary& library);
[[nodiscard]] std::vector<PresetShelfListing> enumerate_preset_shelves(
    const PresetLibrary& library);
[[nodiscard]] std::vector<PresetListing> enumerate_presets(const PresetLibrary& library);
[[nodiscard]] std::vector<PresetListing> enumerate_preset_shelf(const PresetLibrary& library,
                                                                std::string_view shelf_identifier);
[[nodiscard]] ResolvedPreset resolve_preset(const PresetLibrary& library,
                                            std::string_view preset_identifier);

}  // namespace ctex::io

#endif
