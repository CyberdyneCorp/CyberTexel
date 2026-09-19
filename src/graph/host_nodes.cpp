#include <algorithm>
#include <cmath>
#include <ctex/graph/host_nodes.hpp>
#include <set>
#include <tuple>
#include <utility>

namespace ctex::graph {
namespace {

GraphNode registry_output() {
    return {.role = NodeRole::output,
            .type_id = "ctex.registry-output",
            .type_version = 1,
            .display_name = "Registry Output",
            .position = {},
            .inputs = {},
            .outputs = {},
            .properties = {}};
}

GraphNode instantiate(const NodeTypeDeclaration& declaration, NodePosition position) {
    std::vector<NodeProperty> properties;
    properties.reserve(declaration.properties.size());
    for (const NodePropertyDeclaration& property : declaration.properties) {
        properties.push_back({property.identifier, property.default_value});
    }
    return {.role = NodeRole::regular,
            .type_id = declaration.type_id,
            .type_version = declaration.version,
            .display_name = declaration.display_name,
            .position = position,
            .inputs = declaration.inputs,
            .outputs = declaration.outputs,
            .properties = std::move(properties)};
}

bool value_matches(SocketType type, const SocketValue& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return true;
    }
    switch (type) {
        case SocketType::scalar:
            return std::holds_alternative<double>(value);
        case SocketType::vector:
            return std::holds_alternative<VectorValue>(value);
        case SocketType::colour:
            return std::holds_alternative<ColourValue>(value);
        case SocketType::string:
            return std::holds_alternative<std::string>(value);
        case SocketType::image:
            return std::holds_alternative<ImageValue>(value);
        case SocketType::boolean:
            return std::holds_alternative<bool>(value);
    }
    return false;
}

bool value_is_finite(const SocketValue& value) {
    if (const auto* scalar = std::get_if<double>(&value)) {
        return std::isfinite(*scalar);
    }
    if (const auto* vector = std::get_if<VectorValue>(&value)) {
        return std::isfinite(vector->x) && std::isfinite(vector->y) && std::isfinite(vector->z);
    }
    if (const auto* colour = std::get_if<ColourValue>(&value)) {
        return std::isfinite(colour->r) && std::isfinite(colour->g) && std::isfinite(colour->b) &&
               std::isfinite(colour->a);
    }
    return true;
}

bool socket_schema_matches(std::span<const NodeSocket> actual,
                           std::span<const NodeSocket> declared) {
    if (actual.size() != declared.size()) {
        return false;
    }
    for (std::size_t index = 0; index < actual.size(); ++index) {
        if (actual[index].identifier != declared[index].identifier ||
            actual[index].type != declared[index].type) {
            return false;
        }
    }
    return true;
}

bool property_schema_matches(std::span<const NodeProperty> actual,
                             std::span<const NodePropertyDeclaration> declared) {
    if (actual.size() != declared.size()) {
        return false;
    }
    for (std::size_t index = 0; index < actual.size(); ++index) {
        if (actual[index].key != declared[index].identifier ||
            actual[index].value.index() != declared[index].default_value.index() ||
            !value_is_finite(actual[index].value)) {
            return false;
        }
        if (!declared[index].allowed_values.empty()) {
            const auto* selected = std::get_if<std::string>(&actual[index].value);
            if (selected == nullptr || std::find(declared[index].allowed_values.begin(),
                                                 declared[index].allowed_values.end(), *selected) ==
                                           declared[index].allowed_values.end()) {
                return false;
            }
        }
    }
    return true;
}

bool interface_matches(const GraphNode& node, const NodeTypeDeclaration& declaration) {
    return node.type_id == declaration.type_id && node.type_version == declaration.version &&
           socket_schema_matches(node.inputs, declaration.inputs) &&
           socket_schema_matches(node.outputs, declaration.outputs) &&
           property_schema_matches(node.properties, declaration.properties);
}

void validate_property_choices(const NodeTypeDeclaration& declaration) {
    for (const NodePropertyDeclaration& property : declaration.properties) {
        if (property.allowed_values.empty()) {
            continue;
        }
        const auto* selected = std::get_if<std::string>(&property.default_value);
        if (selected == nullptr ||
            std::find(property.allowed_values.begin(), property.allowed_values.end(), *selected) ==
                property.allowed_values.end()) {
            throw NodeRegistrationError("host node property '" + property.identifier +
                                        "' has a default outside its allowed values");
        }
        const std::set<std::string_view> unique(property.allowed_values.begin(),
                                                property.allowed_values.end());
        if (unique.size() != property.allowed_values.size()) {
            throw NodeRegistrationError("host node property '" + property.identifier +
                                        "' repeats an allowed value");
        }
    }
}

void validate_declaration(const HostNodeTypeRegistration& registration) {
    const NodeTypeDeclaration& declaration = registration.declaration;
    if (declaration.type_id.empty() || declaration.type_id.starts_with("ctex.") ||
        declaration.version == 0 || declaration.display_name.empty() ||
        declaration.category != NodeCategory::host || declaration.outputs.empty()) {
        throw NodeRegistrationError(
            "host node requires a non-ctex type, version, display name, host category, and output");
    }
    GraphDocument validation(registry_output());
    static_cast<void>(validation.add_node(instantiate(declaration, {})));
    validate_property_choices(declaration);
}

void validate_dependencies(const HostNodeTypeRegistration& registration) {
    std::set<std::string_view> identifiers;
    for (const NodeSocket& input : registration.declaration.inputs) {
        identifiers.insert(input.identifier);
    }
    for (const NodePropertyDeclaration& property : registration.declaration.properties) {
        identifiers.insert(property.identifier);
    }
    std::set<std::string_view> dependencies;
    for (const NodeResourceDependency& dependency : registration.resource_dependencies) {
        if (!identifiers.contains(dependency.identifier)) {
            throw NodeRegistrationError("host node resource dependency '" + dependency.identifier +
                                        "' is not an input or property");
        }
        if (!dependencies.insert(dependency.identifier).second) {
            throw NodeRegistrationError("host node repeats resource dependency '" +
                                        dependency.identifier + "'");
        }
    }
}

void validate_targets(const HostNodeTypeRegistration& registration) {
    if (registration.supported_targets.empty()) {
        throw NodeRegistrationError("host node declares no supported emission target");
    }
    std::set<EmissionTarget> targets;
    for (EmissionTarget target : registration.supported_targets) {
        if (emission_target_name(target) == "unknown") {
            throw NodeRegistrationError("host node declares an unknown emission target");
        }
        if (!targets.insert(target).second) {
            throw NodeRegistrationError("host node repeats emission target '" +
                                        std::string(emission_target_name(target)) + "'");
        }
    }
}

void validate_fixture_values(const HostNodeTypeRegistration& registration,
                             const NodeParityFixture& fixture) {
    const auto& declaration = registration.declaration;
    if (fixture.identifier.empty() || fixture.inputs.size() != declaration.inputs.size() ||
        fixture.expected_outputs.size() != declaration.outputs.size() ||
        !std::isfinite(fixture.tolerance) || fixture.tolerance < 0.0) {
        throw NodeRegistrationError("host node has a malformed parity fixture");
    }
    for (std::size_t index = 0; index < fixture.inputs.size(); ++index) {
        if (!value_matches(declaration.inputs[index].type, fixture.inputs[index]) ||
            std::holds_alternative<std::monostate>(fixture.inputs[index]) ||
            !value_is_finite(fixture.inputs[index])) {
            throw NodeRegistrationError("parity fixture '" + fixture.identifier +
                                        "' has an input of the wrong type");
        }
    }
    for (std::size_t index = 0; index < fixture.expected_outputs.size(); ++index) {
        if (!value_matches(declaration.outputs[index].type, fixture.expected_outputs[index]) ||
            std::holds_alternative<std::monostate>(fixture.expected_outputs[index]) ||
            !value_is_finite(fixture.expected_outputs[index])) {
            throw NodeRegistrationError("parity fixture '" + fixture.identifier +
                                        "' has an output of the wrong type");
        }
    }
}

void validate_fixtures(const HostNodeTypeRegistration& registration) {
    if (registration.parity_fixtures.empty()) {
        throw NodeRegistrationError("host node requires at least one parity fixture");
    }
    std::set<std::string_view> identifiers;
    for (const NodeParityFixture& fixture : registration.parity_fixtures) {
        validate_fixture_values(registration, fixture);
        if (!identifiers.insert(fixture.identifier).second) {
            throw NodeRegistrationError("host node repeats parity fixture '" + fixture.identifier +
                                        "'");
        }
    }
}

void validate_registration(const HostNodeTypeRegistration& registration) {
    validate_declaration(registration);
    if (!registration.cpu_evaluate) {
        throw NodeRegistrationError("host node '" + registration.declaration.type_id +
                                    "' has no CPU evaluation implementation");
    }
    if (!registration.emit) {
        throw NodeRegistrationError("host node '" + registration.declaration.type_id +
                                    "' has no emission implementation");
    }
    validate_dependencies(registration);
    validate_targets(registration);
    validate_fixtures(registration);
}

bool registration_less(const HostNodeTypeRegistration& left,
                       const HostNodeTypeRegistration& right) {
    return std::tie(left.declaration.type_id, left.declaration.version) <
           std::tie(right.declaration.type_id, right.declaration.version);
}

bool target_supported(const HostNodeTypeRegistration& registration, EmissionTarget target) {
    return std::find(registration.supported_targets.begin(), registration.supported_targets.end(),
                     target) != registration.supported_targets.end();
}

void validate_invocation_inputs(const HostNodeTypeRegistration& registration,
                                std::span<const SocketValue> inputs) {
    if (inputs.size() != registration.declaration.inputs.size()) {
        throw NodeInvocationError("host node CPU evaluation received the wrong input count");
    }
    for (std::size_t index = 0; index < inputs.size(); ++index) {
        if (!value_matches(registration.declaration.inputs[index].type, inputs[index]) ||
            std::holds_alternative<std::monostate>(inputs[index]) ||
            !value_is_finite(inputs[index])) {
            throw NodeInvocationError("host node CPU evaluation input '" +
                                      registration.declaration.inputs[index].identifier +
                                      "' has the wrong type");
        }
    }
}

void validate_outputs(const HostNodeTypeRegistration& registration,
                      std::span<const SocketValue> outputs) {
    if (outputs.size() != registration.declaration.outputs.size()) {
        throw NodeInvocationError("host node CPU evaluation returned the wrong output count");
    }
    for (std::size_t index = 0; index < outputs.size(); ++index) {
        if (!value_matches(registration.declaration.outputs[index].type, outputs[index]) ||
            std::holds_alternative<std::monostate>(outputs[index]) ||
            !value_is_finite(outputs[index])) {
            throw NodeInvocationError("host node CPU evaluation output '" +
                                      registration.declaration.outputs[index].identifier +
                                      "' has the wrong type");
        }
    }
}

bool close(double left, double right, double tolerance) {
    return std::abs(left - right) <= tolerance;
}

bool fixture_value_matches(const SocketValue& actual, const SocketValue& expected,
                           double tolerance) {
    if (actual.index() != expected.index()) {
        return false;
    }
    if (const auto* scalar = std::get_if<double>(&actual)) {
        return close(*scalar, std::get<double>(expected), tolerance);
    }
    if (const auto* vector = std::get_if<VectorValue>(&actual)) {
        const VectorValue other = std::get<VectorValue>(expected);
        return close(vector->x, other.x, tolerance) && close(vector->y, other.y, tolerance) &&
               close(vector->z, other.z, tolerance);
    }
    if (const auto* colour = std::get_if<ColourValue>(&actual)) {
        const ColourValue other = std::get<ColourValue>(expected);
        return close(colour->r, other.r, tolerance) && close(colour->g, other.g, tolerance) &&
               close(colour->b, other.b, tolerance) && close(colour->a, other.a, tolerance);
    }
    return actual == expected;
}

std::string type_label(std::string_view type_id, std::uint32_t version) {
    return std::string(type_id) + "@" + std::to_string(version);
}

bool intrinsic_type(const GraphNode& node) {
    return node.role == NodeRole::output || node.type_id == "ctex.group-instance" ||
           node.type_id == "ctex.group-input" || node.type_id == "ctex.group-output";
}

const NodeTypeDeclaration* known_declaration(const GraphNode& node,
                                             const NodeTypeRegistry& registry) {
    if (const auto* builtin = find_builtin_node_type(node.type_id);
        builtin != nullptr && builtin->version == node.type_version) {
        return builtin;
    }
    const auto* host = registry.find(node.type_id, node.type_version);
    return host == nullptr ? nullptr : &host->declaration;
}

bool semantic_issue_less(const NodeSemanticIssue& left, const NodeSemanticIssue& right) {
    return std::tie(left.node_id, left.code, left.type_id, left.type_version) <
           std::tie(right.node_id, right.code, right.type_id, right.type_version);
}

}  // namespace

std::string_view emission_target_name(EmissionTarget target) noexcept {
    switch (target) {
        case EmissionTarget::wgsl:
            return "wgsl";
        case EmissionTarget::msl:
            return "msl";
        case EmissionTarget::spirv:
            return "spirv";
        case EmissionTarget::hlsl:
            return "hlsl";
    }
    return "unknown";
}

void NodeTypeRegistry::register_type(HostNodeTypeRegistration registration) {
    validate_registration(registration);
    const auto position = std::lower_bound(registrations_.begin(), registrations_.end(),
                                           registration, registration_less);
    if (position != registrations_.end() &&
        position->declaration.type_id == registration.declaration.type_id &&
        position->declaration.version == registration.declaration.version) {
        throw NodeRegistrationError(
            "host node type is already registered: " +
            type_label(registration.declaration.type_id, registration.declaration.version));
    }
    registrations_.insert(position, std::move(registration));
}

const HostNodeTypeRegistration* NodeTypeRegistry::find(std::string_view type_id,
                                                       std::uint32_t version) const noexcept {
    const auto position =
        std::lower_bound(registrations_.begin(), registrations_.end(), std::tuple{type_id, version},
                         [](const HostNodeTypeRegistration& registration, const auto& sought) {
                             return std::tie(registration.declaration.type_id,
                                             registration.declaration.version) < sought;
                         });
    return position != registrations_.end() && position->declaration.type_id == type_id &&
                   position->declaration.version == version
               ? &*position
               : nullptr;
}

GraphNode NodeTypeRegistry::make_node(std::string_view type_id, std::uint32_t version,
                                      NodePosition position) const {
    const auto* registration = find(type_id, version);
    if (registration == nullptr) {
        throw std::out_of_range("host node type is not registered: " +
                                type_label(type_id, version));
    }
    return instantiate(registration->declaration, position);
}

std::vector<SocketValue> NodeTypeRegistry::evaluate(const GraphNode& node,
                                                    std::span<const SocketValue> inputs) const {
    const auto* registration = find(node.type_id, node.type_version);
    if (registration == nullptr) {
        throw NodeInvocationError("cannot evaluate unregistered host node " +
                                  type_label(node.type_id, node.type_version));
    }
    if (!interface_matches(node, registration->declaration)) {
        throw NodeInvocationError("host node interface differs from its registration: " +
                                  type_label(node.type_id, node.type_version));
    }
    validate_invocation_inputs(*registration, inputs);
    std::vector<SocketValue> outputs;
    try {
        outputs = registration->cpu_evaluate({node, inputs});
    } catch (const std::exception& error) {
        throw NodeInvocationError("host node CPU evaluation failed: " + std::string(error.what()));
    } catch (...) {
        throw NodeInvocationError("host node CPU evaluation failed with an unknown exception");
    }
    validate_outputs(*registration, outputs);
    return outputs;
}

NodeEmissionResult NodeTypeRegistry::emit(const GraphNode& node, EmissionTarget target,
                                          std::span<const std::string> input_expressions) const {
    const auto* registration = find(node.type_id, node.type_version);
    if (registration == nullptr) {
        throw NodeInvocationError("cannot emit unregistered host node " +
                                  type_label(node.type_id, node.type_version));
    }
    if (!interface_matches(node, registration->declaration)) {
        throw NodeInvocationError("host node interface differs from its registration: " +
                                  type_label(node.type_id, node.type_version));
    }
    if (!target_supported(*registration, target)) {
        throw NodeInvocationError("host node " + type_label(node.type_id, node.type_version) +
                                  " does not support " + std::string(emission_target_name(target)));
    }
    if (input_expressions.size() != registration->declaration.inputs.size()) {
        throw NodeInvocationError("host node emission received the wrong input count");
    }
    NodeEmissionResult result;
    try {
        result = registration->emit({node, target, input_expressions});
    } catch (const std::exception& error) {
        throw NodeInvocationError("host node emission failed: " + std::string(error.what()));
    } catch (...) {
        throw NodeInvocationError("host node emission failed with an unknown exception");
    }
    if (result.output_expressions.size() != registration->declaration.outputs.size() ||
        std::any_of(result.output_expressions.begin(), result.output_expressions.end(),
                    [](const std::string& expression) { return expression.empty(); })) {
        throw NodeInvocationError("host node emission returned invalid output expressions");
    }
    return result;
}

NodeParityReport NodeTypeRegistry::verify_parity_fixtures(std::string_view type_id,
                                                          std::uint32_t version) const {
    const auto* registration = find(type_id, version);
    if (registration == nullptr) {
        throw std::out_of_range("host node type is not registered: " +
                                type_label(type_id, version));
    }
    NodeParityReport report;
    const GraphNode node = instantiate(registration->declaration, {});
    std::vector<std::string> expressions(registration->declaration.inputs.size(), "fixture_input");
    for (const NodeParityFixture& fixture : registration->parity_fixtures) {
        try {
            const auto actual = evaluate(node, fixture.inputs);
            for (std::size_t index = 0; index < actual.size(); ++index) {
                if (!fixture_value_matches(actual[index], fixture.expected_outputs[index],
                                           fixture.tolerance)) {
                    report.failures.push_back(
                        {fixture.identifier,
                         "CPU output " + std::to_string(index) + " differs from the fixture"});
                }
            }
        } catch (const NodeInvocationError& error) {
            report.failures.push_back(
                {fixture.identifier, "CPU evaluation: " + std::string(error.what())});
        }
        for (EmissionTarget target : registration->supported_targets) {
            try {
                static_cast<void>(emit(node, target, expressions));
            } catch (const NodeInvocationError& error) {
                report.failures.push_back(
                    {fixture.identifier,
                     std::string(emission_target_name(target)) + ": " + error.what()});
            }
        }
    }
    return report;
}

NodeReplayEligibility NodeTypeRegistry::replay_eligibility(
    std::string_view type_id, std::uint32_t version,
    std::span<const std::string> pinned_dependencies) const {
    const auto* registration = find(type_id, version);
    if (registration == nullptr) {
        return {.eligible = false,
                .unpinned_dependencies = {},
                .reason = "node type is not registered: " + type_label(type_id, version)};
    }
    if (!registration->deterministic) {
        return {.eligible = false,
                .unpinned_dependencies = {},
                .reason = "node evaluation is not deterministic"};
    }
    NodeReplayEligibility result{.eligible = true, .unpinned_dependencies = {}, .reason = {}};
    for (const NodeResourceDependency& dependency : registration->resource_dependencies) {
        if (std::find(pinned_dependencies.begin(), pinned_dependencies.end(),
                      dependency.identifier) == pinned_dependencies.end()) {
            result.unpinned_dependencies.push_back(dependency.identifier);
        }
    }
    if (!result.unpinned_dependencies.empty()) {
        result.eligible = false;
        result.reason = "node has unpinned resource dependencies";
    }
    return result;
}

GraphSemanticReport inspect_graph_semantics(const GraphDocument& graph,
                                            const NodeTypeRegistry& registry,
                                            EmissionTarget target) {
    GraphSemanticReport report;
    for (const GraphNode& node : graph.nodes()) {
        if (intrinsic_type(node)) {
            continue;
        }
        const NodeTypeDeclaration* declaration = known_declaration(node, registry);
        if (declaration == nullptr) {
            report.issues.push_back({NodeSemanticIssueCode::missing_type, node.id, node.type_id,
                                     node.type_version,
                                     "node type '" + type_label(node.type_id, node.type_version) +
                                         "' is not registered"});
            continue;
        }
        if (!interface_matches(node, *declaration)) {
            report.issues.push_back({NodeSemanticIssueCode::incompatible_interface, node.id,
                                     node.type_id, node.type_version,
                                     "node interface differs from registered type '" +
                                         type_label(node.type_id, node.type_version) + "'"});
        }
        const auto* host = registry.find(node.type_id, node.type_version);
        if (host != nullptr && !target_supported(*host, target)) {
            report.issues.push_back({NodeSemanticIssueCode::unsupported_target, node.id,
                                     node.type_id, node.type_version,
                                     "node type '" + type_label(node.type_id, node.type_version) +
                                         "' does not support " +
                                         std::string(emission_target_name(target))});
        }
    }
    std::sort(report.issues.begin(), report.issues.end(), semantic_issue_less);
    return report;
}

}  // namespace ctex::graph
