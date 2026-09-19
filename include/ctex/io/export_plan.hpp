#ifndef CTEX_IO_EXPORT_PLAN_HPP
#define CTEX_IO_EXPORT_PLAN_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/io/export_preset.hpp>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::io {

inline constexpr std::string_view default_export_filename_pattern =
    "{project}_{texture_set}{suffix}_{resolution}_{bit_depth}_{udim}_{layer}.{extension}";

enum class ExportTextureSetSelection : std::uint8_t { all, selected };
enum class ExportSpatialScope : std::uint8_t { texture_set, udim_tile, atlas };
enum class ExportLayerScope : std::uint8_t { flatten_visible, flatten_selected, each_selected };
enum class ExportLayerKind : std::uint8_t { content, group };

struct ExportLayerSource {
    std::string identifier;
    std::string display_name;
    std::string parent_identifier;
    ExportLayerKind kind{ExportLayerKind::content};
    bool visible{true};
    friend bool operator==(const ExportLayerSource&, const ExportLayerSource&) = default;
};

struct ExportTextureSetSource {
    std::string identifier;
    std::string display_name;
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<std::uint32_t> occupied_udim_tiles;
    std::vector<ExportLayerSource> layers;
    friend bool operator==(const ExportTextureSetSource&, const ExportTextureSetSource&) = default;
};

struct ExportAtlasSource {
    std::string identifier;
    std::string display_name;
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<std::string> texture_set_identifiers;
    friend bool operator==(const ExportAtlasSource&, const ExportAtlasSource&) = default;
};

struct ExportSourceCatalogue {
    std::string project_name;
    std::vector<ExportTextureSetSource> texture_sets;
    std::vector<ExportAtlasSource> atlases;
    friend bool operator==(const ExportSourceCatalogue&, const ExportSourceCatalogue&) = default;
};

struct ExportPlanRequest {
    ExportTextureSetSelection texture_set_selection{ExportTextureSetSelection::all};
    std::vector<std::string> selected_texture_set_identifiers;
    ExportSpatialScope spatial_scope{ExportSpatialScope::texture_set};
    ExportLayerScope layer_scope{ExportLayerScope::flatten_visible};
    std::map<std::string, std::vector<std::string>, std::less<>> selected_layer_identifiers;
    std::string filename_pattern{default_export_filename_pattern};
    friend bool operator==(const ExportPlanRequest&, const ExportPlanRequest&) = default;
};

struct PlannedTextureExport {
    std::string relative_path;
    std::vector<std::string> texture_set_identifiers;
    std::optional<std::uint32_t> udim_tile;
    std::optional<std::string> atlas_identifier;
    std::map<std::string, std::vector<std::string>, std::less<>> layer_identifiers;
    std::size_t preset_texture_index{};
    std::uint32_t width{};
    std::uint32_t height{};
    ExportImageFormat format{ExportImageFormat::png};
    ExportBitDepth bit_depth{ExportBitDepth::bits_8};
    image::ColorSpace color_space{image::ColorSpace::linear_rec709};
    friend bool operator==(const PlannedTextureExport&, const PlannedTextureExport&) = default;
};

enum class ExportPlanErrorCode : std::uint8_t {
    invalid_catalogue,
    invalid_scope,
    invalid_layer_selection,
    invalid_pattern,
    path_collision,
};

class ExportPlanError : public std::runtime_error {
public:
    ExportPlanError(ExportPlanErrorCode code, std::string message);
    [[nodiscard]] ExportPlanErrorCode code() const noexcept { return code_; }

private:
    ExportPlanErrorCode code_;
};

[[nodiscard]] std::vector<PlannedTextureExport> plan_texture_export(
    const ExportSourceCatalogue& catalogue, const ExportPreset& preset,
    const ExportPlanRequest& request = {});

}  // namespace ctex::io

#endif
