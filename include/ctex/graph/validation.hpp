#ifndef CTEX_GRAPH_VALIDATION_HPP
#define CTEX_GRAPH_VALIDATION_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/graph/document.hpp>
#include <ctex/graph/groups.hpp>
#include <ctex/graph/host_nodes.hpp>
#include <string>
#include <vector>

namespace ctex::graph {

enum class GraphDiagnosticSeverity : std::uint8_t { warning, error };
enum class GraphDiagnosticCode : std::uint8_t {
    unconnected_required_input,
    missing_mesh_map,
    missing_image_resource,
    missing_group,
    missing_node_type,
    incompatible_node_interface,
    unsupported_emission_target,
    unreachable_node,
};

struct GraphDiagnostic {
    GraphDiagnosticSeverity severity;
    GraphDiagnosticCode code;
    NodeId node_id;
    std::string node_type;
    std::string subject;
    std::string message;
    friend bool operator==(const GraphDiagnostic&, const GraphDiagnostic&) = default;
};

struct GraphValidationResources {
    std::vector<std::string> image_resources;
    std::vector<std::string> mesh_maps;
    friend bool operator==(const GraphValidationResources&,
                           const GraphValidationResources&) = default;
};

struct GraphValidationReport {
    std::vector<GraphDiagnostic> diagnostics;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] std::size_t error_count() const noexcept;
    [[nodiscard]] std::size_t warning_count() const noexcept;
    friend bool operator==(const GraphValidationReport&, const GraphValidationReport&) = default;
};

struct OwnedGraphDiagnostic {
    GraphOwnerKind owner_kind;
    std::string owner_identifier;
    GraphDiagnostic diagnostic;
    friend bool operator==(const OwnedGraphDiagnostic&, const OwnedGraphDiagnostic&) = default;
};

struct WorkspaceValidationReport {
    std::vector<OwnedGraphDiagnostic> diagnostics;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] std::size_t error_count() const noexcept;
    [[nodiscard]] std::size_t warning_count() const noexcept;
    friend bool operator==(const WorkspaceValidationReport&,
                           const WorkspaceValidationReport&) = default;
};

[[nodiscard]] GraphValidationReport validate_graph(const GraphDocument& graph,
                                                   const GraphValidationResources& resources = {});
[[nodiscard]] GraphValidationReport validate_graph(const GraphDocument& graph,
                                                   const NodeTypeRegistry& registry,
                                                   EmissionTarget target,
                                                   const GraphValidationResources& resources = {});
[[nodiscard]] WorkspaceValidationReport validate_workspace(
    const GraphWorkspace& workspace, const GraphValidationResources& resources = {});
[[nodiscard]] WorkspaceValidationReport validate_workspace(
    const GraphWorkspace& workspace, const NodeTypeRegistry& registry, EmissionTarget target,
    const GraphValidationResources& resources = {});

}  // namespace ctex::graph

#endif
