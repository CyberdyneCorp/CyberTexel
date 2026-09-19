#include <algorithm>
#include <cmath>
#include <ctex/image/cube_lut.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace ctex::image {
namespace {

RgbColor interpolate(RgbColor left, RgbColor right, double amount) noexcept {
    return {
        std::lerp(left.red, right.red, amount),
        std::lerp(left.green, right.green, amount),
        std::lerp(left.blue, right.blue, amount),
    };
}

double normalize(double value, double minimum, double maximum) noexcept {
    return std::clamp((value - minimum) / (maximum - minimum), 0.0, 1.0);
}

RgbColor read_triplet(std::istringstream& line, std::string_view field) {
    RgbColor value{};
    if (!(line >> value.red >> value.green >> value.blue)) {
        throw std::invalid_argument(std::string("invalid .cube ") + std::string(field));
    }
    return value;
}

}  // namespace

CubeLut CubeLut::from_cube(std::string_view source, std::pmr::memory_resource* memory_resource) {
    if (memory_resource == nullptr) {
        throw std::invalid_argument(".cube LUT requires a memory resource");
    }
    std::istringstream input{std::string(source)};
    std::size_t size = 0;
    RgbColor domain_min{0.0, 0.0, 0.0};
    RgbColor domain_max{1.0, 1.0, 1.0};
    std::pmr::vector<RgbColor> values(memory_resource);
    std::string source_line;

    while (std::getline(input, source_line)) {
        std::istringstream line(source_line);
        std::string first;
        if (!(line >> first) || first.starts_with('#')) {
            continue;
        }
        if (first == "TITLE") {
            continue;
        }
        if (first == "LUT_3D_SIZE") {
            if (!(line >> size) || size < 2 || size > 256) {
                throw std::invalid_argument("invalid .cube LUT_3D_SIZE");
            }
            continue;
        }
        if (first == "DOMAIN_MIN") {
            domain_min = read_triplet(line, "DOMAIN_MIN");
            continue;
        }
        if (first == "DOMAIN_MAX") {
            domain_max = read_triplet(line, "DOMAIN_MAX");
            continue;
        }
        std::istringstream red(first);
        RgbColor value{};
        if (!(red >> value.red) || !(line >> value.green >> value.blue)) {
            throw std::invalid_argument("unsupported or malformed .cube directive");
        }
        values.push_back(value);
    }

    if (size == 0) {
        throw std::invalid_argument(".cube file has no LUT_3D_SIZE");
    }
    if (domain_min.red >= domain_max.red || domain_min.green >= domain_max.green ||
        domain_min.blue >= domain_max.blue) {
        throw std::invalid_argument(".cube domain maximum must exceed its minimum");
    }
    if (values.size() != size * size * size) {
        throw std::invalid_argument(".cube sample count does not match LUT_3D_SIZE");
    }
    return CubeLut(size, domain_min, domain_max, std::move(values));
}

CubeLut::CubeLut(std::size_t size, RgbColor domain_min, RgbColor domain_max,
                 std::pmr::vector<RgbColor> values)
    : size_(size), domain_min_(domain_min), domain_max_(domain_max), values_(std::move(values)) {}

RgbColor CubeLut::apply(RgbColor color) const noexcept {
    const double red = normalize(color.red, domain_min_.red, domain_max_.red) * (size_ - 1);
    const double green = normalize(color.green, domain_min_.green, domain_max_.green) * (size_ - 1);
    const double blue = normalize(color.blue, domain_min_.blue, domain_max_.blue) * (size_ - 1);

    const std::size_t r0 = static_cast<std::size_t>(std::floor(red));
    const std::size_t g0 = static_cast<std::size_t>(std::floor(green));
    const std::size_t b0 = static_cast<std::size_t>(std::floor(blue));
    const std::size_t r1 = std::min(r0 + 1, size_ - 1);
    const std::size_t g1 = std::min(g0 + 1, size_ - 1);
    const std::size_t b1 = std::min(b0 + 1, size_ - 1);

    const RgbColor c00 = interpolate(at(r0, g0, b0), at(r1, g0, b0), red - r0);
    const RgbColor c10 = interpolate(at(r0, g1, b0), at(r1, g1, b0), red - r0);
    const RgbColor c01 = interpolate(at(r0, g0, b1), at(r1, g0, b1), red - r0);
    const RgbColor c11 = interpolate(at(r0, g1, b1), at(r1, g1, b1), red - r0);
    const RgbColor c0 = interpolate(c00, c10, green - g0);
    const RgbColor c1 = interpolate(c01, c11, green - g0);
    return interpolate(c0, c1, blue - b0);
}

const RgbColor& CubeLut::at(std::size_t red, std::size_t green, std::size_t blue) const noexcept {
    return values_[red + (size_ * (green + (size_ * blue)))];
}

}  // namespace ctex::image
