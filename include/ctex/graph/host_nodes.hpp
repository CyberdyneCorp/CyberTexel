#ifndef CTEX_GRAPH_HOST_NODES_HPP
#define CTEX_GRAPH_HOST_NODES_HPP

#include <cstdint>
#include <ctex/graph/catalogue.hpp>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::graph {

enum class EmissionTarget : std::uint8_t { wgsl, msl, spirv, hlsl };

[[nodiscard]] std::string_view emission_target_name(EmissionTarget target) noexcept;

struct NodeEvaluationRequest {
    const GraphNode& node;
    std::span<const SocketValue> inputs;
};

struct NodeEmissionRequest {
    const GraphNode& node;
    EmissionTarget target;
    std::span<const std::string> input_expressions;
};

struct NodeEmissionResult {
    std::vector<std::string> output_expressions;
    std::vector<std::string> resource_identifiers;
    friend bool operator==(const NodeEmissionResult&, const NodeEmissionResult&) = default;
};

using NodeCpuEvaluator =
    std::function<std::vector<SocketValue>(const NodeEvaluationRequest& request)>;
using NodeEmissionCallback = std::function<NodeEmissionResult(const NodeEmissionRequest& request)>;

struct NodeResourceDependency {
    std::string identifier;
    friend bool operator==(const NodeResourceDependency&, const NodeResourceDependency&) = default;
};

struct NodeParityFixture {
    std::string identifier;
    std::vector<SocketValue> inputs;
    std::vector<SocketValue> expected_outputs;
    double tolerance{};
    friend bool operator==(const NodeParityFixture&, const NodeParityFixture&) = default;
};

struct HostNodeTypeRegistration {
    NodeTypeDeclaration declaration;
    NodeCpuEvaluator cpu_evaluate;
    NodeEmissionCallback emit;
    bool deterministic{};
    std::vector<NodeResourceDependency> resource_dependencies;
    std::vector<EmissionTarget> supported_targets;
    std::vector<NodeParityFixture> parity_fixtures;
};

class NodeRegistrationError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

class NodeInvocationError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct NodeParityFailure {
    std::string fixture_identifier;
    std::string detail;
    friend bool operator==(const NodeParityFailure&, const NodeParityFailure&) = default;
};

struct NodeParityReport {
    std::vector<NodeParityFailure> failures;
    [[nodiscard]] bool passed() const noexcept { return failures.empty(); }
    friend bool operator==(const NodeParityReport&, const NodeParityReport&) = default;
};

struct NodeReplayEligibility {
    bool eligible{};
    std::vector<std::string> unpinned_dependencies;
    std::string reason;
    friend bool operator==(const NodeReplayEligibility&, const NodeReplayEligibility&) = default;
};

enum class NodeSemanticIssueCode : std::uint8_t {
    missing_type,
    incompatible_interface,
    unsupported_target,
};

struct NodeSemanticIssue {
    NodeSemanticIssueCode code;
    NodeId node_id;
    std::string type_id;
    std::uint32_t type_version;
    std::string message;
    friend bool operator==(const NodeSemanticIssue&, const NodeSemanticIssue&) = default;
};

struct GraphSemanticReport {
    std::vector<NodeSemanticIssue> issues;
    [[nodiscard]] bool emittable() const noexcept { return issues.empty(); }
    friend bool operator==(const GraphSemanticReport&, const GraphSemanticReport&) = default;
};

class NodeTypeRegistry {
public:
    void register_type(HostNodeTypeRegistration registration);

    [[nodiscard]] const HostNodeTypeRegistration* find(std::string_view type_id,
                                                       std::uint32_t version) const noexcept;
    [[nodiscard]] std::span<const HostNodeTypeRegistration> registrations() const noexcept {
        return registrations_;
    }
    [[nodiscard]] GraphNode make_node(std::string_view type_id, std::uint32_t version,
                                      NodePosition position = {}) const;
    [[nodiscard]] std::vector<SocketValue> evaluate(const GraphNode& node,
                                                    std::span<const SocketValue> inputs) const;
    [[nodiscard]] NodeEmissionResult emit(const GraphNode& node, EmissionTarget target,
                                          std::span<const std::string> input_expressions) const;
    [[nodiscard]] NodeParityReport verify_parity_fixtures(std::string_view type_id,
                                                          std::uint32_t version) const;
    [[nodiscard]] NodeReplayEligibility replay_eligibility(
        std::string_view type_id, std::uint32_t version,
        std::span<const std::string> pinned_dependencies) const;

private:
    std::vector<HostNodeTypeRegistration> registrations_;
};

[[nodiscard]] GraphSemanticReport inspect_graph_semantics(const GraphDocument& graph,
                                                          const NodeTypeRegistry& registry,
                                                          EmissionTarget target);

}  // namespace ctex::graph

#endif
