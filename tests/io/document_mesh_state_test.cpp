#include <algorithm>
#include <array>
#include <cstddef>
#include <ctex/io/document_mesh_state.hpp>
#include <ctex/io/texture_document.hpp>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;

bool expect(bool condition, std::string_view message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}

template <typename Operation>
bool expect_error(Operation&& operation, std::string_view message) {
    try {
        operation();
    } catch (const io::DocumentMeshStateIoError&) {
        return true;
    }
    return expect(false, message);
}

struct Fixture {
    io::ProjectContainer project;
    std::string texture_set_id;
};

Fixture fixture() {
    doc::TextureDocument document;
    const std::string texture_set_id =
        document
            .create_texture_set({.display_name = "Body",
                                 .partition_kind = doc::PartitionSourceKind::material,
                                 .partition_key = "body",
                                 .uv_set = "uv0",
                                 .width = 4,
                                 .height = 4,
                                 .default_bit_depth = 8})
            .id();
    io::ProjectContainer project;
    project.resources.push_back({.identifier = "mesh/source",
                                 .kind = "mesh",
                                 .relative_path = "meshes/source.obj",
                                 .packed_bytes = std::vector<std::byte>{std::byte{'v'}}});
    io::upsert_texture_document(project, "document/main", document);
    return {.project = std::move(project), .texture_set_id = texture_set_id};
}

image::TiledImage ao_image(std::byte upper_right = std::byte{64}) {
    image::TiledImage image(2, 2,
                            {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1});
    image.write_pixel(0, 0, std::array{std::byte{0}});
    image.write_pixel(1, 0, std::array{upper_right});
    image.write_pixel(0, 1, std::array{std::byte{128}});
    image.write_pixel(1, 1, std::array{std::byte{255}});
    return image;
}

io::DocumentMeshState state(std::string texture_set_id, std::byte upper_right = std::byte{64}) {
    return {.document_asset_id = "document/main",
            .mesh_resource_id = "mesh/source",
            .mesh_revision = 7,
            .maps = {{.kind = 3,
                      .texture_set_id = std::move(texture_set_id),
                      .uv_set = "uv0",
                      .produced_mesh_revision = 6,
                      .normal_convention = std::nullopt,
                      .tangent_frame = std::nullopt,
                      .pixels = ao_image(upper_right)}}};
}

bool mesh_reference_and_maps_round_trip() {
    Fixture source = fixture();
    io::upsert_document_mesh_state(source.project, state(source.texture_set_id));
    const std::vector<std::byte> first = io::write_project_container(source.project);
    const io::ProjectContainer reopened = io::read_project_container(first).container;
    const auto restored = io::read_document_mesh_state(reopened, "document/main");
    if (!expect(restored.has_value(), "document mesh state disappeared after project round trip")) {
        return false;
    }
    const std::array expected{std::byte{64}};
    const bool contents =
        expect(restored->mesh_resource_id == "mesh/source" && restored->mesh_revision == 7 &&
                   restored->maps.size() == 1 && restored->maps.front().kind == 3 &&
                   restored->maps.front().texture_set_id == source.texture_set_id &&
                   restored->maps.front().produced_mesh_revision == 6 &&
                   std::ranges::equal(restored->maps.front().pixels.read_pixel(1, 0), expected),
               "document mesh state changed during project round trip");
    io::ProjectContainer repeated = reopened;
    io::upsert_document_mesh_state(repeated, *restored);
    return contents && expect(io::write_project_container(repeated) == first,
                              "unchanged document mesh state did not save byte-identically");
}

bool replacement_is_atomic_and_reclaims_old_images() {
    Fixture source = fixture();
    io::upsert_document_mesh_state(source.project, state(source.texture_set_id));
    const std::size_t image_count = source.project.tiled_images.size();
    io::upsert_document_mesh_state(source.project, state(source.texture_set_id, std::byte{99}));
    const auto restored = io::read_document_mesh_state(source.project, "document/main");
    const std::array expected{std::byte{99}};
    const bool replaced =
        restored && source.project.tiled_images.size() == image_count &&
        std::ranges::equal(restored->maps.front().pixels.read_pixel(1, 0), expected);

    const std::vector<std::byte> before = io::write_project_container(source.project);
    io::DocumentMeshState invalid = state(source.texture_set_id);
    invalid.mesh_resource_id = "mesh/missing";
    const bool missing_resource =
        expect_error([&] { io::upsert_document_mesh_state(source.project, invalid); },
                     "mesh state accepted an unknown mesh resource");
    invalid = state(source.texture_set_id);
    invalid.maps.push_back(invalid.maps.front());
    const bool duplicate =
        expect_error([&] { io::upsert_document_mesh_state(source.project, invalid); },
                     "mesh state accepted a duplicate texture-set map");
    return expect(replaced, "mesh-state replacement retained stale image data") &&
           missing_resource && duplicate &&
           expect(io::write_project_container(source.project) == before,
                  "failed mesh-state replacement changed the project");
}

bool absent_state_is_distinct_from_malformed_state() {
    Fixture source = fixture();
    const bool absent = !io::read_document_mesh_state(source.project, "document/main");
    io::upsert_document_mesh_state(source.project, state(source.texture_set_id));
    auto asset = std::ranges::find(source.project.assets, "document/main/mesh-state",
                                   &io::StandaloneAsset::identifier);
    asset->payload.pop_back();
    return expect(absent, "document without mesh state did not report absence") &&
           expect_error(
               [&] {
                   static_cast<void>(io::read_document_mesh_state(source.project, "document/main"));
               },
               "truncated document mesh state was accepted");
}

bool canonical_order_and_restoration_budget_are_enforced() {
    Fixture source = fixture();
    io::DocumentMeshState ordered = state(source.texture_set_id);
    io::StoredDocumentMeshMap second = ordered.maps.front();
    second.kind = 4;
    ordered.maps.push_back(std::move(second));
    io::DocumentMeshState reversed = ordered;
    std::ranges::reverse(reversed.maps);

    io::ProjectContainer first = source.project;
    io::ProjectContainer second_project = source.project;
    io::upsert_document_mesh_state(first, ordered);
    io::upsert_document_mesh_state(second_project, reversed);
    const bool canonical =
        expect(io::write_project_container(first) == io::write_project_container(second_project),
               "mesh-state map input order changed canonical bytes");

    io::DocumentMeshStateReadLimits limits;
    limits.maximum_total_map_pixel_bytes = image::default_tile_size * image::default_tile_size - 1;
    const bool bounded = expect_error(
        [&] { static_cast<void>(io::read_document_mesh_state(first, "document/main", limits)); },
        "mesh-state read limit ignored restored tile allocation");
    return canonical && bounded;
}

}  // namespace

int main() {
    return mesh_reference_and_maps_round_trip() &&
                   replacement_is_atomic_and_reclaims_old_images() &&
                   absent_state_is_distinct_from_malformed_state() &&
                   canonical_order_and_restoration_budget_are_enforced()
               ? 0
               : 1;
}
