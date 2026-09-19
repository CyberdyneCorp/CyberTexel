#include <ctex/doc/smart_material.hpp>
#include <iostream>
#include <limits>
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

graph::GraphDocument graph_fixture(std::string name) {
    return graph::GraphDocument({.role = graph::NodeRole::output,
                                 .type_id = "ctex.output.smart-material",
                                 .type_version = 1,
                                 .display_name = std::move(name),
                                 .position = {},
                                 .inputs = {{.identifier = "value",
                                             .display_name = "Value",
                                             .type = graph::SocketType::scalar,
                                             .value = 0.5}},
                                 .outputs = {},
                                 .properties = {}});
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
                 .graph = std::nullopt},
                {.identifier = "base",
                 .parent_identifier = "surface",
                 .display_name = "Base layer",
                 .kind = SmartMaterialEntryKind::layer,
                 .enabled = true,
                 .opacity = 0.8,
                 .graph = graph_fixture("Base graph")},
                {.identifier = "wear-mask",
                 .parent_identifier = "base",
                 .display_name = "Wear mask",
                 .kind = SmartMaterialEntryKind::mask,
                 .enabled = true,
                 .opacity = 1.0,
                 .graph = graph_fixture("Mask graph")},
                {.identifier = "levels",
                 .parent_identifier = "wear-mask",
                 .display_name = "Levels filter",
                 .kind = SmartMaterialEntryKind::filter,
                 .enabled = false,
                 .opacity = 0.75,
                 .graph = graph_fixture("Filter graph")},
                {.identifier = "scratches",
                 .parent_identifier = "surface",
                 .display_name = "Scratch generator",
                 .kind = SmartMaterialEntryKind::generator,
                 .enabled = true,
                 .opacity = 0.6,
                 .graph = graph_fixture("Generator graph")},
            },
        .exposed_parameters =
            {
                {.identifier = "wear-amount",
                 .display_name = "Wear amount",
                 .display_group = "Wear",
                 .type = graph::SocketType::scalar,
                 .default_value = 0.4,
                 .minimum = 0.0,
                 .maximum = 1.0},
                {.identifier = "rust-colour",
                 .display_name = "Rust colour",
                 .display_group = "Surface",
                 .type = graph::SocketType::colour,
                 .default_value = graph::ColourValue{0.4F, 0.1F, 0.02F, 1.0F},
                 .minimum = 0.0,
                 .maximum = 1.0},
                {.identifier = "use-scratches",
                 .display_name = "Use scratches",
                 .display_group = "Wear",
                 .type = graph::SocketType::boolean,
                 .default_value = true,
                 .minimum = std::nullopt,
                 .maximum = std::nullopt},
            },
    };
}

bool complete_fragment_round_trips_canonically() {
    const SmartMaterialPreset source = rich_preset();
    const std::string serialized = serialize_smart_material(source);
    const SmartMaterialPreset restored = deserialize_smart_material(serialized);
    return expect(restored == source, "smart material stack and parameters did not round-trip") &&
           expect(serialize_smart_material(restored) == serialized,
                  "smart material serialization is not canonical") &&
           expect(restored.stack.size() == 5 && restored.stack[2].graph.has_value() &&
                      restored.exposed_parameters.size() == 3,
                  "smart material omitted an entry kind, graph, or exposed parameter") &&
           expect(serialized.find(source.display_name) == std::string::npos,
                  "smart material text was not safely length-independent encoded");
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
    return duplicate_refused && parent_refused && type_refused &&
           expect_error([&] { static_cast<void>(serialize_smart_material(outside_range)); },
                        SmartMaterialErrorCode::invalid_preset,
                        "parameter default outside its declared range was accepted");
}

bool malformed_and_future_serializations_are_refused() {
    const std::string valid = serialize_smart_material(rich_preset());
    std::string future = valid;
    future.replace(0, std::string_view("CTEX_SMART_MATERIAL\t1").size(), "CTEX_SMART_MATERIAL\t2");
    const bool future_refused = expect_error(
        [&] { static_cast<void>(deserialize_smart_material(future)); },
        SmartMaterialErrorCode::unsupported_version, "future smart material version was accepted");

    std::string truncated = valid;
    truncated.erase(truncated.rfind("END\n"));
    const bool truncated_refused = expect_error(
        [&] { static_cast<void>(deserialize_smart_material(truncated)); },
        SmartMaterialErrorCode::malformed_serialization, "truncated smart material was accepted");

    SmartMaterialPreset non_finite = rich_preset();
    non_finite.stack.front().opacity = std::numeric_limits<double>::infinity();
    return future_refused && truncated_refused &&
           expect_error([&] { static_cast<void>(serialize_smart_material(non_finite)); },
                        SmartMaterialErrorCode::invalid_preset,
                        "non-finite smart material metadata was accepted");
}

}  // namespace

int main() {
    return complete_fragment_round_trips_canonically() && invalid_fragments_are_refused() &&
                   malformed_and_future_serializations_are_refused()
               ? 0
               : 1;
}
