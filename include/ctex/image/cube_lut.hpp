#ifndef CTEX_IMAGE_CUBE_LUT_HPP
#define CTEX_IMAGE_CUBE_LUT_HPP

#include <cstddef>
#include <ctex/image/color.hpp>
#include <memory_resource>
#include <string_view>
#include <vector>

namespace ctex::image {

class CubeLut {
public:
    [[nodiscard]] static CubeLut from_cube(
        std::string_view source,
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] RgbColor apply(RgbColor color) const noexcept;

private:
    CubeLut(std::size_t size, RgbColor domain_min, RgbColor domain_max,
            std::pmr::vector<RgbColor> values);

    [[nodiscard]] const RgbColor& at(std::size_t red, std::size_t green,
                                     std::size_t blue) const noexcept;

    std::size_t size_;
    RgbColor domain_min_;
    RgbColor domain_max_;
    std::pmr::vector<RgbColor> values_;
};

}  // namespace ctex::image

#endif
