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
    double tracking_em{};
    TextAlignment alignment{TextAlignment::left};
};

struct TextRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    std::size_t line_count{};
    double width_em{};
    double height_em{};
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
    double size{1.0};
    DecalPlacement placement;
    DecalRasterSettings decal;
};

struct TextDecalResult {
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
