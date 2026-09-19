#ifndef CTEX_IMAGE_COLOR_HPP
#define CTEX_IMAGE_COLOR_HPP

#include <string_view>

namespace ctex::image {

enum class ColorSpace {
    linear_rec709,
    srgb_rec709,
};

struct RgbColor {
    double red;
    double green;
    double blue;

    friend constexpr bool operator==(RgbColor, RgbColor) noexcept = default;
};

[[nodiscard]] constexpr ColorSpace working_color_space() noexcept {
    return ColorSpace::linear_rec709;
}

[[nodiscard]] std::string_view color_space_name(ColorSpace color_space);
[[nodiscard]] double srgb_to_linear(double value) noexcept;
[[nodiscard]] double linear_to_srgb(double value) noexcept;
[[nodiscard]] RgbColor convert_color(RgbColor color, ColorSpace source, ColorSpace destination);

}  // namespace ctex::image

#endif
