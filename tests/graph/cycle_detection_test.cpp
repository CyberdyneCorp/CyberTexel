#include <array>
#include <ctex/graph/document.hpp>
#include <iostream>
#include <string_view>

namespace {

ctex::graph::NodeSocket socket(std::string identifier) {
    return {
        .identifier = identifier,
        .display_name = identifier,
        .type = ctex::graph::SocketType::scalar,
        .value = 0.0,
    };
}

ctex::graph::GraphNode node(std::string name, ctex::graph::NodeRole role) {
    return {
        .role = role,
        .type_id = "test." + name,
        .type_version = 1,
        .display_name = std::move(name),
        .position = {},
        .inputs = {socket("in")},
        .outputs = {socket("out")},
        .properties = {},
    };
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool closing_cycle_is_refused_with_the_path() {
    ctex::graph::GraphDocument graph(node("output", ctex::graph::NodeRole::output));
    const auto first = graph.add_node(node("first", ctex::graph::NodeRole::regular));
    const auto second = graph.add_node(node("second", ctex::graph::NodeRole::regular));
    const auto third = graph.add_node(node("third", ctex::graph::NodeRole::regular));
    graph.add_link({first, "out", second, "in"});
    graph.add_link({second, "out", third, "in"});
    const std::string before = ctex::graph::serialize_graph(graph);

    std::vector<ctex::graph::NodeId> path;
    std::string diagnostic;
    try {
        graph.add_link({third, "out", first, "in"});
    } catch (const ctex::graph::GraphCycleError& error) {
        path.assign(error.cycle_path().begin(), error.cycle_path().end());
        diagnostic = error.what();
    }
    const std::array expected{third, first, second, third};
    return expect(path == std::vector<ctex::graph::NodeId>(expected.begin(), expected.end()),
                  "cycle diagnostic did not report the closed node path") &&
           expect(diagnostic.find(std::to_string(first)) != std::string::npos &&
                      diagnostic.find(" -> ") != std::string::npos,
                  "cycle diagnostic did not name its path") &&
           expect(ctex::graph::serialize_graph(graph) == before,
                  "refused cycle changed the graph document");
}

bool self_cycles_are_refused_before_mutation() {
    ctex::graph::GraphDocument graph(node("output", ctex::graph::NodeRole::output));
    const auto first = graph.add_node(node("first", ctex::graph::NodeRole::regular));
    try {
        graph.add_link({first, "out", first, "in"});
    } catch (const ctex::graph::GraphCycleError& error) {
        return expect(error.cycle_path().size() == 2 && error.cycle_path()[0] == first &&
                          error.cycle_path()[1] == first && graph.links().empty(),
                      "self-cycle diagnostic or atomic refusal was incorrect");
    }
    return expect(false, "graph accepted a self-cycle");
}

bool cyclic_serialization_is_rejected() {
    ctex::graph::GraphDocument graph(node("output", ctex::graph::NodeRole::output));
    const auto first = graph.add_node(node("first", ctex::graph::NodeRole::regular));
    const auto second = graph.add_node(node("second", ctex::graph::NodeRole::regular));
    graph.add_link({first, "out", second, "in"});
    std::string serialized = ctex::graph::serialize_graph(graph);
    const std::string reverse =
        "LINK\t" + std::to_string(second) + "\t6f7574\t" + std::to_string(first) + "\t696e\n";
    serialized.insert(serialized.find("END\n"), reverse);
    try {
        static_cast<void>(ctex::graph::deserialize_graph(serialized));
    } catch (const std::invalid_argument&) {
        return true;
    }
    return expect(false, "deserializer accepted a cyclic graph");
}

}  // namespace

int main() {
    return closing_cycle_is_refused_with_the_path() && self_cycles_are_refused_before_mutation() &&
                   cyclic_serialization_is_rejected()
               ? 0
               : 1;
}
