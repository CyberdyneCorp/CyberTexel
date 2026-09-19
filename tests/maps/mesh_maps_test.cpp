#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <ctex/maps/mesh_maps.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {

using namespace ctex;
using namespace ctex::maps;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool near(double actual, double expected, double tolerance = 1.0e-6) {
    return std::abs(actual - expected) <= tolerance;
}

doc::TextureSet& texture_set(doc::TextureDocument& document) {
    return document.create_texture_set({.display_name = "Body",
                                        .partition_kind = doc::PartitionSourceKind::material,
                                        .partition_key = "body",
                                        .uv_set = "paint",
                                        .width = 4,
                                        .height = 4,
                                        .default_bit_depth = 8});
}

std::shared_ptr<image::TiledImage> scalar_map(std::uint32_t width, std::uint32_t height) {
    return std::make_shared<image::TiledImage>(
        width, height,
        image::PixelFormat{.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1});
}

std::shared_ptr<image::TiledImage> scalar_map(std::uint32_t width, std::uint32_t height,
                                              image::ChannelType channel_type) {
    return std::make_shared<image::TiledImage>(
        width, height, image::PixelFormat{.channel_type = channel_type, .channel_count = 1});
}

void write_u8(image::TiledImage& image, std::uint32_t x, std::uint32_t y, std::uint8_t value) {
    const std::array pixel{static_cast<std::byte>(value)};
    image.write_pixel(x, y, pixel);
}

template <typename Value>
void write_value(image::TiledImage& image, Value value) {
    std::array<std::byte, sizeof(Value)> pixel{};
    std::memcpy(pixel.data(), &value, sizeof(value));
    image.write_pixel(0, 0, pixel);
}

bool inventory_is_complete_and_stable() {
    const std::array expected_names{
        std::string_view{"tangent-space-normal"},
        std::string_view{"object-space-normal"},
        std::string_view{"world-space-direction"},
        std::string_view{"ambient-occlusion"},
        std::string_view{"curvature"},
        std::string_view{"thickness"},
        std::string_view{"position"},
        std::string_view{"height"},
        std::string_view{"bent-normal"},
        std::string_view{"material-id"},
        std::string_view{"object-id"},
        std::string_view{"uv-density"},
        std::string_view{"vertex-colour"},
    };
    std::array<std::string_view, all_mesh_map_kinds.size()> actual_names{};
    for (std::size_t index = 0; index < all_mesh_map_kinds.size(); ++index) {
        actual_names[index] = mesh_map_name(all_mesh_map_kinds[index]);
    }
    return expect(actual_names == expected_names, "mesh-map inventory or stable names changed");
}

bool lower_resolution_map_is_bound_filtered_and_reported_once() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set);
    auto ao = scalar_map(2, 2);
    write_u8(*ao, 0, 0, 0);
    write_u8(*ao, 1, 0, 255);
    write_u8(*ao, 0, 1, 0);
    write_u8(*ao, 1, 1, 255);
    const MeshMapBindResult result = maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                                .texture_set_id = set.id(),
                                                .uv_set = "paint",
                                                .pixels = ao});
    const MeshMapSample sample = maps.sample(MeshMapKind::ambient_occlusion, 0.25, 0.75);
    const MapResolutionMismatch expected{.kind = MeshMapKind::ambient_occlusion,
                                         .texture_set_id = set.id(),
                                         .map_width = 2,
                                         .map_height = 2,
                                         .texture_set_width = 4,
                                         .texture_set_height = 4};
    return expect(!result.replaced_existing && result.resolution_mismatch == expected,
                  "lower-resolution map mismatch was not reported by its bind") &&
           expect(maps.contains(MeshMapKind::ambient_occlusion) && maps.size() == 1 &&
                      maps.bound_maps() == std::vector<MeshMapKind>{MeshMapKind::ambient_occlusion},
                  "ambient-occlusion map was not retained per texture set") &&
           expect(sample.component_count == 1 && near(sample.values[0], 0.25),
                  "lower-resolution ambient-occlusion map was not bilinearly filtered");
}

bool exact_resolution_replacement_has_no_mismatch() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set);
    auto first = scalar_map(4, 4);
    auto second = scalar_map(4, 4);
    write_u8(*second, 0, 3, 255);
    const MeshMapBindResult initial = maps.bind({.kind = MeshMapKind::curvature,
                                                 .texture_set_id = set.id(),
                                                 .uv_set = "paint",
                                                 .pixels = first});
    const MeshMapBindResult replacement = maps.bind({.kind = MeshMapKind::curvature,
                                                     .texture_set_id = set.id(),
                                                     .uv_set = "paint",
                                                     .pixels = second});
    return expect(!initial.replaced_existing && !initial.resolution_mismatch &&
                      replacement.replaced_existing && !replacement.resolution_mismatch,
                  "exact-resolution bind or replacement reported the wrong disposition") &&
           expect(near(maps.sample(MeshMapKind::curvature, 0.0, 0.0).values[0], 1.0),
                  "replacement map was not made authoritative");
}

bool identifiers_use_nearest_sampling() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set);
    auto identifiers = scalar_map(2, 1);
    write_u8(*identifiers, 0, 0, 10);
    write_u8(*identifiers, 1, 0, 20);
    static_cast<void>(maps.bind({.kind = MeshMapKind::material_id,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .pixels = identifiers}));
    return expect(
        near(maps.sample(MeshMapKind::material_id, 0.49, 0.5).values[0], 10.0 / 255.0) &&
            near(maps.sample(MeshMapKind::material_id, 0.51, 0.5).values[0], 20.0 / 255.0),
        "identifier map values were blended across an identity boundary");
}

bool supported_storage_precisions_decode_on_read() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set);
    auto unorm16 = scalar_map(1, 1, image::ChannelType::uint16_unorm);
    auto float32 = scalar_map(1, 1, image::ChannelType::float32);
    write_value(*unorm16, std::uint16_t{32'768});
    write_value(*float32, 0.25F);
    static_cast<void>(maps.bind({.kind = MeshMapKind::thickness,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .pixels = unorm16}));
    static_cast<void>(maps.bind({.kind = MeshMapKind::height,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .pixels = float32}));
    return expect(
        near(maps.sample(MeshMapKind::thickness, 0.5, 0.5).values[0], 32'768.0 / 65'535.0) &&
            near(maps.sample(MeshMapKind::height, 0.5, 0.5).values[0], 0.25),
        "16-bit normalized or floating-point mesh-map storage decoded incorrectly");
}

bool incompatible_bindings_and_samples_are_refused() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set);
    auto scalar = scalar_map(4, 4);
    bool texture_set_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                     .texture_set_id = "set:other",
                                     .uv_set = "paint",
                                     .pixels = scalar}));
    } catch (const std::invalid_argument&) {
        texture_set_refused = true;
    }
    bool uv_set_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                     .texture_set_id = set.id(),
                                     .uv_set = "other",
                                     .pixels = scalar}));
    } catch (const std::invalid_argument&) {
        uv_set_refused = true;
    }
    bool channels_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::tangent_space_normal,
                                     .texture_set_id = set.id(),
                                     .uv_set = "paint",
                                     .pixels = scalar}));
    } catch (const std::invalid_argument&) {
        channels_refused = true;
    }
    bool missing_refused = false;
    try {
        static_cast<void>(maps.sample(MeshMapKind::thickness, 0.5, 0.5));
    } catch (const std::out_of_range&) {
        missing_refused = true;
    }
    bool coordinate_refused = false;
    try {
        static_cast<void>(maps.sample(MeshMapKind::ambient_occlusion,
                                      std::numeric_limits<double>::infinity(), 0.5));
    } catch (const std::invalid_argument&) {
        coordinate_refused = true;
    }
    return expect(texture_set_refused && uv_set_refused && channels_refused && missing_refused &&
                      coordinate_refused,
                  "invalid mesh-map binding or sampling input was accepted") &&
           expect(maps.size() == 0, "a refused mesh-map operation mutated the map set");
}

}  // namespace

int main() {
    return inventory_is_complete_and_stable() &&
                   lower_resolution_map_is_bound_filtered_and_reported_once() &&
                   exact_resolution_replacement_has_no_mismatch() &&
                   identifiers_use_nearest_sampling() &&
                   supported_storage_precisions_decode_on_read() &&
                   incompatible_bindings_and_samples_are_refused()
               ? 0
               : 1;
}
