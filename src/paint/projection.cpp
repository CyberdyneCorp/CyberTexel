#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/paint/projection.hpp>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace ctex::paint {
namespace {

constexpr double projection_epsilon = 1.0e-12;

struct Vec4d {
    double x;
    double y;
    double z;
    double w;
};

bool finite(Vec2d value) { return std::isfinite(value.x) && std::isfinite(value.y); }

bool finite(Vec3d value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finite(graph::ColourValue value) {
    return std::isfinite(value.r) && std::isfinite(value.g) && std::isfinite(value.b) &&
           std::isfinite(value.a);
}

std::size_t checked_area(std::uint32_t width, std::uint32_t height, std::string_view role) {
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / height) {
        throw std::invalid_argument(std::string(role) + " dimensions are invalid");
    }
    return static_cast<std::size_t>(width) * height;
}

std::size_t validate_surface(const CachedSurfaceMaps& surface) {
    const std::size_t count =
        checked_area(surface.surface.width, surface.surface.height, "projection surface");
    if (surface.surface.texels.size() != count || surface.coverage.size() != count) {
        throw std::invalid_argument("projection surface maps have inconsistent dimensions");
    }
    for (std::size_t texel = 0; texel < count; ++texel) {
        const SurfaceTexel& geometry = surface.surface.texels[texel];
        const double normal_length_squared = geometry.normal.x * geometry.normal.x +
                                             geometry.normal.y * geometry.normal.y +
                                             geometry.normal.z * geometry.normal.z;
        if (surface.coverage[texel] > 1 ||
            (surface.coverage[texel] != 0) != surface.surface.covered(texel)) {
            throw std::invalid_argument("projection surface coverage is invalid");
        }
        if (surface.coverage[texel] != 0 &&
            (!finite(geometry.position) || !finite(geometry.normal) ||
             !std::isfinite(normal_length_squared) ||
             normal_length_squared <= projection_epsilon)) {
            throw std::invalid_argument("covered projection surface geometry is invalid");
        }
    }
    return count;
}

void validate_material(const DecalMaterial& material) {
    const std::size_t count = checked_area(material.width, material.height, "projection material");
    if (material.opacity.size() != count || material.channels.empty() ||
        !std::all_of(material.opacity.begin(), material.opacity.end(), [](double value) {
            return std::isfinite(value) && value >= 0.0 && value <= 1.0;
        })) {
        throw std::invalid_argument("projection material opacity or channels are invalid");
    }
    std::set<std::string_view> identifiers;
    for (const PaintToolChannelRaster& channel : material.channels) {
        if (channel.semantic_id.empty() || channel.component_count == 0 ||
            channel.component_count > 4 || channel.pixels.size() != count ||
            !identifiers.insert(channel.semantic_id).second ||
            !std::all_of(channel.pixels.begin(), channel.pixels.end(),
                         [](graph::ColourValue value) { return finite(value); })) {
            throw std::invalid_argument("projection material channels are invalid or duplicated");
        }
    }
}

void validate_mask(PaintMaskView mask, std::size_t expected, std::string_view role) {
    if (mask.values.size() != expected ||
        !std::all_of(mask.values.begin(), mask.values.end(), [](double value) {
            return std::isfinite(value) && value >= 0.0 && value <= 1.0;
        })) {
        throw std::invalid_argument(std::string(role) + " is invalid");
    }
}

bool finite_matrix(const pick::Mat4f& matrix) {
    return std::all_of(matrix.values.begin(), matrix.values.end(),
                       [](float value) { return std::isfinite(value); });
}

std::size_t pivot_row(const pick::Mat4f& matrix, std::size_t column) {
    std::size_t pivot = column;
    for (std::size_t row = column + 1; row < 4; ++row) {
        if (std::abs(matrix.values[column * 4 + row]) >
            std::abs(matrix.values[column * 4 + pivot])) {
            pivot = row;
        }
    }
    return pivot;
}

void swap_rows(pick::Mat4f& matrix, std::size_t first, std::size_t second) {
    if (first == second) {
        return;
    }
    for (std::size_t column = 0; column < 4; ++column) {
        std::swap(matrix.values[column * 4 + first], matrix.values[column * 4 + second]);
    }
}

void eliminate_below(pick::Mat4f& matrix, std::size_t column, double pivot) {
    for (std::size_t row = column + 1; row < 4; ++row) {
        const double factor = matrix.values[column * 4 + row] / pivot;
        for (std::size_t entry = column + 1; entry < 4; ++entry) {
            matrix.values[entry * 4 + row] = static_cast<float>(
                matrix.values[entry * 4 + row] - factor * matrix.values[entry * 4 + column]);
        }
    }
}

bool invertible(pick::Mat4f matrix) {
    for (std::size_t column = 0; column < 4; ++column) {
        const std::size_t selected = pivot_row(matrix, column);
        const double pivot_value = matrix.values[column * 4 + selected];
        if (std::abs(pivot_value) <= projection_epsilon) {
            return false;
        }
        swap_rows(matrix, column, selected);
        eliminate_below(matrix, column, pivot_value);
    }
    return true;
}

Vec4d transform(const pick::Mat4f& matrix, Vec3d point) {
    const std::array<double, 4> input{point.x, point.y, point.z, 1.0};
    std::array<double, 4> output{};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            output[row] += matrix.values[column * 4 + row] * input[column];
        }
    }
    return {output[0], output[1], output[2], output[3]};
}

std::size_t image_index(std::uint32_t width, std::uint32_t height, Vec2d uv) {
    if (!finite(uv) || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return no_projection_sample;
    }
    const auto x = std::min(static_cast<std::uint32_t>(std::floor(uv.x * width)), width - 1);
    const auto bottom_up_y =
        std::min(static_cast<std::uint32_t>(std::floor(uv.y * height)), height - 1);
    return static_cast<std::size_t>(height - 1 - bottom_up_y) * width + x;
}

Vec2d camera_coordinate(Vec3d position, const pick::Mat4f& view_projection) {
    const Vec4d clip = transform(view_projection, position);
    if (!std::isfinite(clip.x) || !std::isfinite(clip.y) || !std::isfinite(clip.z) ||
        !std::isfinite(clip.w) || clip.w <= projection_epsilon || clip.x < -clip.w ||
        clip.x > clip.w || clip.y < -clip.w || clip.y > clip.w || clip.z < -clip.w ||
        clip.z > clip.w) {
        return {-1.0, -1.0};
    }
    return {clip.x / clip.w * 0.5 + 0.5, clip.y / clip.w * 0.5 + 0.5};
}

ProjectionSample one_sample(std::size_t index) {
    ProjectionSample result;
    if (index != no_projection_sample) {
        result.source_indices[0] = index;
        result.weights[0] = 1.0;
        result.count = 1;
    }
    return result;
}

std::vector<ProjectionSample> camera_samples(const CachedSurfaceMaps& surface,
                                             const DecalMaterial& material,
                                             const CameraProjection& mapping) {
    const std::size_t count = surface.surface.texels.size();
    validate_mask(mapping.visible_surface, count, "camera visible-surface mask");
    if (!finite_matrix(mapping.view_projection) || !invertible(mapping.view_projection)) {
        throw std::invalid_argument("camera view-projection matrix must be finite and invertible");
    }
    std::vector<ProjectionSample> result(count);
    for (std::size_t texel = 0; texel < count; ++texel) {
        if (surface.coverage[texel] != 0) {
            result[texel] =
                one_sample(image_index(material.width, material.height,
                                       camera_coordinate(surface.surface.texels[texel].position,
                                                         mapping.view_projection)));
        }
    }
    return result;
}

std::vector<ProjectionSample> planar_samples(const CachedSurfaceMaps& surface,
                                             const DecalMaterial& material,
                                             const PlanarProjection& mapping) {
    if (!finite(mapping.extent) || mapping.extent.x <= 0.0 || mapping.extent.y <= 0.0) {
        throw std::invalid_argument("planar projection extent must be finite and positive");
    }
    std::vector<ProjectionSample> result(surface.surface.texels.size());
    const MaterialCoordinateRequest request{.mode = MaterialCoordinateMode::planar,
                                            .planar = mapping.frame};
    for (std::size_t texel = 0; texel < result.size(); ++texel) {
        if (surface.coverage[texel] == 0) {
            continue;
        }
        const Vec2d coordinate =
            material_coordinates(surface.surface.texels[texel], request).projections[0].coordinate;
        const Vec2d uv{coordinate.x / mapping.extent.x + 0.5,
                       coordinate.y / mapping.extent.y + 0.5};
        result[texel] = one_sample(image_index(material.width, material.height, uv));
    }
    return result;
}

double repeated(double value) { return value - std::floor(value); }

std::vector<ProjectionSample> triplanar_samples(const CachedSurfaceMaps& surface,
                                                const DecalMaterial& material,
                                                const TriplanarProjection& mapping) {
    if (!std::isfinite(mapping.scale) || mapping.scale <= 0.0 || !finite(mapping.offset)) {
        throw std::invalid_argument("triplanar projection scale and offset are invalid");
    }
    std::vector<ProjectionSample> result(surface.surface.texels.size());
    const MaterialCoordinateRequest request{.mode = MaterialCoordinateMode::triplanar,
                                            .planar = {}};
    for (std::size_t texel = 0; texel < result.size(); ++texel) {
        if (surface.coverage[texel] == 0) {
            continue;
        }
        const MaterialCoordinates coordinates =
            material_coordinates(surface.surface.texels[texel], request);
        result[texel].count = coordinates.count;
        for (std::size_t plane = 0; plane < coordinates.count; ++plane) {
            const Vec2d source = coordinates.projections[plane].coordinate;
            const Vec2d uv{repeated(source.x * mapping.scale + mapping.offset.x),
                           repeated(source.y * mapping.scale + mapping.offset.y)};
            result[texel].source_indices[plane] = image_index(material.width, material.height, uv);
            result[texel].weights[plane] = coordinates.projections[plane].weight;
        }
    }
    return result;
}

std::vector<ProjectionSample> resolve_samples(const CachedSurfaceMaps& surface,
                                              const DecalMaterial& material,
                                              const ProjectionMapping& mapping) {
    return std::visit(
        [&](const auto& value) -> std::vector<ProjectionSample> {
            using Mapping = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Mapping, CameraProjection>) {
                return camera_samples(surface, material, value);
            } else if constexpr (std::is_same_v<Mapping, PlanarProjection>) {
                return planar_samples(surface, material, value);
            } else {
                return triplanar_samples(surface, material, value);
            }
        },
        mapping);
}

std::vector<double> sample_strength(const DecalMaterial& material,
                                    std::span<const ProjectionSample> samples) {
    std::vector<double> result(samples.size(), 0.0);
    for (std::size_t texel = 0; texel < samples.size(); ++texel) {
        for (std::size_t sample = 0; sample < samples[texel].count; ++sample) {
            const std::size_t source = samples[texel].source_indices[sample];
            if (source != no_projection_sample) {
                result[texel] += samples[texel].weights[sample] * material.opacity[source];
            }
        }
    }
    return result;
}

graph::ColourValue weighted_pixel(const PaintToolChannelRaster& channel,
                                  const DecalMaterial& material, const ProjectionSample& samples,
                                  double opacity) {
    if (opacity <= projection_epsilon) {
        return {};
    }
    graph::ColourValue result{};
    for (std::size_t sample = 0; sample < samples.count; ++sample) {
        const std::size_t source = samples.source_indices[sample];
        if (source == no_projection_sample) {
            continue;
        }
        const double weight = samples.weights[sample] * material.opacity[source] / opacity;
        const graph::ColourValue value = channel.pixels[source];
        result.r += static_cast<float>(weight * value.r);
        result.g += static_cast<float>(weight * value.g);
        result.b += static_cast<float>(weight * value.b);
        result.a += static_cast<float>(weight * value.a);
    }
    return result;
}

std::vector<PaintToolChannelRaster> sample_material(const DecalMaterial& material,
                                                    std::span<const ProjectionSample> samples,
                                                    std::span<const double> opacity) {
    std::vector<PaintToolChannelRaster> result;
    result.reserve(material.channels.size());
    for (const PaintToolChannelRaster& channel : material.channels) {
        PaintToolChannelRaster sampled{.semantic_id = channel.semantic_id,
                                       .component_count = channel.component_count,
                                       .pixels = std::vector<graph::ColourValue>(samples.size())};
        for (std::size_t texel = 0; texel < samples.size(); ++texel) {
            sampled.pixels[texel] =
                weighted_pixel(channel, material, samples[texel], opacity[texel]);
        }
        result.push_back(std::move(sampled));
    }
    return result;
}

void multiply_mask(std::span<double> values, PaintMaskView mask, std::string_view role) {
    validate_mask(mask, values.size(), role);
    for (std::size_t texel = 0; texel < values.size(); ++texel) {
        values[texel] *= mask.values[texel];
    }
}

}  // namespace

ProjectionResult apply_projection(const CachedSurfaceMaps& surface,
                                  std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
                                  const DecalMaterial& material,
                                  const ProjectionSettings& settings) {
    static_cast<void>(validate_surface(surface));
    validate_material(material);
    std::vector<ProjectionSample> samples = resolve_samples(surface, material, settings.mapping);
    std::vector<double> strength = sample_strength(material, samples);
    std::vector<PaintToolChannelRaster> sampled = sample_material(material, samples, strength);
    const CombinedPaintMask masks =
        combine_paint_masks(surface.surface.width, surface.surface.height, settings.masks);
    multiply_mask(strength, PaintMaskView{masks.values}, "projection masks");
    if (settings.rejection_acceptance) {
        multiply_mask(strength, *settings.rejection_acceptance, "projection rejection acceptance");
    }
    if (const auto* camera = std::get_if<CameraProjection>(&settings.mapping)) {
        multiply_mask(strength, camera->visible_surface, "camera visible-surface mask");
    }
    PaintToolShadeResult shaded =
        shade_paint_tool_channels(surface.surface.width, surface.surface.height,
                                  enabled_layer_snapshot, sampled, strength, settings.blend_mode);
    return {.width = surface.surface.width,
            .height = surface.surface.height,
            .samples = std::move(samples),
            .strength = std::move(strength),
            .sampled_material = std::move(sampled),
            .channels = std::move(shaded.channels),
            .applied_channel_ids = std::move(shaded.applied_channel_ids)};
}

}  // namespace ctex::paint
