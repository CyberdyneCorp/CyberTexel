#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/paint/blur_smear.hpp>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace ctex::paint {
namespace {

constexpr std::string_view normal_channel_id = "pbr.normal";
constexpr double normal_epsilon = 1.0e-12;

std::size_t checked_texel_count(const RejectedCoverageRaster& rejected) {
    const std::uint32_t width = rejected.coverage.width;
    const std::uint32_t height = rejected.coverage.height;
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / height) {
        throw std::invalid_argument("blur/smear dimensions are invalid");
    }
    return static_cast<std::size_t>(width) * height;
}

bool finite(graph::ColourValue value) {
    return std::isfinite(value.r) && std::isfinite(value.g) && std::isfinite(value.b) &&
           std::isfinite(value.a);
}

void validate_snapshot(std::span<const PaintToolChannelRaster> snapshot, std::size_t texel_count) {
    if (snapshot.empty()) {
        throw std::invalid_argument("blur/smear requires an enabled layer snapshot");
    }
    for (const PaintToolChannelRaster& channel : snapshot) {
        if (channel.pixels.size() != texel_count ||
            !std::all_of(channel.pixels.begin(), channel.pixels.end(), finite)) {
            throw std::invalid_argument("blur/smear stroke-start snapshot is invalid");
        }
        if (channel.semantic_id == normal_channel_id && channel.component_count != 3) {
            throw std::invalid_argument("pbr.normal must contain three components");
        }
    }
}

double component(graph::ColourValue value, std::size_t index) {
    switch (index) {
        case 0:
            return value.r;
        case 1:
            return value.g;
        case 2:
            return value.b;
        default:
            return value.a;
    }
}

void set_component(graph::ColourValue& value, std::size_t index, double component_value) {
    const float narrowed = static_cast<float>(component_value);
    switch (index) {
        case 0:
            value.r = narrowed;
            break;
        case 1:
            value.g = narrowed;
            break;
        case 2:
            value.b = narrowed;
            break;
        default:
            value.a = narrowed;
            break;
    }
}

Vec3d normalized(Vec3d value) {
    const double magnitude = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    if (!std::isfinite(magnitude) || magnitude <= normal_epsilon) {
        throw std::invalid_argument("filtered tangent-space normal cannot be normalized");
    }
    return {value.x / magnitude, value.y / magnitude, value.z / magnitude};
}

Vec3d decode_normal(graph::ColourValue value) {
    return normalized({static_cast<double>(value.r) * 2.0 - 1.0,
                       static_cast<double>(value.g) * 2.0 - 1.0,
                       static_cast<double>(value.b) * 2.0 - 1.0});
}

void encode_normal(graph::ColourValue& pixel, Vec3d value) {
    const Vec3d unit = normalized(value);
    pixel.r = static_cast<float>(unit.x * 0.5 + 0.5);
    pixel.g = static_cast<float>(unit.y * 0.5 + 0.5);
    pixel.b = static_cast<float>(unit.z * 0.5 + 0.5);
}

std::vector<double> channel_component(std::span<const graph::ColourValue> pixels,
                                      std::size_t component_index) {
    std::vector<double> result;
    result.reserve(pixels.size());
    for (const graph::ColourValue pixel : pixels) {
        result.push_back(component(pixel, component_index));
    }
    return result;
}

std::vector<Vec3d> channel_normals(std::span<const graph::ColourValue> pixels) {
    std::vector<Vec3d> result;
    result.reserve(pixels.size());
    for (const graph::ColourValue pixel : pixels) {
        result.push_back(decode_normal(pixel));
    }
    return result;
}

std::span<const SurfaceAdjacentSample> blur_samples(const BlurNeighborhood& neighborhood,
                                                    bool horizontal) {
    return horizontal ? std::span<const SurfaceAdjacentSample>(neighborhood.horizontal_samples)
                      : std::span<const SurfaceAdjacentSample>(neighborhood.vertical_samples);
}

SamplingFootprint blur_footprint(std::uint32_t radius, bool horizontal) {
    return horizontal ? SamplingFootprint{.radius_x = radius, .radius_y = 0}
                      : SamplingFootprint{.radius_x = 0, .radius_y = radius};
}

std::vector<double> blur_scalar_pass(std::span<const double> source,
                                     std::span<const BlurNeighborhood> neighborhoods,
                                     std::uint32_t radius, bool horizontal) {
    std::vector<double> result(source.size());
    for (std::size_t texel = 0; texel < source.size(); ++texel) {
        result[texel] = filter_surface_scalar(
            source, {.operation = SurfaceFilterOperation::blur,
                     .footprint = blur_footprint(radius, horizontal),
                     .output_frame = neighborhoods[texel].output_frame,
                     .samples = blur_samples(neighborhoods[texel], horizontal)});
    }
    return result;
}

std::vector<Vec3d> blur_normal_pass(std::span<const Vec3d> source,
                                    std::span<const BlurNeighborhood> neighborhoods,
                                    std::uint32_t radius, bool horizontal) {
    std::vector<Vec3d> result(source.size());
    for (std::size_t texel = 0; texel < source.size(); ++texel) {
        result[texel] = filter_surface_tangent_vector(
            source, {.operation = SurfaceFilterOperation::blur,
                     .footprint = blur_footprint(radius, horizontal),
                     .output_frame = neighborhoods[texel].output_frame,
                     .samples = blur_samples(neighborhoods[texel], horizontal)});
    }
    return result;
}

PaintToolChannelRaster blur_channel(const PaintToolChannelRaster& source,
                                    std::span<const BlurNeighborhood> neighborhoods,
                                    std::uint32_t radius) {
    PaintToolChannelRaster result = source;
    for (std::size_t component_index = 0; component_index < 4; ++component_index) {
        if (source.semantic_id == normal_channel_id && component_index < 3) {
            continue;
        }
        const std::vector<double> values = channel_component(source.pixels, component_index);
        const std::vector<double> horizontal =
            blur_scalar_pass(values, neighborhoods, radius, true);
        const std::vector<double> vertical =
            blur_scalar_pass(horizontal, neighborhoods, radius, false);
        for (std::size_t texel = 0; texel < vertical.size(); ++texel) {
            set_component(result.pixels[texel], component_index, vertical[texel]);
        }
    }
    if (source.semantic_id == normal_channel_id) {
        const std::vector<Vec3d> horizontal =
            blur_normal_pass(channel_normals(source.pixels), neighborhoods, radius, true);
        const std::vector<Vec3d> vertical =
            blur_normal_pass(horizontal, neighborhoods, radius, false);
        for (std::size_t texel = 0; texel < vertical.size(); ++texel) {
            encode_normal(result.pixels[texel], vertical[texel]);
        }
    }
    return result;
}

std::vector<PaintToolChannelRaster> blur_snapshot(std::span<const PaintToolChannelRaster> source,
                                                  std::span<const BlurNeighborhood> neighborhoods,
                                                  std::uint32_t radius) {
    std::vector<PaintToolChannelRaster> result;
    result.reserve(source.size());
    for (const PaintToolChannelRaster& channel : source) {
        result.push_back(blur_channel(channel, neighborhoods, radius));
    }
    return result;
}

SurfaceFilterRequest smear_request(const SmearMapping& mapping, SamplingFootprint footprint) {
    return {.operation = SurfaceFilterOperation::smear,
            .footprint = footprint,
            .output_frame = mapping.output_frame,
            .samples = {&mapping.upstream_sample, 1}};
}

PaintToolChannelRaster smear_channel(const PaintToolChannelRaster& source,
                                     std::span<const SmearMapping> mappings,
                                     SamplingFootprint footprint) {
    PaintToolChannelRaster result = source;
    const std::array<std::vector<double>, 4> components{
        channel_component(source.pixels, 0), channel_component(source.pixels, 1),
        channel_component(source.pixels, 2), channel_component(source.pixels, 3)};
    const std::vector<Vec3d> normals = source.semantic_id == normal_channel_id
                                           ? channel_normals(source.pixels)
                                           : std::vector<Vec3d>{};
    for (std::size_t texel = 0; texel < source.pixels.size(); ++texel) {
        const SurfaceFilterRequest request = smear_request(mappings[texel], footprint);
        for (std::size_t component_index = 0; component_index < 4; ++component_index) {
            if (source.semantic_id != normal_channel_id || component_index == 3) {
                set_component(result.pixels[texel], component_index,
                              filter_surface_scalar(components[component_index], request));
            }
        }
        if (source.semantic_id == normal_channel_id) {
            encode_normal(result.pixels[texel], filter_surface_tangent_vector(normals, request));
        }
    }
    return result;
}

std::vector<PaintToolChannelRaster> smear_snapshot(std::span<const PaintToolChannelRaster> source,
                                                   std::span<const SmearMapping> mappings,
                                                   SamplingFootprint footprint) {
    std::vector<PaintToolChannelRaster> result;
    result.reserve(source.size());
    for (const PaintToolChannelRaster& channel : source) {
        result.push_back(smear_channel(channel, mappings, footprint));
    }
    return result;
}

DepositionRaster tool_deposition(const ResolvedStroke& stroke,
                                 const RejectedCoverageRaster& rejected,
                                 const PaintMaskInputs& masks, DepositionMode mode) {
    return evaluate_deposition(stroke, apply_paint_masks(rejected, masks), mode);
}

void normalize_output_normals(std::span<PaintToolChannelRaster> channels,
                              std::span<const double> strength) {
    for (PaintToolChannelRaster& channel : channels) {
        if (channel.semantic_id != normal_channel_id) {
            continue;
        }
        for (std::size_t texel = 0; texel < channel.pixels.size(); ++texel) {
            if (strength[texel] > 0.0) {
                encode_normal(channel.pixels[texel], decode_normal(channel.pixels[texel]));
            }
        }
    }
}

}  // namespace

BlurResult apply_blur(const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
                      std::span<const PaintToolChannelRaster> enabled_layer_stroke_start_snapshot,
                      const BlurSettings& settings) {
    const std::size_t texel_count = checked_texel_count(rejected);
    ToolParameterReport parameter_report;
    const auto radius = static_cast<std::uint32_t>(
        validate_tool_parameter(blur_radius_parameter, settings.radius, parameter_report));
    if (settings.neighborhoods.size() != texel_count) {
        throw std::invalid_argument("blur neighborhood count is invalid");
    }
    validate_snapshot(enabled_layer_stroke_start_snapshot, texel_count);
    DepositionRaster deposition =
        tool_deposition(stroke, rejected, settings.masks, settings.deposition_mode);
    std::vector<PaintToolChannelRaster> filtered =
        blur_snapshot(enabled_layer_stroke_start_snapshot, settings.neighborhoods, radius);
    PaintToolShadeResult shaded = shade_paint_tool_channels(
        rejected.coverage.width, rejected.coverage.height, enabled_layer_stroke_start_snapshot,
        filtered, deposition.strength, settings.blend_mode);
    normalize_output_normals(shaded.channels, deposition.strength);
    return {.width = rejected.coverage.width,
            .height = rejected.coverage.height,
            .parameter_report = std::move(parameter_report),
            .footprint = {.radius_x = radius, .radius_y = radius},
            .deposition = std::move(deposition),
            .filtered_snapshot = std::move(filtered),
            .channels = std::move(shaded.channels),
            .applied_channel_ids = std::move(shaded.applied_channel_ids)};
}

SmearResult apply_smear(const ResolvedStroke& stroke, const RejectedCoverageRaster& rejected,
                        std::span<const PaintToolChannelRaster> enabled_layer_stroke_start_snapshot,
                        const SmearSettings& settings) {
    const std::size_t texel_count = checked_texel_count(rejected);
    ToolParameterReport parameter_report;
    const double strength =
        validate_tool_parameter(smear_strength_parameter, settings.strength, parameter_report);
    const SamplingFootprint footprint{
        .radius_x = static_cast<std::uint32_t>(validate_tool_parameter(
            smear_footprint_radius_x_parameter, settings.footprint.radius_x, parameter_report)),
        .radius_y = static_cast<std::uint32_t>(validate_tool_parameter(
            smear_footprint_radius_y_parameter, settings.footprint.radius_y, parameter_report))};
    if ((footprint.radius_x == 0 && footprint.radius_y == 0) ||
        settings.mappings.size() != texel_count) {
        throw std::invalid_argument("smear footprint or mapping count is invalid");
    }
    validate_snapshot(enabled_layer_stroke_start_snapshot, texel_count);
    DepositionRaster deposition =
        tool_deposition(stroke, rejected, settings.masks, settings.deposition_mode);
    std::vector<double> effective_strength = deposition.strength;
    for (double& value : effective_strength) {
        value *= strength;
    }
    std::vector<PaintToolChannelRaster> dragged =
        smear_snapshot(enabled_layer_stroke_start_snapshot, settings.mappings, footprint);
    PaintToolShadeResult shaded = shade_paint_tool_channels(
        rejected.coverage.width, rejected.coverage.height, enabled_layer_stroke_start_snapshot,
        dragged, effective_strength, settings.blend_mode);
    normalize_output_normals(shaded.channels, effective_strength);
    return {.width = rejected.coverage.width,
            .height = rejected.coverage.height,
            .parameter_report = std::move(parameter_report),
            .footprint = footprint,
            .strength = strength,
            .deposition = std::move(deposition),
            .effective_strength = std::move(effective_strength),
            .dragged_snapshot = std::move(dragged),
            .channels = std::move(shaded.channels),
            .applied_channel_ids = std::move(shaded.applied_channel_ids)};
}

}  // namespace ctex::paint
