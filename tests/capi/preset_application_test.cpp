#include <ctex/capi.h>

#include <ctex/doc/smart_mask.hpp>
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
                                       .type_id = "ctex.output.preset-application-capi",
                                       .type_version = 1,
                                       .display_name = std::move(name),
                                       .position = {},
                                       .inputs = {},
                                       .outputs = {},
                                       .properties = {}});
}

ctex::doc::SmartMaterialPreset material_fixture() {
    ctex::doc::SmartMaterialPreset material{
        .schema_version = ctex::doc::current_smart_material_schema_version,
        .identifier = "materials/capi-application",
        .display_name = "C API application",
        .stack = {},
        .exposed_parameters = {},
        .anchor_entries = {},
        .anchor_references = {},
        .resource_references = {},
    };
    for (std::size_t index = 0; index < 12; ++index) {
        material.stack.push_back({
            .identifier = "layer-" + std::to_string(index),
            .parent_identifier = {},
            .display_name = "Layer " + std::to_string(index),
            .kind = ctex::doc::SmartMaterialEntryKind::layer,
            .enabled = true,
            .opacity = 1.0,
            .graph = graph_fixture("Layer graph"),
            .content_kind = ctex::doc::SmartMaterialContentKind::derived,
            .pixel_payloads = {},
        });
    }
    return material;
}

ctex::doc::SmartMaskPreset mask_fixture() {
    return {
        .schema_version = ctex::doc::current_smart_mask_schema_version,
        .definition = {.schema_version = ctex::doc::current_smart_material_schema_version,
                       .identifier = "masks/capi-application",
                       .display_name = "C API mask",
                       .stack = {{.identifier = "root",
                                  .parent_identifier = {},
                                  .display_name = "Root mask",
                                  .kind = ctex::doc::SmartMaterialEntryKind::mask,
                                  .enabled = true,
                                  .opacity = 1.0,
                                  .graph = graph_fixture("Mask graph"),
                                  .content_kind = ctex::doc::SmartMaterialContentKind::derived,
                                  .pixel_payloads = {}}},
                       .exposed_parameters = {},
                       .anchor_entries = {},
                       .anchor_references = {},
                       .resource_references = {}},
    };
}

bool create_document(ctex_document** document, std::string& texture_set_id) {
    const ctex_texture_set_descriptor descriptor{
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Body",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "body",
        .uv_set = "uv0",
        .width = 32,
        .height = 32,
        .default_bit_depth = 8,
    };
    if (!expect(ctex_document_create(document) == CTEX_RESULT_SUCCESS && *document != nullptr,
                "document creation failed") ||
        !expect(ctex_document_create_texture_set(*document, &descriptor) == CTEX_RESULT_SUCCESS,
                "texture-set creation failed")) {
        return false;
    }
    std::size_t required_size = 0;
    std::size_t count = 0;
    if (!expect(ctex_document_get_texture_set_ids(*document, nullptr, 0, &required_size, &count) ==
                    CTEX_RESULT_SUCCESS,
                "texture-set identifier sizing failed")) {
        return false;
    }
    texture_set_id.resize(required_size);
    return expect(
        ctex_document_get_texture_set_ids(*document, texture_set_id.data(), texture_set_id.size(),
                                          &required_size, &count) == CTEX_RESULT_SUCCESS &&
            count == 1,
        "texture-set identifier query failed");
}

std::string application_report(const ctex_document* document, const char* texture_set_id) {
    std::size_t required_size = 0;
    if (ctex_texture_set_get_preset_applications(document, texture_set_id, nullptr, 0,
                                                 &required_size) != CTEX_RESULT_SUCCESS) {
        return {};
    }
    std::vector<char> output(required_size);
    if (ctex_texture_set_get_preset_applications(document, texture_set_id, output.data(),
                                                 output.size(),
                                                 &required_size) != CTEX_RESULT_SUCCESS) {
        return {};
    }
    return output.data();
}

bool applications_are_transactional_and_inspectable() {
    ctex_document* document = nullptr;
    std::string texture_set_id;
    if (!create_document(&document, texture_set_id)) {
        ctex_document_destroy(document);
        return false;
    }
    const std::string material = ctex::doc::serialize_smart_material(material_fixture());
    ctex_preset_application_info material_info{};
    material_info.size = CTEX_PRESET_APPLICATION_INFO_CURRENT_SIZE;
    bool passed =
        expect(ctex_texture_set_apply_smart_material(document, texture_set_id.c_str(),
                                                     material.data(), material.size(), "material",
                                                     &material_info) == CTEX_RESULT_SUCCESS,
               "smart material application failed") &&
        expect(
            material_info.kind == CTEX_APPLIED_PRESET_SMART_MATERIAL &&
                material_info.schema_version == ctex::doc::current_smart_material_schema_version &&
                material_info.entry_count == 12 && material_info.application_count == 1 &&
                material_info.undo_step_count == 1,
            "smart material was not one twelve-entry undo step");

    const std::string first_report = application_report(document, texture_set_id.c_str());
    passed = expect(first_report.find("\"preset\":\"materials/capi-application\"") !=
                            std::string::npos &&
                        first_report.find("\"id\":\"material/layer-0\"") != std::string::npos,
                    "applied material entries or origin were not inspectable") &&
             passed;
    passed = expect(ctex_texture_set_set_applied_entry_state(document, texture_set_id.c_str(),
                                                             "material/layer-0", 0,
                                                             0.25) == CTEX_RESULT_SUCCESS,
                    "applied entry edit failed") &&
             passed;
    const std::string edited_report = application_report(document, texture_set_id.c_str());
    passed = expect(edited_report.find("\"id\":\"material/layer-0\",\"parent\":\"\","
                                       "\"kind\":0,\"enabled\":false,\"opacity\":0.250000") !=
                        std::string::npos,
                    "applied entry edit or origin-preserving inspection failed") &&
             passed;

    const std::string mask = ctex::doc::serialize_smart_mask(mask_fixture());
    ctex_preset_application_info mask_info{};
    mask_info.size = CTEX_PRESET_APPLICATION_INFO_CURRENT_SIZE;
    passed =
        expect(ctex_texture_set_apply_smart_mask(document, texture_set_id.c_str(), mask.data(),
                                                 mask.size(), "mask", "material/layer-0",
                                                 &mask_info) == CTEX_RESULT_SUCCESS,
               "smart mask application failed") &&
        expect(mask_info.kind == CTEX_APPLIED_PRESET_SMART_MASK && mask_info.entry_count == 1 &&
                   mask_info.application_count == 2 && mask_info.undo_step_count == 2,
               "smart mask was not an independent application") &&
        passed;
    const std::string mask_report = application_report(document, texture_set_id.c_str());
    passed =
        expect(mask_report.find("\"kind\":\"smart-mask\"") != std::string::npos &&
                   mask_report.find("\"target\":\"material/layer-0\"") != std::string::npos &&
                   mask_report.find("\"preset\":\"masks/capi-application\"") != std::string::npos,
               "smart mask target or origin was not inspectable") &&
        passed;

    ctex_preset_application_info refused_info{};
    refused_info.size = CTEX_PRESET_APPLICATION_INFO_CURRENT_SIZE;
    passed =
        expect(ctex_texture_set_apply_smart_mask(document, texture_set_id.c_str(), mask.data(),
                                                 mask.size(), "bad-mask", "missing",
                                                 &refused_info) == CTEX_RESULT_INVALID_ARGUMENT,
               "smart mask accepted a missing target") &&
        expect(
            application_report(document, texture_set_id.c_str()).find("\"application_count\":2") !=
                std::string::npos,
            "refused smart mask application changed the document") &&
        passed;

    ctex_preset_undo_info undo{};
    undo.size = CTEX_PRESET_UNDO_INFO_CURRENT_SIZE;
    passed = expect(ctex_texture_set_undo_last_preset_application(document, texture_set_id.c_str(),
                                                                  &undo) == CTEX_RESULT_SUCCESS &&
                        undo.removed == 1 && undo.removed_entry_count == 1 &&
                        undo.application_count == 1 && undo.undo_step_count == 1,
                    "one undo did not remove the smart mask application") &&
             passed;
    undo.size = CTEX_PRESET_UNDO_INFO_CURRENT_SIZE;
    passed = expect(ctex_texture_set_undo_last_preset_application(document, texture_set_id.c_str(),
                                                                  &undo) == CTEX_RESULT_SUCCESS &&
                        undo.removed == 1 && undo.removed_entry_count == 12 &&
                        undo.application_count == 0 && undo.undo_step_count == 0,
                    "one undo did not remove all smart material entries") &&
             passed;
    undo.size = CTEX_PRESET_UNDO_INFO_CURRENT_SIZE;
    passed = expect(ctex_texture_set_undo_last_preset_application(document, texture_set_id.c_str(),
                                                                  &undo) == CTEX_RESULT_SUCCESS &&
                        undo.removed == 0,
                    "empty undo reported a removal") &&
             passed;
    ctex_document_destroy(document);
    return passed;
}

}  // namespace

int main() { return applications_are_transactional_and_inspectable() ? 0 : 1; }
