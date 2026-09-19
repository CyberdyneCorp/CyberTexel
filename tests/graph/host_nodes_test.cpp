#include <algorithm>
#include <ctex/graph/groups.hpp>
#include <ctex/graph/host_nodes.hpp>
#include <ctex/graph/validation.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using ctex::graph::EmissionTarget;
using ctex::graph::GraphDiagnosticCode;
using ctex::graph::NodeCategory;
using ctex::graph::NodeSocket;
using ctex::graph::SocketType;
using ctex::graph::SocketValue;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

NodeSocket socket(std::string identifier, SocketType type, SocketValue value = {}) {
    return {.identifier = identifier,
            .display_name = std::move(identifier),
            .type = type,
            .value = std::move(value)};
}

ctex::graph::GraphNode scalar_output() {
    return {.role = ctex::graph::NodeRole::output,
            .type_id = "test.output",
            .type_version = 1,
            .display_name = "Output",
            .position = {},
            .inputs = {socket("value", SocketType::scalar, 0.0)},
            .outputs = {},
            .properties = {}};
}

double scalar_property(const ctex::graph::GraphNode& node, std::string_view key) {
    const auto found = std::find_if(node.properties.begin(), node.properties.end(),
                                    [&](const auto& property) { return property.key == key; });
    return std::get<double>(found->value);
}

const ctex::graph::ImageValue& image_property(const ctex::graph::GraphNode& node,
                                              std::string_view key) {
    const auto found = std::find_if(node.properties.begin(), node.properties.end(),
                                    [&](const auto& property) { return property.key == key; });
    return std::get<ctex::graph::ImageValue>(found->value);
}

ctex::graph::HostNodeTypeRegistration scale_registration(std::uint32_t version = 1) {
    ctex::graph::HostNodeTypeRegistration registration{
        .declaration = {.type_id = "acme.texture.scale",
                        .version = version,
                        .display_name = "Host Scale",
                        .category = NodeCategory::host,
                        .inputs = {socket("value", SocketType::scalar, 1.0)},
                        .outputs = {socket("result", SocketType::scalar)},
                        .properties = {{.identifier = "factor",
                                        .display_name = "Factor",
                                        .default_value = 3.0,
                                        .allowed_values = {}},
                                       {.identifier = "source_image",
                                        .display_name = "Source Image",
                                        .default_value =
                                            ctex::graph::ImageValue{"textures/source.png"},
                                        .allowed_values = {}}}},
        .cpu_evaluate =
            [](const ctex::graph::NodeEvaluationRequest& request) {
                return std::vector<SocketValue>{std::get<double>(request.inputs[0]) *
                                                scalar_property(request.node, "factor")};
            },
        .emit =
            [](const ctex::graph::NodeEmissionRequest& request) {
                return ctex::graph::NodeEmissionResult{
                    .output_expressions = {"(" + request.input_expressions[0] + " * " +
                                           std::to_string(scalar_property(request.node, "factor")) +
                                           ")"},
                    .resource_identifiers = {
                        image_property(request.node, "source_image").resource_id}};
            },
        .deterministic = true,
        .resource_dependencies = {{"source_image"}},
        .supported_targets = {EmissionTarget::wgsl, EmissionTarget::msl},
        .parity_fixtures = {{.identifier = "positive-scale",
                             .inputs = {2.0},
                             .expected_outputs = {6.0},
                             .tolerance = 0.0}},
    };
    return registration;
}

bool registration_provides_checked_cpu_and_emission_semantics() {
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(scale_registration());
    const auto node = registry.make_node("acme.texture.scale", 1, {8.0F, 13.0F});
    const std::vector<SocketValue> inputs{4.0};
    const std::vector<std::string> expressions{"source_value"};
    const auto outputs = registry.evaluate(node, inputs);
    const auto emitted = registry.emit(node, EmissionTarget::wgsl, expressions);
    return expect(node.position == ctex::graph::NodePosition{8.0F, 13.0F} &&
                      std::get<double>(outputs[0]) == 12.0,
                  "registered CPU semantics or node factory returned the wrong result") &&
           expect(
               emitted.output_expressions.size() == 1 &&
                   emitted.output_expressions[0].find("source_value") != std::string::npos &&
                   emitted.resource_identifiers == std::vector<std::string>{"textures/source.png"},
               "registered emission semantics lost expressions or resources") &&
           expect(registry.verify_parity_fixtures("acme.texture.scale", 1).passed(),
                  "valid host-node parity fixtures did not pass");
}

bool emission_only_registration_is_refused_by_name() {
    auto registration = scale_registration();
    registration.cpu_evaluate = {};
    ctex::graph::NodeTypeRegistry registry;
    try {
        registry.register_type(std::move(registration));
    } catch (const ctex::graph::NodeRegistrationError& error) {
        return expect(
            std::string_view(error.what()).find("CPU evaluation") != std::string_view::npos &&
                registry.registrations().empty(),
            "missing CPU implementation was not named before registration");
    }
    return expect(false, "emission-only host node registration was accepted");
}

bool custom_nodes_serialize_validate_and_group() {
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(scale_registration());
    ctex::graph::GraphDocument graph(scalar_output());
    const auto custom = graph.add_node(registry.make_node("acme.texture.scale", 1));
    static_cast<void>(graph.add_link({custom, "result", graph.output_node_id(), "value"}));
    const std::string serialized = ctex::graph::serialize_graph(graph);
    const auto restored = ctex::graph::deserialize_graph(serialized);
    const ctex::graph::GraphValidationResources resources{
        .image_resources = {"textures/source.png"}, .mesh_maps = {}};
    const auto validation =
        ctex::graph::validate_graph(restored, registry, EmissionTarget::wgsl, resources);

    ctex::graph::GraphWorkspace workspace;
    workspace.create_group("host-scale", "Host Scale Group", {},
                           {socket("result", SocketType::scalar)});
    auto& group = workspace.group_graph("host-scale");
    const auto grouped = group.add_node(registry.make_node("acme.texture.scale", 1));
    static_cast<void>(group.add_link({grouped, "result", group.output_node_id(), "result"}));
    const auto group_semantics =
        ctex::graph::inspect_graph_semantics(group, registry, EmissionTarget::wgsl);
    return expect(restored == graph && ctex::graph::serialize_graph(restored) == serialized,
                  "host node did not serialize canonically") &&
           expect(validation.valid(), "registered host node did not participate in validation") &&
           expect(group_semantics.emittable(), "registered host node did not work in a group");
}

bool unknown_nodes_remain_opaque_and_name_the_missing_type() {
    ctex::graph::NodeTypeRegistry registered;
    registered.register_type(scale_registration());
    ctex::graph::GraphDocument graph(scalar_output());
    const auto custom = graph.add_node(registered.make_node("acme.texture.scale", 1));
    static_cast<void>(graph.add_link({custom, "result", graph.output_node_id(), "value"}));
    const std::string serialized = ctex::graph::serialize_graph(graph);
    const auto opaque = ctex::graph::deserialize_graph(serialized);

    const ctex::graph::NodeTypeRegistry empty_registry;
    const auto semantics =
        ctex::graph::inspect_graph_semantics(opaque, empty_registry, EmissionTarget::wgsl);
    const ctex::graph::GraphValidationResources resources{
        .image_resources = {"textures/source.png"}, .mesh_maps = {}};
    const auto validation =
        ctex::graph::validate_graph(opaque, empty_registry, EmissionTarget::wgsl, resources);
    const auto missing = std::find_if(
        validation.diagnostics.begin(), validation.diagnostics.end(), [](const auto& diagnostic) {
            return diagnostic.code == GraphDiagnosticCode::missing_node_type;
        });
    return expect(!semantics.emittable() && semantics.issues.size() == 1 &&
                      semantics.issues[0].type_id == "acme.texture.scale" &&
                      semantics.issues[0].type_version == 1,
                  "unknown node did not make the graph non-emittable by named type") &&
           expect(!validation.valid() && missing != validation.diagnostics.end() &&
                      missing->subject == "acme.texture.scale@1",
                  "validation did not name the missing host node type") &&
           expect(ctex::graph::serialize_graph(opaque) == serialized,
                  "unknown node payload was not preserved opaquely");
}

bool replay_requires_determinism_and_pinned_inputs() {
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(scale_registration());
    const std::vector<std::string> none;
    const std::vector<std::string> pinned{"source_image"};
    const auto unpinned = registry.replay_eligibility("acme.texture.scale", 1, none);
    const auto eligible = registry.replay_eligibility("acme.texture.scale", 1, pinned);

    auto nondeterministic = scale_registration(2);
    nondeterministic.deterministic = false;
    registry.register_type(std::move(nondeterministic));
    const auto unstable = registry.replay_eligibility("acme.texture.scale", 2, pinned);
    return expect(!unpinned.eligible &&
                      unpinned.unpinned_dependencies == std::vector<std::string>{"source_image"},
                  "unpinned resource dependency was replay eligible") &&
           expect(eligible.eligible, "deterministic node with pinned inputs was not replayable") &&
           expect(
               !unstable.eligible && unstable.reason.find("not deterministic") != std::string::npos,
               "non-deterministic host node was replay eligible");
}

bool parity_and_target_failures_are_reported() {
    auto registration = scale_registration();
    registration.cpu_evaluate = [](const ctex::graph::NodeEvaluationRequest&) {
        return std::vector<SocketValue>{5.0};
    };
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(std::move(registration));
    const auto parity = registry.verify_parity_fixtures("acme.texture.scale", 1);
    ctex::graph::GraphDocument graph(scalar_output());
    static_cast<void>(graph.add_node(registry.make_node("acme.texture.scale", 1)));
    const auto unsupported =
        ctex::graph::inspect_graph_semantics(graph, registry, EmissionTarget::hlsl);
    return expect(!parity.passed() && parity.failures[0].fixture_identifier == "positive-scale",
                  "CPU parity drift was not tied to its fixture") &&
           expect(!unsupported.emittable() && unsupported.issues.size() == 1 &&
                      unsupported.issues[0].code ==
                          ctex::graph::NodeSemanticIssueCode::unsupported_target,
                  "unsupported host-node emission target was not reported");
}

bool registered_interfaces_are_enforced_before_emission() {
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(scale_registration());
    auto malformed = registry.make_node("acme.texture.scale", 1);
    malformed.outputs[0].identifier = "renamed-result";
    ctex::graph::GraphDocument graph(scalar_output());
    static_cast<void>(graph.add_node(std::move(malformed)));
    const auto semantics =
        ctex::graph::inspect_graph_semantics(graph, registry, EmissionTarget::wgsl);
    return expect(
        !semantics.emittable() && semantics.issues.size() == 1 &&
            semantics.issues[0].code == ctex::graph::NodeSemanticIssueCode::incompatible_interface,
        "host node with a stale interface was considered emittable");
}

}  // namespace

int main() {
    return registration_provides_checked_cpu_and_emission_semantics() &&
                   emission_only_registration_is_refused_by_name() &&
                   custom_nodes_serialize_validate_and_group() &&
                   unknown_nodes_remain_opaque_and_name_the_missing_type() &&
                   replay_requires_determinism_and_pinned_inputs() &&
                   parity_and_target_failures_are_reported() &&
                   registered_interfaces_are_enforced_before_emission()
               ? 0
               : 1;
}
