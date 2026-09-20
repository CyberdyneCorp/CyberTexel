#include <ctex/capi.h>

#include <cstddef>
#include <ctex/doc/smart_material.hpp>
#include <ctex/io/project_container.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ctex::graph::GraphDocument graph_fixture(std::string name) {
    return ctex::graph::GraphDocument(
        {.role = ctex::graph::NodeRole::output,
         .type_id = "ctex.output.smart-material-capi",
         .type_version = 1,
         .display_name = std::move(name),
         .position = {},
         .inputs = {{.identifier = "value",
                     .display_name = "Value",
                     .type = ctex::graph::SocketType::scalar,
                     .value = 0.25},
                    {.identifier = "anchor",
                     .display_name = "Anchor",
                     .type = ctex::graph::SocketType::image,
                     .value = ctex::graph::ImageValue{}},
                    {.identifier = "texture",
                     .display_name = "Texture",
                     .type = ctex::graph::SocketType::image,
                     .value = ctex::graph::ImageValue{"images/noise"}}},
         .outputs = {},
         .properties = {}});
}

ctex::doc::SmartMaterialPreset material_fixture() {
    using namespace ctex::doc;
    return {
        .schema_version = current_smart_material_schema_version,
        .identifier = "materials/capi",
        .display_name = "C API material",
        .stack = {{.identifier = "base",
                   .parent_identifier = {},
                   .display_name = "Base",
                   .kind = SmartMaterialEntryKind::layer,
                   .enabled = true,
                   .opacity = 1.0,
                   .graph = graph_fixture("Base graph"),
                   .content_kind = SmartMaterialContentKind::derived,
                   .pixel_payloads = {}},
                  {.identifier = "coat",
                   .parent_identifier = {},
                   .display_name = "Coat",
                   .kind = SmartMaterialEntryKind::layer,
                   .enabled = true,
                   .opacity = 1.0,
                   .graph = graph_fixture("Coat graph"),
                   .content_kind = SmartMaterialContentKind::derived,
                   .pixel_payloads = {}},
                  {.identifier = "painted-mask",
                   .parent_identifier = "coat",
                   .display_name = "Painted mask",
                   .kind = SmartMaterialEntryKind::mask,
                   .enabled = true,
                   .opacity = 1.0,
                   .graph = std::nullopt,
                   .content_kind = SmartMaterialContentKind::model_specific,
                   .pixel_payloads = {{.identifier = "mask",
                                       .width = 1,
                                       .height = 1,
                                       .format = {ctex::image::ChannelType::uint8_unorm, 1},
                                       .pixels = {std::byte{127}}}}}},
        .exposed_parameters = {{.identifier = "wear",
                                .display_name = "Wear",
                                .display_group = "Surface",
                                .type = ctex::graph::SocketType::scalar,
                                .default_value = 0.25,
                                .minimum = 0.0,
                                .maximum = 1.0,
                                .binding_state = SmartMaterialParameterBindingState::bound,
                                .bindings = {{.entry_identifier = "base",
                                              .node_id = 1,
                                              .target_kind = SmartMaterialBindingTargetKind::input,
                                              .target_identifier = "value"},
                                             {.entry_identifier = "coat",
                                              .node_id = 1,
                                              .target_kind = SmartMaterialBindingTargetKind::input,
                                              .target_identifier = "value"}}}},
        .anchor_entries = {"base"},
        .anchor_references = {{.anchor_entry_identifier = "base",
                               .consumer_entry_identifier = "coat",
                               .consumer_node_id = 1,
                               .consumer_input_identifier = "anchor"}},
        .resource_references = {{.identifier = "images/noise", .kind = "image"}},
    };
}

struct MaterialResult {
    MaterialResult() { info.size = CTEX_SMART_MATERIAL_INFO_CURRENT_SIZE; }

    ctex_smart_material_info info{};
    std::string canonical;
    std::string report;
};

bool inspect_material(std::string_view source, MaterialResult& result) {
    if (!expect(ctex_smart_material_inspect(source.data(), source.size(), &result.info, nullptr, 0,
                                            nullptr, 0) == CTEX_RESULT_SUCCESS,
                "smart material sizing inspection failed")) {
        return false;
    }
    result.canonical.assign(result.info.canonical_size, static_cast<char>(0xa5));
    const std::string untouched = result.canonical;
    char short_report = 'x';
    if (!expect(ctex_smart_material_inspect(source.data(), source.size(), &result.info,
                                            result.canonical.data(), result.canonical.size(),
                                            &short_report, 1) == CTEX_RESULT_BUFFER_TOO_SMALL,
                "smart material inspection accepted a short report") ||
        !expect(result.canonical == untouched && short_report == 'x',
                "smart material inspection wrote a partial output")) {
        return false;
    }
    std::vector<char> report(result.info.report_size);
    if (!expect(ctex_smart_material_inspect(source.data(), source.size(), &result.info,
                                            result.canonical.data(), result.canonical.size(),
                                            report.data(), report.size()) == CTEX_RESULT_SUCCESS,
                "smart material inspection failed")) {
        return false;
    }
    result.report = report.data();
    return true;
}

bool inspection_reports_content(std::string_view serialized) {
    MaterialResult result;
    return inspect_material(serialized, result) &&
           expect(result.canonical == serialized,
                  "current smart material did not stay canonical") &&
           expect(result.info.entry_count == 3 && result.info.derived_entry_count == 2 &&
                      result.info.model_specific_entry_count == 1 &&
                      result.info.model_specific_pixel_bytes == 1 &&
                      result.info.exposed_parameter_count == 1 && result.info.anchor_count == 1 &&
                      result.info.anchor_reference_count == 1 &&
                      result.info.resource_reference_count == 1,
                  "smart material content metrics are incomplete") &&
           expect(result.report.find("\"id\":\"painted-mask\"") != std::string::npos &&
                      result.report.find("\"content\":\"model-specific\"") != std::string::npos &&
                      result.report.find("\"id\":\"wear\"") != std::string::npos,
                  "smart material JSON inventory omitted content or parameters");
}

bool parameter_updates_all_bindings(std::string_view serialized) {
    ctex_smart_material_info info{};
    info.size = CTEX_SMART_MATERIAL_INFO_CURRENT_SIZE;
    const ctex_smart_material_value_descriptor value{
        .size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE,
        .type = CTEX_SMART_MATERIAL_VALUE_SCALAR,
        .scalar = 0.75,
        .vector = {},
        .colour = {},
        .text = nullptr,
        .boolean = 0,
    };
    if (!expect(
            ctex_smart_material_set_parameter(serialized.data(), serialized.size(), "wear", &value,
                                              &info, nullptr, 0, nullptr, 0) == CTEX_RESULT_SUCCESS,
            "smart material parameter sizing failed")) {
        return false;
    }
    std::string updated(info.canonical_size, '\0');
    std::vector<char> report(info.report_size);
    if (!expect(ctex_smart_material_set_parameter(
                    serialized.data(), serialized.size(), "wear", &value, &info, updated.data(),
                    updated.size(), report.data(), report.size()) == CTEX_RESULT_SUCCESS,
                "smart material parameter update failed")) {
        return false;
    }
    const ctex::doc::SmartMaterialPreset material = ctex::doc::deserialize_smart_material(updated);
    const auto value_at = [&](std::size_t entry) {
        return std::get<double>(material.stack[entry].graph->node(1).inputs.front().value);
    };
    return expect(value_at(0) == 0.75 && value_at(1) == 0.75,
                  "one parameter did not update every graph binding");
}

bool anchors_are_atomic_and_planned(std::string_view serialized) {
    ctex_smart_material_info info{};
    info.size = CTEX_SMART_MATERIAL_INFO_CURRENT_SIZE;
    if (!expect(
            ctex_smart_material_set_anchor(serialized.data(), serialized.size(), "coat", 1, &info,
                                           nullptr, 0, nullptr, 0) == CTEX_RESULT_SUCCESS,
            "second anchor sizing failed")) {
        return false;
    }
    std::string two_anchors(info.canonical_size, '\0');
    std::vector<char> report(info.report_size);
    if (!expect(ctex_smart_material_set_anchor(serialized.data(), serialized.size(), "coat", 1,
                                               &info, two_anchors.data(), two_anchors.size(),
                                               report.data(), report.size()) == CTEX_RESULT_SUCCESS,
                "second anchor creation failed")) {
        return false;
    }

    std::string untouched(info.canonical_size, static_cast<char>(0xa5));
    const ctex_result cycle = ctex_smart_material_add_anchor_reference(
        two_anchors.data(), two_anchors.size(), "coat", "base", 1, "anchor", &info,
        untouched.data(), untouched.size(), nullptr, 0);
    const char* changed[] = {"base"};
    std::size_t plan_size = 0;
    if (!expect(cycle == CTEX_RESULT_INVALID_ARGUMENT &&
                    ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_SMART_MATERIAL,
                "anchor cycle was accepted") ||
        !expect(untouched.front() == static_cast<char>(0xa5) &&
                    untouched.back() == static_cast<char>(0xa5),
                "refused anchor cycle changed the output") ||
        !expect(ctex_smart_material_plan_anchor_evaluation(two_anchors.data(), two_anchors.size(),
                                                           changed, 1, nullptr, 0,
                                                           &plan_size) == CTEX_RESULT_SUCCESS,
                "anchor evaluation sizing failed")) {
        return false;
    }
    std::vector<char> plan(plan_size);
    return expect(ctex_smart_material_plan_anchor_evaluation(two_anchors.data(), two_anchors.size(),
                                                             changed, 1, plan.data(), plan.size(),
                                                             &plan_size) == CTEX_RESULT_SUCCESS,
                  "anchor evaluation planning failed") &&
           expect(std::string_view(plan.data()) == "[\"coat\"]",
                  "anchor evaluation plan is not dependency ordered");
}

std::string schema_five(std::string_view current) {
    std::string result;
    std::size_t begin = 0;
    while (begin < current.size()) {
        const std::size_t end = current.find('\n', begin);
        std::string line(current.substr(begin, end - begin));
        if (line.starts_with("CTEX_SMART_MATERIAL\t")) {
            line = "CTEX_SMART_MATERIAL\t5";
        } else if (line.starts_with("PARAM\t")) {
            line.erase(line.rfind('\t'));
        }
        result += line + '\n';
        begin = end + 1;
    }
    return result;
}

bool versions_are_migrated_or_refused(std::string_view serialized) {
    MaterialResult migrated;
    const std::string old = schema_five(serialized);
    if (!inspect_material(old, migrated)) {
        return false;
    }
    std::string future(serialized);
    future.replace(20, 1, "9");
    ctex_smart_material_info info{};
    info.size = CTEX_SMART_MATERIAL_INFO_CURRENT_SIZE;
    return expect(migrated.info.source_schema_version == 5 &&
                      migrated.info.canonical_schema_version ==
                          ctex::doc::current_smart_material_schema_version &&
                      migrated.canonical == serialized,
                  "older smart material was not migrated canonically") &&
           expect(ctex_smart_material_inspect(future.data(), future.size(), &info, nullptr, 0,
                                              nullptr, 0) == CTEX_RESULT_UNSUPPORTED_OPERATION &&
                      ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_SMART_MATERIAL,
                  "future smart material schema was not refused by name");
}

bool package_material(std::string_view serialized, const ctex_project_resource_descriptor& resource,
                      const ctex_project_asset_export_options_descriptor& options,
                      std::vector<std::byte>& package) {
    ctex_project_container_info info{};
    info.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE;
    if (!expect(ctex_smart_material_package(serialized.data(), serialized.size(), &resource, 1,
                                            &options, &info, nullptr, 0, nullptr,
                                            0) == CTEX_RESULT_SUCCESS,
                "smart material package sizing failed")) {
        return false;
    }
    package.resize(info.canonical_size);
    std::vector<char> report(info.report_size);
    return expect(ctex_smart_material_package(serialized.data(), serialized.size(), &resource, 1,
                                              &options, &info, package.data(), package.size(),
                                              report.data(), report.size()) == CTEX_RESULT_SUCCESS,
                  "smart material packaging failed") &&
           expect(info.asset_count == 1 && info.resource_count == 1,
                  "smart material package omitted its asset or resource");
}

bool import_material(const std::vector<std::byte>& package,
                     const ctex_project_asset_search_paths_descriptor* search_paths,
                     MaterialResult& result) {
    if (!expect(
            ctex_smart_material_import(package.data(), package.size(), nullptr, search_paths,
                                       &result.info, nullptr, 0, nullptr, 0) == CTEX_RESULT_SUCCESS,
            "smart material import sizing failed")) {
        return false;
    }
    result.canonical.resize(result.info.canonical_size);
    std::vector<char> report(result.info.report_size);
    if (!expect(ctex_smart_material_import(package.data(), package.size(), nullptr, search_paths,
                                           &result.info, result.canonical.data(),
                                           result.canonical.size(), report.data(),
                                           report.size()) == CTEX_RESULT_SUCCESS,
                "smart material import failed")) {
        return false;
    }
    result.report = report.data();
    return true;
}

bool resources_are_portable_and_self_contained(std::string_view serialized) {
    const std::filesystem::path root = "ctex-capi-smart-material-resources";
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::filesystem::create_directories(root / "textures");
    {
        std::ofstream file(root / "textures/noise.bin", std::ios::binary);
        file.write("noise", 5);
    }
    const ctex_project_resource_descriptor resource{
        .size = CTEX_PROJECT_RESOURCE_DESCRIPTOR_CURRENT_SIZE,
        .identifier = "images/noise",
        .kind = "image",
        .relative_path = "textures/noise.bin",
        .packed = 0,
        .packed_bytes = nullptr,
        .packed_byte_count = 0,
    };
    const std::string root_text = root.string();
    const ctex_project_asset_export_options_descriptor self_contained{
        .size = CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE,
        .self_contained = 1,
        .source_directory = root_text.c_str(),
    };
    const ctex_project_asset_export_options_descriptor referenced{
        .size = CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE,
        .self_contained = 0,
        .source_directory = nullptr,
    };
    std::vector<std::byte> packed_package;
    std::vector<std::byte> referenced_package;
    if (!package_material(serialized, resource, self_contained, packed_package) ||
        !package_material(serialized, resource, referenced, referenced_package)) {
        std::filesystem::remove_all(root, ignored);
        return false;
    }
    const ctex::io::ProjectContainerReadResult packed =
        ctex::io::read_project_container(packed_package);
    MaterialResult packed_import;
    if (!expect(packed.container.resources.front().packed_bytes.has_value() &&
                    packed.container.resources.front().packed_bytes->size() == 5,
                "self-contained package did not embed its resource") ||
        !import_material(packed_package, nullptr, packed_import)) {
        std::filesystem::remove_all(root, ignored);
        return false;
    }
    const char* paths[] = {root_text.c_str()};
    const ctex_project_asset_search_paths_descriptor search_paths{
        .size = CTEX_PROJECT_ASSET_SEARCH_PATHS_DESCRIPTOR_CURRENT_SIZE,
        .paths = paths,
        .path_count = 1,
    };
    MaterialResult referenced_import;
    if (!import_material(referenced_package, &search_paths, referenced_import)) {
        std::filesystem::remove_all(root, ignored);
        return false;
    }
    std::filesystem::remove(root / "textures/noise.bin", ignored);
    MaterialResult missing_import;
    const bool missing_succeeded =
        import_material(referenced_package, &search_paths, missing_import);
    std::filesystem::remove_all(root, ignored);
    return expect(packed_import.canonical == serialized &&
                      packed_import.report.find("\"status\":\"packed\"") != std::string::npos &&
                      packed_import.report.find("\"resources_complete\":true") != std::string::npos,
                  "self-contained import did not report packed resources") &&
           expect(referenced_import.report.find("\"status\":\"referenced\"") != std::string::npos,
                  "moved resource was not resolved through the search path") &&
           expect(
               missing_succeeded &&
                   missing_import.report.find("\"id\":\"images/noise\"") != std::string::npos &&
                   missing_import.report.find("\"status\":\"missing\"") != std::string::npos &&
                   missing_import.report.find("\"resources_complete\":false") != std::string::npos,
               "missing resource was substituted or not reported by identity");
}

}  // namespace

int main() {
    const std::string serialized = ctex::doc::serialize_smart_material(material_fixture());
    return inspection_reports_content(serialized) && parameter_updates_all_bindings(serialized) &&
                   anchors_are_atomic_and_planned(serialized) &&
                   versions_are_migrated_or_refused(serialized) &&
                   resources_are_portable_and_self_contained(serialized)
               ? 0
               : 1;
}
