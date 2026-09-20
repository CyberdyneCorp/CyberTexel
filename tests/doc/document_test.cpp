#include <array>
#include <cstddef>
#include <ctex/doc/document.hpp>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

using namespace ctex;
using namespace ctex::doc;
using ctex::doc::PartitionSourceKind;
using ctex::doc::TextureDocument;
using ctex::doc::TextureSetDescriptor;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_application_error(Callable&& callable, std::string_view message) {
    try {
        callable();
    } catch (const SmartMaterialError& error) {
        return expect(error.code() == SmartMaterialErrorCode::invalid_preset, message);
    } catch (...) {
    }
    return expect(false, message);
}

graph::GraphDocument application_graph(double value) {
    return graph::GraphDocument({.role = graph::NodeRole::output,
                                 .type_id = "ctex.output.application-test",
                                 .type_version = 1,
                                 .display_name = "Application output",
                                 .position = {},
                                 .inputs = {{.identifier = "amount",
                                             .display_name = "Amount",
                                             .type = graph::SocketType::scalar,
                                             .value = value}},
                                 .outputs = {},
                                 .properties = {}});
}

SmartMaterialPreset twelve_layer_material() {
    SmartMaterialPreset preset{.schema_version = current_smart_material_schema_version,
                               .identifier = "materials/twelve-layer",
                               .display_name = "Twelve layer material",
                               .stack = {},
                               .exposed_parameters = {},
                               .anchor_entries = {},
                               .anchor_references = {},
                               .resource_references = {}};
    for (std::size_t index = 0; index < 12; ++index) {
        preset.stack.push_back(
            {.identifier = "layer-" + std::to_string(index),
             .parent_identifier = {},
             .display_name = "Layer " + std::to_string(index),
             .kind = SmartMaterialEntryKind::layer,
             .enabled = true,
             .opacity = 1.0,
             .graph = index == 0 ? std::optional<graph::GraphDocument>{application_graph(0.9)}
                                 : std::nullopt,
             .content_kind = index == 11 ? SmartMaterialContentKind::model_specific
                                         : SmartMaterialContentKind::derived,
             .pixel_payloads =
                 index == 11
                     ? std::vector<SmartMaterialPixelPayload>{{.identifier = "paint",
                                                               .width = 1,
                                                               .height = 1,
                                                               .format =
                                                                   {image::ChannelType::uint8_unorm,
                                                                    1},
                                                               .pixels = {std::byte{128}}}}
                     : std::vector<SmartMaterialPixelPayload>{}});
    }
    preset.exposed_parameters.push_back(
        {.identifier = "amount",
         .display_name = "Amount",
         .display_group = "Surface",
         .type = graph::SocketType::scalar,
         .default_value = 0.25,
         .minimum = 0.0,
         .maximum = 1.0,
         .bindings = {{.entry_identifier = "layer-0",
                       .node_id = 1,
                       .target_kind = SmartMaterialBindingTargetKind::input,
                       .target_identifier = "amount"}}});
    return preset;
}

SmartMaskPreset application_mask() {
    SmartMaterialPreset definition{
        .schema_version = current_smart_material_schema_version,
        .identifier = "masks/wear",
        .display_name = "Wear mask",
        .stack = {{.identifier = "root",
                   .parent_identifier = {},
                   .display_name = "Wear",
                   .kind = SmartMaterialEntryKind::mask,
                   .enabled = true,
                   .opacity = 1.0,
                   .graph = application_graph(0.6),
                   .content_kind = SmartMaterialContentKind::derived,
                   .pixel_payloads = {}}},
        .exposed_parameters = {{.identifier = "amount",
                                .display_name = "Amount",
                                .display_group = "Wear",
                                .type = graph::SocketType::scalar,
                                .default_value = 0.6,
                                .minimum = 0.0,
                                .maximum = 1.0,
                                .bindings = {{.entry_identifier = "root",
                                              .node_id = 1,
                                              .target_kind = SmartMaterialBindingTargetKind::input,
                                              .target_identifier = "amount"}}}},
        .anchor_entries = {},
        .anchor_references = {},
        .resource_references = {},
    };
    return {.schema_version = current_smart_mask_schema_version,
            .definition = std::move(definition)};
}

TextureSetDescriptor descriptor(std::string name, std::string partition, std::string uv_set,
                                std::uint32_t resolution, std::uint8_t bit_depth) {
    return {
        .display_name = std::move(name),
        .partition_kind = PartitionSourceKind::material,
        .partition_key = std::move(partition),
        .uv_set = std::move(uv_set),
        .width = resolution,
        .height = resolution,
        .default_bit_depth = bit_depth,
    };
}

bool sets_have_independent_storage() {
    TextureDocument document;
    auto& body = document.create_texture_set(descriptor("Body", "body-material", "uv0", 4096, 8));
    const std::string body_id = body.id();
    auto& eyes = document.create_texture_set(descriptor("Eyes", "eye-material", "uv1", 1024, 16));
    const std::string eyes_id = eyes.id();

    document.texture_set(body_id).channels().enable("pbr.base_color");
    const std::array red{std::byte{255}, std::byte{0}, std::byte{0}};
    document.texture_set(body_id).channels().pixels("pbr.base_color").write_pixel(0, 0, red);

    return expect(document.texture_set_count() == 2, "document did not retain both texture sets") &&
           expect(document.texture_set(body_id).descriptor().width == 4096,
                  "body resolution changed") &&
           expect(document.texture_set(eyes_id).descriptor().width == 1024,
                  "eye resolution changed") &&
           expect(document.texture_set(eyes_id).descriptor().default_bit_depth == 16,
                  "eye precision changed") &&
           expect(!document.texture_set(eyes_id).channels().is_enabled("pbr.base_color"),
                  "painting body allocated eye storage");
}

bool identity_survives_reordering_and_rename() {
    TextureDocument first;
    const std::string body_id =
        first.create_texture_set(descriptor("Body", "body-material", "uv0", 2048, 8)).id();
    const std::string eyes_id =
        first.create_texture_set(descriptor("Eyes", "eye-material", "uv0", 2048, 8)).id();

    TextureDocument reordered;
    const std::string renamed_eyes_id =
        reordered.create_texture_set(descriptor("Renamed eyes", "eye-material", "uv0", 2048, 8))
            .id();
    const std::string renamed_body_id =
        reordered.create_texture_set(descriptor("Renamed body", "body-material", "uv0", 2048, 8))
            .id();

    return expect(body_id == renamed_body_id, "body identity depended on order or display name") &&
           expect(eyes_id == renamed_eyes_id, "eye identity depended on order or display name") &&
           expect(first.texture_set_ids() == reordered.texture_set_ids(),
                  "stable identity enumeration changed after reorder");
}

bool uv_binding_participates_in_identity() {
    const auto uv0 = descriptor("Body UV0", "body-material", "uv0", 1024, 8);
    const auto uv1 = descriptor("Body UV1", "body-material", "uv1", 1024, 8);
    return expect(ctex::doc::texture_set_stable_id(uv0) != ctex::doc::texture_set_stable_id(uv1),
                  "UV binding was absent from texture-set identity");
}

bool smart_material_application_is_one_editable_undo_step() {
    TextureDocument document;
    TextureSet& set = document.create_texture_set(descriptor("Body", "body", "uv0", 32, 8));
    const SmartMaterialPreset preset = twelve_layer_material();
    const PresetApplicationReport report = set.apply_smart_material(preset, "application-1");
    const std::string first_entry = "application-1/layer-0";
    const auto& applied_input = set.applied_entry(first_entry).graph->node(1).inputs.front().value;
    const bool applied =
        expect(report.entry_identifiers.size() == 12 && report.content.derived_entry_count == 11 &&
                   report.content.model_specific_entry_count == 1 &&
                   report.content.model_specific_pixel_bytes == 1 &&
                   set.preset_application_count() == 1 && set.preset_undo_step_count() == 1 &&
                   set.layer_stack().size() == 12 &&
                   set.layer_stack().entry(first_entry).kind == LayerEntryKind::paint_layer,
               "twelve-layer material was not one application and undo step") &&
        expect(std::get<double>(applied_input) == 0.25,
               "smart material application did not reset its exposed default") &&
        expect(set.applied_entry_origin(first_entry) ==
                   AppliedPresetOrigin{.preset_identifier = preset.identifier,
                                       .schema_version = preset.schema_version},
               "applied entry did not retain its preset origin");

    const std::uint64_t original_revision = set.layer_stack().entry(first_entry).content_revision;
    static_cast<void>(set.set_applied_preset_parameter_value("application-1", "amount", 0.75));
    SmartMaterialEntry edited = set.applied_entry(first_entry);
    edited.display_name = "Edited layer";
    set.replace_applied_entry(first_entry, std::move(edited));
    SmartMaterialEntry renamed = set.applied_entry(first_entry);
    renamed.identifier = "changed-identity";
    const bool identity_refused = expect_application_error(
        [&] { set.replace_applied_entry(first_entry, std::move(renamed)); },
        "applied entry edit changed its stable identity");
    const bool editable =
        identity_refused &&
        expect(
            set.applied_entry(first_entry).display_name == "Edited layer" &&
                std::get<double>(
                    set.applied_entry(first_entry).graph->node(1).inputs.front().value) == 0.75 &&
                set.layer_stack().entry(first_entry).content_revision == original_revision + 1 &&
                set.applied_entry_origin(first_entry).preset_identifier == preset.identifier,
            "ordinary applied entry edit or parameter update lost origin metadata");

    const PresetApplicationUndoReport undone = set.undo_last_preset_application();
    return applied && editable &&
           expect(undone.removed && undone.entry_identifiers.size() == 12 &&
                      set.preset_application_count() == 0 && set.preset_undo_step_count() == 0 &&
                      set.layer_stack().empty(),
                  "one undo did not remove all twelve instantiated entries") &&
           expect_application_error([&] { static_cast<void>(set.applied_entry(first_entry)); },
                                    "undone smart material entry remained addressable");
}

bool smart_mask_application_is_independent_and_transactional() {
    TextureDocument document;
    TextureSet& set = document.create_texture_set(descriptor("Body", "body", "uv0", 32, 8));
    static_cast<void>(set.apply_smart_material(twelve_layer_material(), "material"));
    const std::string target = "material/layer-0";
    const SmartMaskPreset mask = application_mask();
    const PresetApplicationReport applied = set.apply_smart_mask(mask, "mask-1", target);
    const std::string mask_entry = "mask-1/root";
    const bool attached =
        expect(applied.entry_identifiers == std::vector<std::string>{mask_entry} &&
                   set.preset_applications().back().target_entry_identifier == target &&
                   set.applied_entry_origin(mask_entry) ==
                       AppliedPresetOrigin{.preset_identifier = mask.definition.identifier,
                                           .schema_version = mask.schema_version},
               "smart mask did not attach independently with outer preset origin");
    const std::size_t before = set.preset_application_count();
    const bool invalid_target = expect_application_error(
        [&] { static_cast<void>(set.apply_smart_mask(mask, "mask-2", "missing")); },
        "smart mask accepted a missing target");
    const PresetApplicationUndoReport undone = set.undo_last_preset_application();
    return attached && invalid_target &&
           expect(set.preset_application_count() == before - 1 && undone.removed &&
                      undone.application_identifier == "mask-1" &&
                      set.applied_entry(target).identifier == target,
                  "smart mask refusal or undo changed its target material");
}

}  // namespace

int main() {
    return sets_have_independent_storage() && identity_survives_reordering_and_rename() &&
                   uv_binding_participates_in_identity() &&
                   smart_material_application_is_one_editable_undo_step() &&
                   smart_mask_application_is_independent_and_transactional()
               ? 0
               : 1;
}
