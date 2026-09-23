#include <algorithm>
#include <cstdlib>
#include <ctex/emit/graph_emission.hpp>
#include <ctex/graph/catalogue.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace {

using ctex::graph::GraphDocument;
using ctex::graph::GraphNode;
using ctex::graph::NodeCategory;
using ctex::graph::NodeRole;
using ctex::graph::NodeSocket;
using ctex::graph::SocketType;

NodeSocket socket(std::string identifier, SocketType type, ctex::graph::SocketValue value = {}) {
    return {.identifier = identifier,
            .display_name = identifier,
            .type = type,
            .value = std::move(value)};
}

GraphNode output_node(std::vector<NodeSocket> inputs) {
    return {.role = NodeRole::output,
            .type_id = "ctex.output",
            .type_version = 1,
            .display_name = "Output",
            .position = {},
            .inputs = std::move(inputs),
            .outputs = {},
            .properties = {}};
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

std::size_t occurrences(std::string_view text, std::string_view sought) {
    std::size_t count = 0;
    for (std::size_t position = text.find(sought); position != std::string_view::npos;
         position = text.find(sought, position + sought.size())) {
        ++count;
    }
    return count;
}

ctex::graph::HostNodeTypeRegistration host_constant_registration(int& emission_calls) {
    return {
        .declaration = {.type_id = "test.constant",
                        .version = 1,
                        .display_name = "Test Constant",
                        .category = NodeCategory::host,
                        .inputs = {},
                        .outputs = {socket("value", SocketType::scalar)},
                        .properties = {}},
        .cpu_evaluate =
            [](const ctex::graph::NodeEvaluationRequest&) {
                return std::vector<ctex::graph::SocketValue>{0.375};
            },
        .emit =
            [&emission_calls](const ctex::graph::NodeEmissionRequest&) {
                ++emission_calls;
                return ctex::graph::NodeEmissionResult{
                    .output_expressions = {"3.75000000000000000e-01"},
                    .resource_identifiers = {"fixture/shared-resource"}};
            },
        .deterministic = true,
        .resource_dependencies = {},
        .supported_targets = {ctex::graph::EmissionTarget::wgsl},
        .parity_fixtures = {{.identifier = "constant",
                             .inputs = {},
                             .expected_outputs = {0.375},
                             .tolerance = 0.0}},
    };
}

bool fan_out_emits_one_expression_and_three_references() {
    int emission_calls = 0;
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(host_constant_registration(emission_calls));
    GraphDocument graph(
        output_node({socket("a", SocketType::scalar, 0.0), socket("b", SocketType::scalar, 0.0),
                     socket("c", SocketType::scalar, 0.0)}));
    const auto source = graph.add_node(registry.make_node("test.constant", 1));
    for (std::string_view target : {"a", "b", "c"}) {
        static_cast<void>(
            graph.add_link({source, "value", graph.output_node_id(), std::string(target)}));
    }

    const auto emitted = ctex::emit::emit_wgsl_expressions(graph, registry);
    constexpr std::string_view result_name = "ctex_n2_s76616c7565";
    const std::vector<ctex::emit::ShaderNodeAttribution> expected_attributions{
        ctex::emit::ShaderNodeAttribution{std::string(result_name), "test.constant[2]",
                                          "test.constant", 2, "value"}};
    return expect(emission_calls == 1,
                  "fan-out invoked the node emission callback more than once") &&
           expect(occurrences(emitted.source, "3.75000000000000000e-01") == 1,
                  "fan-out duplicated the node expression") &&
           expect(occurrences(emitted.source, result_name) == 4,
                  "fan-out did not declare once and reference the result three times") &&
           expect(emitted.node_attributions == expected_attributions,
                  "fan-out debug metadata did not identify its one emitted declaration") &&
           expect(
               emitted.resource_identifiers == std::vector<std::string>{"fixture/shared-resource"},
               "fan-out duplicated or lost the node resource dependency");
}

bool unreachable_nodes_do_not_invoke_emission() {
    int emission_calls = 0;
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(host_constant_registration(emission_calls));
    GraphDocument graph(output_node({socket("value", SocketType::scalar, 0.125)}));
    static_cast<void>(graph.add_node(registry.make_node("test.constant", 1)));

    const auto emitted = ctex::emit::emit_wgsl_expressions(graph, registry);
    return expect(emission_calls == 0, "unreachable node invoked its emission callback") &&
           expect(emitted.source.find("test.constant") == std::string::npos,
                  "unreachable node appeared in emitted source");
}

bool colour_to_scalar_coercion_is_emitted_at_codegen() {
    GraphDocument graph(output_node({socket("roughness", SocketType::scalar, 0.0)}));
    GraphNode colour = ctex::graph::make_builtin_node("ctex.input.constant-colour");
    colour.properties.front().value = ctex::graph::ColourValue{0.2F, 0.4F, 0.6F, 1.0F};
    const auto source = graph.add_node(std::move(colour));
    static_cast<void>(graph.add_link({source, "colour", graph.output_node_id(), "roughness"}));

    const auto emitted = ctex::emit::emit_wgsl_expressions(graph);
    return expect(emitted.source.find("dot((ctex_n2_s636f6c6f7572).rgb") != std::string::npos,
                  "colour-to-scalar luminance coercion was not emitted at code generation") &&
           expect(emitted.source.find("2.126000000e-01") != std::string::npos,
                  "emitted luminance coercion did not use the documented Rec. 709 weights");
}

void populate_constant_group(ctex::graph::GraphWorkspace& workspace, std::string_view identifier,
                             double value) {
    workspace.create_group(std::string(identifier), std::string(identifier), {},
                           {socket("value", SocketType::scalar)});
    GraphNode constant = ctex::graph::make_builtin_node("ctex.input.constant-value");
    constant.properties.front().value = value;
    GraphDocument& graph = workspace.group_graph(identifier);
    const auto source = graph.add_node(std::move(constant));
    static_cast<void>(graph.add_link({source, "value", graph.output_node_id(), "value"}));
}

ctex::emit::WgslExpressionProgram qualified_group_fixture() {
    ctex::graph::GraphWorkspace workspace;
    populate_constant_group(workspace, "left", 0.25);
    populate_constant_group(workspace, "right", 0.75);
    workspace.add_material("fixture",
                           GraphDocument(output_node({socket("left", SocketType::scalar, 0.0),
                                                      socket("right", SocketType::scalar, 0.0)})));
    const auto left = workspace.instantiate_group_in_material("left", "fixture");
    const auto right = workspace.instantiate_group_in_material("right", "fixture");
    GraphDocument& material = workspace.material("fixture");
    static_cast<void>(material.add_link({left, "value", material.output_node_id(), "left"}));
    static_cast<void>(material.add_link({right, "value", material.output_node_id(), "right"}));
    return ctex::emit::emit_material_wgsl_expressions(workspace, "fixture");
}

bool group_paths_qualify_colliding_internal_node_ids() {
    const auto first = qualified_group_fixture();
    const auto repeated = qualified_group_fixture();
    constexpr std::string_view left_name = "ctex_g6c656674_i2_n3_s76616c7565";
    constexpr std::string_view right_name = "ctex_g7269676874_i3_n3_s76616c7565";
    return expect(first == repeated, "repeated graph emission was not byte-identical") &&
           expect(first.source.find(left_name) != std::string::npos,
                  "left group path did not qualify its internal node result") &&
           expect(first.source.find(right_name) != std::string::npos,
                  "right group path did not qualify its internal node result") &&
           expect(left_name != right_name,
                  "identical internal node identities collided across group paths") &&
           expect(first.source.find("// node left[2]/ ctex.input.constant-value[3]") !=
                          std::string::npos &&
                      first.source.find("// node right[3]/ ctex.input.constant-value[3]") !=
                          std::string::npos,
                  "emitted source did not retain node-attribution comments") &&
           expect(std::ranges::any_of(first.node_attributions,
                                      [](const auto& attribution) {
                                          return attribution.node_path ==
                                                 "left[2]/ ctex.input.constant-value[3]";
                                      }) &&
                      std::ranges::any_of(first.node_attributions,
                                          [](const auto& attribution) {
                                              return attribution.node_path ==
                                                     "right[3]/ ctex.input.constant-value[3]";
                                          }),
                  "structured attribution omitted a qualified group path");
}

bool nested_group_path_contains_every_enclosing_instance() {
    ctex::graph::GraphWorkspace workspace;
    populate_constant_group(workspace, "leaf", 0.5);
    workspace.create_group("middle", "middle", {}, {socket("value", SocketType::scalar)});
    const auto leaf = workspace.instantiate_group_in_group("leaf", "middle");
    GraphDocument& middle = workspace.group_graph("middle");
    static_cast<void>(middle.add_link({leaf, "value", middle.output_node_id(), "value"}));
    workspace.add_material("nested",
                           GraphDocument(output_node({socket("value", SocketType::scalar, 0.0)})));
    const auto outer = workspace.instantiate_group_in_material("middle", "nested");
    GraphDocument& material = workspace.material("nested");
    static_cast<void>(material.add_link({outer, "value", material.output_node_id(), "value"}));

    const auto emitted = ctex::emit::emit_material_wgsl_expressions(workspace, "nested");
    constexpr std::string_view nested_name = "ctex_g6d6964646c65_i2_g6c656166_i3_n3_s76616c7565";
    return expect(emitted.source.find(nested_name) != std::string::npos,
                  "nested result name omitted an enclosing group identifier or instance ID") &&
           expect(emitted.source.find("// node middle[2]/ leaf[3]/ ") != std::string::npos,
                  "nested node attribution omitted an enclosing group instance");
}

bool write_determinism_fixture(const ctex::emit::WgslExpressionProgram& emitted) {
    const char* directory = std::getenv("CTEX_DETERMINISM_OUTPUT_DIR");
    if (directory == nullptr) {
        return true;
    }
    const std::filesystem::path output =
        std::filesystem::path(directory) / "shader-emission.wgsl-body";
    std::ofstream stream(output, std::ios::binary);
    stream.write(emitted.source.data(), static_cast<std::streamsize>(emitted.source.size()));
    return expect(stream.good(), "could not write the shader determinism fixture");
}

}  // namespace

int main() {
    const bool tests_passed = fan_out_emits_one_expression_and_three_references() &&
                              unreachable_nodes_do_not_invoke_emission() &&
                              colour_to_scalar_coercion_is_emitted_at_codegen() &&
                              group_paths_qualify_colliding_internal_node_ids() &&
                              nested_group_path_contains_every_enclosing_instance();
    return tests_passed && write_determinism_fixture(qualified_group_fixture()) ? 0 : 1;
}
