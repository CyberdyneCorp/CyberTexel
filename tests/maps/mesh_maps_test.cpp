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

constexpr mesh::MeshRevision fixture_mesh_revision = 17;

struct MeshFixture {
    std::array<mesh::Vec3f, 3> positions{
        mesh::Vec3f{-1.0F, -1.0F, 0.0F},
        mesh::Vec3f{1.0F, -1.0F, 0.0F},
        mesh::Vec3f{0.0F, 1.0F, 0.0F},
    };
    std::array<mesh::Vec3f, 3> normals{
        mesh::Vec3f{0.0F, 0.0F, 1.0F},
        mesh::Vec3f{0.0F, 0.0F, 1.0F},
        mesh::Vec3f{0.0F, 0.0F, 1.0F},
    };
    std::array<mesh::Vec2f, 3> uv{
        mesh::Vec2f{0.0F, 0.0F},
        mesh::Vec2f{1.0F, 0.0F},
        mesh::Vec2f{0.5F, 1.0F},
    };
    std::array<std::uint32_t, 3> indices{0, 1, 2};
    std::array<mesh::UvSetView, 1> uv_sets{mesh::UvSetView{"paint", uv}};
    std::array<mesh::MeshPartition, 1> partitions{
        mesh::MeshPartition{mesh::PartitionKind::material, "body", "Body"}};
    std::array<std::uint32_t, 1> face_partitions{0};
    std::array<std::uint32_t, 1> face_materials{1};

    [[nodiscard]] mesh::MeshDescriptor descriptor() const {
        return {.positions = positions,
                .normals = normals,
                .vertex_colors = {},
                .triangle_indices = indices,
                .uv_sets = uv_sets,
                .default_uv_set = "paint",
                .partitions = partitions,
                .face_partition_indices = face_partitions,
                .face_material_ids = face_materials};
    }
};

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

std::shared_ptr<image::TiledImage> vector_map(std::uint32_t width, std::uint32_t height) {
    return std::make_shared<image::TiledImage>(
        width, height,
        image::PixelFormat{.channel_type = image::ChannelType::uint8_unorm, .channel_count = 3});
}

void write_u8(image::TiledImage& image, std::uint32_t x, std::uint32_t y, std::uint8_t value) {
    const std::array pixel{static_cast<std::byte>(value)};
    image.write_pixel(x, y, pixel);
}

void write_rgb8(image::TiledImage& image, std::uint32_t x, std::uint32_t y, std::uint8_t red,
                std::uint8_t green, std::uint8_t blue) {
    const std::array pixel{static_cast<std::byte>(red), static_cast<std::byte>(green),
                           static_cast<std::byte>(blue)};
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
    MeshMapSet maps(set, fixture_mesh_revision);
    auto ao = scalar_map(2, 2);
    write_u8(*ao, 0, 0, 0);
    write_u8(*ao, 1, 0, 255);
    write_u8(*ao, 0, 1, 0);
    write_u8(*ao, 1, 1, 255);
    const MeshMapBindResult result = maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                                .texture_set_id = set.id(),
                                                .uv_set = "paint",
                                                .mesh_revision = fixture_mesh_revision,
                                                .pixels = ao});
    const MeshMapReadResult sample = maps.sample(MeshMapKind::ambient_occlusion, 0.25, 0.75);
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
           expect(sample.sample.component_count == 1 && near(sample.sample.values[0], 0.25) &&
                      !sample.staleness,
                  "lower-resolution ambient-occlusion map was not bilinearly filtered");
}

bool exact_resolution_replacement_has_no_mismatch() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    auto first = scalar_map(4, 4);
    auto second = scalar_map(4, 4);
    write_u8(*second, 0, 3, 255);
    const MeshMapBindResult initial = maps.bind({.kind = MeshMapKind::curvature,
                                                 .texture_set_id = set.id(),
                                                 .uv_set = "paint",
                                                 .mesh_revision = fixture_mesh_revision,
                                                 .pixels = first});
    const MeshMapBindResult replacement = maps.bind({.kind = MeshMapKind::curvature,
                                                     .texture_set_id = set.id(),
                                                     .uv_set = "paint",
                                                     .mesh_revision = fixture_mesh_revision,
                                                     .pixels = second});
    return expect(!initial.replaced_existing && !initial.resolution_mismatch &&
                      replacement.replaced_existing && !replacement.resolution_mismatch,
                  "exact-resolution bind or replacement reported the wrong disposition") &&
           expect(near(maps.sample(MeshMapKind::curvature, 0.0, 0.0).sample.values[0], 1.0),
                  "replacement map was not made authoritative");
}

bool identifiers_use_nearest_sampling() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    auto identifiers = scalar_map(2, 1);
    write_u8(*identifiers, 0, 0, 10);
    write_u8(*identifiers, 1, 0, 20);
    static_cast<void>(maps.bind({.kind = MeshMapKind::material_id,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_mesh_revision,
                                 .pixels = identifiers}));
    return expect(
        near(maps.sample(MeshMapKind::material_id, 0.49, 0.5).sample.values[0], 10.0 / 255.0) &&
            near(maps.sample(MeshMapKind::material_id, 0.51, 0.5).sample.values[0], 20.0 / 255.0),
        "identifier map values were blended across an identity boundary");
}

bool supported_storage_precisions_decode_on_read() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    auto unorm16 = scalar_map(1, 1, image::ChannelType::uint16_unorm);
    auto float32 = scalar_map(1, 1, image::ChannelType::float32);
    write_value(*unorm16, std::uint16_t{32'768});
    write_value(*float32, 0.25F);
    static_cast<void>(maps.bind({.kind = MeshMapKind::thickness,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_mesh_revision,
                                 .pixels = unorm16}));
    static_cast<void>(maps.bind({.kind = MeshMapKind::height,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_mesh_revision,
                                 .pixels = float32}));
    return expect(
        near(maps.sample(MeshMapKind::thickness, 0.5, 0.5).sample.values[0], 32'768.0 / 65'535.0) &&
            near(maps.sample(MeshMapKind::height, 0.5, 0.5).sample.values[0], 0.25),
        "16-bit normalized or floating-point mesh-map storage decoded incorrectly");
}

bool normal_conventions_are_recorded_and_canonicalized_on_read() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    auto normal = vector_map(1, 1);
    write_rgb8(*normal, 0, 0, 128, 64, 255);
    static_cast<void>(maps.bind({.kind = MeshMapKind::tangent_space_normal,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_mesh_revision,
                                 .normal_convention = NormalMapConvention::open_gl,
                                 .pixels = normal}));
    const MeshMapSample open_gl = maps.sample(MeshMapKind::tangent_space_normal, 0.5, 0.5).sample;
    static_cast<void>(maps.bind({.kind = MeshMapKind::tangent_space_normal,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_mesh_revision,
                                 .normal_convention = NormalMapConvention::direct_x,
                                 .pixels = normal}));
    const MeshMapSample direct_x = maps.sample(MeshMapKind::tangent_space_normal, 0.5, 0.5).sample;
    return expect(near(open_gl.values[0], direct_x.values[0]) &&
                      near(open_gl.values[1], 64.0 / 255.0) &&
                      near(direct_x.values[1], 191.0 / 255.0) &&
                      near(open_gl.values[2], direct_x.values[2]) &&
                      maps.map(MeshMapKind::tangent_space_normal).normal_convention ==
                          NormalMapConvention::direct_x,
                  "normal-map convention was not retained or converted to canonical OpenGL");
}

bool map_memory_is_accounted_and_host_releasable() {
    doc::TextureDocument document;
    doc::TextureSet& set = texture_set(document);
    set.channels().enable("pbr.base_color");
    const std::array colour{std::byte{10}, std::byte{20}, std::byte{30}};
    set.channels().pixels("pbr.base_color").write_pixel(0, 0, colour);
    const std::size_t channel_bytes = set.channels().resident_pixel_bytes();
    MeshMapSet maps(set, fixture_mesh_revision);
    auto ao = scalar_map(1, 1);
    auto curvature = scalar_map(1, 1);
    write_u8(*ao, 0, 0, 64);
    write_u8(*curvature, 0, 0, 192);
    const std::size_t bytes_per_map = ao->resident_pixel_bytes();
    static_cast<void>(maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_mesh_revision,
                                 .pixels = ao}));
    static_cast<void>(maps.bind({.kind = MeshMapKind::curvature,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_mesh_revision,
                                 .pixels = curvature}));
    const MeshMapMemoryReport bound = maps.memory_report();
    const doc::TextureDocumentMemoryReport document_bound = document.memory_report();
    const MeshMapReleaseResult absent = maps.release_map(MeshMapKind::thickness);
    const MeshMapReleaseResult one = maps.release_map(MeshMapKind::ambient_occlusion);
    bool released_map_is_missing = false;
    try {
        static_cast<void>(maps.sample(MeshMapKind::ambient_occlusion, 0.5, 0.5));
    } catch (const MissingMeshMapsError&) {
        released_map_is_missing = true;
    }
    const doc::TextureDocumentMemoryReport document_after_one = document.memory_report();
    const bool remaining_map_present = maps.contains(MeshMapKind::curvature);
    const MeshMapReleaseResult all = maps.release_all_maps();
    const doc::TextureDocumentMemoryReport document_after_all = document.memory_report();

    bool account_lifetime_released = false;
    {
        MeshMapSet temporary(set, fixture_mesh_revision);
        static_cast<void>(temporary.bind({.kind = MeshMapKind::ambient_occlusion,
                                          .texture_set_id = set.id(),
                                          .uv_set = "paint",
                                          .mesh_revision = fixture_mesh_revision,
                                          .pixels = ao}));
        account_lifetime_released = document.memory_report().mesh_map_pixel_bytes == bytes_per_map;
        {
            const MeshMapSet copy = temporary;
            account_lifetime_released =
                account_lifetime_released && copy.size() == 1 &&
                document.memory_report().mesh_map_pixel_bytes == bytes_per_map * 2;
        }
        account_lifetime_released = account_lifetime_released &&
                                    document.memory_report().mesh_map_pixel_bytes == bytes_per_map;
    }
    account_lifetime_released =
        account_lifetime_released && document.memory_report().mesh_map_pixel_bytes == 0;

    return expect(bytes_per_map != 0 && bound.maps.size() == 2 &&
                      bound.resident_pixel_bytes == bytes_per_map * 2 &&
                      document_bound.texture_sets.size() == 1 &&
                      document_bound.channel_pixel_bytes == channel_bytes &&
                      document_bound.mesh_map_pixel_bytes == bytes_per_map * 2 &&
                      document_bound.total_resident_bytes == channel_bytes + bytes_per_map * 2,
                  "bound mesh maps were absent from map-set or document memory accounting") &&
           expect(absent.released_maps.empty() && absent.resident_pixel_bytes_released == 0,
                  "releasing an absent map was not idempotent") &&
           expect(one.released_maps == std::vector<MeshMapKind>{MeshMapKind::ambient_occlusion} &&
                      one.resident_pixel_bytes_released == bytes_per_map,
                  "selective map release reported the wrong map or byte count") &&
           expect(released_map_is_missing && remaining_map_present &&
                      document_after_one.mesh_map_pixel_bytes == bytes_per_map,
                  "selective map release was not observable in reads or accounting") &&
           expect(all.released_maps == std::vector<MeshMapKind>{MeshMapKind::curvature} &&
                      all.resident_pixel_bytes_released == bytes_per_map && maps.size() == 0 &&
                      document_after_all.mesh_map_pixel_bytes == 0 &&
                      document_after_all.total_resident_bytes == channel_bytes &&
                      document.contains_texture_set(set.id()) && account_lifetime_released,
                  "bulk or lifetime map release damaged the document or retained accounting");
}

bool required_maps_are_reported_without_neutral_substitution() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    auto curvature = scalar_map(4, 4);
    static_cast<void>(maps.bind({.kind = MeshMapKind::curvature,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_mesh_revision,
                                 .pixels = curvature}));
    const std::array required{MeshMapKind::thickness, MeshMapKind::ambient_occlusion,
                              MeshMapKind::curvature, MeshMapKind::ambient_occlusion};
    const MeshMapRequirementReport report =
        maps.check_required_maps("weathering material", required);
    const std::vector expected{MeshMapKind::ambient_occlusion, MeshMapKind::thickness};
    if (!expect(!report.satisfied() && report.consumer == "weathering material" &&
                    report.texture_set_id == set.id() && report.missing_maps == expected &&
                    report.message.find("ambient-occlusion") != std::string::npos &&
                    report.message.find("thickness") != std::string::npos &&
                    report.message.find(set.id()) != std::string::npos,
                "missing-map report was incomplete, duplicated, or unnamed")) {
        return false;
    }

    try {
        static_cast<void>(maps.require_maps("weathering material", required));
    } catch (const MissingMeshMapsError& error) {
        return expect(error.report() == report &&
                          std::string_view(error.what()) == report.message && maps.size() == 1,
                      "missing-map refusal changed bindings or lost its structured report");
    }
    return expect(false, "consumer execution continued with missing mesh maps");
}

bool complete_requirements_and_direct_missing_reads_are_distinct() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    auto ao = scalar_map(4, 4);
    static_cast<void>(maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                 .texture_set_id = set.id(),
                                 .uv_set = "paint",
                                 .mesh_revision = fixture_mesh_revision,
                                 .pixels = ao}));
    const std::array complete{MeshMapKind::ambient_occlusion};
    const MeshMapRequirementReport ready = maps.check_required_maps("AO generator", complete);
    if (!expect(ready.satisfied() && ready.missing_maps.empty() && ready.message.empty(),
                "satisfied map requirements produced a missing-map diagnostic")) {
        return false;
    }
    const MeshMapRequirementReport required = maps.require_maps("AO generator", complete);
    if (!expect(required == ready, "requirement guard changed a satisfied report")) {
        return false;
    }

    try {
        static_cast<void>(maps.sample(MeshMapKind::thickness, 0.5, 0.5));
    } catch (const MissingMeshMapsError& error) {
        return expect(
            error.report().missing_maps == std::vector<MeshMapKind>{MeshMapKind::thickness} &&
                error.report().message.find("thickness") != std::string::npos,
            "direct missing-map read did not preserve its typed named report");
    }
    return expect(false, "direct missing-map read returned a neutral sample");
}

bool mesh_revision_changes_report_retained_stale_maps() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshFixture mesh_buffers;
    mesh::MeshBinding mesh_binding(mesh_buffers.descriptor());
    const mesh::MeshRevision source_revision = mesh_binding.revision();
    MeshMapSet maps(set, mesh_binding);
    auto ao = scalar_map(4, 4);
    write_u8(*ao, 0, 3, 255);
    const MeshMapBindResult initial = maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                                 .texture_set_id = set.id(),
                                                 .uv_set = "paint",
                                                 .mesh_revision = source_revision,
                                                 .pixels = ao});
    const MeshMapReadResult fresh = maps.sample(MeshMapKind::ambient_occlusion, 0.0, 0.0);
    mesh_buffers.positions[0].x = -0.5F;
    mesh_binding.replace(mesh_buffers.descriptor());
    const mesh::MeshRevision replacement_revision = mesh_binding.revision();
    const std::vector<MeshMapStaleness> changed = maps.synchronize_mesh_revision(mesh_binding);
    const MeshMapStaleness expected{.kind = MeshMapKind::ambient_occlusion,
                                    .produced_mesh_revision = source_revision,
                                    .current_mesh_revision = replacement_revision};
    const MeshMapReadResult stale = maps.sample(MeshMapKind::ambient_occlusion, 0.0, 0.0);
    const std::array required{MeshMapKind::ambient_occlusion};
    const MeshMapRequirementReport report = maps.require_maps("dirt generator", required);
    if (!expect(!initial.staleness && !fresh.staleness &&
                    changed == std::vector<MeshMapStaleness>{expected} &&
                    maps.stale_maps() == changed && stale.staleness == expected &&
                    near(stale.sample.values[0], 1.0) && maps.contains(expected.kind) &&
                    maps.size() == 1,
                "mesh replacement discarded, hid, or silently sampled a stale map")) {
        return false;
    }
    if (!expect(report.satisfied() && report.stale_maps == changed &&
                    report.message.find("ambient-occlusion") != std::string::npos &&
                    report.message.find(std::to_string(source_revision)) != std::string::npos &&
                    report.message.find(std::to_string(replacement_revision)) != std::string::npos,
                "consumer preflight did not report the stale map and both revisions")) {
        return false;
    }

    auto replacement = scalar_map(4, 4);
    const MeshMapBindResult rebound = maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                                 .texture_set_id = set.id(),
                                                 .uv_set = "paint",
                                                 .mesh_revision = replacement_revision,
                                                 .pixels = replacement});
    return expect(rebound.replaced_existing && !rebound.staleness && maps.stale_maps().empty() &&
                      !maps.sample(MeshMapKind::ambient_occlusion, 0.0, 0.0).staleness,
                  "rebaking against the current mesh did not clear staleness");
}

bool incompatible_bindings_and_samples_are_refused() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    auto scalar = scalar_map(4, 4);
    bool texture_set_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                     .texture_set_id = "set:other",
                                     .uv_set = "paint",
                                     .mesh_revision = fixture_mesh_revision,
                                     .pixels = scalar}));
    } catch (const std::invalid_argument&) {
        texture_set_refused = true;
    }
    bool uv_set_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                     .texture_set_id = set.id(),
                                     .uv_set = "other",
                                     .mesh_revision = fixture_mesh_revision,
                                     .pixels = scalar}));
    } catch (const std::invalid_argument&) {
        uv_set_refused = true;
    }
    bool channels_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::tangent_space_normal,
                                     .texture_set_id = set.id(),
                                     .uv_set = "paint",
                                     .mesh_revision = fixture_mesh_revision,
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
    const std::array required{MeshMapKind::ambient_occlusion};
    bool unnamed_consumer_refused = false;
    try {
        static_cast<void>(maps.check_required_maps("", required));
    } catch (const std::invalid_argument&) {
        unnamed_consumer_refused = true;
    }
    const std::array invalid_kind{static_cast<MeshMapKind>(255)};
    bool invalid_requirement_refused = false;
    try {
        static_cast<void>(maps.check_required_maps("invalid fixture", invalid_kind));
    } catch (const std::invalid_argument&) {
        invalid_requirement_refused = true;
    }
    bool zero_revision_refused = false;
    try {
        static_cast<void>(MeshMapSet(set, 0));
    } catch (const std::invalid_argument&) {
        zero_revision_refused = true;
    }
    bool zero_source_revision_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                     .texture_set_id = set.id(),
                                     .uv_set = "paint",
                                     .mesh_revision = 0,
                                     .pixels = scalar}));
    } catch (const std::invalid_argument&) {
        zero_source_revision_refused = true;
    }
    auto normal = vector_map(1, 1);
    bool missing_normal_convention_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::tangent_space_normal,
                                     .texture_set_id = set.id(),
                                     .uv_set = "paint",
                                     .mesh_revision = fixture_mesh_revision,
                                     .pixels = normal}));
    } catch (const std::invalid_argument&) {
        missing_normal_convention_refused = true;
    }
    bool convention_on_data_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::ambient_occlusion,
                                     .texture_set_id = set.id(),
                                     .uv_set = "paint",
                                     .mesh_revision = fixture_mesh_revision,
                                     .normal_convention = NormalMapConvention::open_gl,
                                     .pixels = scalar}));
    } catch (const std::invalid_argument&) {
        convention_on_data_refused = true;
    }
    bool invalid_convention_refused = false;
    try {
        static_cast<void>(maps.bind({.kind = MeshMapKind::tangent_space_normal,
                                     .texture_set_id = set.id(),
                                     .uv_set = "paint",
                                     .mesh_revision = fixture_mesh_revision,
                                     .normal_convention = static_cast<NormalMapConvention>(255),
                                     .pixels = normal}));
    } catch (const std::invalid_argument&) {
        invalid_convention_refused = true;
    }
    return expect(texture_set_refused && uv_set_refused && channels_refused && missing_refused &&
                      coordinate_refused && unnamed_consumer_refused &&
                      invalid_requirement_refused && zero_revision_refused &&
                      zero_source_revision_refused && missing_normal_convention_refused &&
                      convention_on_data_refused && invalid_convention_refused,
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
                   normal_conventions_are_recorded_and_canonicalized_on_read() &&
                   map_memory_is_accounted_and_host_releasable() &&
                   required_maps_are_reported_without_neutral_substitution() &&
                   complete_requirements_and_direct_missing_reads_are_distinct() &&
                   mesh_revision_changes_report_retained_stale_maps() &&
                   incompatible_bindings_and_samples_are_refused()
               ? 0
               : 1;
}
