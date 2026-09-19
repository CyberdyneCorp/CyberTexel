#include <ctex/doc/smart_mask.hpp>
#include <iostream>
#include <string_view>

namespace {

using namespace ctex;
using namespace ctex::doc;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, SmartMaterialErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const SmartMaterialError& error) {
        return expect(error.code() == code, message);
    } catch (...) {
    }
    return expect(false, message);
}

graph::GraphDocument mask_graph(std::string name) {
    return graph::GraphDocument({.role = graph::NodeRole::output,
                                 .type_id = "ctex.output.smart-mask",
                                 .type_version = 1,
                                 .display_name = std::move(name),
                                 .position = {},
                                 .inputs = {{.identifier = "amount",
                                             .display_name = "Amount",
                                             .type = graph::SocketType::scalar,
                                             .value = 0.4}},
                                 .outputs = {},
                                 .properties = {{.key = "strength", .value = 0.4}}});
}

SmartMaskPreset mask_preset() {
    return {
        .schema_version = current_smart_mask_schema_version,
        .definition = {.schema_version = current_smart_material_schema_version,
                       .identifier = "masks/edge-wear",
                       .display_name = "Edge wear",
                       .stack = {{.identifier = "mask",
                                  .parent_identifier = {},
                                  .display_name = "Mask",
                                  .kind = SmartMaterialEntryKind::mask,
                                  .enabled = true,
                                  .opacity = 1.0,
                                  .graph = mask_graph("Mask graph"),
                                  .content_kind = SmartMaterialContentKind::derived,
                                  .pixel_payloads = {}},
                                 {.identifier = "generator",
                                  .parent_identifier = "mask",
                                  .display_name = "Edge generator",
                                  .kind = SmartMaterialEntryKind::generator,
                                  .enabled = true,
                                  .opacity = 1.0,
                                  .graph = mask_graph("Generator graph"),
                                  .content_kind = SmartMaterialContentKind::derived,
                                  .pixel_payloads = {}},
                                 {.identifier = "filter",
                                  .parent_identifier = "generator",
                                  .display_name = "Levels",
                                  .kind = SmartMaterialEntryKind::filter,
                                  .enabled = true,
                                  .opacity = 1.0,
                                  .graph = mask_graph("Filter graph"),
                                  .content_kind = SmartMaterialContentKind::derived,
                                  .pixel_payloads = {}}},
                       .exposed_parameters =
                           {{.identifier = "wear-amount",
                             .display_name = "Wear amount",
                             .display_group = "Wear",
                             .type = graph::SocketType::scalar,
                             .default_value = 0.4,
                             .minimum = 0.0,
                             .maximum = 1.0,
                             .bindings = {{.entry_identifier = "mask",
                                           .node_id = 1,
                                           .target_kind = SmartMaterialBindingTargetKind::input,
                                           .target_identifier = "amount"},
                                          {.entry_identifier = "generator",
                                           .node_id = 1,
                                           .target_kind = SmartMaterialBindingTargetKind::property,
                                           .target_identifier = "strength"}}}},
                       .anchor_entries = {},
                       .anchor_references = {}},
    };
}

double input_amount(const SmartMaskInstance& instance, std::size_t entry_index) {
    return std::get<double>(
        instance.fragment.stack[entry_index].graph->node(1).inputs.front().value);
}

double property_strength(const SmartMaskInstance& instance, std::size_t entry_index) {
    return std::get<double>(
        instance.fragment.stack[entry_index].graph->node(1).properties.front().value);
}

bool preset_round_trips_canonically() {
    const SmartMaskPreset source = mask_preset();
    const std::string serialized = serialize_smart_mask(source);
    const SmartMaskPreset restored = deserialize_smart_mask(serialized);
    return expect(restored == source, "smart mask definition did not round-trip") &&
           expect(serialize_smart_mask(restored) == serialized,
                  "smart mask serialization is not canonical");
}

bool instances_own_independent_parameter_state() {
    const SmartMaskPreset preset = mask_preset();
    SmartMaskInstance layer =
        instantiate_smart_mask(preset, "instance/layer", "layer/base", SmartMaskTargetKind::layer);
    SmartMaskInstance group = instantiate_smart_mask(preset, "instance/group", "group/details",
                                                     SmartMaskTargetKind::group);
    const SmartMaterialParameterUpdate update =
        set_smart_mask_parameter_value(layer, "wear-amount", 0.85);
    return expect(update.updated_bindings.size() == 2 && input_amount(layer, 0) == 0.85 &&
                      property_strength(layer, 1) == 0.85,
                  "smart mask instance did not update every bound target") &&
           expect(input_amount(group, 0) == 0.4 && property_strength(group, 1) == 0.4,
                  "editing one smart mask instance changed another") &&
           expect(
               std::get<double>(preset.definition.stack[0].graph->node(1).inputs[0].value) == 0.4,
               "editing a smart mask instance changed its preset") &&
           expect(layer.target_kind == SmartMaskTargetKind::layer &&
                      group.target_kind == SmartMaskTargetKind::group &&
                      layer.origin_preset_identifier == preset.definition.identifier,
                  "smart mask instance did not preserve target or origin metadata");
}

bool invalid_definitions_and_updates_are_refused() {
    SmartMaskPreset layer_entry = mask_preset();
    layer_entry.definition.stack.front().kind = SmartMaterialEntryKind::layer;
    const bool layer_refused =
        expect_error([&] { validate_smart_mask(layer_entry); },
                     SmartMaterialErrorCode::invalid_preset, "smart mask accepted a layer entry");

    SmartMaskPreset second_root = mask_preset();
    second_root.definition.stack[1].parent_identifier.clear();
    const bool second_root_refused =
        expect_error([&] { validate_smart_mask(second_root); },
                     SmartMaterialErrorCode::invalid_preset, "smart mask accepted a second root");

    SmartMaskPreset painted = mask_preset();
    painted.definition.stack.front().content_kind = SmartMaterialContentKind::model_specific;
    painted.definition.stack.front().pixel_payloads = {
        {.identifier = "mask",
         .width = 1,
         .height = 1,
         .format = {image::ChannelType::uint8_unorm, 1},
         .pixels = {std::byte{255}}}};
    const bool painted_refused =
        expect_error([&] { validate_smart_mask(painted); }, SmartMaterialErrorCode::invalid_preset,
                     "smart mask accepted model-specific painted pixels");

    SmartMaskPreset no_graph = mask_preset();
    for (SmartMaterialEntry& entry : no_graph.definition.stack) {
        entry.graph.reset();
    }
    const bool no_graph_refused =
        expect_error([&] { validate_smart_mask(no_graph); }, SmartMaterialErrorCode::invalid_preset,
                     "smart mask accepted a definition without a graph");

    SmartMaskInstance instance =
        instantiate_smart_mask(mask_preset(), "instance", "layer", SmartMaskTargetKind::layer);
    const SmartMaskInstance before = instance;
    const bool range_refused = expect_error(
        [&] { static_cast<void>(set_smart_mask_parameter_value(instance, "wear-amount", 2.0)); },
        SmartMaterialErrorCode::invalid_parameter_value,
        "smart mask instance accepted an out-of-range parameter");
    const bool target_refused = expect_error(
        [&] {
            static_cast<void>(instantiate_smart_mask(mask_preset(), "bad", "target",
                                                     static_cast<SmartMaskTargetKind>(99)));
        },
        SmartMaterialErrorCode::invalid_preset, "smart mask accepted an unknown target kind");
    return layer_refused && second_root_refused && painted_refused && no_graph_refused &&
           range_refused && target_refused &&
           expect(instance == before, "refused smart mask update changed the instance");
}

bool malformed_and_future_presets_are_refused() {
    const std::string valid = serialize_smart_mask(mask_preset());
    std::string future = valid;
    future.replace(0, std::string_view("CTEX_SMART_MASK\t1").size(), "CTEX_SMART_MASK\t2");
    const bool future_refused = expect_error(
        [&] { static_cast<void>(deserialize_smart_mask(future)); },
        SmartMaterialErrorCode::unsupported_version, "future smart mask version was accepted");
    return future_refused &&
           expect_error([&] { static_cast<void>(deserialize_smart_mask(valid.substr(0, 20))); },
                        SmartMaterialErrorCode::malformed_serialization,
                        "truncated smart mask was accepted");
}

}  // namespace

int main() {
    return preset_round_trips_canonically() && instances_own_independent_parameter_state() &&
                   invalid_definitions_and_updates_are_refused() &&
                   malformed_and_future_presets_are_refused()
               ? 0
               : 1;
}
