#include <array>
#include <ctex/graph/document.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

ctex::graph::NodeSocket socket(std::string identifier, ctex::graph::SocketType type,
                               ctex::graph::SocketValue value = {}) {
    return {
        .identifier = identifier,
        .display_name = identifier,
        .type = type,
        .value = std::move(value),
    };
}

ctex::graph::GraphNode output_node() {
    return {
        .role = ctex::graph::NodeRole::output,
        .type_id = "ctex.output",
        .type_version = 1,
        .display_name = "Material Output",
        .position = {640.25F, -40.5F},
        .inputs =
            {
                socket("base_color", ctex::graph::SocketType::colour,
                       ctex::graph::ColourValue{0.8F, 0.8F, 0.8F, 1.0F}),
                socket("roughness", ctex::graph::SocketType::scalar, 0.4),
            },
        .outputs = {},
        .properties = {},
    };
}

ctex::graph::GraphNode source_node() {
    return {
        .type_id = "example.source",
        .type_version = 3,
        .display_name = "Source\nWith Tabs\tAnd UTF-8 ☃",
        .position = {-12.5F, 33.25F},
        .inputs =
            {
                socket("enabled", ctex::graph::SocketType::boolean, true),
                socket("direction", ctex::graph::SocketType::vector,
                       ctex::graph::VectorValue{1.0F, -2.0F, 3.5F}),
                socket("label", ctex::graph::SocketType::string, std::string("wood\nrough")),
                socket("image", ctex::graph::SocketType::image,
                       ctex::graph::ImageValue{"library/materials/wood.png"}),
            },
        .outputs =
            {
                socket("colour", ctex::graph::SocketType::colour),
                socket("factor", ctex::graph::SocketType::scalar),
            },
        .properties =
            {
                {"seed", 42.0},
                {"note", std::string("preserved property")},
            },
    };
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool round_trip_preserves_the_complete_document() {
    ctex::graph::GraphDocument original(output_node());
    const ctex::graph::NodeId source = original.add_node(source_node());
    static_cast<void>(
        original.add_link({source, "colour", original.output_node_id(), "base_color"}));

    const std::string serialized = ctex::graph::serialize_graph(original);
    const auto restored = ctex::graph::deserialize_graph(serialized);
    return expect(restored == original,
                  "serialized graph did not compare equal after round trip") &&
           expect(ctex::graph::serialize_graph(restored) == serialized,
                  "canonical graph serialization changed after round trip") &&
           expect(restored.node(source).position == ctex::graph::NodePosition{-12.5F, 33.25F},
                  "node position was not preserved") &&
           expect(std::get<ctex::graph::ImageValue>(restored.node(source).inputs[3].value)
                          .resource_id == "library/materials/wood.png",
                  "unconnected image socket value was not preserved");
}

bool clones_are_independent_and_comparable() {
    ctex::graph::GraphDocument original(output_node());
    const ctex::graph::NodeId source = original.add_node(source_node());
    auto clone = original.clone();
    const bool initially_equal = clone == original;
    clone.set_node_position(source, {9.0F, 11.0F});
    clone.set_input_value(source, "label", std::string("changed"));
    return expect(initially_equal, "fresh graph clone did not compare equal") &&
           expect(clone != original, "mutated clone still compared equal to its source") &&
           expect(original.node(source).position == ctex::graph::NodePosition{-12.5F, 33.25F},
                  "mutating a clone changed the source graph");
}

bool property_updates_preserve_type_and_finiteness() {
    ctex::graph::GraphDocument graph(output_node());
    const ctex::graph::NodeId source = graph.add_node(source_node());
    graph.set_property_value(source, "seed", 7.0);
    const std::string after_valid_update = ctex::graph::serialize_graph(graph);

    bool wrong_type_refused = false;
    bool non_finite_refused = false;
    bool missing_refused = false;
    try {
        graph.set_property_value(source, "seed", std::string("wrong"));
    } catch (const std::invalid_argument&) {
        wrong_type_refused = true;
    }
    try {
        graph.set_property_value(source, "seed", std::numeric_limits<double>::infinity());
    } catch (const std::invalid_argument&) {
        non_finite_refused = true;
    }
    try {
        graph.set_property_value(source, "missing", 1.0);
    } catch (const std::out_of_range&) {
        missing_refused = true;
    }
    return expect(std::get<double>(graph.node(source).properties.front().value) == 7.0,
                  "graph property update did not publish its valid value") &&
           expect(wrong_type_refused && non_finite_refused && missing_refused,
                  "graph property update accepted an invalid target or value") &&
           expect(ctex::graph::serialize_graph(graph) == after_valid_update,
                  "refused graph property update changed the document");
}

bool output_and_node_id_invariants_are_preserved() {
    ctex::graph::GraphDocument graph(output_node());
    const ctex::graph::NodeId first = graph.add_node(source_node());
    const ctex::graph::NodeId removed = graph.add_node(source_node());
    graph.remove_node(removed);
    auto restored = ctex::graph::deserialize_graph(ctex::graph::serialize_graph(graph));
    const ctex::graph::NodeId after_round_trip = restored.add_node(source_node());

    bool second_output_refused = false;
    bool output_removal_refused = false;
    try {
        static_cast<void>(graph.add_node(output_node()));
    } catch (const std::invalid_argument&) {
        second_output_refused = true;
    }
    try {
        graph.remove_node(graph.output_node_id());
    } catch (const std::invalid_argument&) {
        output_removal_refused = true;
    }
    return expect(first == 2 && removed == 3 && after_round_trip == 4,
                  "node identities were reused or lost during serialization") &&
           expect(second_output_refused && output_removal_refused,
                  "graph allowed its exactly-one-output invariant to be broken");
}

bool links_require_declared_socket_endpoints() {
    ctex::graph::GraphDocument graph(output_node());
    const ctex::graph::NodeId source = graph.add_node(source_node());
    const ctex::graph::GraphLink link{source, "colour", graph.output_node_id(), "base_color"};
    static_cast<void>(graph.add_link(link));
    const bool removed = graph.remove_link(link);
    bool invalid_refused = false;
    try {
        static_cast<void>(
            graph.add_link({source, "missing", graph.output_node_id(), "base_color"}));
    } catch (const std::invalid_argument&) {
        invalid_refused = true;
    }
    return expect(removed && graph.links().empty(), "declared graph link was not removable") &&
           expect(invalid_refused, "graph accepted a link to an undeclared socket");
}

bool malformed_serialization_is_refused() {
    bool bad_version_refused = false;
    bool missing_output_refused = false;
    try {
        static_cast<void>(ctex::graph::deserialize_graph("CTEX_GRAPH\t2\nEND\n"));
    } catch (const std::invalid_argument&) {
        bad_version_refused = true;
    }
    try {
        static_cast<void>(ctex::graph::deserialize_graph("CTEX_GRAPH\t1\nNEXT\t1\nEND\n"));
    } catch (const std::invalid_argument&) {
        missing_output_refused = true;
    }
    return expect(bad_version_refused && missing_output_refused,
                  "malformed or unsupported graph serialization was accepted");
}

bool interface_updates_validate_before_preserving_values() {
    ctex::graph::GraphDocument graph(output_node());
    const auto source = graph.add_node(source_node());
    graph.set_input_value(source, "enabled", false);
    const std::string before = ctex::graph::serialize_graph(graph);
    auto inputs = graph.node(source).inputs;
    inputs[0].value = 1.0;
    try {
        static_cast<void>(
            graph.update_node_interface(source, 4, std::move(inputs), graph.node(source).outputs));
    } catch (const std::invalid_argument&) {
        return expect(ctex::graph::serialize_graph(graph) == before,
                      "invalid interface declaration changed the graph");
    }
    return expect(false, "invalid interface default was hidden by a preserved value");
}

}  // namespace

int main() {
    return round_trip_preserves_the_complete_document() &&
                   clones_are_independent_and_comparable() &&
                   property_updates_preserve_type_and_finiteness() &&
                   output_and_node_id_invariants_are_preserved() &&
                   links_require_declared_socket_endpoints() &&
                   malformed_serialization_is_refused() &&
                   interface_updates_validate_before_preserving_values()
               ? 0
               : 1;
}
