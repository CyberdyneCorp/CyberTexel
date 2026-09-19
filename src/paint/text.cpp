#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ctex/paint/text.hpp>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace ctex::paint {
namespace {

constexpr double metric_epsilon = 1.0e-9;
constexpr std::size_t no_glyph_pixel = std::numeric_limits<std::size_t>::max();

using FontIndex = std::unordered_map<char32_t, const SuppliedFontGlyph*>;

struct GlyphPlacement {
    const SuppliedFontGlyph* glyph;
    double pen_x;
};

struct LineLayout {
    std::vector<GlyphPlacement> glyphs;
    double minimum_x{};
    double maximum_x{};
    double width{};
};

bool valid_scalar(char32_t codepoint) {
    return codepoint <= 0x10FFFF && !(codepoint >= 0xD800 && codepoint <= 0xDFFF);
}

std::size_t utf8_sequence_length(std::uint8_t lead) {
    if (lead <= 0x7F) {
        return 1;
    }
    if ((lead & 0xE0U) == 0xC0U) {
        return 2;
    }
    if ((lead & 0xF0U) == 0xE0U) {
        return 3;
    }
    if ((lead & 0xF8U) == 0xF0U) {
        return 4;
    }
    throw std::invalid_argument("text contains an invalid UTF-8 leading byte");
}

char32_t decode_codepoint(std::string_view text, std::size_t& offset) {
    const auto lead = static_cast<std::uint8_t>(text[offset]);
    const std::size_t length = utf8_sequence_length(lead);
    if (offset + length > text.size()) {
        throw std::invalid_argument("text ends inside a UTF-8 sequence");
    }
    const std::uint8_t first_mask =
        length == 1 ? 0x7FU : static_cast<std::uint8_t>(0x7FU >> length);
    char32_t result = static_cast<char32_t>(lead & first_mask);
    for (std::size_t byte = 1; byte < length; ++byte) {
        const auto continuation = static_cast<std::uint8_t>(text[offset + byte]);
        if ((continuation & 0xC0U) != 0x80U) {
            throw std::invalid_argument("text contains an invalid UTF-8 continuation byte");
        }
        result = static_cast<char32_t>((result << 6U) | (continuation & 0x3FU));
    }
    const char32_t minimum =
        length == 1 ? 0 : (length == 2 ? 0x80 : (length == 3 ? 0x800 : 0x10000));
    if (result < minimum || !valid_scalar(result)) {
        throw std::invalid_argument("text contains a non-canonical UTF-8 scalar");
    }
    offset += length;
    return result;
}

std::vector<char32_t> decode_utf8(std::string_view text) {
    if (text.empty()) {
        throw std::invalid_argument("text must not be empty");
    }
    std::vector<char32_t> result;
    for (std::size_t offset = 0; offset < text.size();) {
        result.push_back(decode_codepoint(text, offset));
    }
    return result;
}

std::size_t glyph_area(const SuppliedFontGlyph& glyph) {
    if ((glyph.width == 0) != (glyph.height == 0)) {
        throw std::invalid_argument("font glyph dimensions must both be zero or both be non-zero");
    }
    if (glyph.width == 0) {
        return 0;
    }
    if (static_cast<std::size_t>(glyph.width) >
        std::numeric_limits<std::size_t>::max() / glyph.height) {
        throw std::invalid_argument("font glyph dimensions overflow addressable storage");
    }
    return static_cast<std::size_t>(glyph.width) * glyph.height;
}

void validate_glyph(const SuppliedFontGlyph& glyph, const SuppliedFont& font) {
    const std::size_t area = glyph_area(glyph);
    const bool metrics_valid = valid_scalar(glyph.codepoint) && glyph.codepoint != U'\n' &&
                               glyph.codepoint != U'\r' && std::isfinite(glyph.bearing_x) &&
                               std::isfinite(glyph.bearing_y) && std::isfinite(glyph.advance) &&
                               glyph.advance >= 0.0 &&
                               glyph.bearing_y <= font.ascent + metric_epsilon &&
                               glyph.bearing_y - glyph.height >= -font.descent - metric_epsilon;
    const bool coverage_valid =
        glyph.coverage.size() == area &&
        std::all_of(glyph.coverage.begin(), glyph.coverage.end(), [](double value) {
            return std::isfinite(value) && value >= 0.0 && value <= 1.0;
        });
    if (!metrics_valid || !coverage_valid) {
        throw std::invalid_argument("supplied font glyph metrics or coverage are invalid");
    }
}

FontIndex index_font(const SuppliedFont& font) {
    if (font.identity.empty() || !std::isfinite(font.pixels_per_em) || font.pixels_per_em <= 0.0 ||
        !std::isfinite(font.ascent) || font.ascent <= 0.0 || !std::isfinite(font.descent) ||
        font.descent < 0.0 || !std::isfinite(font.line_gap) || font.line_gap < 0.0 ||
        font.glyphs.empty()) {
        throw std::invalid_argument("supplied font identity or global metrics are invalid");
    }
    FontIndex result;
    result.reserve(font.glyphs.size());
    for (const SuppliedFontGlyph& glyph : font.glyphs) {
        validate_glyph(glyph, font);
        if (!result.emplace(glyph.codepoint, &glyph).second) {
            throw std::invalid_argument("supplied font contains a duplicate Unicode scalar");
        }
    }
    return result;
}

std::vector<std::vector<char32_t>> split_lines(std::span<const char32_t> codepoints) {
    std::vector<std::vector<char32_t>> result(1);
    for (std::size_t index = 0; index < codepoints.size(); ++index) {
        if (codepoints[index] == U'\r' || codepoints[index] == U'\n') {
            if (codepoints[index] == U'\r' && index + 1 < codepoints.size() &&
                codepoints[index + 1] == U'\n') {
                ++index;
            }
            result.emplace_back();
        } else {
            result.back().push_back(codepoints[index]);
        }
    }
    return result;
}

const SuppliedFontGlyph& find_glyph(const FontIndex& font, char32_t codepoint) {
    const auto found = font.find(codepoint);
    if (found == font.end()) {
        throw std::invalid_argument("supplied font is missing Unicode scalar " +
                                    std::to_string(static_cast<std::uint32_t>(codepoint)));
    }
    return *found->second;
}

LineLayout layout_line(std::span<const char32_t> codepoints, const FontIndex& font,
                       double tracking) {
    LineLayout result;
    result.glyphs.reserve(codepoints.size());
    double pen = 0.0;
    for (std::size_t index = 0; index < codepoints.size(); ++index) {
        const SuppliedFontGlyph& glyph = find_glyph(font, codepoints[index]);
        result.glyphs.push_back({.glyph = &glyph, .pen_x = pen});
        result.minimum_x = std::min(result.minimum_x, pen + glyph.bearing_x);
        result.maximum_x = std::max(result.maximum_x, pen + glyph.bearing_x + glyph.width);
        pen += glyph.advance;
        result.maximum_x = std::max(result.maximum_x, pen);
        if (index + 1 < codepoints.size()) {
            pen += tracking;
            result.minimum_x = std::min(result.minimum_x, pen);
            result.maximum_x = std::max(result.maximum_x, pen);
        }
    }
    result.width = result.maximum_x - result.minimum_x;
    return result;
}

std::vector<LineLayout> layout_lines(const std::vector<std::vector<char32_t>>& lines,
                                     const FontIndex& font, double tracking) {
    std::vector<LineLayout> result;
    result.reserve(lines.size());
    for (const std::vector<char32_t>& line : lines) {
        result.push_back(layout_line(line, font, tracking));
    }
    return result;
}

std::uint32_t checked_dimension(double value, std::string_view role) {
    const double rounded = std::ceil(value);
    if (!std::isfinite(rounded) || rounded <= 0.0 ||
        rounded > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string("text ") + std::string(role) + " is invalid");
    }
    return static_cast<std::uint32_t>(rounded);
}

double alignment_offset(TextAlignment alignment, double canvas_width, double line_width) {
    switch (alignment) {
        case TextAlignment::left:
            return 0.0;
        case TextAlignment::centre:
            return (canvas_width - line_width) * 0.5;
        case TextAlignment::right:
            return canvas_width - line_width;
    }
    throw std::invalid_argument("text alignment is invalid");
}

std::size_t glyph_pixel(const SuppliedFontGlyph& glyph, double x, double y) {
    const double source_x = std::floor(x);
    const double source_y = std::floor(y);
    if (source_x < 0.0 || source_y < 0.0 || source_x >= glyph.width || source_y >= glyph.height) {
        return no_glyph_pixel;
    }
    return static_cast<std::size_t>(source_y) * glyph.width + static_cast<std::size_t>(source_x);
}

void draw_glyph(std::span<double> destination, std::uint32_t width, std::uint32_t height,
                const SuppliedFontGlyph& glyph, double left, double top) {
    const auto first_x = static_cast<std::uint32_t>(std::max(0.0, std::floor(left)));
    const auto first_y = static_cast<std::uint32_t>(std::max(0.0, std::floor(top)));
    const auto end_x = static_cast<std::uint32_t>(
        std::min(static_cast<double>(width), std::ceil(left + glyph.width)));
    const auto end_y = static_cast<std::uint32_t>(
        std::min(static_cast<double>(height), std::ceil(top + glyph.height)));
    for (std::uint32_t y = first_y; y < end_y; ++y) {
        for (std::uint32_t x = first_x; x < end_x; ++x) {
            const std::size_t source = glyph_pixel(glyph, x + 0.5 - left, y + 0.5 - top);
            if (source != no_glyph_pixel) {
                const std::size_t target = static_cast<std::size_t>(y) * width + x;
                destination[target] = std::max(destination[target], glyph.coverage[source]);
            }
        }
    }
}

std::vector<double> render(const SuppliedFont& font, std::span<const LineLayout> lines,
                           TextAlignment alignment, std::uint32_t width, std::uint32_t height,
                           double canvas_width) {
    if (static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / height) {
        throw std::invalid_argument("text raster dimensions overflow addressable storage");
    }
    std::vector<double> result(static_cast<std::size_t>(width) * height, 0.0);
    const double line_advance = font.ascent + font.descent + font.line_gap;
    for (std::size_t line = 0; line < lines.size(); ++line) {
        const double offset =
            alignment_offset(alignment, canvas_width, lines[line].width) - lines[line].minimum_x;
        for (const GlyphPlacement& placement : lines[line].glyphs) {
            const double left = offset + placement.pen_x + placement.glyph->bearing_x;
            const double top = line * line_advance + font.ascent - placement.glyph->bearing_y;
            draw_glyph(result, width, height, *placement.glyph, left, top);
        }
    }
    return result;
}

DecalMaterial text_material(const TextRaster& text, std::span<const TextMaterialValue> values) {
    if (values.empty()) {
        throw std::invalid_argument("text decal requires at least one material channel");
    }
    const std::size_t count = text.opacity.size();
    DecalMaterial result{
        .width = text.width, .height = text.height, .channels = {}, .opacity = text.opacity};
    result.channels.reserve(values.size());
    for (const TextMaterialValue& source : values) {
        result.channels.push_back({.semantic_id = source.semantic_id,
                                   .component_count = source.component_count,
                                   .pixels = std::vector<graph::ColourValue>(count, source.value)});
    }
    return result;
}

}  // namespace

TextRaster rasterize_text(const SuppliedFont& font, std::string_view utf8,
                          const TextLayoutSettings& settings) {
    const FontIndex indexed = index_font(font);
    std::vector<char32_t> codepoints = decode_utf8(utf8);
    ToolParameterReport parameter_report;
    const double tracking_em =
        validate_tool_parameter(text_tracking_parameter, settings.tracking_em, parameter_report);
    const auto logical_lines = split_lines(codepoints);
    const auto lines = layout_lines(logical_lines, indexed, tracking_em * font.pixels_per_em);
    double canvas_width = 0.0;
    for (const LineLayout& line : lines) {
        canvas_width = std::max(canvas_width, line.width);
    }
    const double line_height = font.ascent + font.descent;
    const double canvas_height =
        line_height + static_cast<double>(lines.size() - 1) * (line_height + font.line_gap);
    const std::uint32_t width = checked_dimension(canvas_width, "width");
    const std::uint32_t height = checked_dimension(canvas_height, "height");
    return {.width = width,
            .height = height,
            .line_count = lines.size(),
            .width_em = canvas_width / font.pixels_per_em,
            .height_em = canvas_height / font.pixels_per_em,
            .tracking_em = tracking_em,
            .parameter_report = std::move(parameter_report),
            .codepoints = std::move(codepoints),
            .opacity = render(font, lines, settings.alignment, width, height, canvas_width)};
}

TextDecalResult apply_text_decal(const CachedSurfaceMaps& surface,
                                 std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
                                 std::span<const TextMaterialValue> material,
                                 const SuppliedFont& font, std::string_view utf8,
                                 const TextDecalSettings& settings) {
    TextRaster text = rasterize_text(font, utf8, settings.layout);
    ToolParameterReport parameter_report = text.parameter_report;
    const double size =
        validate_tool_parameter(text_size_parameter, settings.size, parameter_report);
    DecalPlacement placement = settings.placement;
    placement.transform.axis_scale.x *= text.width_em * size;
    placement.transform.axis_scale.y *= text.height_em * size;
    DecalMaterial projected = text_material(text, material);
    DecalRasterResult decal =
        rasterize_decal(surface, enabled_layer_snapshot, placement, projected, settings.decal);
    parameter_report.clamps.insert(parameter_report.clamps.end(),
                                   decal.parameter_report.clamps.begin(),
                                   decal.parameter_report.clamps.end());
    return {.size = size,
            .parameter_report = std::move(parameter_report),
            .text = std::move(text),
            .decal = std::move(decal)};
}

}  // namespace ctex::paint
