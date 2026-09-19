#include <array>
#include <cmath>
#include <ctex/paint/text.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool near(double actual, double expected, double tolerance = 1.0e-6) {
    return std::abs(actual - expected) <= tolerance;
}

SuppliedFontGlyph glyph(char32_t codepoint, std::uint32_t width, std::uint32_t height,
                        double bearing_y, double advance, std::initializer_list<double> coverage) {
    return {.codepoint = codepoint,
            .width = width,
            .height = height,
            .bearing_x = 0.0,
            .bearing_y = bearing_y,
            .advance = advance,
            .coverage = coverage};
}

SuppliedFont font() {
    return {.identity = "font:test:v1",
            .pixels_per_em = 4.0,
            .ascent = 3.0,
            .descent = 1.0,
            .line_gap = 1.0,
            .glyphs = {glyph(U'A', 2, 3, 3.0, 3.0, {1, 1, 1, 1, 1, 1}),
                       glyph(U'\u00e7', 2, 4, 3.0, 3.0, {1, 1, 1, 1, 1, 1, 1, 1}),
                       glyph(U'B', 1, 3, 3.0, 1.0, {1, 1, 1})}};
}

double pixel(const TextRaster& raster, std::uint32_t x, std::uint32_t y) {
    return raster.opacity[static_cast<std::size_t>(y) * raster.width + x];
}

bool utf8_tracking_and_all_alignments_are_rasterized() {
    const std::string text = "A\xc3\xa7\nB";
    const TextRaster left =
        rasterize_text(font(), text, {.tracking_em = 0.25, .alignment = TextAlignment::left});
    const TextRaster centre =
        rasterize_text(font(), text, {.tracking_em = 0.25, .alignment = TextAlignment::centre});
    const TextRaster right =
        rasterize_text(font(), text, {.tracking_em = 0.25, .alignment = TextAlignment::right});

    return expect(left.width == 7 && left.height == 9 && left.line_count == 2 &&
                      near(left.width_em, 1.75) && near(left.height_em, 2.25),
                  "UTF-8 text layout did not preserve font metrics or tracking") &&
           expect(left.codepoints == std::vector<char32_t>({U'A', U'\u00e7', U'\n', U'B'}),
                  "UTF-8 text was not decoded into the expected Unicode scalars") &&
           expect(pixel(left, 0, 5) == 1.0 && pixel(left, 3, 5) == 0.0 &&
                      pixel(centre, 3, 5) == 1.0 && pixel(centre, 0, 5) == 0.0 &&
                      pixel(right, 6, 5) == 1.0 && pixel(right, 3, 5) == 0.0,
                  "left, centre or right line alignment did not affect glyph placement");
}

PaintToolChannelRaster channel(std::string semantic_id, float red) {
    return {.semantic_id = std::move(semantic_id),
            .component_count = 3,
            .pixels = {{red, 0.0F, 0.0F, 1.0F}}};
}

CachedSurfaceMaps surface() {
    return {.texture_set_id = "set:body",
            .uv_set = "uv0",
            .mesh_revision = 1,
            .surface = {.width = 1,
                        .height = 1,
                        .tile_origin = {},
                        .texels = {{.position = {},
                                    .normal = {0.0, 0.0, 1.0},
                                    .geometric_normal = {0.0, 0.0, 1.0},
                                    .uv = {0.5, 0.5},
                                    .triangle = 0}}},
            .coverage = {1},
            .triangle_identity = {0},
            .uv_island_identity = {0}};
}

bool requested_size_is_applied_through_the_decal_frame() {
    const std::array layer{channel("pbr.base_color", 0.0F)};
    const std::array<double, 1> selection{0.5};
    const std::array<double, 1> rejection{0.5};
    const std::array material{TextMaterialValue{
        .semantic_id = "pbr.base_color", .component_count = 3, .value = {1.0F, 0.0F, 0.0F, 1.0F}}};
    const TextDecalResult result =
        apply_text_decal(surface(), layer, material, font(), "A",
                         {.layout = {.tracking_em = 0.0, .alignment = TextAlignment::left},
                          .size = 2.0,
                          .placement = {.position = {},
                                        .surface_normal = {0.0, 0.0, 1.0},
                                        .transform = {.rotation_radians = 0.0,
                                                      .uniform_scale = 1.0,
                                                      .axis_scale = {1.0, 1.0}}},
                          .decal = {.blend_mode = "normal",
                                    .masks = {.active_layer_masks = {},
                                              .colour_id_selection = std::nullopt,
                                              .geometry_selection = std::nullopt,
                                              .screen_selection = PaintMaskView{selection},
                                              .uv_island_selection = std::nullopt},
                                    .rejection_acceptance = PaintMaskView{rejection}}});

    return expect(result.text.width == 3 && result.text.height == 4 &&
                      near(result.decal.frame.scale.x, 1.5) &&
                      near(result.decal.frame.scale.y, 2.0),
                  "requested text size did not set the projected decal dimensions") &&
           expect(result.decal.editable_revision == 0 &&
                      result.decal.strength == std::vector<double>({0.25}) &&
                      near(result.decal.channels[0].pixels[0].r, 0.25),
                  "rasterized text did not inherit decal masks, rejection and shading");
}

bool invalid_utf8_missing_glyph_and_size_are_refused() {
    bool utf8_refused = false;
    try {
        static_cast<void>(rasterize_text(font(), "\xc0\x80"));
    } catch (const std::invalid_argument&) {
        utf8_refused = true;
    }
    bool glyph_refused = false;
    try {
        static_cast<void>(rasterize_text(font(), "Z"));
    } catch (const std::invalid_argument&) {
        glyph_refused = true;
    }
    bool size_refused = false;
    try {
        const std::array layer{channel("pbr.base_color", 0.0F)};
        const std::array material{TextMaterialValue{.semantic_id = "pbr.base_color",
                                                    .component_count = 3,
                                                    .value = {1.0F, 0.0F, 0.0F, 1.0F}}};
        static_cast<void>(
            apply_text_decal(surface(), layer, material, font(), "A",
                             {.layout = {}, .size = 0.0, .placement = {}, .decal = {}}));
    } catch (const std::invalid_argument&) {
        size_refused = true;
    }
    return expect(utf8_refused && glyph_refused && size_refused,
                  "invalid UTF-8, a missing glyph or invalid text size was not refused");
}

}  // namespace

int main() {
    return utf8_tracking_and_all_alignments_are_rasterized() &&
                   requested_size_is_applied_through_the_decal_frame() &&
                   invalid_utf8_missing_glyph_and_size_are_refused()
               ? 0
               : 1;
}
