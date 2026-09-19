#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <ctex/maps/generators.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::maps;

constexpr mesh::MeshRevision fixture_mesh_revision = 23;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool near(double actual, double expected) { return std::abs(actual - expected) <= 1.0e-5; }

doc::TextureSet& texture_set(doc::TextureDocument& document) {
    return document.create_texture_set({.display_name = "Body",
                                        .partition_kind = doc::PartitionSourceKind::material,
                                        .partition_key = "body",
                                        .uv_set = "paint",
                                        .width = 1,
                                        .height = 1,
                                        .default_bit_depth = 8});
}

std::shared_ptr<image::TiledImage> float_map(std::span<const float> values) {
    auto result = std::make_shared<image::TiledImage>(
        1, 1,
        image::PixelFormat{.channel_type = image::ChannelType::float32,
                           .channel_count = static_cast<std::uint8_t>(values.size())});
    std::array<std::byte, sizeof(float) * 3> bytes{};
    std::memcpy(bytes.data(), values.data(), values.size_bytes());
    result->write_pixel(0, 0, std::span(bytes).first(values.size_bytes()));
    return result;
}

void bind_map(MeshMapSet& maps, const doc::TextureSet& set, MeshMapKind kind,
              std::span<const float> values, mesh::MeshRevision revision = fixture_mesh_revision) {
    static_cast<void>(maps.bind({.kind = kind,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = revision,
                                 .pixels = float_map(values)}));
}

float output_value(const MeshMapGeneratorResult& result) {
    float value{};
    const std::span<const std::byte> pixel = result.mask->read_pixel(0, 0);
    std::memcpy(&value, pixel.data(), sizeof(value));
    return value;
}

bool inventory_names_and_requirements_are_stable() {
    const std::array expected_names{
        std::string_view{"ambient-occlusion"},
        std::string_view{"curvature"},
        std::string_view{"thickness"},
        std::string_view{"position-gradient"},
        std::string_view{"world-space-direction"},
        std::string_view{"dirt"},
        std::string_view{"edge-wear"},
        std::string_view{"scratches"},
    };
    std::array<std::string_view, all_mesh_map_generator_kinds.size()> actual_names{};
    for (std::size_t index = 0; index < all_mesh_map_generator_kinds.size(); ++index) {
        const MeshMapGeneratorInfo info =
            mesh_map_generator_info(all_mesh_map_generator_kinds[index]);
        actual_names[index] = info.name;
        if (!expect(!info.required_maps.empty(), "generator did not declare any required maps")) {
            return false;
        }
    }
    const MeshMapGeneratorInfo dirt = mesh_map_generator_info(MeshMapGeneratorKind::dirt);
    const MeshMapGeneratorInfo scratches = mesh_map_generator_info(MeshMapGeneratorKind::scratches);
    const std::array expected_dirt_maps{MeshMapKind::ambient_occlusion, MeshMapKind::curvature};
    const std::array expected_scratch_maps{MeshMapKind::position,
                                           MeshMapKind::world_space_direction};
    return expect(actual_names == expected_names, "generator inventory or names changed") &&
           expect(std::ranges::equal(dirt.required_maps, expected_dirt_maps),
                  "dirt did not declare AO and curvature") &&
           expect(std::ranges::equal(scratches.required_maps, expected_scratch_maps),
                  "scratches did not declare position and direction");
}

bool all_generators_produce_their_baseline_masks() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    const std::array ao{0.25F};
    const std::array curvature{0.75F};
    const std::array thickness{0.25F};
    const std::array position{0.3F / 31.0F, 0.6F, 0.0F};
    const std::array direction{0.5F, 0.8F, 0.5F};
    bind_map(maps, set, MeshMapKind::ambient_occlusion, ao);
    bind_map(maps, set, MeshMapKind::curvature, curvature);
    bind_map(maps, set, MeshMapKind::thickness, thickness);
    bind_map(maps, set, MeshMapKind::position, position);
    bind_map(maps, set, MeshMapKind::world_space_direction, direction);

    const std::array expected{0.75, 0.75, 0.75, 0.6, 0.8, 0.75, 0.5, 1.0};
    for (std::size_t index = 0; index < all_mesh_map_generator_kinds.size(); ++index) {
        const MeshMapGeneratorResult result =
            generate_mesh_map_mask(all_mesh_map_generator_kinds[index], maps, 1, 1);
        const double actual = output_value(result);
        if (!expect(result.mask->format() ==
                            image::PixelFormat{.channel_type = image::ChannelType::float32,
                                               .channel_count = 1} &&
                        result.mask->width() == 1 && result.mask->height() == 1 &&
                        result.mask->dirty_tiles().empty() && result.map_report.satisfied() &&
                        near(actual, expected[index]),
                    "generator produced the wrong baseline mask")) {
            std::cerr << "generator index " << index << " produced " << actual << ", expected "
                      << expected[index] << '\n';
            return false;
        }
    }
    return true;
}

bool edge_wear_is_concentrated_on_convex_curvature() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    const std::array concave{0.25F};
    bind_map(maps, set, MeshMapKind::curvature, concave);
    const double concave_value =
        output_value(generate_mesh_map_mask(MeshMapGeneratorKind::edge_wear, maps, 1, 1));
    const std::array convex{1.0F};
    bind_map(maps, set, MeshMapKind::curvature, convex);
    const double convex_value =
        output_value(generate_mesh_map_mask(MeshMapGeneratorKind::edge_wear, maps, 1, 1));
    return expect(near(concave_value, 0.0) && near(convex_value, 1.0),
                  "edge wear was not concentrated on convex curvature");
}

bool failures_are_named_and_staleness_is_reported() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    bool missing_named = false;
    bool all_missing_named = false;
    try {
        static_cast<void>(generate_mesh_map_mask(MeshMapGeneratorKind::edge_wear, maps, 1, 1));
    } catch (const MissingMeshMapsError& error) {
        missing_named =
            error.report().consumer == "edge-wear" &&
            error.report().missing_maps == std::vector<MeshMapKind>{MeshMapKind::curvature};
    }
    try {
        static_cast<void>(generate_mesh_map_mask(MeshMapGeneratorKind::scratches, maps, 1, 1));
    } catch (const MissingMeshMapsError& error) {
        all_missing_named =
            error.report().consumer == "scratches" &&
            error.report().missing_maps ==
                std::vector<MeshMapKind>{MeshMapKind::world_space_direction, MeshMapKind::position};
    }
    const std::array curvature{0.75F};
    bind_map(maps, set, MeshMapKind::curvature, curvature, fixture_mesh_revision - 1);
    const MeshMapGeneratorResult stale =
        generate_mesh_map_mask(MeshMapGeneratorKind::edge_wear, maps, 1, 1);
    bool zero_size_refused = false;
    bool invalid_kind_refused = false;
    try {
        static_cast<void>(generate_mesh_map_mask(MeshMapGeneratorKind::edge_wear, maps, 0, 1));
    } catch (const std::invalid_argument&) {
        zero_size_refused = true;
    }
    try {
        static_cast<void>(mesh_map_generator_info(static_cast<MeshMapGeneratorKind>(255)));
    } catch (const std::invalid_argument&) {
        invalid_kind_refused = true;
    }
    return expect(missing_named && all_missing_named,
                  "complete missing generator inputs were not reported by name") &&
           expect(stale.map_report.stale_maps.size() == 1 &&
                      stale.map_report.stale_maps.front().kind == MeshMapKind::curvature,
                  "generator did not propagate map staleness") &&
           expect(zero_size_refused && invalid_kind_refused,
                  "invalid generator request was accepted");
}

}  // namespace

int main() {
    return inventory_names_and_requirements_are_stable() &&
                   all_generators_produce_their_baseline_masks() &&
                   edge_wear_is_concentrated_on_convex_curvature() &&
                   failures_are_named_and_staleness_is_reported()
               ? 0
               : 1;
}
