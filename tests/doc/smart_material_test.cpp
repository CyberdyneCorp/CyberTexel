#include <algorithm>
#include <array>
#include <ctex/doc/smart_material.hpp>
#include <iostream>
#include <limits>
#include <string>
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

template <typename Callable>
bool expect_error_text(Callable&& callable, SmartMaterialErrorCode code, std::string_view text,
                       std::string_view message) {
    try {
        callable();
    } catch (const SmartMaterialError& error) {
        return expect(error.code() == code &&
                          std::string_view(error.what()).find(text) != std::string_view::npos,
                      message);
    } catch (...) {
    }
    return expect(false, message);
}

graph::GraphDocument graph_fixture(std::string name) {
    return graph::GraphDocument({.role = graph::NodeRole::output,
                                 .type_id = "ctex.output.smart-material",
                                 .type_version = 1,
                                 .display_name = std::move(name),
                                 .position = {},
                                 .inputs = {{.identifier = "value",
                                             .display_name = "Value",
                                             .type = graph::SocketType::scalar,
                                             .value = 0.4},
                                            {.identifier = "colour",
                                             .display_name = "Colour",
                                             .type = graph::SocketType::colour,
                                             .value = graph::ColourValue{0.4F, 0.1F, 0.02F, 1.0F}},
                                            {.identifier = "enabled",
                                             .display_name = "Enabled",
                                             .type = graph::SocketType::boolean,
                                             .value = true},
                                            {.identifier = "anchor",
                                             .display_name = "Anchor",
                                             .type = graph::SocketType::image,
                                             .value = graph::ImageValue{}}},
                                 .outputs = {},
                                 .properties = {{.key = "strength", .value = 0.4}}});
}

SmartMaterialPreset rich_preset() {
    return {
        .schema_version = current_smart_material_schema_version,
        .identifier = "materials/weathered-steel",
        .display_name = "Weathered\tSteel\n\xc3\xa9",
        .stack =
            {
                {.identifier = "surface",
                 .parent_identifier = {},
                 .display_name = "Surface",
                 .kind = SmartMaterialEntryKind::group,
                 .enabled = true,
                 .opacity = 1.0,
                 .graph = std::nullopt,
                 .content_kind = SmartMaterialContentKind::derived,
                 .pixel_payloads = {}},
                {.identifier = "base",
                 .parent_identifier = "surface",
                 .display_name = "Base layer",
                 .kind = SmartMaterialEntryKind::layer,
                 .enabled = true,
                 .opacity = 0.8,
                 .graph = graph_fixture("Base graph"),
                 .content_kind = SmartMaterialContentKind::derived,
                 .pixel_payloads = {}},
                {.identifier = "wear-mask",
                 .parent_identifier = "base",
                 .display_name = "Wear mask",
                 .kind = SmartMaterialEntryKind::mask,
                 .enabled = true,
                 .opacity = 1.0,
                 .graph = graph_fixture("Mask graph"),
                 .content_kind = SmartMaterialContentKind::derived,
                 .pixel_payloads = {}},
                {.identifier = "levels",
                 .parent_identifier = "wear-mask",
                 .display_name = "Levels filter",
                 .kind = SmartMaterialEntryKind::filter,
                 .enabled = false,
                 .opacity = 0.75,
                 .graph = graph_fixture("Filter graph"),
                 .content_kind = SmartMaterialContentKind::derived,
                 .pixel_payloads = {}},
                {.identifier = "scratches",
                 .parent_identifier = "surface",
                 .display_name = "Scratch generator",
                 .kind = SmartMaterialEntryKind::generator,
                 .enabled = true,
                 .opacity = 0.6,
                 .graph = graph_fixture("Generator graph"),
                 .content_kind = SmartMaterialContentKind::derived,
                 .pixel_payloads = {}},
                {.identifier = "top-coat",
                 .parent_identifier = "surface",
                 .display_name = "Top coat",
                 .kind = SmartMaterialEntryKind::layer,
                 .enabled = true,
                 .opacity = 1.0,
                 .graph = graph_fixture("Top coat graph"),
                 .content_kind = SmartMaterialContentKind::derived,
                 .pixel_payloads = {}},
                {.identifier = "paint-mask",
                 .parent_identifier = "base",
                 .display_name = "Hand-painted wear",
                 .kind = SmartMaterialEntryKind::mask,
                 .enabled = true,
                 .opacity = 1.0,
                 .graph = std::nullopt,
                 .content_kind = SmartMaterialContentKind::model_specific,
                 .pixel_payloads = {{.identifier = "mask",
                                     .width = 2,
                                     .height = 2,
                                     .format = {image::ChannelType::uint8_unorm, 1},
                                     .pixels = {std::byte{0}, std::byte{64}, std::byte{128},
                                                std::byte{255}}}}},
            },
        .exposed_parameters =
            {
                {.identifier = "wear-amount",
                 .display_name = "Wear amount",
                 .display_group = "Wear",
                 .type = graph::SocketType::scalar,
                 .default_value = 0.4,
                 .minimum = 0.0,
                 .maximum = 1.0,
                 .bindings = {{.entry_identifier = "base",
                               .node_id = 1,
                               .target_kind = SmartMaterialBindingTargetKind::input,
                               .target_identifier = "value"},
                              {.entry_identifier = "wear-mask",
                               .node_id = 1,
                               .target_kind = SmartMaterialBindingTargetKind::input,
                               .target_identifier = "value"},
                              {.entry_identifier = "scratches",
                               .node_id = 1,
                               .target_kind = SmartMaterialBindingTargetKind::property,
                               .target_identifier = "strength"}}},
                {.identifier = "rust-colour",
                 .display_name = "Rust colour",
                 .display_group = "Surface",
                 .type = graph::SocketType::colour,
                 .default_value = graph::ColourValue{0.4F, 0.1F, 0.02F, 1.0F},
                 .minimum = 0.0,
                 .maximum = 1.0,
                 .bindings = {{.entry_identifier = "base",
                               .node_id = 1,
                               .target_kind = SmartMaterialBindingTargetKind::input,
                               .target_identifier = "colour"}}},
                {.identifier = "use-scratches",
                 .display_name = "Use scratches",
                 .display_group = "Wear",
                 .type = graph::SocketType::boolean,
                 .default_value = true,
                 .minimum = std::nullopt,
                 .maximum = std::nullopt,
                 .bindings = {{.entry_identifier = "scratches",
                               .node_id = 1,
                               .target_kind = SmartMaterialBindingTargetKind::input,
                               .target_identifier = "enabled"}}},
            },
        .anchor_entries = {"base"},
        .anchor_references = {{.anchor_entry_identifier = "base",
                               .consumer_entry_identifier = "top-coat",
                               .consumer_node_id = 1,
                               .consumer_input_identifier = "anchor"}},
        .resource_references = {},
    };
}

std::string as_historical_schema(const SmartMaterialPreset& preset, std::uint32_t version) {
    const std::string current = serialize_smart_material(preset);
    std::string result;
    std::size_t begin = 0;
    while (begin < current.size()) {
        const std::size_t end = current.find('\n', begin);
        std::string line = current.substr(begin, end - begin);
        const std::string_view tag = std::string_view(line).substr(0, line.find('\t'));
        bool retain = true;
        if (tag == "CTEX_SMART_MATERIAL") {
            line = "CTEX_SMART_MATERIAL\t" + std::to_string(version);
        } else if (tag == "ENTRY" && version == 1) {
            line.erase(line.rfind('\t'));
        } else if (tag == "PIXELS" && version == 1) {
            retain = false;
        } else if ((tag == "ANCHOR" || tag == "ANCHOR_REF") && version < 4) {
            retain = false;
        } else if (tag == "RESOURCE" && version < 5) {
            retain = false;
        } else if (tag == "PARAM") {
            line.erase(line.rfind('\t'));
        } else if (tag == "BIND" && version < 3) {
            retain = false;
        }
        if (retain) {
            result += line + '\n';
        }
        begin = end + 1;
    }
    return result;
}

const graph::SocketValue& input_value(const SmartMaterialPreset& preset, std::size_t entry_index,
                                      std::string_view identifier) {
    const auto& inputs = preset.stack[entry_index].graph->node(1).inputs;
    return std::find_if(
               inputs.begin(), inputs.end(),
               [&](const graph::NodeSocket& input) { return input.identifier == identifier; })
        ->value;
}

const graph::SocketValue& property_value(const SmartMaterialPreset& preset, std::size_t entry_index,
                                         std::string_view key) {
    const auto& properties = preset.stack[entry_index].graph->node(1).properties;
    return std::find_if(properties.begin(), properties.end(),
                        [&](const graph::NodeProperty& item) { return item.key == key; })
        ->value;
}

bool one_parameter_updates_every_bound_entry() {
    SmartMaterialPreset preset = rich_preset();
    const SmartMaterialParameterUpdate update =
        set_smart_material_parameter_value(preset, "wear-amount", 0.85);
    const graph::ColourValue rust{0.2F, 0.3F, 0.4F, 1.0F};
    static_cast<void>(set_smart_material_parameter_value(preset, "rust-colour", rust));
    static_cast<void>(set_smart_material_parameter_value(preset, "use-scratches", false));
    return expect(
               update.parameter_identifier == "wear-amount" && update.updated_bindings.size() == 3,
               "parameter update did not report every bound target") &&
           expect(std::get<double>(input_value(preset, 1, "value")) == 0.85 &&
                      std::get<double>(input_value(preset, 2, "value")) == 0.85 &&
                      std::get<double>(property_value(preset, 4, "strength")) == 0.85,
                  "one exposed parameter did not update all bound graph values") &&
           expect(std::get<graph::ColourValue>(input_value(preset, 1, "colour")) == rust &&
                      !std::get<bool>(input_value(preset, 4, "enabled")),
                  "typed colour or boolean binding did not update") &&
           expect(std::get<double>(preset.exposed_parameters.front().default_value) == 0.4,
                  "setting a parameter changed its declared reset default");
}

bool invalid_parameter_updates_are_atomic() {
    SmartMaterialPreset preset = rich_preset();
    const std::string before = serialize_smart_material(preset);
    const bool range_refused = expect_error(
        [&] { static_cast<void>(set_smart_material_parameter_value(preset, "wear-amount", 2.0)); },
        SmartMaterialErrorCode::invalid_parameter_value,
        "out-of-range smart material parameter update was accepted");
    const bool type_refused = expect_error(
        [&] { static_cast<void>(set_smart_material_parameter_value(preset, "wear-amount", true)); },
        SmartMaterialErrorCode::invalid_parameter_value,
        "wrong-type smart material parameter update was accepted");
    const bool unknown_refused = expect_error(
        [&] { static_cast<void>(set_smart_material_parameter_value(preset, "missing", 0.5)); },
        SmartMaterialErrorCode::unknown_parameter,
        "unknown smart material parameter update was accepted");
    return range_refused && type_refused && unknown_refused &&
           expect(serialize_smart_material(preset) == before,
                  "refused smart material parameter update changed the preset");
}

bool complete_fragment_round_trips_canonically() {
    const SmartMaterialPreset source = rich_preset();
    const std::string serialized = serialize_smart_material(source);
    const SmartMaterialPreset restored = deserialize_smart_material(serialized);
    return expect(restored == source, "smart material stack and parameters did not round-trip") &&
           expect(serialize_smart_material(restored) == serialized,
                  "smart material serialization is not canonical") &&
           expect(restored.stack.size() == 7 && restored.stack[2].graph.has_value() &&
                      restored.exposed_parameters.size() == 3 &&
                      restored.exposed_parameters.front().bindings.size() == 3,
                  "smart material omitted an entry kind, graph, or exposed parameter") &&
           expect(serialized.find(source.display_name) == std::string::npos,
                  "smart material text was not safely length-independent encoded");
}

bool mixed_content_is_classified_and_reported() {
    const SmartMaterialPreset source = rich_preset();
    const SmartMaterialContentReport report = report_smart_material_content(source);
    return expect(report.entries.size() == 7 && report.derived_entry_count == 6,
                  "derived smart material entries were not reported") &&
           expect(report.contains_model_specific_content() &&
                      report.model_specific_entry_count == 1 &&
                      report.model_specific_pixel_bytes == 4,
                  "model-specific hand-painted pixels were not reported") &&
           expect(report.entries.back() ==
                      SmartMaterialContentReportEntry{
                          .entry_identifier = "paint-mask",
                          .content_kind = SmartMaterialContentKind::model_specific,
                          .pixel_payload_count = 1,
                          .stored_pixel_bytes = 4},
                  "model-specific report did not name and measure its source entry");
}

bool invalid_fragments_are_refused() {
    SmartMaterialPreset duplicate_entry = rich_preset();
    duplicate_entry.stack.back().identifier = duplicate_entry.stack.front().identifier;
    const bool duplicate_refused = expect_error(
        [&] { static_cast<void>(serialize_smart_material(duplicate_entry)); },
        SmartMaterialErrorCode::invalid_preset, "duplicate stack identity was accepted");

    SmartMaterialPreset missing_parent = rich_preset();
    missing_parent.stack[1].parent_identifier = "not-present";
    const bool parent_refused = expect_error(
        [&] { static_cast<void>(serialize_smart_material(missing_parent)); },
        SmartMaterialErrorCode::invalid_preset, "missing or forward stack parent was accepted");

    SmartMaterialPreset wrong_type = rich_preset();
    wrong_type.exposed_parameters.front().default_value = true;
    const bool type_refused =
        expect_error([&] { static_cast<void>(serialize_smart_material(wrong_type)); },
                     SmartMaterialErrorCode::invalid_preset,
                     "parameter default with the wrong type was accepted");

    SmartMaterialPreset outside_range = rich_preset();
    outside_range.exposed_parameters.front().default_value = 2.0;
    const bool range_refused =
        expect_error([&] { static_cast<void>(serialize_smart_material(outside_range)); },
                     SmartMaterialErrorCode::invalid_preset,
                     "parameter default outside its declared range was accepted");

    SmartMaterialPreset derived_pixels = rich_preset();
    derived_pixels.stack.front().pixel_payloads = derived_pixels.stack.back().pixel_payloads;
    const bool derived_pixels_refused = expect_error(
        [&] { static_cast<void>(serialize_smart_material(derived_pixels)); },
        SmartMaterialErrorCode::invalid_preset, "derived content accepted rasterized output");

    SmartMaterialPreset wrong_pixel_size = rich_preset();
    wrong_pixel_size.stack.back().pixel_payloads.front().pixels.pop_back();
    const bool pixel_size_refused =
        expect_error([&] { static_cast<void>(serialize_smart_material(wrong_pixel_size)); },
                     SmartMaterialErrorCode::invalid_preset,
                     "model-specific content accepted a mismatched pixel byte count");

    SmartMaterialPreset duplicate_binding = rich_preset();
    duplicate_binding.exposed_parameters.front().bindings.push_back(
        duplicate_binding.exposed_parameters.front().bindings.front());
    const bool duplicate_binding_refused =
        expect_error([&] { static_cast<void>(serialize_smart_material(duplicate_binding)); },
                     SmartMaterialErrorCode::invalid_preset,
                     "one smart material target accepted duplicate bindings");

    SmartMaterialPreset missing_target = rich_preset();
    missing_target.exposed_parameters.front().bindings.front().target_identifier = "missing";
    const bool missing_target_refused =
        expect_error([&] { static_cast<void>(serialize_smart_material(missing_target)); },
                     SmartMaterialErrorCode::invalid_preset,
                     "smart material accepted a binding to a missing graph target");

    SmartMaterialPreset linked_target = rich_preset();
    graph::GraphDocument& graph = *linked_target.stack[1].graph;
    const graph::NodeId source = graph.add_node({.role = graph::NodeRole::regular,
                                                 .type_id = "ctex.input.test",
                                                 .type_version = 1,
                                                 .display_name = "Source",
                                                 .position = {},
                                                 .inputs = {},
                                                 .outputs = {{.identifier = "value",
                                                              .display_name = "Value",
                                                              .type = graph::SocketType::scalar,
                                                              .value = 0.0}},
                                                 .properties = {}});
    const graph::AddLinkResult link = graph.add_link({.source_node = source,
                                                      .source_socket = "value",
                                                      .target_node = 1,
                                                      .target_socket = "value"});
    const bool linked_target_refused =
        expect_error([&] { static_cast<void>(serialize_smart_material(linked_target)); },
                     SmartMaterialErrorCode::invalid_preset,
                     "smart material accepted an inert binding to a linked graph input");

    SmartMaterialPreset non_layer_consumer = rich_preset();
    non_layer_consumer.anchor_references.front().consumer_entry_identifier = "wear-mask";
    const bool non_layer_consumer_refused =
        expect_error([&] { static_cast<void>(serialize_smart_material(non_layer_consumer)); },
                     SmartMaterialErrorCode::invalid_anchor_reference,
                     "smart material accepted an anchor reference from a non-layer consumer");
    return duplicate_refused && parent_refused && type_refused && range_refused &&
           derived_pixels_refused && pixel_size_refused && duplicate_binding_refused &&
           missing_target_refused && linked_target_refused && non_layer_consumer_refused &&
           expect(link.coercion == graph::SocketCoercion::identity,
                  "linked-binding refusal fixture did not create its graph link");
}

bool malformed_and_future_serializations_are_refused() {
    const std::string valid = serialize_smart_material(rich_preset());
    std::string future = valid;
    future.replace(0, std::string_view("CTEX_SMART_MATERIAL\t6").size(), "CTEX_SMART_MATERIAL\t7");
    SmartMaterialPreset destination = rich_preset();
    const std::string before_future = serialize_smart_material(destination);
    const bool future_refused =
        expect_error_text([&] { destination = deserialize_smart_material(future); },
                          SmartMaterialErrorCode::unsupported_version, "7",
                          "future smart material version was not refused by name");

    std::string truncated = valid;
    truncated.erase(truncated.rfind("END\n"));
    const bool truncated_refused = expect_error(
        [&] { static_cast<void>(deserialize_smart_material(truncated)); },
        SmartMaterialErrorCode::malformed_serialization, "truncated smart material was accepted");

    SmartMaterialPreset non_finite = rich_preset();
    non_finite.stack.front().opacity = std::numeric_limits<double>::infinity();
    return future_refused && truncated_refused &&
           expect(serialize_smart_material(destination) == before_future,
                  "future-version refusal partially replaced the destination preset") &&
           expect_error([&] { static_cast<void>(serialize_smart_material(non_finite)); },
                        SmartMaterialErrorCode::invalid_preset,
                        "non-finite smart material metadata was accepted");
}

bool historical_schemas_migrate_with_documented_defaults() {
    const SmartMaterialPreset source = rich_preset();
    bool complete = true;
    for (std::uint32_t version = 1; version < current_smart_material_schema_version; ++version) {
        SmartMaterialPreset migrated =
            deserialize_smart_material(as_historical_schema(source, version));
        complete =
            complete &&
            expect(migrated.schema_version == current_smart_material_schema_version,
                   "historical smart material did not migrate to the current schema") &&
            expect(deserialize_smart_material(serialize_smart_material(migrated)) == migrated,
                   "migrated smart material did not serialize canonically");
        if (version == 1) {
            complete =
                complete && expect(std::all_of(migrated.stack.begin(), migrated.stack.end(),
                                               [](const SmartMaterialEntry& entry) {
                                                   return entry.content_kind ==
                                                              SmartMaterialContentKind::derived &&
                                                          entry.pixel_payloads.empty();
                                               }),
                                   "schema 1 content did not take the documented derived default");
        }
        if (version <= 2) {
            const std::string before = serialize_smart_material(migrated);
            complete =
                complete &&
                expect(std::all_of(
                           migrated.exposed_parameters.begin(), migrated.exposed_parameters.end(),
                           [](const ExposedSmartMaterialParameter& parameter) {
                               return parameter.binding_state ==
                                          SmartMaterialParameterBindingState::legacy_unbound &&
                                      parameter.bindings.empty();
                           }),
                       "schema 1 or 2 parameters were not preserved as legacy unbound") &&
                expect_error(
                    [&] {
                        static_cast<void>(
                            set_smart_material_parameter_value(migrated, "wear-amount", 0.8));
                    },
                    SmartMaterialErrorCode::read_only_parameter,
                    "legacy unbound parameter accepted an inert update") &&
                expect(serialize_smart_material(migrated) == before,
                       "refused legacy parameter update changed the migrated preset");
        } else {
            complete =
                complete && expect(migrated.exposed_parameters.front().binding_state ==
                                       SmartMaterialParameterBindingState::bound,
                                   "bound historical parameter did not take the bound default");
        }
        if (version < 4) {
            complete = complete &&
                       expect(migrated.anchor_entries.empty() && migrated.anchor_references.empty(),
                              "pre-anchor schema did not take empty anchor defaults");
        }
    }

    SmartMaterialPreset resource_source = rich_preset();
    resource_source.stack[1].graph->set_input_value(1, "anchor",
                                                    graph::ImageValue{"textures/grain.png"});
    resource_source.resource_references = {{.identifier = "textures/grain.png", .kind = "image"}};
    const SmartMaterialPreset migrated_resources =
        deserialize_smart_material(as_historical_schema(resource_source, 4));
    return complete && expect(migrated_resources.resource_references ==
                                  std::vector<SmartMaterialResourceReference>{
                                      {.identifier = "textures/grain.png", .kind = "image"}},
                              "pre-resource schema did not infer declared image dependencies");
}

bool anchor_ordering_and_cycles_are_refused_atomically() {
    SmartMaterialPreset backward = rich_preset();
    set_smart_material_anchor(backward, "paint-mask", true);
    const std::string before_backward = serialize_smart_material(backward);
    const bool ordering_refused = expect_error_text(
        [&] {
            add_smart_material_anchor_reference(backward, {.anchor_entry_identifier = "paint-mask",
                                                           .consumer_entry_identifier = "base",
                                                           .consumer_node_id = 1,
                                                           .consumer_input_identifier = "anchor"});
        },
        SmartMaterialErrorCode::anchor_ordering_violation, "ordering rule",
        "reference from below an anchor did not name the ordering rule");

    SmartMaterialPreset cyclic = rich_preset();
    set_smart_material_anchor(cyclic, "top-coat", true);
    const std::string before_cycle = serialize_smart_material(cyclic);
    const bool cycle_refused = expect_error_text(
        [&] {
            add_smart_material_anchor_reference(cyclic, {.anchor_entry_identifier = "top-coat",
                                                         .consumer_entry_identifier = "base",
                                                         .consumer_node_id = 1,
                                                         .consumer_input_identifier = "anchor"});
        },
        SmartMaterialErrorCode::anchor_cycle, "base -> top-coat -> base",
        "mutual anchor reference did not report its cycle path");
    return ordering_refused && cycle_refused &&
           expect(serialize_smart_material(backward) == before_backward,
                  "refused backward anchor reference changed the preset") &&
           expect(serialize_smart_material(cyclic) == before_cycle,
                  "refused cyclic anchor reference changed the preset");
}

SmartMaterialPreset twenty_layer_anchor_fixture() {
    SmartMaterialPreset preset{.schema_version = current_smart_material_schema_version,
                               .identifier = "materials/bounded-anchor",
                               .display_name = "Bounded anchor",
                               .stack = {},
                               .exposed_parameters = {},
                               .anchor_entries = {"layer-0", "layer-3"},
                               .anchor_references = {},
                               .resource_references = {}};
    for (std::size_t index = 0; index < 20; ++index) {
        preset.stack.push_back({.identifier = "layer-" + std::to_string(index),
                                .parent_identifier = {},
                                .display_name = "Layer " + std::to_string(index),
                                .kind = SmartMaterialEntryKind::layer,
                                .enabled = true,
                                .opacity = 1.0,
                                .graph = graph_fixture("Layer graph"),
                                .content_kind = SmartMaterialContentKind::derived,
                                .pixel_payloads = {}});
    }
    preset.anchor_references = {
        {.anchor_entry_identifier = "layer-0",
         .consumer_entry_identifier = "layer-3",
         .consumer_node_id = 1,
         .consumer_input_identifier = "anchor"},
        {.anchor_entry_identifier = "layer-3",
         .consumer_entry_identifier = "layer-7",
         .consumer_node_id = 1,
         .consumer_input_identifier = "anchor"},
        {.anchor_entry_identifier = "layer-0",
         .consumer_entry_identifier = "layer-11",
         .consumer_node_id = 1,
         .consumer_input_identifier = "anchor"},
    };
    return preset;
}

bool anchor_updates_are_dependency_ordered_and_bounded() {
    const SmartMaterialPreset preset = twenty_layer_anchor_fixture();
    const std::array changed{std::string_view{"layer-0"}};
    const SmartMaterialAnchorEvaluationPlan plan =
        plan_smart_material_anchor_evaluation(preset, changed);
    return expect(
        plan.entry_identifiers == std::vector<std::string>{"layer-3", "layer-7", "layer-11"},
        "anchor update did not return only affected layers in dependency order");
}

}  // namespace

int main() {
    return complete_fragment_round_trips_canonically() &&
                   mixed_content_is_classified_and_reported() &&
                   one_parameter_updates_every_bound_entry() &&
                   invalid_parameter_updates_are_atomic() && invalid_fragments_are_refused() &&
                   malformed_and_future_serializations_are_refused() &&
                   historical_schemas_migrate_with_documented_defaults() &&
                   anchor_ordering_and_cycles_are_refused_atomically() &&
                   anchor_updates_are_dependency_ordered_and_bounded()
               ? 0
               : 1;
}
