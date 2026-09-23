#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <ctex/maps/generators.hpp>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

namespace ctex::maps {
namespace {

constexpr std::array ao_maps{MeshMapKind::ambient_occlusion};
constexpr std::array curvature_maps{MeshMapKind::curvature};
constexpr std::array thickness_maps{MeshMapKind::thickness};
constexpr std::array position_maps{MeshMapKind::position};
constexpr std::array direction_maps{MeshMapKind::world_space_direction};
constexpr std::array dirt_maps{MeshMapKind::ambient_occlusion, MeshMapKind::curvature};
constexpr std::array scratches_maps{MeshMapKind::position, MeshMapKind::world_space_direction};

constexpr MeshMapGeneratorParameterDescriptor strength{
    .name = "strength",
    .default_value = 1.0,
    .minimum = 0.0,
    .maximum = 2.0,
    .meaning = "Multiplier applied to the generated mask"};
constexpr MeshMapGeneratorParameterDescriptor contrast{
    .name = "contrast",
    .default_value = 1.0,
    .minimum = 0.1,
    .maximum = 4.0,
    .meaning = "Positive power applied before strength"};
constexpr MeshMapGeneratorParameterDescriptor curvature_weight{
    .name = "curvature-weight",
    .default_value = 1.0,
    .minimum = 0.0,
    .maximum = 1.0,
    .meaning = "Contribution of concave curvature to dirt"};
constexpr MeshMapGeneratorParameterDescriptor edge_threshold{
    .name = "threshold",
    .default_value = 0.5,
    .minimum = 0.0,
    .maximum = 0.99,
    .meaning = "Encoded curvature at which edge wear begins"};
constexpr MeshMapGeneratorParameterDescriptor scratch_scale{
    .name = "scale",
    .default_value = 1.0,
    .minimum = 1.0,
    .maximum = 128.0,
    .meaning = "World-position frequency of scratch lines"};
constexpr MeshMapGeneratorParameterDescriptor scratch_width{
    .name = "width",
    .default_value = 1.0 / 24.0,
    .minimum = 0.001,
    .maximum = 0.25,
    .meaning = "Half-width of each scratch line in phase space"};

constexpr std::array common_parameters{strength, contrast};
constexpr std::array dirt_parameters{strength, contrast, curvature_weight};
constexpr std::array edge_parameters{strength, contrast, edge_threshold};
constexpr std::array scratches_parameters{strength, contrast, scratch_scale, scratch_width};

struct EvaluationParameters {
    double strength{1.0};
    double contrast{1.0};
    double curvature_weight{1.0};
    double edge_threshold{0.5};
    double scratch_scale{1.0};
    double scratch_width{1.0 / 24.0};
};

/// The maps one generator reads, each resolved and validated once.
///
/// `MeshMapSet::sample` resolves the map, revalidates its tangent binding and
/// recomputes its staleness per call. Those are loop invariants for a dense
/// sweep, so a generator binds what it needs before the loop and samples
/// through the bound samplers.
struct BoundMaps {
    std::optional<MeshMapSet::BoundSampler> ambient_occlusion;
    std::optional<MeshMapSet::BoundSampler> curvature;
    std::optional<MeshMapSet::BoundSampler> thickness;
    std::optional<MeshMapSet::BoundSampler> position;
    std::optional<MeshMapSet::BoundSampler> direction;
};

BoundMaps bind_maps(MeshMapGeneratorKind kind, const MeshMapSet& maps) {
    BoundMaps bound;
    for (MeshMapKind required : mesh_map_generator_info(kind).required_maps) {
        switch (required) {
            case MeshMapKind::ambient_occlusion:
                bound.ambient_occlusion = maps.bind_sampler(required);
                break;
            case MeshMapKind::curvature:
                bound.curvature = maps.bind_sampler(required);
                break;
            case MeshMapKind::thickness:
                bound.thickness = maps.bind_sampler(required);
                break;
            case MeshMapKind::position:
                bound.position = maps.bind_sampler(required);
                break;
            case MeshMapKind::world_space_direction:
                bound.direction = maps.bind_sampler(required);
                break;
            default:
                throw std::invalid_argument("generator requires a map it cannot bind");
        }
    }
    return bound;
}

double component(const std::optional<MeshMapSet::BoundSampler>& sampler, double u, double v,
                 std::size_t index = 0) {
    return sampler->at(u, v).values[index];
}

double saturate(double value) { return std::clamp(value, 0.0, 1.0); }

double scratches_value(const BoundMaps& maps, double u, double v,
                       const EvaluationParameters& parameters) {
    const MeshMapSample position = maps.position->at(u, v);
    const MeshMapSample direction = maps.direction->at(u, v);
    const double phase =
        (position.values[0] * 31.0 + position.values[1] * 7.0 + position.values[2] * 13.0) *
        parameters.scratch_scale;
    const double distance_to_line = std::abs((phase - std::floor(phase)) - 0.5);
    const double line = saturate(1.0 - distance_to_line / parameters.scratch_width);
    const double grazing = 1.0 - std::abs(direction.values[2] * 2.0 - 1.0);
    return line * saturate(grazing);
}

double baseline_value(MeshMapGeneratorKind kind, const BoundMaps& maps, double u, double v,
                      const EvaluationParameters& parameters) {
    switch (kind) {
        case MeshMapGeneratorKind::ambient_occlusion:
            return saturate(1.0 - component(maps.ambient_occlusion, u, v));
        case MeshMapGeneratorKind::curvature:
            return saturate(component(maps.curvature, u, v));
        case MeshMapGeneratorKind::thickness:
            return saturate(1.0 - component(maps.thickness, u, v));
        case MeshMapGeneratorKind::position_gradient:
            return saturate(component(maps.position, u, v, 1));
        case MeshMapGeneratorKind::world_space_direction:
            return saturate(component(maps.direction, u, v, 1));
        case MeshMapGeneratorKind::dirt: {
            const double occlusion = 1.0 - component(maps.ambient_occlusion, u, v);
            const double concavity =
                (0.5 - component(maps.curvature, u, v)) * 2.0 * parameters.curvature_weight;
            return saturate(std::max(occlusion, concavity));
        }
        case MeshMapGeneratorKind::edge_wear: {
            const double curvature = component(maps.curvature, u, v);
            return saturate((curvature - parameters.edge_threshold) /
                            (1.0 - parameters.edge_threshold));
        }
        case MeshMapGeneratorKind::scratches:
            return scratches_value(maps, u, v, parameters);
    }
    throw std::invalid_argument("mesh-map generator kind is invalid");
}

double generator_value(MeshMapGeneratorKind kind, const BoundMaps& maps, double u, double v,
                       const EvaluationParameters& parameters) {
    const double baseline = saturate(baseline_value(kind, maps, u, v, parameters));
    // pow(x, 1.0) is x, and contrast defaults to 1.0. Taking the branch keeps a
    // transcendental out of the per-texel path in the common case; the result is
    // bit-identical because std::pow(x, 1.0) is exact.
    const double shaped =
        parameters.contrast == 1.0 ? baseline : std::pow(baseline, parameters.contrast);
    return saturate(shaped * parameters.strength);
}

MeshMapGeneratorParameterReport resolve_parameters(
    const MeshMapGeneratorInfo& info, std::span<const MeshMapGeneratorParameter> supplied) {
    std::set<std::string_view> names;
    for (const MeshMapGeneratorParameter& parameter : supplied) {
        if (parameter.name.empty() || !names.insert(parameter.name).second) {
            throw std::invalid_argument("generator parameters contain an empty or duplicate name");
        }
        if (!std::isfinite(parameter.value)) {
            throw std::invalid_argument("generator parameter '" + std::string(parameter.name) +
                                        "' must be finite");
        }
        if (std::ranges::find(info.parameters, parameter.name,
                              &MeshMapGeneratorParameterDescriptor::name) ==
            info.parameters.end()) {
            throw std::invalid_argument("generator '" + std::string(info.name) +
                                        "' has no parameter '" + std::string(parameter.name) + "'");
        }
    }
    MeshMapGeneratorParameterReport report;
    report.resolved.reserve(info.parameters.size());
    for (const MeshMapGeneratorParameterDescriptor& descriptor : info.parameters) {
        const auto found =
            std::ranges::find(supplied, descriptor.name, &MeshMapGeneratorParameter::name);
        const double value = found == supplied.end() ? descriptor.default_value : found->value;
        const double resolved = std::clamp(value, descriptor.minimum, descriptor.maximum);
        report.resolved.push_back({.name = std::string(descriptor.name), .value = resolved});
        if (resolved != value) {
            report.clamps.push_back(
                {.name = std::string(descriptor.name), .supplied = value, .resolved = resolved});
        }
    }
    return report;
}

double resolved_value(const MeshMapGeneratorParameterReport& report, std::string_view name,
                      double fallback) {
    return report.value_for(name).value_or(fallback);
}

EvaluationParameters evaluation_parameters(const MeshMapGeneratorParameterReport& report) {
    return {.strength = resolved_value(report, "strength", 1.0),
            .contrast = resolved_value(report, "contrast", 1.0),
            .curvature_weight = resolved_value(report, "curvature-weight", 1.0),
            .edge_threshold = resolved_value(report, "threshold", 0.5),
            .scratch_scale = resolved_value(report, "scale", 1.0),
            .scratch_width = resolved_value(report, "width", 1.0 / 24.0)};
}

void encode_float(std::byte* target, double value) {
    const float encoded = static_cast<float>(value);
    std::memcpy(target, &encoded, sizeof(encoded));
}

}  // namespace

std::optional<double> MeshMapGeneratorParameterReport::value_for(
    std::string_view name) const noexcept {
    const auto found = std::ranges::find(resolved, name, &MeshMapGeneratorResolvedParameter::name);
    return found == resolved.end() ? std::nullopt : std::optional<double>(found->value);
}

std::optional<MeshMapGeneratorParameterClamp> MeshMapGeneratorParameterReport::clamp_for(
    std::string_view name) const {
    const auto found = std::ranges::find(clamps, name, &MeshMapGeneratorParameterClamp::name);
    return found == clamps.end() ? std::nullopt
                                 : std::optional<MeshMapGeneratorParameterClamp>(*found);
}

MeshMapGeneratorInfo mesh_map_generator_info(MeshMapGeneratorKind kind) {
    switch (kind) {
        case MeshMapGeneratorKind::ambient_occlusion:
            return {.kind = kind,
                    .name = "ambient-occlusion",
                    .required_maps = ao_maps,
                    .parameters = common_parameters};
        case MeshMapGeneratorKind::curvature:
            return {.kind = kind,
                    .name = "curvature",
                    .required_maps = curvature_maps,
                    .parameters = common_parameters};
        case MeshMapGeneratorKind::thickness:
            return {.kind = kind,
                    .name = "thickness",
                    .required_maps = thickness_maps,
                    .parameters = common_parameters};
        case MeshMapGeneratorKind::position_gradient:
            return {.kind = kind,
                    .name = "position-gradient",
                    .required_maps = position_maps,
                    .parameters = common_parameters};
        case MeshMapGeneratorKind::world_space_direction:
            return {.kind = kind,
                    .name = "world-space-direction",
                    .required_maps = direction_maps,
                    .parameters = common_parameters};
        case MeshMapGeneratorKind::dirt:
            return {.kind = kind,
                    .name = "dirt",
                    .required_maps = dirt_maps,
                    .parameters = dirt_parameters};
        case MeshMapGeneratorKind::edge_wear:
            return {.kind = kind,
                    .name = "edge-wear",
                    .required_maps = curvature_maps,
                    .parameters = edge_parameters};
        case MeshMapGeneratorKind::scratches:
            return {.kind = kind,
                    .name = "scratches",
                    .required_maps = scratches_maps,
                    .parameters = scratches_parameters};
    }
    throw std::invalid_argument("mesh-map generator kind is invalid");
}

MeshMapGeneratorResult generate_mesh_map_mask(
    MeshMapGeneratorKind kind, const MeshMapSet& maps, std::uint32_t width, std::uint32_t height,
    std::span<const MeshMapGeneratorParameter> parameters) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("mesh-map generator output dimensions must be non-zero");
    }
    const MeshMapGeneratorInfo info = mesh_map_generator_info(kind);
    MeshMapGeneratorParameterReport parameter_report = resolve_parameters(info, parameters);
    MeshMapRequirementReport report = maps.require_maps(info.name, info.required_maps);
    const EvaluationParameters evaluation = evaluation_parameters(parameter_report);
    auto mask = std::make_shared<image::TiledImage>(
        width, height,
        image::PixelFormat{.channel_type = image::ChannelType::float32, .channel_count = 1});
    const BoundMaps bound = bind_maps(kind, maps);
    // One tile at a time: filling densely through write_pixel would publish a
    // revision and a change-index update for every texel.
    std::vector<std::byte> tile_pixels;
    for (std::uint32_t tile_y = 0; tile_y < mask->tile_rows(); ++tile_y) {
        for (std::uint32_t tile_x = 0; tile_x < mask->tile_columns(); ++tile_x) {
            const image::TileCoordinate tile{tile_x, tile_y};
            const image::TileExtent extent = mask->tile_extent(tile);
            const std::uint32_t origin_x = tile_x * mask->tile_size();
            const std::uint32_t origin_y = tile_y * mask->tile_size();
            tile_pixels.assign(
                static_cast<std::size_t>(extent.width) * extent.height * mask->pixel_bytes(),
                std::byte{});
            for (std::uint32_t row = 0; row < extent.height; ++row) {
                const std::uint32_t y = origin_y + row;
                const double v = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height);
                std::byte* target = tile_pixels.data() + static_cast<std::size_t>(row) *
                                                             extent.width * mask->pixel_bytes();
                for (std::uint32_t column = 0; column < extent.width; ++column) {
                    const std::uint32_t x = origin_x + column;
                    const double u = (static_cast<double>(x) + 0.5) / static_cast<double>(width);
                    encode_float(target + static_cast<std::size_t>(column) * mask->pixel_bytes(),
                                 generator_value(kind, bound, u, v, evaluation));
                }
            }
            mask->write_tile(tile, tile_pixels);
        }
    }
    mask->clear_dirty();
    return {.mask = std::move(mask),
            .map_report = std::move(report),
            .parameter_report = std::move(parameter_report)};
}

}  // namespace ctex::maps
