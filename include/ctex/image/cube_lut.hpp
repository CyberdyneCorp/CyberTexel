#ifndef CTEX_IMAGE_CUBE_LUT_HPP
#define CTEX_IMAGE_CUBE_LUT_HPP

#include <cstddef>
#include <ctex/image/color.hpp>
#include <string_view>
#include <vector>

namespace ctex::image {

class CubeLut {
public:
    [[nodiscard]] static CubeLut from_cube(std::string_view source);

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] RgbColor apply(RgbColor color) const noexcept;

private:
    CubeLut(std::size_t size, RgbColor domain_min, RgbColor domain_max,
            std::vector<RgbColor> values);

    [[nodiscard]] const RgbColor& at(std::size_t red, std::size_t green,
                                     std::size_t blue) const noexcept;

    std::size_t size_;
    RgbColor domain_min_;
    RgbColor domain_max_;
    std::vector<RgbColor> values_;
};

}  // namespace ctex::image

#endif
