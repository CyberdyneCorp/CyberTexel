#include <cmath>
#include <ctex/image/color.hpp>
#include <stdexcept>

namespace ctex::image {
namespace {

void validate_color_space(ColorSpace color_space) {
    switch (color_space) {
        case ColorSpace::linear_rec709:
        case ColorSpace::srgb_rec709:
            return;
    }
    throw std::invalid_argument("unsupported color space");
}

}  // namespace

std::string_view color_space_name(ColorSpace color_space) {
    switch (color_space) {
        case ColorSpace::linear_rec709:
            return "Linear Rec. 709";
        case ColorSpace::srgb_rec709:
            return "sRGB (Rec. 709 primaries)";
    }
    throw std::invalid_argument("unsupported color space");
}

double srgb_to_linear(double value) noexcept {
    if (value <= 0.04045) {
        return value / 12.92;
    }
    return std::pow((value + 0.055) / 1.055, 2.4);
}

double linear_to_srgb(double value) noexcept {
    if (value <= 0.0031308) {
        return value * 12.92;
    }
    return (1.055 * std::pow(value, 1.0 / 2.4)) - 0.055;
}

RgbColor convert_color(RgbColor color, ColorSpace source, ColorSpace destination) {
    validate_color_space(source);
    validate_color_space(destination);
    if (source == destination) {
        return color;
    }
    if (source == ColorSpace::srgb_rec709) {
        return {
            srgb_to_linear(color.red),
            srgb_to_linear(color.green),
            srgb_to_linear(color.blue),
        };
    }
    return {
        linear_to_srgb(color.red),
        linear_to_srgb(color.green),
        linear_to_srgb(color.blue),
    };
}

}  // namespace ctex::image
