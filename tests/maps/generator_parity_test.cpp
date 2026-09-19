#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <ctex/exec/cpu_reference.hpp>
#include <ctex/exec/host_execution.hpp>
#include <ctex/exec/parity_gate.hpp>
#include <ctex/maps/generators.hpp>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::maps;

constexpr mesh::MeshRevision fixture_revision = 31;
constexpr std::uint32_t output_width = 2;
constexpr std::uint32_t output_height = 2;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

doc::TextureSet& texture_set(doc::TextureDocument& document) {
    return document.create_texture_set({.display_name = "Parity",
                                        .partition_kind = doc::PartitionSourceKind::material,
                                        .partition_key = "parity",
                                        .uv_set = "paint",
                                        .width = 2,
                                        .height = 2,
                                        .default_bit_depth = 32});
}

std::shared_ptr<image::TiledImage> constant_map(std::span<const float> values) {
    auto result = std::make_shared<image::TiledImage>(
        2, 2,
        image::PixelFormat{.channel_type = image::ChannelType::float32,
                           .channel_count = static_cast<std::uint8_t>(values.size())});
    std::array<std::byte, sizeof(float) * 3> bytes{};
    std::memcpy(bytes.data(), values.data(), values.size_bytes());
    const auto pixel = std::span(bytes).first(values.size_bytes());
    for (std::uint32_t y = 0; y < 2; ++y) {
        for (std::uint32_t x = 0; x < 2; ++x) {
            result->write_pixel(x, y, pixel);
        }
    }
    return result;
}

void bind_map(MeshMapSet& maps, const doc::TextureSet& set, MeshMapKind kind,
              std::span<const float> values) {
    static_cast<void>(maps.bind({.kind = kind,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_revision,
                                 .pixels = constant_map(values)}));
}

void bind_fixture_maps(MeshMapSet& maps, const doc::TextureSet& set) {
    const std::array ao{0.2F};
    const std::array curvature{0.8F};
    const std::array thickness{0.3F};
    const std::array position{0.3F / 31.0F, 0.6F, 0.0F};
    const std::array direction{0.4F, 0.7F, 0.5F};
    bind_map(maps, set, MeshMapKind::ambient_occlusion, ao);
    bind_map(maps, set, MeshMapKind::curvature, curvature);
    bind_map(maps, set, MeshMapKind::thickness, thickness);
    bind_map(maps, set, MeshMapKind::position, position);
    bind_map(maps, set, MeshMapKind::world_space_direction, direction);
}

std::vector<MeshMapGeneratorParameter> parameters_for(MeshMapGeneratorKind kind) {
    std::vector<MeshMapGeneratorParameter> result{{.name = "strength", .value = 0.8},
                                                  {.name = "contrast", .value = 1.5}};
    switch (kind) {
        case MeshMapGeneratorKind::dirt:
            result.push_back({.name = "curvature-weight", .value = 0.6});
            break;
        case MeshMapGeneratorKind::edge_wear:
            result.push_back({.name = "threshold", .value = 0.4});
            break;
        case MeshMapGeneratorKind::scratches:
            result.push_back({.name = "scale", .value = 3.0});
            result.push_back({.name = "width", .value = 0.08});
            break;
        case MeshMapGeneratorKind::ambient_occlusion:
        case MeshMapGeneratorKind::curvature:
        case MeshMapGeneratorKind::thickness:
        case MeshMapGeneratorKind::position_gradient:
        case MeshMapGeneratorKind::world_space_direction:
            break;
    }
    return result;
}

double read_float(const image::TiledImage& image, std::uint32_t x, std::uint32_t y) {
    float value{};
    const auto pixel = image.read_pixel(x, y);
    std::memcpy(&value, pixel.data(), sizeof(value));
    return value;
}

std::vector<double> cpu_channel(MeshMapGeneratorKind kind, const MeshMapSet& maps) {
    const std::vector parameters = parameters_for(kind);
    const MeshMapGeneratorResult result =
        generate_mesh_map_mask(kind, maps, output_width, output_height, parameters);
    std::vector<double> values;
    values.reserve(output_width * output_height);
    for (std::uint32_t y = 0; y < output_height; ++y) {
        for (std::uint32_t x = 0; x < output_width; ++x) {
            values.push_back(read_float(*result.mask, x, y));
        }
    }
    return values;
}

double saturate(double value) { return std::clamp(value, 0.0, 1.0); }

double portable_baseline(MeshMapGeneratorKind kind, const MeshMapSet& maps, double u, double v) {
    const auto scalar = [&](MeshMapKind map) { return maps.sample(map, u, v).sample.values[0]; };
    switch (kind) {
        case MeshMapGeneratorKind::ambient_occlusion:
            return 1.0 - scalar(MeshMapKind::ambient_occlusion);
        case MeshMapGeneratorKind::curvature:
            return scalar(MeshMapKind::curvature);
        case MeshMapGeneratorKind::thickness:
            return 1.0 - scalar(MeshMapKind::thickness);
        case MeshMapGeneratorKind::position_gradient:
            return maps.sample(MeshMapKind::position, u, v).sample.values[1];
        case MeshMapGeneratorKind::world_space_direction:
            return maps.sample(MeshMapKind::world_space_direction, u, v).sample.values[1];
        case MeshMapGeneratorKind::dirt: {
            const double occlusion = 1.0 - scalar(MeshMapKind::ambient_occlusion);
            const double concavity = (0.5 - scalar(MeshMapKind::curvature)) * 2.0 * 0.6;
            return std::max(occlusion, concavity);
        }
        case MeshMapGeneratorKind::edge_wear:
            return (scalar(MeshMapKind::curvature) - 0.4) / 0.6;
        case MeshMapGeneratorKind::scratches: {
            const MeshMapSample position = maps.sample(MeshMapKind::position, u, v).sample;
            const MeshMapSample direction =
                maps.sample(MeshMapKind::world_space_direction, u, v).sample;
            const double phase =
                (position.values[0] * 31.0 + position.values[1] * 7.0 + position.values[2] * 13.0) *
                3.0;
            const double distance = std::abs((phase - std::floor(phase)) - 0.5);
            const double line = saturate(1.0 - distance / 0.08);
            const double grazing = 1.0 - std::abs(direction.values[2] * 2.0 - 1.0);
            return line * saturate(grazing);
        }
    }
    return 0.0;
}

std::vector<double> portable_channel(MeshMapGeneratorKind kind, const MeshMapSet& maps) {
    std::vector<double> values;
    values.reserve(output_width * output_height);
    for (std::uint32_t y = 0; y < output_height; ++y) {
        const double v = 1.0 - (static_cast<double>(y) + 0.5) / output_height;
        for (std::uint32_t x = 0; x < output_width; ++x) {
            const double u = (static_cast<double>(x) + 0.5) / output_width;
            const double baseline = saturate(portable_baseline(kind, maps, u, v));
            values.push_back(saturate(std::pow(baseline, 1.5) * 0.8));
        }
    }
    return values;
}

exec::ParityRenderedFixture render(bool portable) {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_revision);
    bind_fixture_maps(maps, set);
    exec::ParityRenderedFixture result{
        .width = output_width, .height = output_height, .channels = {}};
    for (const MeshMapGeneratorKind kind : all_mesh_map_generator_kinds) {
        const MeshMapGeneratorInfo info = mesh_map_generator_info(kind);
        result.channels.push_back(
            {.semantic = std::string(info.name),
             .values = portable ? portable_channel(kind, maps) : cpu_channel(kind, maps)});
    }
    return result;
}

exec::ParityFixture fixture() {
    exec::ParityFixture result{.identifier = "mesh-map-generators",
                               .document = "mesh-maps:v1;size=2x2",
                               .stroke = "not-applicable:generator",
                               .camera = "not-applicable:uv-space",
                               .material = "generator-parameters:v1",
                               .channels = {}};
    for (const MeshMapGeneratorKind kind : all_mesh_map_generator_kinds) {
        result.channels.push_back({.semantic = std::string(mesh_map_generator_info(kind).name),
                                   .value_class = exec::ParityValueClass::floating_point,
                                   .filtered = true,
                                   .component_count = 1});
    }
    return result;
}

emit::DeviceFeatureSet host_features() {
    return {.binding_budget = 16,
            .maximum_texture_dimension = 8192,
            .supported_texture_formats = {emit::TextureFormat::r32_float},
            .floating_point_filtering = true,
            .compute_available = true};
}

bool generator_fixture_is_repeatable_and_cross_executor_equivalent() {
    const exec::ParityRenderedFixture first = render(false);
    const exec::ParityRenderedFixture second = render(false);
    bool repeatable = first.width == second.width && first.height == second.height &&
                      first.channels.size() == second.channels.size();
    for (std::size_t index = 0; repeatable && index < first.channels.size(); ++index) {
        repeatable = first.channels[index].semantic == second.channels[index].semantic &&
                     first.channels[index].values == second.channels[index].values;
    }
    exec::CpuReferenceExecutor cpu;
    exec::HostExecutedExecutor host("Portable fixture backend", host_features(), true);
    const std::array fixtures{fixture()};
    const std::array bindings{
        exec::ParityExecutorBinding{
            .executor = &cpu, .render = [](const exec::ParityFixture&) { return render(false); }},
        exec::ParityExecutorBinding{
            .executor = &host, .render = [](const exec::ParityFixture&) { return render(true); }},
    };
    const exec::ParityGateReport report = exec::run_parity_gate(fixtures, bindings);
    return expect(repeatable, "repeated CPU generator evaluation was not byte-stable") &&
           expect(report.passed() && report.executors.size() == 2 &&
                      report.executors[1].status == exec::ParityMeasurementStatus::passed &&
                      report.executors[1].measured_fixtures == 1,
                  "portable generator evaluation exceeded cross-executor tolerance");
}

}  // namespace

int main() { return generator_fixture_is_repeatable_and_cross_executor_equivalent() ? 0 : 1; }
