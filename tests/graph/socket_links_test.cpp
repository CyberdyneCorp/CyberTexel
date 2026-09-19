#include <algorithm>
#include <array>
#include <ctex/graph/document.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

using ctex::graph::SocketCoercion;
using ctex::graph::SocketType;

ctex::graph::NodeSocket socket(std::string identifier, SocketType type,
                               ctex::graph::SocketValue value = {}) {
    return {
        .identifier = identifier,
        .display_name = identifier,
        .type = type,
        .value = std::move(value),
    };
}

ctex::graph::GraphNode output_node(SocketType input_type = SocketType::scalar) {
    return {
        .role = ctex::graph::NodeRole::output,
        .type_id = "test.output",
        .type_version = 1,
        .display_name = "Output",
        .position = {},
        .inputs = {socket("in", input_type,
                          input_type == SocketType::scalar ? ctex::graph::SocketValue{0.5}
                                                           : ctex::graph::SocketValue{})},
        .outputs = {},
        .properties = {},
    };
}

ctex::graph::GraphNode node(std::string name, SocketType output_type,
                            SocketType input_type = SocketType::scalar) {
    return {
        .type_id = "test." + name,
        .type_version = 1,
        .display_name = std::move(name),
        .position = {},
        .inputs = {socket("in", input_type)},
        .outputs = {socket("out", output_type)},
        .properties = {},
    };
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

struct CoercionCase {
    SocketType source;
    SocketType target;
    SocketCoercion coercion;
};

bool coercion_matrix_is_exact() {
    constexpr std::array types{SocketType::scalar, SocketType::vector, SocketType::colour,
                               SocketType::string, SocketType::image,  SocketType::boolean};
    constexpr std::array accepted{
        CoercionCase{SocketType::scalar, SocketType::scalar, SocketCoercion::identity},
        CoercionCase{SocketType::vector, SocketType::vector, SocketCoercion::identity},
        CoercionCase{SocketType::colour, SocketType::colour, SocketCoercion::identity},
        CoercionCase{SocketType::string, SocketType::string, SocketCoercion::identity},
        CoercionCase{SocketType::image, SocketType::image, SocketCoercion::identity},
        CoercionCase{SocketType::boolean, SocketType::boolean, SocketCoercion::identity},
        CoercionCase{SocketType::scalar, SocketType::vector, SocketCoercion::scalar_to_vector},
        CoercionCase{SocketType::vector, SocketType::scalar, SocketCoercion::vector_to_scalar},
        CoercionCase{SocketType::colour, SocketType::vector, SocketCoercion::colour_to_vector},
        CoercionCase{SocketType::colour, SocketType::scalar, SocketCoercion::colour_to_scalar},
    };

    for (SocketType source : types) {
        for (SocketType target : types) {
            const auto expected =
                std::find_if(accepted.begin(), accepted.end(), [&](const CoercionCase& candidate) {
                    return candidate.source == source && candidate.target == target;
                });
            const auto actual = ctex::graph::socket_coercion(source, target);
            if (!expect((expected == accepted.end() && !actual) ||
                            (expected != accepted.end() && actual == expected->coercion),
                        "socket coercion matrix differs from its declared contract")) {
                return false;
            }
        }
    }
    return true;
}

bool coercion_is_reported_without_changing_socket_values() {
    ctex::graph::GraphDocument graph(output_node());
    const auto source = graph.add_node(node("colour", SocketType::colour));
    const auto result = graph.add_link({source, "out", graph.output_node_id(), "in"});
    return expect(result.coercion == SocketCoercion::colour_to_scalar && !result.replaced_link,
                  "colour-to-scalar coercion was not reported") &&
           expect(std::get<double>(graph.node(graph.output_node_id()).inputs[0].value) == 0.5,
                  "edit-time linking changed the stored input value") &&
           expect(ctex::graph::linear_rec709_luminance_weights ==
                      ctex::graph::VectorValue{0.2126F, 0.7152F, 0.0722F},
                  "declared luminance weights changed");
}

bool incompatible_types_are_refused_atomically() {
    ctex::graph::GraphDocument graph(output_node());
    const auto source = graph.add_node(node("image", SocketType::image));
    const std::string before = ctex::graph::serialize_graph(graph);
    try {
        static_cast<void>(graph.add_link({source, "out", graph.output_node_id(), "in"}));
    } catch (const ctex::graph::SocketTypeError& error) {
        const std::string_view diagnostic = error.what();
        return expect(error.source_type() == SocketType::image &&
                          error.target_type() == SocketType::scalar &&
                          diagnostic.find("image") != std::string_view::npos &&
                          diagnostic.find("scalar") != std::string_view::npos,
                      "type refusal did not name both socket types") &&
               expect(ctex::graph::serialize_graph(graph) == before,
                      "type refusal changed the graph");
    }
    return expect(false, "image output was connected to a scalar input");
}

bool occupied_inputs_are_replaced_and_reported() {
    ctex::graph::GraphDocument graph(output_node());
    const auto scalar = graph.add_node(node("scalar", SocketType::scalar));
    const auto colour = graph.add_node(node("colour", SocketType::colour));
    const ctex::graph::GraphLink first{scalar, "out", graph.output_node_id(), "in"};
    const ctex::graph::GraphLink second{colour, "out", graph.output_node_id(), "in"};
    const auto initial = graph.add_link(first);
    const auto replacement = graph.add_link(second);
    return expect(!initial.replaced_link && replacement.replaced_link == first,
                  "occupied input replacement was not reported") &&
           expect(replacement.coercion == SocketCoercion::colour_to_scalar,
                  "replacement coercion was not reported") &&
           expect(graph.links().size() == 1 && graph.links()[0] == second,
                  "occupied input did not retain exactly the replacement link");
}

bool refused_cycle_preserves_the_occupied_input() {
    ctex::graph::GraphDocument graph(output_node());
    const auto external = graph.add_node(node("external", SocketType::scalar));
    const auto first = graph.add_node(node("first", SocketType::scalar));
    const auto second = graph.add_node(node("second", SocketType::scalar));
    static_cast<void>(graph.add_link({external, "out", first, "in"}));
    static_cast<void>(graph.add_link({first, "out", second, "in"}));
    const std::string before = ctex::graph::serialize_graph(graph);
    try {
        static_cast<void>(graph.add_link({second, "out", first, "in"}));
    } catch (const ctex::graph::GraphCycleError&) {
        return expect(ctex::graph::serialize_graph(graph) == before,
                      "cycle refusal removed the existing input link");
    }
    return expect(false, "cycle-closing replacement was accepted");
}

bool invalid_serialized_links_are_refused() {
    ctex::graph::GraphDocument graph(output_node());
    const auto scalar = graph.add_node(node("scalar", SocketType::scalar));
    const auto second_scalar = graph.add_node(node("second-scalar", SocketType::scalar));
    static_cast<void>(graph.add_link({scalar, "out", graph.output_node_id(), "in"}));
    std::string duplicate = ctex::graph::serialize_graph(graph);
    duplicate.insert(duplicate.find("END\n"),
                     "LINK\t" + std::to_string(second_scalar) + "\t6f7574\t1\t696e\n");
    bool duplicate_refused = false;
    try {
        static_cast<void>(ctex::graph::deserialize_graph(duplicate));
    } catch (const std::invalid_argument&) {
        duplicate_refused = true;
    }

    ctex::graph::GraphDocument incompatible(output_node());
    const auto incompatible_image = incompatible.add_node(node("image", SocketType::image));
    std::string encoded = ctex::graph::serialize_graph(incompatible);
    encoded.insert(encoded.find("END\n"),
                   "LINK\t" + std::to_string(incompatible_image) + "\t6f7574\t1\t696e\n");
    bool type_refused = false;
    try {
        static_cast<void>(ctex::graph::deserialize_graph(encoded));
    } catch (const ctex::graph::SocketTypeError&) {
        type_refused = true;
    }
    return expect(duplicate_refused && type_refused,
                  "deserializer accepted duplicate-input or incompatible links");
}

}  // namespace

int main() {
    return coercion_matrix_is_exact() && coercion_is_reported_without_changing_socket_values() &&
                   incompatible_types_are_refused_atomically() &&
                   occupied_inputs_are_replaced_and_reported() &&
                   refused_cycle_preserves_the_occupied_input() &&
                   invalid_serialized_links_are_refused()
               ? 0
               : 1;
}
