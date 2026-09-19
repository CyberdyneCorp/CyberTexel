#include <array>
#include <ctex/io/smart_material_package.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {

using namespace ctex;
using namespace ctex::doc;
using namespace ctex::io;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_material_error(Callable&& callable, SmartMaterialErrorCode code,
                           std::string_view message) {
    try {
        callable();
    } catch (const SmartMaterialError& error) {
        return expect(error.code() == code, message);
    } catch (...) {
    }
    return expect(false, message);
}

template <typename Callable>
bool expect_project_error(Callable&& callable, ProjectContainerErrorCode code,
                          std::string_view message) {
    try {
        callable();
    } catch (const ProjectContainerError& error) {
        return expect(error.code() == code, message);
    } catch (...) {
    }
    return expect(false, message);
}

graph::GraphDocument resource_graph() {
    return graph::GraphDocument({.role = graph::NodeRole::output,
                                 .type_id = "ctex.output.resource-test",
                                 .type_version = 1,
                                 .display_name = "Resource output",
                                 .position = {},
                                 .inputs = {{.identifier = "surface",
                                             .display_name = "Surface image",
                                             .type = graph::SocketType::image,
                                             .value = graph::ImageValue{"images/noise"}}},
                                 .outputs = {},
                                 .properties = {}});
}

SmartMaterialPreset resource_material() {
    return {
        .schema_version = current_smart_material_schema_version,
        .identifier = "materials/portable-noise",
        .display_name = "Portable noise",
        .stack = {{.identifier = "surface",
                   .parent_identifier = {},
                   .display_name = "Surface",
                   .kind = SmartMaterialEntryKind::layer,
                   .enabled = true,
                   .opacity = 1.0,
                   .graph = resource_graph(),
                   .content_kind = SmartMaterialContentKind::derived,
                   .pixel_payloads = {}}},
        .exposed_parameters = {{.identifier = "surface-image",
                                .display_name = "Surface image",
                                .display_group = "Surface",
                                .type = graph::SocketType::image,
                                .default_value = graph::ImageValue{"images/noise"},
                                .minimum = std::nullopt,
                                .maximum = std::nullopt,
                                .bindings = {{.entry_identifier = "surface",
                                              .node_id = 1,
                                              .target_kind = SmartMaterialBindingTargetKind::input,
                                              .target_identifier = "surface"}}}},
        .anchor_entries = {},
        .anchor_references = {},
        .resource_references = {{.identifier = "images/noise", .kind = "image"},
                                {.identifier = "fonts/labels", .kind = "font"}}};
}

std::vector<ProjectResource> resource_manifest() {
    return {{.identifier = "images/noise",
             .kind = "image",
             .relative_path = "textures/noise.bin",
             .packed_bytes = std::nullopt},
            {.identifier = "fonts/labels",
             .kind = "font",
             .relative_path = "fonts/labels.bin",
             .packed_bytes = std::nullopt}};
}

void write_file(const std::filesystem::path& path, std::string_view bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary);
    stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

bool moved_shelf_and_missing_input_are_reported() {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "ctex-smart-material-resource-test";
    const std::filesystem::path original = root / "original-shelf";
    const std::filesystem::path moved = root / "moved-shelf";
    const std::filesystem::path empty = root / "empty-shelf";
    std::error_code error;
    std::filesystem::remove_all(root, error);
    write_file(original / "textures/noise.bin", "noise");
    write_file(original / "fonts/labels.bin", "font");
    std::filesystem::create_directories(empty, error);

    const SmartMaterialPreset material = resource_material();
    const std::vector<ProjectResource> resources = resource_manifest();
    const ProjectContainer referenced = package_smart_material(
        material, resources, {.self_contained = false, .source_directory = original});
    const ProjectContainer packed = package_smart_material(
        material, resources, {.self_contained = true, .source_directory = original});
    const std::vector<std::byte> referenced_bytes = write_project_container(referenced);
    const std::vector<std::byte> packed_bytes = write_project_container(packed);
    std::filesystem::rename(original, moved, error);
    if (!expect(!error, "could not move smart material shelf fixture")) {
        return false;
    }

    const std::array moved_search_path{empty, moved};
    const SmartMaterialImportResult relocated =
        import_smart_material(referenced_bytes, moved_search_path);
    std::filesystem::remove(moved / "textures/noise.bin", error);
    const std::array missing_search_path{moved};
    const SmartMaterialImportResult missing =
        import_smart_material(referenced_bytes, missing_search_path);
    std::filesystem::remove_all(moved, error);
    const std::array<std::filesystem::path, 0> no_search_paths{};
    const SmartMaterialImportResult self_contained =
        import_smart_material(packed_bytes, no_search_paths);
    std::filesystem::remove_all(root, error);

    const auto& missing_image = std::get<graph::ImageValue>(
        missing.material.stack.front().graph->node(1).inputs.front().value);
    return expect(relocated.material == material && relocated.resources_complete() &&
                      relocated.image_inputs.size() == 1 &&
                      relocated.image_inputs.front().status == ProjectResourceStatus::referenced,
                  "moved shelf did not resolve through the ordered search path") &&
           expect(!missing.resources_complete() &&
                      missing.asset.resources.missing_identifiers ==
                          std::vector<std::string>{"images/noise"} &&
                      missing.image_inputs.size() == 1 &&
                      missing.image_inputs.front().status == ProjectResourceStatus::missing &&
                      missing_image.resource_id == "images/noise",
                  "missing resource was substituted or not reported by input and identity") &&
           expect(self_contained.asset.self_contained && self_contained.resources_complete() &&
                      self_contained.asset.resources.resources.size() == 2 &&
                      self_contained.image_inputs.front().status == ProjectResourceStatus::packed,
                  "self-contained smart material required the exporter's shelf");
}

bool nonportable_and_undeclared_resources_are_refused() {
    SmartMaterialPreset absolute = resource_material();
    absolute.resource_references.front().identifier = "/tmp/noise";
    const bool absolute_refused =
        expect_material_error([&] { static_cast<void>(serialize_smart_material(absolute)); },
                              SmartMaterialErrorCode::invalid_resource_reference,
                              "absolute smart material resource identity was accepted");

    SmartMaterialPreset undeclared = resource_material();
    undeclared.resource_references.erase(undeclared.resource_references.begin());
    return absolute_refused &&
           expect_material_error([&] { static_cast<void>(serialize_smart_material(undeclared)); },
                                 SmartMaterialErrorCode::invalid_resource_reference,
                                 "undeclared smart material image resource was accepted");
}

bool package_manifest_must_match_payload() {
    ProjectContainer package = package_smart_material(resource_material(), resource_manifest());
    package.resources.front().kind = "font";
    const std::vector<std::byte> bytes = write_project_container(package);
    const std::array<std::filesystem::path, 0> no_search_paths{};
    return expect_project_error(
        [&] { static_cast<void>(import_smart_material(bytes, no_search_paths)); },
        ProjectContainerErrorCode::invalid_asset,
        "smart material package accepted a resource kind that contradicted its payload");
}

}  // namespace

int main() {
    return moved_shelf_and_missing_input_are_reported() &&
                   nonportable_and_undeclared_resources_are_refused() &&
                   package_manifest_must_match_payload()
               ? 0
               : 1;
}
