#ifndef CTEX_PAINT_TEXT_HPP
#define CTEX_PAINT_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/graph/document.hpp>
#include <ctex/paint/decal_stencil.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

inline constexpr ToolParameterDescriptor text_tracking_parameter{"text.tracking_em", 0.0, -10.0,
                                                                 10.0};
inline constexpr ToolParameterDescriptor text_size_parameter{
    "text.size", 1.0, stroke_position_tolerance, maximum_stroke_radius};

struct SuppliedFontGlyph {
    char32_t codepoint{};
    std::uint32_t width{};
    std::uint32_t height{};
    double bearing_x{};
    double bearing_y{};
    double advance{};
    std::vector<double> coverage;
};

struct SuppliedFont {
    std::string identity;
    double pixels_per_em{};
    double ascent{};
    double descent{};
    double line_gap{};
    std::vector<SuppliedFontGlyph> glyphs;
};

enum class TextAlignment : std::uint8_t { left, centre, right };

struct TextLayoutSettings {
    double tracking_em{text_tracking_parameter.default_value};
    TextAlignment alignment{TextAlignment::left};
};

struct TextRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    std::size_t line_count{};
    double width_em{};
    double height_em{};
    double tracking_em{};
    ToolParameterReport parameter_report;
    std::vector<char32_t> codepoints;
    std::vector<double> opacity;
};

[[nodiscard]] TextRaster rasterize_text(const SuppliedFont& font, std::string_view utf8,
                                        const TextLayoutSettings& settings = {});

struct TextMaterialValue {
    std::string semantic_id;
    std::uint8_t component_count{};
    graph::ColourValue value;
};

struct TextDecalSettings {
    TextLayoutSettings layout;
    double size{text_size_parameter.default_value};
    DecalPlacement placement;
    DecalRasterSettings decal;
};

struct TextDecalResult {
    double size{};
    ToolParameterReport parameter_report;
    TextRaster text;
    DecalRasterResult decal;
};

[[nodiscard]] TextDecalResult apply_text_decal(
    const CachedSurfaceMaps& surface,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
    std::span<const TextMaterialValue> material, const SuppliedFont& font, std::string_view utf8,
    const TextDecalSettings& settings);

}  // namespace ctex::paint

#endif
