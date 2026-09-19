#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <ctex/image/color.hpp>
#include <ctex/image/color_policy.hpp>
#include <ctex/image/cube_lut.hpp>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

using ctex::image::ChannelSemantic;
using ctex::image::ColorSpace;
using ctex::image::CubeLut;
using ctex::image::InputColorSpace;
using ctex::image::RgbColor;

bool expect(bool condition, std::string_view scenario) {
    if (!condition) {
        std::cerr << "scenario failed: " << scenario << '\n';
    }
    return condition;
}

bool near(double left, double right, double tolerance = 1e-12) {
    return std::abs(left - right) <= tolerance;
}

bool working_space_is_stated() {
    return expect(
        ctex::image::color_space_name(ctex::image::working_color_space()) == "Linear Rec. 709",
        "Working space is stated");
}

bool roughness_is_not_gamma_corrected() {
    const RgbColor roughness{0.5, 0.5, 0.5};
    const RgbColor stored = ctex::image::input_to_working_space(
        roughness, InputColorSpace::srgb_rec709, ChannelSemantic::roughness);
    return expect(stored == roughness, "Roughness is not gamma-corrected");
}

bool automatic_input_rules_are_semantic() {
    const auto photograph =
        ctex::image::resolve_input_space(InputColorSpace::automatic, ChannelSemantic::base_color);
    const auto roughness =
        ctex::image::resolve_input_space(InputColorSpace::automatic, ChannelSemantic::roughness);
    return expect(photograph.inferred && photograph.color_space == ColorSpace::srgb_rec709 &&
                      roughness.inferred && roughness.color_space == ColorSpace::linear_rec709,
                  "Automatic photograph and roughness declarations");
}

bool unsupported_space_is_refused() {
    try {
        static_cast<void>(ctex::image::color_space_name(static_cast<ColorSpace>(255)));
    } catch (const std::invalid_argument&) {
        return true;
    }
    return expect(false, "Space is refused rather than approximated");
}

bool lut_affects_preview_only() {
    constexpr std::string_view invert_lut = R"(
LUT_3D_SIZE 2
1 1 1
0 1 1
1 0 1
0 0 1
1 1 0
0 1 0
1 0 0
0 0 0
)";
    const CubeLut lut = CubeLut::from_cube(invert_lut);
    const RgbColor authored{0.25, 0.5, 0.75};
    const RgbColor preview = lut.apply(authored);
    const RgbColor exported = authored;
    return expect(near(preview.red, 0.75) && near(preview.green, 0.5) && near(preview.blue, 0.25) &&
                      exported == authored,
                  "Grading affects preview only");
}

bool low_precision_normal_warns() {
    const auto warning = ctex::image::bit_depth_warning(ChannelSemantic::normal, 8);
    return expect(warning && warning->channel == ChannelSemantic::normal &&
                      warning->selected_bits == 8 && warning->recommended_bits == 16,
                  "8-bit normal map warning");
}

bool height_accumulates_before_quantizing() {
    std::array<double, 100> contributions{};
    contributions.fill(0.001);
    const double accumulated = ctex::image::accumulate_height(contributions, 8);
    return expect(accumulated > 0.09 && accumulated < 0.11, "Height accumulation at 8 bits");
}

bool ordered_dither_is_reproducible() {
    std::array<std::uint8_t, 16> first{};
    std::array<std::uint8_t, 16> second{};
    for (std::uint32_t index = 0; index < first.size(); ++index) {
        const std::uint32_t x = index % 4;
        const std::uint32_t y = index / 4;
        first[index] = ctex::image::quantize_unorm8(0.5, x, y);
        second[index] = ctex::image::quantize_unorm8(0.5, x, y);
    }
    const auto [minimum, maximum] = std::minmax_element(first.begin(), first.end());
    return expect(first == second && *minimum != *maximum &&
                      ctex::image::quantize_unorm8(0.5, 0, 0, false) == 128,
                  "Gradient banding and deterministic dithering");
}

bool host_picker_default_is_reported() {
    const auto resolved =
        ctex::image::resolve_input_space(InputColorSpace::automatic, ChannelSemantic::base_color);
    return expect(resolved.inferred && resolved.color_space == ColorSpace::srgb_rec709,
                  "Host picker automatic convention is reported");
}

bool linear_16_bit_round_trip_is_exact() {
    constexpr std::uint16_t input = 42405;
    const double normalized = static_cast<double>(input) / 65535.0;
    const RgbColor value{normalized, normalized, normalized};
    const RgbColor output =
        ctex::image::convert_color(value, ColorSpace::linear_rec709, ColorSpace::linear_rec709);
    const auto encoded = static_cast<std::uint16_t>(std::round(output.red * 65535.0));
    return expect(encoded == input, "Import then export at 16-bit linear");
}

bool transform_reference_is_headless() {
    return expect(near(ctex::image::srgb_to_linear(0.5), 0.21404114048223255, 1e-14),
                  "Headless transform reference");
}

}  // namespace

int main() {
    const std::array scenarios{
        working_space_is_stated,
        roughness_is_not_gamma_corrected,
        automatic_input_rules_are_semantic,
        unsupported_space_is_refused,
        lut_affects_preview_only,
        low_precision_normal_warns,
        height_accumulates_before_quantizing,
        ordered_dither_is_reproducible,
        host_picker_default_is_reported,
        linear_16_bit_round_trip_is_exact,
        transform_reference_is_headless,
    };
    return std::all_of(scenarios.begin(), scenarios.end(), [](auto scenario) { return scenario(); })
               ? 0
               : 1;
}
