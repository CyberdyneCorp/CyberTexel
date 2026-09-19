#include <algorithm>
#include <cmath>
#include <ctex/image/seam_dilation.hpp>
#include <limits>
#include <optional>
#include <stdexcept>

namespace ctex::image {
namespace {

std::size_t checked_texel_count(const SeamDilationRaster& raster) {
    if (raster.width == 0 || raster.height == 0 || raster.component_count == 0 ||
        raster.component_count > 4 ||
        static_cast<std::size_t>(raster.width) >
            std::numeric_limits<std::size_t>::max() / raster.height) {
        throw std::invalid_argument("seam-dilation raster dimensions or components are invalid");
    }
    const std::size_t texel_count = static_cast<std::size_t>(raster.width) * raster.height;
    if (texel_count > std::numeric_limits<std::size_t>::max() / raster.component_count ||
        raster.pixels.size() != texel_count * raster.component_count ||
        !std::all_of(raster.pixels.begin(), raster.pixels.end(),
                     [](double value) { return std::isfinite(value); })) {
        throw std::invalid_argument("seam-dilation raster pixels are invalid");
    }
    return texel_count;
}

void validate_coverage(std::span<const std::uint8_t> coverage, std::size_t texel_count) {
    if (coverage.size() != texel_count ||
        !std::all_of(coverage.begin(), coverage.end(),
                     [](std::uint8_t value) { return value == 0 || value == 1; })) {
        throw std::invalid_argument("seam-dilation coverage must be one binary value per texel");
    }
}

struct Point {
    std::int64_t x;
    std::int64_t y;
};

std::size_t texel_index(Point point, std::uint32_t width) {
    return static_cast<std::size_t>(point.y) * width + static_cast<std::size_t>(point.x);
}

double squared_distance(Point first, Point second) {
    const double x = static_cast<double>(first.x - second.x);
    const double y = static_cast<double>(first.y - second.y);
    return x * x + y * y;
}

std::optional<Point> nearest_covered(Point target, std::uint32_t width, std::uint32_t height,
                                     std::span<const std::uint8_t> coverage, std::uint32_t radius) {
    const std::int64_t minimum_x = std::max<std::int64_t>(0, target.x - radius);
    const std::int64_t maximum_x =
        std::min<std::int64_t>(width - 1, target.x + static_cast<std::int64_t>(radius));
    const std::int64_t minimum_y = std::max<std::int64_t>(0, target.y - radius);
    const std::int64_t maximum_y =
        std::min<std::int64_t>(height - 1, target.y + static_cast<std::int64_t>(radius));
    const double radius_squared = static_cast<double>(radius) * radius;
    std::optional<Point> nearest;
    double nearest_distance = std::numeric_limits<double>::infinity();
    for (std::int64_t y = minimum_y; y <= maximum_y; ++y) {
        for (std::int64_t x = minimum_x; x <= maximum_x; ++x) {
            const Point candidate{x, y};
            const double distance = squared_distance(target, candidate);
            if (coverage[texel_index(candidate, width)] != 0 && distance <= radius_squared &&
                distance < nearest_distance) {
                nearest = candidate;
                nearest_distance = distance;
            }
        }
    }
    return nearest;
}

struct InteriorSample {
    Point point;
    double projected_distance;
};

struct InteriorSearch {
    std::optional<InteriorSample> best;
    double alignment{-1.0};
    double distance{std::numeric_limits<double>::infinity()};
};

void consider_interior(Point candidate, Point edge, double direction_x, double direction_y,
                       std::uint32_t width, std::span<const std::uint8_t> coverage,
                       InteriorSearch& search) {
    if ((candidate.x == edge.x && candidate.y == edge.y) ||
        coverage[texel_index(candidate, width)] == 0) {
        return;
    }
    const double delta_x = static_cast<double>(candidate.x - edge.x);
    const double delta_y = static_cast<double>(candidate.y - edge.y);
    const double distance = std::hypot(delta_x, delta_y);
    const double projection = delta_x * direction_x + delta_y * direction_y;
    if (projection <= 0.0) {
        return;
    }
    const double alignment = projection / distance;
    if (alignment < search.alignment ||
        (alignment == search.alignment && distance >= search.distance)) {
        return;
    }
    search.best = InteriorSample{.point = candidate, .projected_distance = projection};
    search.alignment = alignment;
    search.distance = distance;
}

std::optional<InteriorSample> interior_sample(Point target, Point edge, std::uint32_t width,
                                              std::uint32_t height,
                                              std::span<const std::uint8_t> coverage,
                                              std::uint32_t radius) {
    const double outward_distance = std::sqrt(static_cast<double>(squared_distance(target, edge)));
    const double direction_x = static_cast<double>(edge.x - target.x) / outward_distance;
    const double direction_y = static_cast<double>(edge.y - target.y) / outward_distance;
    const std::int64_t search_radius = std::max<std::int64_t>(1, radius);
    InteriorSearch search;
    for (std::int64_t y = std::max<std::int64_t>(0, edge.y - search_radius);
         y <= std::min<std::int64_t>(height - 1, edge.y + search_radius); ++y) {
        for (std::int64_t x = std::max<std::int64_t>(0, edge.x - search_radius);
             x <= std::min<std::int64_t>(width - 1, edge.x + search_radius); ++x) {
            consider_interior({x, y}, edge, direction_x, direction_y, width, coverage, search);
        }
    }
    return search.best;
}

bool extrapolate_texel(SeamDilationRaster& output, const SeamDilationRaster& source, Point target,
                       Point edge, std::span<const std::uint8_t> coverage, std::uint32_t radius) {
    const auto interior =
        interior_sample(target, edge, source.width, source.height, coverage, radius);
    const std::size_t target_offset = texel_index(target, source.width) * source.component_count;
    const std::size_t edge_offset = texel_index(edge, source.width) * source.component_count;
    if (!interior) {
        std::copy_n(source.pixels.begin() + static_cast<std::ptrdiff_t>(edge_offset),
                    source.component_count,
                    output.pixels.begin() + static_cast<std::ptrdiff_t>(target_offset));
        return false;
    }
    const double distance = std::sqrt(static_cast<double>(squared_distance(target, edge)));
    const double scale = distance / interior->projected_distance;
    const std::size_t interior_offset =
        texel_index(interior->point, source.width) * source.component_count;
    for (std::size_t component = 0; component < source.component_count; ++component) {
        const double edge_value = source.pixels[edge_offset + component];
        const double value =
            edge_value + (edge_value - source.pixels[interior_offset + component]) * scale;
        if (!std::isfinite(value)) {
            throw std::overflow_error("seam-dilation extrapolation produced a non-finite value");
        }
        output.pixels[target_offset + component] = value;
    }
    return true;
}

}  // namespace

SeamDilationPixels extrapolate_uv_seams(const SeamDilationRaster& source,
                                        std::span<const std::uint8_t> coverage,
                                        std::uint32_t radius) {
    if (radius > maximum_uv_dilation_radius) {
        throw std::invalid_argument("seam-dilation radius exceeds the supported maximum");
    }
    const std::size_t texel_count = checked_texel_count(source);
    validate_coverage(coverage, texel_count);
    SeamDilationPixels result{.raster = source};
    if (radius == 0) {
        return result;
    }
    for (std::uint32_t y = 0; y < source.height; ++y) {
        for (std::uint32_t x = 0; x < source.width; ++x) {
            const Point target{.x = x, .y = y};
            if (coverage[texel_index(target, source.width)] != 0) {
                continue;
            }
            const auto edge =
                nearest_covered(target, source.width, source.height, coverage, radius);
            if (!edge) {
                continue;
            }
            ++result.dilated_texel_count;
            if (!extrapolate_texel(result.raster, source, target, *edge, coverage, radius)) {
                ++result.zero_gradient_texel_count;
            }
        }
    }
    return result;
}

}  // namespace ctex::image
