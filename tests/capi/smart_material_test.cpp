#include <ctex/capi.h>

#include <cstddef>
#include <ctex/doc/smart_material.hpp>
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
    return ctex::graph::GraphDocument({.role = ctex::graph::NodeRole::output,
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
                                                   .value = ctex::graph::ImageValue{}}},
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
        .resource_references = {},
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
                      result.info.anchor_reference_count == 1,
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

}  // namespace

int main() {
    const std::string serialized = ctex::doc::serialize_smart_material(material_fixture());
    return inspection_reports_content(serialized) && parameter_updates_all_bindings(serialized) &&
                   anchors_are_atomic_and_planned(serialized) &&
                   versions_are_migrated_or_refused(serialized)
               ? 0
               : 1;
}
