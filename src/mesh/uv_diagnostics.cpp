#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <ctex/mesh/uv_diagnostics.hpp>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ctex::mesh {
namespace {

struct Point {
    double x{};
    double y{};
};

struct Triangle {
    std::uint32_t face{};
    std::array<Point, 3> points{};
    double minimum_x{};
    double maximum_x{};
    double minimum_y{};
    double maximum_y{};
};

struct SampleBounds {
    std::uint32_t minimum_x{};
    std::uint32_t maximum_x{};
    std::uint32_t minimum_y{};
    std::uint32_t maximum_y{};
};

double cross(Point first, Point second, Point third) {
    return (second.x - first.x) * (third.y - first.y) - (second.y - first.y) * (third.x - first.x);
}

double signed_double_area(const std::vector<Point>& polygon) {
    double area = 0.0;
    for (std::size_t index = 0; index < polygon.size(); ++index) {
        const Point first = polygon[index];
        const Point second = polygon[(index + 1) % polygon.size()];
        area += first.x * second.y - first.y * second.x;
    }
    return area;
}

double coordinate_scale(const Triangle& left, const Triangle& right) {
    double scale = 1.0;
    for (const Point point : left.points) {
        scale = std::max({scale, std::abs(point.x), std::abs(point.y)});
    }
    for (const Point point : right.points) {
        scale = std::max({scale, std::abs(point.x), std::abs(point.y)});
    }
    return scale;
}

Point line_intersection(Point start, Point end, double start_distance, double end_distance) {
    const double denominator = start_distance - end_distance;
    if (denominator == 0.0) {
        return start;
    }
    const double fraction = start_distance / denominator;
    return {start.x + (end.x - start.x) * fraction, start.y + (end.y - start.y) * fraction};
}

std::vector<Point> clip_to_edge(const std::vector<Point>& input, Point edge_start, Point edge_end,
                                double orientation, double tolerance) {
    std::vector<Point> output;
    if (input.empty()) {
        return output;
    }
    output.reserve(input.size() + 1);
    Point previous = input.back();
    double previous_distance = orientation * cross(edge_start, edge_end, previous);
    bool previous_inside = previous_distance >= -tolerance;
    for (const Point current : input) {
        const double current_distance = orientation * cross(edge_start, edge_end, current);
        const bool current_inside = current_distance >= -tolerance;
        if (current_inside != previous_inside) {
            output.push_back(
                line_intersection(previous, current, previous_distance, current_distance));
        }
        if (current_inside) {
            output.push_back(current);
        }
        previous = current;
        previous_distance = current_distance;
        previous_inside = current_inside;
    }
    return output;
}

bool positive_area_intersection(const Triangle& left, const Triangle& right) {
    const double scale = coordinate_scale(left, right);
    const double tolerance = 64.0 * std::numeric_limits<double>::epsilon() * scale * scale;
    const double right_area = cross(right.points[0], right.points[1], right.points[2]);
    const double left_area = cross(left.points[0], left.points[1], left.points[2]);
    if (std::abs(left_area) <= tolerance || std::abs(right_area) <= tolerance) {
        return false;
    }
    const double orientation = right_area < 0.0 ? -1.0 : 1.0;
    std::vector<Point> intersection(left.points.begin(), left.points.end());
    for (std::size_t edge = 0; edge < right.points.size() && !intersection.empty(); ++edge) {
        intersection =
            clip_to_edge(intersection, right.points[edge],
                         right.points[(edge + 1) % right.points.size()], orientation, tolerance);
    }
    return intersection.size() >= 3 && std::abs(signed_double_area(intersection)) > tolerance;
}

bool bounding_boxes_overlap(const Triangle& left, const Triangle& right) {
    return left.maximum_x >= right.minimum_x && right.maximum_x >= left.minimum_x &&
           left.maximum_y >= right.minimum_y && right.maximum_y >= left.minimum_y;
}

Triangle make_triangle(const MeshDescriptor& descriptor, const UvSetView& uv_set,
                       std::uint32_t face) {
    const std::size_t first = static_cast<std::size_t>(face) * 3;
    const auto point = [&](std::size_t corner) {
        const Vec2f value = uv_set.values[descriptor.triangle_indices[first + corner]];
        return Point{value.x, value.y};
    };
    Triangle result{.face = face, .points = {point(0), point(1), point(2)}};
    result.minimum_x = std::min({result.points[0].x, result.points[1].x, result.points[2].x});
    result.maximum_x = std::max({result.points[0].x, result.points[1].x, result.points[2].x});
    result.minimum_y = std::min({result.points[0].y, result.points[1].y, result.points[2].y});
    result.maximum_y = std::max({result.points[0].y, result.points[1].y, result.points[2].y});
    return result;
}

void increment(std::size_t& value, const char* subject) {
    if (value == std::numeric_limits<std::size_t>::max()) {
        throw std::overflow_error(subject);
    }
    ++value;
}

std::size_t coverage_sample_count(UvCoverageRequest request) {
    if (request.width == 0 || request.height == 0) {
        throw std::invalid_argument("UV coverage dimensions must be non-zero");
    }
    const std::uint64_t samples =
        static_cast<std::uint64_t>(request.width) * static_cast<std::uint64_t>(request.height);
    if (samples > maximum_uv_coverage_samples ||
        samples > std::numeric_limits<std::size_t>::max()) {
        throw std::length_error("UV coverage sample count exceeds the 268435456 sample limit");
    }
    return static_cast<std::size_t>(samples);
}

std::optional<std::pair<std::uint32_t, std::uint32_t>> sample_axis_bounds(double minimum,
                                                                          double maximum,
                                                                          std::uint32_t size) {
    const double first = std::ceil(minimum * size - 0.5);
    const double last = std::floor(maximum * size - 0.5);
    if (last < 0.0 || first > static_cast<double>(size - 1) || first > last) {
        return std::nullopt;
    }
    return std::pair{
        static_cast<std::uint32_t>(std::max(0.0, first)),
        static_cast<std::uint32_t>(std::min(static_cast<double>(size - 1), last)),
    };
}

std::optional<SampleBounds> sample_bounds(const Triangle& triangle, UvCoverageRequest request) {
    const auto x = sample_axis_bounds(triangle.minimum_x, triangle.maximum_x, request.width);
    const auto y = sample_axis_bounds(triangle.minimum_y, triangle.maximum_y, request.height);
    if (!x || !y) {
        return std::nullopt;
    }
    return SampleBounds{.minimum_x = x->first,
                        .maximum_x = x->second,
                        .minimum_y = y->first,
                        .maximum_y = y->second};
}

double triangle_tolerance(const Triangle& triangle) {
    double scale = 1.0;
    for (const Point point : triangle.points) {
        scale = std::max({scale, std::abs(point.x), std::abs(point.y)});
    }
    return 64.0 * std::numeric_limits<double>::epsilon() * scale * scale;
}

bool contains_sample(const Triangle& triangle, Point sample, double orientation, double tolerance) {
    for (std::size_t edge = 0; edge < triangle.points.size(); ++edge) {
        if (orientation * cross(triangle.points[edge],
                                triangle.points[(edge + 1) % triangle.points.size()], sample) <
            -tolerance) {
            return false;
        }
    }
    return true;
}

bool outside_unit_square(const Triangle& triangle) {
    return std::ranges::any_of(triangle.points, [](Point point) {
        return point.x < 0.0 || point.x > 1.0 || point.y < 0.0 || point.y > 1.0;
    });
}

bool sample_is_covered(const std::vector<std::uint64_t>& covered, std::size_t sample) {
    return (covered[sample / 64] & (std::uint64_t{1} << (sample % 64))) != 0;
}

void mark_sample_covered(std::vector<std::uint64_t>& covered, std::size_t sample) {
    covered[sample / 64] |= std::uint64_t{1} << (sample % 64);
}

void rasterize_coverage(const Triangle& triangle, UvCoverageRequest request,
                        std::vector<std::uint64_t>& covered, std::size_t& tested_samples) {
    const double area = cross(triangle.points[0], triangle.points[1], triangle.points[2]);
    const double tolerance = triangle_tolerance(triangle);
    const auto bounds = sample_bounds(triangle, request);
    if (std::abs(area) <= tolerance || !bounds) {
        return;
    }
    const double orientation = area < 0.0 ? -1.0 : 1.0;
    for (std::uint32_t y = bounds->minimum_y; y <= bounds->maximum_y; ++y) {
        for (std::uint32_t x = bounds->minimum_x; x <= bounds->maximum_x; ++x) {
            const std::size_t sample = static_cast<std::size_t>(y) * request.width + x;
            if (sample_is_covered(covered, sample)) {
                continue;
            }
            increment(tested_samples, "UV coverage tested sample count overflow");
            const Point coordinate{(static_cast<double>(x) + 0.5) / request.width,
                                   (static_cast<double>(y) + 0.5) / request.height};
            if (contains_sample(triangle, coordinate, orientation, tolerance)) {
                mark_sample_covered(covered, sample);
            }
        }
    }
}

std::size_t covered_sample_count(const std::vector<std::uint64_t>& covered) {
    std::size_t result = 0;
    for (const std::uint64_t word : covered) {
        result += std::popcount(word);
    }
    return result;
}

}  // namespace

UvOverlapReport analyze_uv_overlaps(const MeshView& mesh, std::string_view uv_set_name,
                                    std::uint32_t partition_index) {
    const MeshDescriptor& descriptor = mesh.descriptor();
    if (partition_index >= descriptor.partitions.size()) {
        throw std::out_of_range("UV overlap partition index is outside the mesh partition table");
    }
    const UvSetView& uv_set = mesh.uv_set(uv_set_name);
    std::vector<Triangle> triangles;
    triangles.reserve(mesh.triangle_count());
    for (std::size_t face = 0; face < mesh.triangle_count(); ++face) {
        if (descriptor.face_partition_indices[face] == partition_index) {
            triangles.push_back(
                make_triangle(descriptor, uv_set, static_cast<std::uint32_t>(face)));
        }
    }
    std::ranges::sort(triangles, {}, [](const Triangle& triangle) {
        return std::array{triangle.minimum_x, triangle.minimum_y,
                          static_cast<double>(triangle.face)};
    });

    UvOverlapReport report;
    std::vector<bool> affected(mesh.triangle_count());
    std::vector<const Triangle*> active;
    for (std::size_t current = 0; current < triangles.size(); ++current) {
        const Triangle& triangle = triangles[current];
        std::erase_if(active, [&](const Triangle* preceding) {
            return preceding->maximum_x < triangle.minimum_x;
        });
        for (const Triangle* preceding : active) {
            increment(report.candidate_pair_count, "UV overlap candidate count overflow");
            if (!bounding_boxes_overlap(*preceding, triangle)) {
                continue;
            }
            if (positive_area_intersection(*preceding, triangle)) {
                increment(report.overlap_pair_count, "UV overlap pair count overflow");
                affected[preceding->face] = true;
                affected[triangle.face] = true;
            }
        }
        active.push_back(&triangle);
    }
    for (std::size_t face = 0; face < affected.size(); ++face) {
        if (affected[face]) {
            report.face_indices.push_back(static_cast<std::uint32_t>(face));
        }
    }
    return report;
}

UvCoverageReport analyze_uv_coverage(const MeshView& mesh, std::string_view uv_set_name,
                                     std::uint32_t partition_index, UvCoverageRequest request) {
    const MeshDescriptor& descriptor = mesh.descriptor();
    if (partition_index >= descriptor.partitions.size()) {
        throw std::out_of_range("UV coverage partition index is outside the mesh partition table");
    }
    const std::size_t sample_count = coverage_sample_count(request);
    const UvSetView& uv_set = mesh.uv_set(uv_set_name);
    std::vector<std::uint64_t> covered((sample_count + 63) / 64);
    UvCoverageReport report;
    for (std::size_t face = 0; face < mesh.triangle_count(); ++face) {
        if (descriptor.face_partition_indices[face] != partition_index) {
            continue;
        }
        increment(report.selected_face_count, "UV coverage selected face count overflow");
        const Triangle triangle =
            make_triangle(descriptor, uv_set, static_cast<std::uint32_t>(face));
        if (outside_unit_square(triangle)) {
            report.outside_face_indices.push_back(static_cast<std::uint32_t>(face));
        }
        rasterize_coverage(triangle, request, covered, report.tested_sample_count);
    }
    report.covered_sample_count = covered_sample_count(covered);
    report.uncovered_sample_count = sample_count - report.covered_sample_count;
    report.uncovered_fraction =
        static_cast<double>(report.uncovered_sample_count) / static_cast<double>(sample_count);
    return report;
}

}  // namespace ctex::mesh
