#include <array>
#include <ctex/graph/groups.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using ctex::graph::NodeSocket;
using ctex::graph::SocketType;

NodeSocket socket(std::string identifier, SocketType type, ctex::graph::SocketValue value = {}) {
    return {.identifier = identifier,
            .display_name = std::move(identifier),
            .type = type,
            .value = std::move(value)};
}

ctex::graph::GraphNode material_output() {
    return {.role = ctex::graph::NodeRole::output,
            .type_id = "test.material-output",
            .type_version = 1,
            .display_name = "Material Output",
            .position = {},
            .inputs = {socket("surface", SocketType::colour,
                              ctex::graph::ColourValue{0.0F, 0.0F, 0.0F, 1.0F})},
            .outputs = {},
            .properties = {}};
}

ctex::graph::GraphNode colour_source() {
    return {.role = ctex::graph::NodeRole::regular,
            .type_id = "test.colour-source",
            .type_version = 1,
            .display_name = "Colour Source",
            .position = {},
            .inputs = {},
            .outputs = {socket("colour", SocketType::colour)},
            .properties = {}};
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

const NodeSocket* find_socket(std::span<const NodeSocket> sockets, std::string_view identifier) {
    for (const NodeSocket& candidate : sockets) {
        if (candidate.identifier == identifier) {
            return &candidate;
        }
    }
    return nullptr;
}

void add_materials(ctex::graph::GraphWorkspace& workspace) {
    workspace.add_material("oak", ctex::graph::GraphDocument(material_output()));
    workspace.add_material("pine", ctex::graph::GraphDocument(material_output()));
    workspace.add_material("walnut", ctex::graph::GraphDocument(material_output()));
}

bool interface_changes_propagate_to_three_materials() {
    ctex::graph::GraphWorkspace workspace;
    add_materials(workspace);
    workspace.create_group(
        "weathering", "Weathering",
        {socket("factor", SocketType::scalar, 0.25),
         socket("tint", SocketType::colour, ctex::graph::ColourValue{0.5F, 0.5F, 0.5F, 1.0F})},
        {socket("colour", SocketType::colour)});

    constexpr std::array material_ids{std::string_view{"oak"}, std::string_view{"pine"},
                                      std::string_view{"walnut"}};
    constexpr std::array stored_values{0.1, 0.2, 0.3};
    std::array<ctex::graph::NodeId, material_ids.size()> instances{};
    for (std::size_t index = 0; index < material_ids.size(); ++index) {
        instances[index] =
            workspace.instantiate_group_in_material("weathering", material_ids[index]);
        auto& graph = workspace.material(material_ids[index]);
        graph.set_input_value(instances[index], "factor", stored_values[index]);
        static_cast<void>(
            graph.add_link({instances[index], "colour", graph.output_node_id(), "surface"}));
    }
    auto& oak = workspace.material("oak");
    const auto source = oak.add_node(colour_source());
    static_cast<void>(oak.add_link({source, "colour", instances[0], "tint"}));

    const auto update = workspace.update_group_interface(
        "weathering",
        {socket("detail", SocketType::scalar, 0.75), socket("factor", SocketType::scalar, 0.5)},
        {socket("colour", SocketType::colour)});

    if (!expect(update.instances_updated == 3 && update.removed_links.size() == 1,
                "group interface edit did not report all instances and removed links")) {
        return false;
    }
    for (std::size_t index = 0; index < material_ids.size(); ++index) {
        const auto& graph = workspace.material(material_ids[index]);
        const auto& instance = graph.node(instances[index]);
        const NodeSocket* detail = find_socket(instance.inputs, "detail");
        const NodeSocket* factor = find_socket(instance.inputs, "factor");
        if (!expect(instance.type_version == 2 && instance.inputs.size() == 2 &&
                        instance.inputs[0].identifier == "detail" && detail != nullptr &&
                        std::get<double>(detail->value) == 0.75 && factor != nullptr &&
                        std::get<double>(factor->value) == stored_values[index],
                    "group socket propagation lost order, defaults, version, or stored values") ||
            !expect(
                graph.links().size() == 1 && graph.links()[0].source_node == instances[index] &&
                    ctex::graph::deserialize_graph(ctex::graph::serialize_graph(graph)) == graph,
                "group propagation removed a compatible link or broke serialization")) {
            return false;
        }
    }
    const auto& definition = workspace.group("weathering");
    const auto& input_boundary = definition.graph.node(definition.input_node_id);
    const auto& output_boundary = definition.graph.node(definition.graph.output_node_id());
    return expect(definition.version == 2 && input_boundary.outputs.size() == 2 &&
                      input_boundary.outputs[0].identifier == "detail" &&
                      output_boundary.inputs.size() == 1 &&
                      output_boundary.inputs[0].identifier == "colour",
                  "group boundary nodes did not follow the public interface");
}

bool nested_instances_also_propagate() {
    ctex::graph::GraphWorkspace workspace;
    workspace.create_group("inner", "Inner", {socket("amount", SocketType::scalar, 0.25)},
                           {socket("value", SocketType::scalar)});
    workspace.create_group("outer", "Outer", {}, {});
    const auto instance = workspace.instantiate_group_in_group("inner", "outer");
    workspace.group_graph("outer").set_input_value(instance, "amount", 0.8);

    const auto update = workspace.update_group_interface(
        "inner",
        {socket("amount", SocketType::scalar, 0.1), socket("seed", SocketType::scalar, 4.0)},
        {socket("value", SocketType::scalar)});
    const auto& propagated = workspace.group_graph("outer").node(instance);
    return expect(update.instances_updated == 1 && propagated.inputs.size() == 2 &&
                      std::get<double>(propagated.inputs[0].value) == 0.8 &&
                      std::get<double>(propagated.inputs[1].value) == 4.0,
                  "nested group instance did not receive its interface update");
}

bool changed_socket_types_reset_values_and_links() {
    ctex::graph::GraphWorkspace workspace;
    workspace.add_material("material", ctex::graph::GraphDocument(material_output()));
    workspace.create_group("convert", "Convert", {socket("value", SocketType::scalar, 0.25)},
                           {socket("colour", SocketType::colour)});
    const auto instance = workspace.instantiate_group_in_material("convert", "material");
    auto& graph = workspace.material("material");
    graph.set_input_value(instance, "value", 0.9);
    static_cast<void>(graph.add_link({instance, "colour", graph.output_node_id(), "surface"}));

    const auto update = workspace.update_group_interface(
        "convert", {socket("value", SocketType::vector, ctex::graph::VectorValue{1, 2, 3})},
        {socket("colour", SocketType::vector)});
    const auto& changed = workspace.material("material").node(instance);
    return expect(std::get<ctex::graph::VectorValue>(changed.inputs[0].value) ==
                      ctex::graph::VectorValue{1, 2, 3},
                  "changed socket type retained an incompatible stored value") &&
           expect(
               workspace.material("material").links().empty() && update.removed_links.size() == 1,
               "changed output type retained its old link");
}

bool recursive_placement_is_refused_before_mutation() {
    ctex::graph::GraphWorkspace workspace;
    workspace.create_group("a", "A", {}, {});
    workspace.create_group("b", "B", {}, {});
    workspace.create_group("c", "C", {}, {});
    static_cast<void>(workspace.instantiate_group_in_group("b", "a"));
    static_cast<void>(workspace.instantiate_group_in_group("c", "b"));
    const std::string before = ctex::graph::serialize_graph(workspace.group_graph("c"));

    std::vector<std::string> path;
    std::string diagnostic;
    try {
        static_cast<void>(workspace.instantiate_group_in_group("a", "c"));
    } catch (const ctex::graph::GroupRecursionError& error) {
        path.assign(error.cycle_path().begin(), error.cycle_path().end());
        diagnostic = error.what();
    }
    const std::vector<std::string> expected{"c", "a", "b", "c"};
    return expect(path == expected && diagnostic.find("c -> a -> b -> c") != std::string::npos,
                  "transitive group recursion diagnostic did not name its path") &&
           expect(ctex::graph::serialize_graph(workspace.group_graph("c")) == before,
                  "recursive placement changed the containing group");
}

bool self_reference_and_invalid_edits_are_atomic() {
    ctex::graph::GraphWorkspace workspace;
    workspace.create_group("self", "Self", {socket("value", SocketType::scalar, 0.5)}, {});
    const auto before = workspace;
    bool self_refused = false;
    try {
        static_cast<void>(workspace.instantiate_group_in_group("self", "self"));
    } catch (const ctex::graph::GroupRecursionError& error) {
        self_refused = error.cycle_path().size() == 2 && error.cycle_path()[0] == "self" &&
                       error.cycle_path()[1] == "self";
    }
    bool invalid_refused = false;
    try {
        static_cast<void>(workspace.update_group_interface(
            "self",
            {socket("duplicate", SocketType::scalar), socket("duplicate", SocketType::scalar)},
            {}));
    } catch (const std::invalid_argument&) {
        invalid_refused = true;
    }
    return expect(self_refused && invalid_refused && workspace == before,
                  "self-reference or invalid interface edit was not atomic");
}

}  // namespace

int main() {
    return interface_changes_propagate_to_three_materials() && nested_instances_also_propagate() &&
                   changed_socket_types_reset_values_and_links() &&
                   recursive_placement_is_refused_before_mutation() &&
                   self_reference_and_invalid_edits_are_atomic()
               ? 0
               : 1;
}
