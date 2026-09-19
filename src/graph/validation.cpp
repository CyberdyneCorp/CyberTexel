#include <algorithm>
#include <ctex/graph/validation.hpp>
#include <optional>
#include <set>
#include <tuple>
#include <utility>

namespace ctex::graph {
namespace {

constexpr std::string_view group_instance_type = "ctex.group-instance";
constexpr std::string_view group_identifier_property = "group_id";

bool contains(std::span<const std::string> values, std::string_view sought) {
    return std::find(values.begin(), values.end(), sought) != values.end();
}

std::size_t node_index(std::span<const GraphNode> nodes, NodeId id) {
    const auto found =
        std::lower_bound(nodes.begin(), nodes.end(), id,
                         [](const GraphNode& node, NodeId sought) { return node.id < sought; });
    return static_cast<std::size_t>(found - nodes.begin());
}

std::vector<bool> reachable_nodes(const GraphDocument& graph) {
    const auto nodes = graph.nodes();
    std::vector<std::size_t> offsets(nodes.size() + 1, 0);
    for (const GraphLink& link : graph.links()) {
        ++offsets[node_index(nodes, link.target_node) + 1];
    }
    for (std::size_t index = 1; index < offsets.size(); ++index) {
        offsets[index] += offsets[index - 1];
    }
    std::vector<std::size_t> cursor = offsets;
    std::vector<std::size_t> sources(graph.links().size());
    for (const GraphLink& link : graph.links()) {
        const std::size_t target = node_index(nodes, link.target_node);
        sources[cursor[target]++] = node_index(nodes, link.source_node);
    }

    std::vector<bool> reachable(nodes.size(), false);
    std::vector<std::size_t> queue{node_index(nodes, graph.output_node_id())};
    reachable[queue.front()] = true;
    for (std::size_t position = 0; position < queue.size(); ++position) {
        const std::size_t target = queue[position];
        for (std::size_t edge = offsets[target]; edge < offsets[target + 1]; ++edge) {
            const std::size_t source = sources[edge];
            if (!reachable[source]) {
                reachable[source] = true;
                queue.push_back(source);
            }
        }
    }
    return reachable;
}

std::set<std::pair<NodeId, std::string_view>> linked_inputs(const GraphDocument& graph) {
    std::set<std::pair<NodeId, std::string_view>> result;
    for (const GraphLink& link : graph.links()) {
        result.emplace(link.target_node, link.target_socket);
    }
    return result;
}

std::string node_label(const GraphNode& node) {
    return "node " + std::to_string(node.id) + " (" + node.display_name + ")";
}

void add_unconnected_inputs(std::vector<GraphDiagnostic>& diagnostics, const GraphNode& node,
                            const std::set<std::pair<NodeId, std::string_view>>& connected) {
    for (const NodeSocket& socket : node.inputs) {
        if (!std::holds_alternative<std::monostate>(socket.value) ||
            connected.contains({node.id, socket.identifier})) {
            continue;
        }
        diagnostics.push_back({GraphDiagnosticSeverity::error,
                               GraphDiagnosticCode::unconnected_required_input, node.id,
                               node.type_id, socket.identifier,
                               "required input '" + socket.identifier + "' on " + node_label(node) +
                                   " is unconnected"});
    }
}

void add_image_diagnostic(std::vector<GraphDiagnostic>& diagnostics, const GraphNode& node,
                          std::string location, const ImageValue& image,
                          const GraphValidationResources& resources) {
    if (!image.resource_id.empty() && contains(resources.image_resources, image.resource_id)) {
        return;
    }
    const std::string resource = image.resource_id.empty() ? "<unset>" : image.resource_id;
    diagnostics.push_back({
        GraphDiagnosticSeverity::error,
        GraphDiagnosticCode::missing_image_resource,
        node.id,
        node.type_id,
        resource,
        "image resource '" + resource + "' referenced by " + location + " on " + node_label(node) +
            " is unavailable",
    });
}

void add_image_values(std::vector<GraphDiagnostic>& diagnostics, const GraphNode& node,
                      const GraphValidationResources& resources) {
    for (const NodeSocket& socket : node.inputs) {
        if (const auto* image = std::get_if<ImageValue>(&socket.value)) {
            add_image_diagnostic(diagnostics, node, "input:" + socket.identifier, *image,
                                 resources);
        }
    }
    for (const NodeSocket& socket : node.outputs) {
        if (const auto* image = std::get_if<ImageValue>(&socket.value)) {
            add_image_diagnostic(diagnostics, node, "output:" + socket.identifier, *image,
                                 resources);
        }
    }
    for (const NodeProperty& property : node.properties) {
        if (const auto* image = std::get_if<ImageValue>(&property.value)) {
            add_image_diagnostic(diagnostics, node, "property:" + property.key, *image, resources);
        }
    }
}

void add_mesh_map(std::vector<GraphDiagnostic>& diagnostics, const GraphNode& node,
                  const GraphValidationResources& resources) {
    if (node.type_id != "ctex.input.mesh-map") {
        return;
    }
    const auto found =
        std::find_if(node.properties.begin(), node.properties.end(),
                     [](const NodeProperty& property) { return property.key == "map"; });
    const auto* map =
        found == node.properties.end() ? nullptr : std::get_if<std::string>(&found->value);
    if (map != nullptr && !map->empty() && contains(resources.mesh_maps, *map)) {
        return;
    }
    const std::string resource = map == nullptr || map->empty() ? "<unset>" : *map;
    diagnostics.push_back(
        {GraphDiagnosticSeverity::error, GraphDiagnosticCode::missing_mesh_map, node.id,
         node.type_id, resource,
         "mesh map '" + resource + "' required by " + node_label(node) + " is unavailable"});
}

bool diagnostic_less(const GraphDiagnostic& left, const GraphDiagnostic& right) {
    return std::tie(left.node_id, left.code, left.subject, left.message) <
           std::tie(right.node_id, right.code, right.subject, right.message);
}

std::size_t count_severity(std::span<const GraphDiagnostic> diagnostics,
                           GraphDiagnosticSeverity severity) noexcept {
    return static_cast<std::size_t>(
        std::count_if(diagnostics.begin(), diagnostics.end(),
                      [&](const GraphDiagnostic& value) { return value.severity == severity; }));
}

std::optional<std::string_view> group_identifier(const GraphNode& node) {
    if (node.type_id != group_instance_type) {
        return std::nullopt;
    }
    const auto found = std::find_if(
        node.properties.begin(), node.properties.end(),
        [](const NodeProperty& property) { return property.key == group_identifier_property; });
    if (found == node.properties.end()) {
        return std::string_view{};
    }
    const auto* value = std::get_if<std::string>(&found->value);
    return value == nullptr ? std::string_view{} : std::string_view(*value);
}

bool group_exists(const GraphWorkspace& workspace, std::string_view identifier) {
    const auto groups = workspace.groups();
    const auto found =
        std::lower_bound(groups.begin(), groups.end(), identifier,
                         [](const NodeGroupDefinition& group, std::string_view sought) {
                             return group.identifier < sought;
                         });
    return found != groups.end() && found->identifier == identifier;
}

void add_missing_groups(GraphValidationReport& report, const GraphWorkspace& workspace,
                        const GraphDocument& graph) {
    for (const GraphNode& node : graph.nodes()) {
        const auto identifier = group_identifier(node);
        if (!identifier || (!identifier->empty() && group_exists(workspace, *identifier))) {
            continue;
        }
        const std::string group_name = identifier->empty() ? "<unset>" : std::string(*identifier);
        report.diagnostics.push_back(
            {GraphDiagnosticSeverity::error, GraphDiagnosticCode::missing_group, node.id,
             node.type_id, group_name,
             "group '" + group_name + "' required by " + node_label(node) + " is unavailable"});
    }
    std::sort(report.diagnostics.begin(), report.diagnostics.end(), diagnostic_less);
}

void append_owned_diagnostics(WorkspaceValidationReport& destination, GraphOwnerKind owner_kind,
                              std::string_view owner_identifier, GraphValidationReport source) {
    for (GraphDiagnostic& diagnostic : source.diagnostics) {
        destination.diagnostics.push_back(
            {owner_kind, std::string(owner_identifier), std::move(diagnostic)});
    }
}

bool owned_diagnostic_less(const OwnedGraphDiagnostic& left, const OwnedGraphDiagnostic& right) {
    return std::tie(left.owner_kind, left.owner_identifier, left.diagnostic.node_id,
                    left.diagnostic.code, left.diagnostic.subject) <
           std::tie(right.owner_kind, right.owner_identifier, right.diagnostic.node_id,
                    right.diagnostic.code, right.diagnostic.subject);
}

std::size_t count_owned_severity(std::span<const OwnedGraphDiagnostic> diagnostics,
                                 GraphDiagnosticSeverity severity) noexcept {
    return static_cast<std::size_t>(std::count_if(
        diagnostics.begin(), diagnostics.end(),
        [&](const OwnedGraphDiagnostic& value) { return value.diagnostic.severity == severity; }));
}

}  // namespace

bool GraphValidationReport::valid() const noexcept { return error_count() == 0; }

std::size_t GraphValidationReport::error_count() const noexcept {
    return count_severity(diagnostics, GraphDiagnosticSeverity::error);
}

std::size_t GraphValidationReport::warning_count() const noexcept {
    return count_severity(diagnostics, GraphDiagnosticSeverity::warning);
}

bool WorkspaceValidationReport::valid() const noexcept { return error_count() == 0; }

std::size_t WorkspaceValidationReport::error_count() const noexcept {
    return count_owned_severity(diagnostics, GraphDiagnosticSeverity::error);
}

std::size_t WorkspaceValidationReport::warning_count() const noexcept {
    return count_owned_severity(diagnostics, GraphDiagnosticSeverity::warning);
}

GraphValidationReport validate_graph(const GraphDocument& graph,
                                     const GraphValidationResources& resources) {
    GraphValidationReport report;
    const auto connected = linked_inputs(graph);
    const auto reachable = reachable_nodes(graph);
    for (std::size_t index = 0; index < graph.nodes().size(); ++index) {
        const GraphNode& node = graph.nodes()[index];
        add_unconnected_inputs(report.diagnostics, node, connected);
        add_image_values(report.diagnostics, node, resources);
        add_mesh_map(report.diagnostics, node, resources);
        if (!reachable[index]) {
            report.diagnostics.push_back({GraphDiagnosticSeverity::warning,
                                          GraphDiagnosticCode::unreachable_node, node.id,
                                          node.type_id, node.display_name,
                                          node_label(node) + " cannot reach the graph output"});
        }
    }
    std::sort(report.diagnostics.begin(), report.diagnostics.end(), diagnostic_less);
    return report;
}

WorkspaceValidationReport validate_workspace(const GraphWorkspace& workspace,
                                             const GraphValidationResources& resources) {
    WorkspaceValidationReport report;
    for (const OwnedGraph& material : workspace.materials()) {
        GraphValidationReport graph_report = validate_graph(material.graph, resources);
        add_missing_groups(graph_report, workspace, material.graph);
        append_owned_diagnostics(report, GraphOwnerKind::material, material.identifier,
                                 std::move(graph_report));
    }
    for (const NodeGroupDefinition& group : workspace.groups()) {
        GraphValidationReport graph_report = validate_graph(group.graph, resources);
        add_missing_groups(graph_report, workspace, group.graph);
        append_owned_diagnostics(report, GraphOwnerKind::group, group.identifier,
                                 std::move(graph_report));
    }
    std::sort(report.diagnostics.begin(), report.diagnostics.end(), owned_diagnostic_less);
    return report;
}

}  // namespace ctex::graph
