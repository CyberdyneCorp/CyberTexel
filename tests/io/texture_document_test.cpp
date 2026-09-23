#include <array>
#include <cstddef>
#include <ctex/io/document_mesh_state.hpp>
#include <ctex/io/texture_document.hpp>
#include <filesystem>
#include <fstream>
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

template <typename Callable>
bool expect_error(Callable&& callable, std::string_view message) {
    try {
        callable();
    } catch (const io::TextureDocumentIoError&) {
        return true;
    }
    return expect(false, message);
}

void write_u32(std::span<std::byte> bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned index = 0; index < 4; ++index) {
        bytes[offset + index] = static_cast<std::byte>(value >> (index * 8U));
    }
}

doc::TextureSetDescriptor descriptor(std::string name, std::string partition, bool udim = false) {
    return {.display_name = std::move(name),
            .partition_kind = doc::PartitionSourceKind::material,
            .partition_key = std::move(partition),
            .uv_set = "uv0",
            .width = 8,
            .height = 8,
            .default_bit_depth = 8,
            .udim_tiling = udim};
}

graph::GraphDocument material_graph() {
    return graph::GraphDocument({.role = graph::NodeRole::output,
                                 .type_id = "ctex.output.document-archive-test",
                                 .type_version = 1,
                                 .display_name = "Archive output",
                                 .position = {},
                                 .inputs = {{.identifier = "amount",
                                             .display_name = "Amount",
                                             .type = graph::SocketType::scalar,
                                             .value = 0.25},
                                            {.identifier = "image",
                                             .display_name = "Image",
                                             .type = graph::SocketType::image,
                                             .value = graph::ImageValue{"images/source"}}},
                                 .outputs = {},
                                 .properties = {}});
}

doc::SmartMaterialPreset smart_material() {
    return {.schema_version = doc::current_smart_material_schema_version,
            .identifier = "materials/archive",
            .display_name = "Archive material",
            .stack = {{.identifier = "surface",
                       .parent_identifier = {},
                       .display_name = "Surface",
                       .kind = doc::SmartMaterialEntryKind::layer,
                       .enabled = true,
                       .opacity = 0.75,
                       .graph = material_graph(),
                       .content_kind = doc::SmartMaterialContentKind::derived,
                       .pixel_payloads = {}}},
            .exposed_parameters = {{.identifier = "amount",
                                    .display_name = "Amount",
                                    .display_group = "Surface",
                                    .type = graph::SocketType::scalar,
                                    .default_value = 0.25,
                                    .minimum = 0.0,
                                    .maximum = 1.0,
                                    .bindings = {{.entry_identifier = "surface",
                                                  .node_id = 1,
                                                  .target_kind =
                                                      doc::SmartMaterialBindingTargetKind::input,
                                                  .target_identifier = "amount"}}}},
            .anchor_entries = {},
            .anchor_references = {},
            .resource_references = {{.identifier = "images/source", .kind = "image"}}};
}

doc::TextureDocument populated_document() {
    doc::TextureDocument document;
    doc::TextureSet& body = document.create_texture_set(descriptor("Body", "body"));
    const std::string body_id = body.id();
    body.channels().register_descriptor(
        {.semantic_id = "custom.mask",
         .component_count = 1,
         .scalar_representation = doc::ScalarRepresentation::floating_point,
         .preferred_bit_depth = 32,
         .default_value = {0.125},
         .classification = doc::ChannelClassification::data,
         .blending_policy = doc::BlendingPolicy::scalar,
         .export_mapping = "customMask",
         .evaluable = false});
    body.channels().enable("pbr.base_color", 8);
    body.channels().enable("custom.mask", 32);
    const std::array orange{std::byte{220}, std::byte{90}, std::byte{12}};
    const std::array mask{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0x3f}};
    body.channels().pixels("pbr.base_color").write_pixel(2, 3, orange);
    body.channels().pixels("custom.mask").write_pixel(7, 7, mask);
    static_cast<void>(body.apply_smart_material(smart_material(), "archive-application"));
    static_cast<void>(
        body.set_applied_preset_parameter_value("archive-application", "amount", 0.625));
    body.layer_stack().set_channel_modulation(
        "archive-application/surface",
        {.semantic_id = "pbr.base_color", .enabled = true, .opacity = 0.8});
    static_cast<void>(body.editable_authoring().add(
        {.identifier = "editable-label",
         .kind = doc::EditableEntryKind::text,
         .revision = 1,
         .placement = {},
         .material_identity = "materials/archive",
         .material_parameters = {},
         .text = "CyberTexel",
         .font_identity = "fonts/reference",
         .mesh_revision = 0,
         .surface_points = {},
         .dependent_tiles = {{.semantic_id = "pbr.base_color", .tile_x = 0, .tile_y = 0}}}));

    doc::TextureSet& cloth = document.create_texture_set(descriptor("Cloth", "cloth", true));
    const std::string cloth_id = cloth.id();
    cloth.channels().enable("pbr.roughness", 16);
    const std::array roughness{std::byte{0x34}, std::byte{0x12}};
    const doc::UdimPixelWrite write{.u = 1.25, .v = 0.25, .pixel = roughness};
    static_cast<void>(cloth.write_udim_pixels("pbr.roughness", std::span(&write, 1)));
    cloth.udim_channels(1002).disable("pbr.base_color");

    static_cast<void>(document.create_atlas(
        {.identifier = "main-atlas",
         .display_name = "Main atlas",
         .width = 16,
         .height = 8,
         .regions = {
             {.texture_set_identifier = body_id, .x = 0, .y = 0, .width = 8, .height = 8},
             {.texture_set_identifier = cloth_id, .x = 8, .y = 0, .width = 8, .height = 8}}}));
    return document;
}

bool same_descriptor(const doc::ChannelDescriptor& left, const doc::ChannelDescriptor& right) {
    return left.semantic_id == right.semantic_id && left.component_count == right.component_count &&
           left.scalar_representation == right.scalar_representation &&
           left.preferred_bit_depth == right.preferred_bit_depth &&
           std::ranges::equal(left.default_value, right.default_value) &&
           left.classification == right.classification &&
           left.blending_policy == right.blending_policy &&
           left.export_mapping == right.export_mapping && left.evaluable == right.evaluable;
}

std::string texture_set_with_partition(const doc::TextureDocument& document,
                                       std::string_view partition) {
    for (const std::string& identity : document.texture_set_ids()) {
        if (document.texture_set(identity).descriptor().partition_key == partition) return identity;
    }
    return {};
}

bool documents_match(const doc::TextureDocument& expected, const doc::TextureDocument& actual) {
    if (!expect(expected.texture_set_ids() == actual.texture_set_ids(),
                "texture-set identities changed after project round trip") ||
        !expect(expected.atlas_ids() == actual.atlas_ids(),
                "atlas identities changed after project round trip")) {
        return false;
    }
    for (const std::string& identity : expected.texture_set_ids()) {
        const doc::TextureSet& left = expected.texture_set(identity);
        const doc::TextureSet& right = actual.texture_set(identity);
        if (!expect(
                left.descriptor().display_name == right.descriptor().display_name &&
                    left.descriptor().partition_kind == right.descriptor().partition_kind &&
                    left.descriptor().partition_key == right.descriptor().partition_key &&
                    left.descriptor().uv_set == right.descriptor().uv_set &&
                    left.descriptor().width == right.descriptor().width &&
                    left.descriptor().height == right.descriptor().height &&
                    left.descriptor().default_bit_depth == right.descriptor().default_bit_depth &&
                    left.descriptor().udim_tiling == right.descriptor().udim_tiling,
                "texture-set descriptor changed after project round trip") ||
            !expect(left.channels().semantic_ids() == right.channels().semantic_ids(),
                    "channel identities changed after project round trip") ||
            !expect(left.layer_stack() == right.layer_stack(),
                    "layer stack changed after project round trip") ||
            !expect(std::ranges::equal(left.editable_authoring().entries(),
                                       right.editable_authoring().entries()),
                    "editable entries changed after project round trip") ||
            !expect(std::ranges::equal(left.preset_applications(), right.preset_applications()),
                    "preset applications changed after project round trip") ||
            !expect(left.occupied_udim_tiles() == right.occupied_udim_tiles(),
                    "UDIM tile identities changed after project round trip")) {
            return false;
        }
        for (const std::string& channel : left.channels().semantic_ids()) {
            if (!expect(same_descriptor(left.channels().descriptor(channel),
                                        right.channels().descriptor(channel)),
                        "channel descriptor changed after project round trip") ||
                !expect(left.channels().is_enabled(channel) == right.channels().is_enabled(channel),
                        "base channel enablement changed after project round trip")) {
                return false;
            }
        }
    }
    return expect(expected.atlas("main-atlas") == actual.atlas("main-atlas"),
                  "atlas layout changed after project round trip");
}

bool complete_document_round_trips() {
    const doc::TextureDocument source = populated_document();
    io::ProjectContainer project;
    project.resources.push_back({.identifier = "images/source",
                                 .kind = "image",
                                 .relative_path = "images/source.png",
                                 .packed_bytes = std::vector<std::byte>{std::byte{1}}});
    io::upsert_texture_document(project, "document/main", source);
    const std::vector<std::byte> first = io::write_project_container(project);
    const io::ProjectContainer reopened = io::read_project_container(first).container;
    const doc::TextureDocument restored = io::unpack_texture_document(reopened, "document/main");
    const std::vector<io::TextureDocumentAssetInfo> inventory =
        io::list_texture_documents(reopened);
    if (!expect(
            inventory == std::vector<io::TextureDocumentAssetInfo>{{.identifier = "document/main",
                                                                    .texture_set_count = 2,
                                                                    .tiled_image_count = 4,
                                                                    .layer_entry_count = 1,
                                                                    .atlas_count = 1,
                                                                    .editable_entry_count = 1,
                                                                    .preset_application_count = 1}},
            "texture-document inventory is incomplete") ||
        !documents_match(source, restored)) {
        return false;
    }
    const std::string body_id = texture_set_with_partition(source, "body");
    const doc::TextureSet& restored_body = restored.texture_set(body_id);
    const auto body_pixel = restored_body.channels().pixels("pbr.base_color").read_pixel(2, 3);
    const doc::TextureSet& restored_cloth =
        restored.texture_set(texture_set_with_partition(source, "cloth"));
    const auto roughness = restored_cloth.read_udim_pixel("pbr.roughness", 1002, 2, 2);
    const std::array expected_body{std::byte{220}, std::byte{90}, std::byte{12}};
    const bool pixels =
        expect(std::ranges::equal(body_pixel, expected_body),
               "base channel pixel changed after project round trip") &&
        expect(roughness == std::vector<std::byte>{std::byte{0x34}, std::byte{0x12}},
               "UDIM channel pixel changed after project round trip") &&
        expect(!restored_cloth.udim_channels(1002).is_enabled("pbr.base_color"),
               "UDIM-specific disabled channel was re-enabled");
    io::upsert_texture_document(project, "document/main", source);
    return pixels && expect(io::write_project_container(project) == first,
                            "unchanged texture document did not save byte-identically");
}

bool invalid_archives_are_refused_atomically() {
    const doc::TextureDocument source = populated_document();
    io::ProjectContainer project;
    const std::vector<std::byte> before = io::write_project_container(project);
    const bool missing_resource =
        expect_error([&] { io::upsert_texture_document(project, "document/main", source); },
                     "document accepted a graph resource without project metadata");
    if (!expect(io::write_project_container(project) == before,
                "failed document upsert changed the project")) {
        return false;
    }
    project.resources.push_back({.identifier = "images/source",
                                 .kind = "image",
                                 .relative_path = "images/source.png",
                                 .packed_bytes = std::vector<std::byte>{std::byte{1}}});
    io::upsert_texture_document(project, "document/main", source);
    const io::ProjectContainer valid = project;
    const bool bounded = expect_error(
        [&] {
            static_cast<void>(io::unpack_texture_document(project, "document/main",
                                                          {.maximum_payload_bytes = 8}));
        },
        "document payload ignored its configured input limit");
    project.assets.front().format_version += 1;
    const bool future = expect_error(
        [&] { static_cast<void>(io::unpack_texture_document(project, "document/main")); },
        "future texture-document asset version was decoded");

    io::ProjectContainer impossible_count = valid;
    write_u32(impossible_count.assets.front().payload, 12, 1'000'000);
    const bool count_bounded = expect_error(
        [&] { static_cast<void>(io::unpack_texture_document(impossible_count, "document/main")); },
        "texture-set count allocated before fitting the remaining payload");

    io::ProjectContainer unused_dependency = valid;
    image::TiledImage unused_image(
        1, 1, {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1});
    unused_dependency.tiled_images.push_back(
        io::snapshot_tiled_image("document/main/pixels/unused", unused_image));
    unused_dependency.assets.front().tiled_image_dependencies.push_back(
        "document/main/pixels/unused");
    const bool exact_dependencies = expect_error(
        [&] { static_cast<void>(io::unpack_texture_document(unused_dependency, "document/main")); },
        "unused texture-document tiled dependency was accepted");

    io::ProjectContainer undeclared_resource = valid;
    undeclared_resource.assets.front().resource_dependencies.clear();
    const bool undeclared_resource_refused = expect_error(
        [&] {
            static_cast<void>(io::unpack_texture_document(undeclared_resource, "document/main"));
        },
        "undeclared texture-document graph resource was accepted");

    io::ProjectContainer unused_resource = valid;
    unused_resource.resources.push_back({.identifier = "images/unused",
                                         .kind = "image",
                                         .relative_path = "images/unused.png",
                                         .packed_bytes = std::vector<std::byte>{std::byte{2}}});
    unused_resource.assets.front().resource_dependencies.push_back("images/unused");
    const bool unused_resource_refused = expect_error(
        [&] { static_cast<void>(io::unpack_texture_document(unused_resource, "document/main")); },
        "unused texture-document resource dependency was accepted");
    return missing_resource && bounded && future && count_bounded && exact_dependencies &&
           undeclared_resource_refused && unused_resource_refused;
}

std::string write_cli_fixture(const std::filesystem::path& project_path,
                              const std::filesystem::path& preset_path,
                              const std::filesystem::path& missing_preset_path) {
    const doc::TextureDocument document = populated_document();
    io::ProjectContainer project;
    project.resources.push_back({.identifier = "images/source",
                                 .kind = "image",
                                 .relative_path = "images/source.png",
                                 .packed_bytes = std::vector<std::byte>{std::byte{1}}});
    constexpr std::string_view mesh =
        "o Fixture\n"
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nv 2 0 0\nv 3 0 0\nv 2 1 0\n"
        "vn 0 0 1\nvt 0 0\nvt 1 0\nvt 0 1\n"
        "usemtl body\nf 1/1/1 2/2/1 3/3/1\n"
        "usemtl cloth\nf 4/1/1 5/2/1 6/3/1\n";
    project.resources.push_back(
        {.identifier = "mesh/source",
         .kind = "mesh",
         .relative_path = "meshes/source.obj",
         .packed_bytes = std::vector<std::byte>(
             reinterpret_cast<const std::byte*>(mesh.data()),
             reinterpret_cast<const std::byte*>(mesh.data() + mesh.size()))});
    io::upsert_texture_document(project, "document/main", document);
    image::TiledImage ao(2, 2,
                         {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1});
    ao.write_pixel(0, 0, std::array{std::byte{0}});
    ao.write_pixel(1, 0, std::array{std::byte{64}});
    ao.write_pixel(0, 1, std::array{std::byte{128}});
    ao.write_pixel(1, 1, std::array{std::byte{255}});
    io::upsert_document_mesh_state(
        project, {.document_asset_id = "document/main",
                  .mesh_resource_id = "mesh/source",
                  .mesh_revision = 1,
                  .maps = {{.kind = 3,
                            .texture_set_id = texture_set_with_partition(document, "body"),
                            .uv_set = "uv0",
                            .produced_mesh_revision = 1,
                            .pixels = std::move(ao)}}});
    io::save_project_container_atomic(project_path, project);
    const std::string preset = doc::serialize_smart_material(smart_material());
    std::ofstream stream(preset_path, std::ios::binary);
    stream.write(preset.data(), static_cast<std::streamsize>(preset.size()));
    if (!stream) throw std::runtime_error("could not write CLI smart-material fixture");
    doc::SmartMaterialPreset missing = smart_material();
    missing.resource_references.front().identifier = "images/missed";
    missing.stack.front().graph->set_input_value(1, "image",
                                                 graph::ImageValue{.resource_id = "images/missed"});
    const std::string missing_preset = doc::serialize_smart_material(missing);
    std::ofstream missing_stream(missing_preset_path, std::ios::binary);
    missing_stream.write(missing_preset.data(),
                         static_cast<std::streamsize>(missing_preset.size()));
    if (!missing_stream) {
        throw std::runtime_error("could not write missing-resource smart-material fixture");
    }
    return texture_set_with_partition(document, "body");
}

void write_cli_interrupt_fixture(const std::filesystem::path& project_path) {
    doc::TextureDocument document;
    for (std::uint32_t index = 0; index < 20; ++index) {
        doc::TextureSetDescriptor texture_set =
            descriptor("Texture " + std::to_string(index), "part-" + std::to_string(index));
        texture_set.width = 1024;
        texture_set.height = 1024;
        document.create_texture_set(std::move(texture_set)).channels().enable("pbr.base_color", 8);
    }
    io::ProjectContainer project;
    io::upsert_texture_document(project, "document/main", document);
    io::save_project_container_atomic(project_path, project);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 5 && std::string_view(argv[1]) == "--write-cli-fixture") {
        std::cout << write_cli_fixture(argv[2], argv[3], argv[4]) << '\n';
        return 0;
    }
    if (argc == 3 && std::string_view(argv[1]) == "--write-cli-interrupt-fixture") {
        write_cli_interrupt_fixture(argv[2]);
        return 0;
    }
    return complete_document_round_trips() && invalid_archives_are_refused_atomically() ? 0 : 1;
}
