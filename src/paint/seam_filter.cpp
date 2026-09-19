#include <algorithm>
#include <cmath>
#include <ctex/paint/seam_filter.hpp>
#include <ctex/paint/surface_cache.hpp>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>

namespace ctex::paint {
namespace {

constexpr double frame_tolerance = 1.0e-6;

bool finite(Vec3d value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

double length(Vec3d value) { return std::sqrt(dot(value, value)); }

Vec3d add(Vec3d left, Vec3d right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3d multiply(Vec3d value, double scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}

bool unit(Vec3d value) { return finite(value) && std::abs(length(value) - 1.0) <= frame_tolerance; }

void validate_frame(StrokeFrame frame) {
    if (!unit(frame.tangent) || !unit(frame.bitangent) || !unit(frame.normal) ||
        std::abs(dot(frame.tangent, frame.bitangent)) > frame_tolerance ||
        std::abs(dot(frame.tangent, frame.normal)) > frame_tolerance ||
        std::abs(dot(frame.bitangent, frame.normal)) > frame_tolerance) {
        throw std::invalid_argument("surface filter tangent frame must be orthonormal");
    }
}

std::uint64_t offset_magnitude(std::int32_t offset) {
    const std::int64_t widened = offset;
    return static_cast<std::uint64_t>(widened < 0 ? -widened : widened);
}

void validate_request(std::size_t value_count, const SurfaceFilterRequest& request) {
    validate_frame(request.output_frame);
    if (request.operation != SurfaceFilterOperation::blur &&
        request.operation != SurfaceFilterOperation::smear &&
        request.operation != SurfaceFilterOperation::derivative &&
        request.operation != SurfaceFilterOperation::mip_generation) {
        throw std::invalid_argument("surface filter operation is invalid");
    }
    if (request.samples.empty()) {
        throw std::invalid_argument("surface filter requires surface-adjacent samples");
    }
    double total_weight = 0.0;
    for (const SurfaceAdjacentSample& sample : request.samples) {
        validate_frame(sample.tangent_frame);
        if (sample.texel_index >= value_count || !std::isfinite(sample.weight) ||
            offset_magnitude(sample.offset_x) > request.footprint.radius_x ||
            offset_magnitude(sample.offset_y) > request.footprint.radius_y) {
            throw std::invalid_argument("surface filter sample is invalid");
        }
        total_weight += sample.weight;
    }
    if (!std::isfinite(total_weight) ||
        (request.operation != SurfaceFilterOperation::derivative && total_weight <= 0.0)) {
        throw std::invalid_argument("surface filter weights are invalid");
    }
}

double weight_sum(const SurfaceFilterRequest& request) {
    double result = 0.0;
    for (const SurfaceAdjacentSample& sample : request.samples) {
        result += sample.weight;
    }
    return result;
}

Vec3d tangent_to_world(Vec3d value, StrokeFrame frame) {
    return add(add(multiply(frame.tangent, value.x), multiply(frame.bitangent, value.y)),
               multiply(frame.normal, value.z));
}

Vec3d world_to_tangent(Vec3d value, StrokeFrame frame) {
    return {dot(value, frame.tangent), dot(value, frame.bitangent), dot(value, frame.normal)};
}

std::size_t checked_texel_count(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / height) {
        throw std::invalid_argument("island padding dimensions are invalid");
    }
    return static_cast<std::size_t>(width) * height;
}

std::uint32_t required_radius(SamplingFootprint footprint, std::uint32_t mip_level) {
    if (mip_level >= 31) {
        throw std::invalid_argument("island padding mip level is too large");
    }
    const std::uint64_t scale = std::uint64_t{1} << mip_level;
    const std::uint64_t base_radius = std::max(footprint.radius_x, footprint.radius_y);
    const std::uint64_t radius = (base_radius + 1) * scale - 1;
    if (radius > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("island padding radius is too large");
    }
    return static_cast<std::uint32_t>(radius);
}

struct GridPoint {
    std::uint32_t x{};
    std::uint32_t y{};
};

std::size_t texel_index(GridPoint point, std::uint32_t width) {
    return static_cast<std::size_t>(point.y) * width + point.x;
}

std::uint32_t chebyshev_distance(GridPoint left, GridPoint right) {
    const std::uint32_t x = left.x > right.x ? left.x - right.x : right.x - left.x;
    const std::uint32_t y = left.y > right.y ? left.y - right.y : right.y - left.y;
    return std::max(x, y);
}

struct GridBounds {
    std::uint32_t minimum_x{};
    std::uint32_t maximum_x{};
    std::uint32_t minimum_y{};
    std::uint32_t maximum_y{};
};

GridBounds search_bounds(GridPoint center, std::uint32_t width, std::uint32_t height,
                         std::uint64_t radius) {
    const auto minimum = [radius](std::uint32_t coordinate) {
        return radius >= coordinate ? 0U : coordinate - static_cast<std::uint32_t>(radius);
    };
    const auto maximum = [radius](std::uint32_t coordinate, std::uint32_t limit) {
        return static_cast<std::uint32_t>(
            std::min<std::uint64_t>(limit - 1, static_cast<std::uint64_t>(coordinate) + radius));
    };
    return {.minimum_x = minimum(center.x),
            .maximum_x = maximum(center.x, width),
            .minimum_y = minimum(center.y),
            .maximum_y = maximum(center.y, height)};
}

void collect_conflicts(GridPoint source, std::uint32_t width, std::uint32_t height,
                       std::span<const std::uint32_t> islands, std::uint32_t radius,
                       std::set<std::uint32_t>& conflicts) {
    const std::uint32_t source_island = islands[texel_index(source, width)];
    const GridBounds bounds =
        search_bounds(source, width, height, static_cast<std::uint64_t>(radius) * 2);
    for (std::uint32_t y = bounds.minimum_y; y <= bounds.maximum_y; ++y) {
        for (std::uint32_t x = bounds.minimum_x; x <= bounds.maximum_x; ++x) {
            const std::uint32_t neighbor = islands[texel_index({x, y}, width)];
            if (neighbor != no_uv_island && neighbor != source_island &&
                chebyshev_distance(source, {x, y}) <= static_cast<std::uint64_t>(radius) * 2) {
                conflicts.insert(source_island);
                conflicts.insert(neighbor);
            }
        }
    }
}

std::vector<std::uint32_t> conflicting_islands(std::uint32_t width, std::uint32_t height,
                                               std::span<const std::uint32_t> islands,
                                               std::uint32_t radius) {
    std::set<std::uint32_t> conflicts;
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            if (islands[texel_index({x, y}, width)] != no_uv_island) {
                collect_conflicts({x, y}, width, height, islands, radius, conflicts);
            }
        }
    }
    return {conflicts.begin(), conflicts.end()};
}

std::uint32_t nearest_island(GridPoint target, std::uint32_t width, std::uint32_t height,
                             std::span<const std::uint32_t> islands, std::uint32_t radius) {
    std::uint32_t owner = no_uv_island;
    std::uint64_t nearest = static_cast<std::uint64_t>(radius) + 1;
    bool contested = false;
    const GridBounds bounds = search_bounds(target, width, height, radius);
    for (std::uint32_t y = bounds.minimum_y; y <= bounds.maximum_y; ++y) {
        for (std::uint32_t x = bounds.minimum_x; x <= bounds.maximum_x; ++x) {
            const GridPoint candidate{x, y};
            const std::uint32_t candidate_island = islands[texel_index(candidate, width)];
            const std::uint32_t distance = chebyshev_distance(target, candidate);
            if (candidate_island == no_uv_island || distance > nearest) {
                continue;
            }
            if (distance < nearest) {
                owner = candidate_island;
                nearest = distance;
                contested = false;
            } else if (candidate_island != owner) {
                contested = true;
            }
        }
    }
    return contested ? no_uv_island : owner;
}

std::optional<GridPoint> nearest_source(GridPoint target, std::uint32_t width, std::uint32_t height,
                                        std::span<const std::uint32_t> islands, std::uint32_t owner,
                                        std::uint32_t radius) {
    GridPoint nearest{};
    std::uint32_t nearest_distance = std::numeric_limits<std::uint32_t>::max();
    const GridBounds bounds = search_bounds(target, width, height, radius);
    for (std::uint32_t y = bounds.minimum_y; y <= bounds.maximum_y; ++y) {
        for (std::uint32_t x = bounds.minimum_x; x <= bounds.maximum_x; ++x) {
            const GridPoint candidate{x, y};
            if (islands[texel_index(candidate, width)] != owner) {
                continue;
            }
            const std::uint32_t distance = chebyshev_distance(target, candidate);
            if (distance < nearest_distance) {
                nearest = candidate;
                nearest_distance = distance;
            }
        }
    }
    if (nearest_distance == std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return nearest;
}

void validate_raster(const SeamDilationRaster& raster, std::size_t texel_count) {
    if (raster.width == 0 || raster.height == 0 || raster.component_count == 0 ||
        raster.component_count > 4 ||
        texel_count > std::numeric_limits<std::size_t>::max() / raster.component_count ||
        raster.pixels.size() != texel_count * raster.component_count ||
        !std::all_of(raster.pixels.begin(), raster.pixels.end(),
                     [](double value) { return std::isfinite(value); })) {
        throw std::invalid_argument("island padding raster is invalid");
    }
}

}  // namespace

SurfaceAdjacentSample sample_across_surface_adjacency(const SurfaceAdjacencyLink& link,
                                                      std::size_t source_texel,
                                                      std::int32_t offset_x, std::int32_t offset_y,
                                                      double weight) {
    validate_frame(link.first_frame);
    validate_frame(link.second_frame);
    if (link.first_texel == link.second_texel) {
        throw std::invalid_argument("surface adjacency must connect distinct texels");
    }
    if (source_texel == link.first_texel) {
        return {.texel_index = link.second_texel,
                .tangent_frame = link.second_frame,
                .offset_x = offset_x,
                .offset_y = offset_y,
                .weight = weight};
    }
    if (source_texel == link.second_texel) {
        return {.texel_index = link.first_texel,
                .tangent_frame = link.first_frame,
                .offset_x = offset_x,
                .offset_y = offset_y,
                .weight = weight};
    }
    throw std::invalid_argument("surface adjacency does not contain the source texel");
}

double filter_surface_scalar(std::span<const double> values, const SurfaceFilterRequest& request) {
    validate_request(values.size(), request);
    double result = 0.0;
    for (const SurfaceAdjacentSample& sample : request.samples) {
        if (!std::isfinite(values[sample.texel_index])) {
            throw std::invalid_argument("surface filter scalar value is not finite");
        }
        result += values[sample.texel_index] * sample.weight;
    }
    if (request.operation != SurfaceFilterOperation::derivative) {
        result /= weight_sum(request);
    }
    if (!std::isfinite(result)) {
        throw std::overflow_error("surface filter scalar result is not finite");
    }
    return result;
}

Vec3d filter_surface_tangent_vector(std::span<const Vec3d> values,
                                    const SurfaceFilterRequest& request) {
    validate_request(values.size(), request);
    Vec3d accumulated{};
    for (const SurfaceAdjacentSample& sample : request.samples) {
        const Vec3d value = values[sample.texel_index];
        if (!finite(value)) {
            throw std::invalid_argument("surface filter tangent vector is not finite");
        }
        const Vec3d transformed =
            world_to_tangent(tangent_to_world(value, sample.tangent_frame), request.output_frame);
        accumulated = add(accumulated, multiply(transformed, sample.weight));
    }
    const double magnitude = length(accumulated);
    if (!std::isfinite(magnitude) || magnitude <= frame_tolerance) {
        throw std::invalid_argument("surface filter tangent vector cannot be renormalized");
    }
    return multiply(accumulated, 1.0 / magnitude);
}

IslandPaddingPlan plan_island_padding(std::uint32_t width, std::uint32_t height,
                                      std::span<const std::uint32_t> island_identity,
                                      SamplingFootprint footprint,
                                      std::uint32_t requested_mip_levels) {
    const std::size_t texel_count = checked_texel_count(width, height);
    if (island_identity.size() != texel_count || requested_mip_levels == 0) {
        throw std::invalid_argument("island padding request is invalid");
    }
    const std::uint32_t padding_radius = required_radius(footprint, requested_mip_levels - 1);
    IslandPaddingPlan plan{.width = width,
                           .height = height,
                           .requested_mip_levels = requested_mip_levels,
                           .padding_radius = padding_radius,
                           .ownership = {island_identity.begin(), island_identity.end()},
                           .unsupported_mip_levels = {}};
    for (std::uint32_t mip = 0; mip < requested_mip_levels; ++mip) {
        const std::uint32_t radius = required_radius(footprint, mip);
        std::vector<std::uint32_t> conflicts =
            conflicting_islands(width, height, island_identity, radius);
        if (!conflicts.empty()) {
            plan.unsupported_mip_levels.push_back({.mip_level = mip,
                                                   .required_gutter_radius = radius,
                                                   .affected_islands = std::move(conflicts)});
        }
    }
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t index = texel_index({x, y}, width);
            if (island_identity[index] == no_uv_island) {
                plan.ownership[index] =
                    nearest_island({x, y}, width, height, island_identity, padding_radius);
            }
        }
    }
    return plan;
}

IslandPaddingResult apply_island_padding(const SeamDilationRaster& source,
                                         std::span<const std::uint32_t> island_identity,
                                         const IslandPaddingPlan& plan) {
    const std::size_t texel_count = checked_texel_count(plan.width, plan.height);
    validate_raster(source, texel_count);
    if (source.width != plan.width || source.height != plan.height ||
        island_identity.size() != texel_count || plan.ownership.size() != texel_count) {
        throw std::invalid_argument("island padding plan does not match the raster");
    }
    for (std::size_t texel = 0; texel < texel_count; ++texel) {
        if (island_identity[texel] != no_uv_island &&
            plan.ownership[texel] != island_identity[texel]) {
            throw std::invalid_argument("island padding plan changed valid island ownership");
        }
    }
    IslandPaddingResult result{.raster = source, .padded_texel_count = 0};
    for (std::uint32_t y = 0; y < plan.height; ++y) {
        for (std::uint32_t x = 0; x < plan.width; ++x) {
            const std::size_t destination = texel_index({x, y}, plan.width);
            const std::uint32_t owner = plan.ownership[destination];
            if (island_identity[destination] != no_uv_island || owner == no_uv_island) {
                continue;
            }
            const std::optional<GridPoint> source_point = nearest_source(
                {x, y}, plan.width, plan.height, island_identity, owner, plan.padding_radius);
            if (!source_point) {
                throw std::invalid_argument("island padding owner has no valid source texel");
            }
            const std::size_t source_index = texel_index(*source_point, plan.width);
            std::copy_n(source.pixels.begin() + source_index * source.component_count,
                        source.component_count,
                        result.raster.pixels.begin() + destination * source.component_count);
            ++result.padded_texel_count;
        }
    }
    return result;
}

}  // namespace ctex::paint
