#include <cmath>
#include <ctex/image/color.hpp>
#include <iostream>
#include <string_view>

namespace {

using ctex::image::ColorSpace;
using ctex::image::RgbColor;

bool expect_near(double actual, double expected, double tolerance, std::string_view message) {
    if (std::abs(actual - expected) <= tolerance) {
        return true;
    }
    std::cerr << message << ": expected " << expected << ", found " << actual << '\n';
    return false;
}

bool test_reference_values() {
    bool passed = true;
    passed &= expect_near(ctex::image::srgb_to_linear(0.04045), 0.00313080495356037, 1e-14,
                          "sRGB breakpoint");
    passed &=
        expect_near(ctex::image::srgb_to_linear(0.5), 0.21404114048223255, 1e-14, "sRGB midpoint");
    passed &= expect_near(ctex::image::linear_to_srgb(0.18), 0.46135612950044164, 1e-14,
                          "linear 18 percent gray");
    passed &= expect_near(ctex::image::linear_to_srgb(1.0), 1.0, 1e-14, "linear white");
    return passed;
}

bool test_working_space_round_trip() {
    const RgbColor encoded{0.1, 0.5, 1.25};
    const RgbColor working = ctex::image::convert_color(encoded, ColorSpace::srgb_rec709,
                                                        ctex::image::working_color_space());
    const RgbColor round_trip = ctex::image::convert_color(
        working, ctex::image::working_color_space(), ColorSpace::srgb_rec709);

    bool passed = true;
    passed &= expect_near(round_trip.red, encoded.red, 1e-12, "red round trip");
    passed &= expect_near(round_trip.green, encoded.green, 1e-12, "green round trip");
    passed &= expect_near(round_trip.blue, encoded.blue, 1e-12, "HDR blue round trip");
    passed &= working.blue > 1.0;
    return passed;
}

bool test_declared_names() {
    return ctex::image::color_space_name(ctex::image::working_color_space()) == "Linear Rec. 709" &&
           ctex::image::color_space_name(ColorSpace::srgb_rec709) == "sRGB (Rec. 709 primaries)";
}

}  // namespace

int main() {
    return test_reference_values() && test_working_space_round_trip() && test_declared_names() ? 0
                                                                                               : 1;
}
