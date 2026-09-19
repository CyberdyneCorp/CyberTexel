#include <ctex/doc/channels.hpp>
#include <ctex/doc/material_graph.hpp>
#include <ctex/graph/document.hpp>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

const ctex::graph::NodeSocket& input(const ctex::graph::GraphNode& output,
                                     std::string_view identifier) {
    for (const auto& socket : output.inputs) {
        if (socket.identifier == identifier) {
            return socket;
        }
    }
    throw std::out_of_range("test output socket not found");
}

bool built_in_channels_become_output_inputs_with_defaults() {
    const auto channels = ctex::doc::metallic_roughness_channels();
    const auto graph = ctex::doc::make_material_graph(channels);
    const auto& output = graph.node(graph.output_node_id());
    const auto& base_color = input(output, "pbr.base_color");
    const auto& roughness = input(output, "pbr.roughness");
    const auto& normal = input(output, "pbr.normal");
    const auto colour = std::get<ctex::graph::ColourValue>(base_color.value);
    const auto vector = std::get<ctex::graph::VectorValue>(normal.value);
    return expect(graph.nodes().size() == 1 && output.role == ctex::graph::NodeRole::output,
                  "material graph did not contain exactly its output node") &&
           expect(output.inputs.size() == 9, "built-in output did not expose all nine channels") &&
           expect(base_color.type == ctex::graph::SocketType::colour && colour.r == 0.5F &&
                      colour.g == 0.5F && colour.b == 0.5F && colour.a == 1.0F,
                  "base-colour output input did not use its descriptor default") &&
           expect(roughness.type == ctex::graph::SocketType::scalar &&
                      std::get<double>(roughness.value) == 0.5,
                  "roughness output input did not use its descriptor default") &&
           expect(normal.type == ctex::graph::SocketType::vector && vector.x == 0.5F &&
                      vector.y == 0.5F && vector.z == 1.0F,
                  "normal output input did not use its descriptor default");
}

bool custom_channel_order_and_identity_are_preserved() {
    auto channels = ctex::doc::metallic_roughness_channels();
    std::swap(channels[0], channels[8]);
    const auto graph = ctex::doc::make_material_graph(channels);
    const auto& inputs = graph.node(graph.output_node_id()).inputs;
    return expect(inputs.front().identifier == "pbr.subsurface" &&
                      inputs.back().identifier == "pbr.base_color",
                  "registered channel order was not retained by the output node") &&
           expect(ctex::graph::deserialize_graph(ctex::graph::serialize_graph(graph)) == graph,
                  "channel-derived output node did not survive graph serialization");
}

}  // namespace

int main() {
    return built_in_channels_become_output_inputs_with_defaults() &&
                   custom_channel_order_and_identity_are_preserved()
               ? 0
               : 1;
}
