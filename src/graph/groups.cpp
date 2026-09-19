#include <algorithm>
#include <ctex/graph/groups.hpp>
#include <limits>
#include <map>
#include <optional>
#include <utility>

namespace ctex::graph {
namespace {

constexpr std::string_view group_instance_type = "ctex.group-instance";
constexpr std::string_view group_identifier_property = "group_id";

std::vector<NodeSocket> without_values(std::span<const NodeSocket> sockets) {
    std::vector<NodeSocket> result(sockets.begin(), sockets.end());
    for (NodeSocket& socket : result) {
        socket.value = std::monostate{};
    }
    return result;
}

GraphNode workspace_output() {
    return {.role = NodeRole::output,
            .type_id = "ctex.workspace-output",
            .type_version = 1,
            .display_name = "Material Output",
            .position = {},
            .inputs = {},
            .outputs = {},
            .properties = {}};
}

GraphNode group_instance_node(const NodeGroupDefinition& group, NodePosition position) {
    return {.role = NodeRole::regular,
            .type_id = std::string(group_instance_type),
            .type_version = group.version,
            .display_name = group.display_name,
            .position = position,
            .inputs = group.inputs,
            .outputs = group.outputs,
            .properties = {{std::string(group_identifier_property), group.identifier}}};
}

void validate_interface(std::span<const NodeSocket> inputs, std::span<const NodeSocket> outputs) {
    GraphDocument validation(workspace_output());
    GraphNode node{.role = NodeRole::regular,
                   .type_id = std::string(group_instance_type),
                   .type_version = 1,
                   .display_name = "Group",
                   .position = {},
                   .inputs = {inputs.begin(), inputs.end()},
                   .outputs = without_values(outputs),
                   .properties = {{std::string(group_identifier_property), std::string("group")}}};
    static_cast<void>(validation.add_node(std::move(node)));
}

GraphNode group_output_node(std::string_view display_name, std::uint32_t version,
                            std::span<const NodeSocket> outputs) {
    return {.role = NodeRole::output,
            .type_id = "ctex.group-output",
            .type_version = version,
            .display_name = std::string(display_name) + " Output",
            .position = {640.0F, 0.0F},
            .inputs = without_values(outputs),
            .outputs = {},
            .properties = {}};
}

GraphNode group_input_node(std::string_view display_name, std::uint32_t version,
                           std::span<const NodeSocket> inputs) {
    return {.role = NodeRole::regular,
            .type_id = "ctex.group-input",
            .type_version = version,
            .display_name = std::string(display_name) + " Input",
            .position = {-640.0F, 0.0F},
            .inputs = {},
            .outputs = without_values(inputs),
            .properties = {}};
}

std::optional<std::string_view> instance_group_id(const GraphNode& node) {
    if (node.type_id != group_instance_type) {
        return std::nullopt;
    }
    const auto found = std::find_if(
        node.properties.begin(), node.properties.end(),
        [](const NodeProperty& property) { return property.key == group_identifier_property; });
    if (found == node.properties.end()) {
        throw std::invalid_argument("group instance has no group identifier");
    }
    const auto* identifier = std::get_if<std::string>(&found->value);
    if (identifier == nullptr || identifier->empty()) {
        throw std::invalid_argument("group instance has an invalid group identifier");
    }
    return *identifier;
}

const NodeGroupDefinition* find_group(std::span<const NodeGroupDefinition> groups,
                                      std::string_view identifier) {
    const auto found =
        std::lower_bound(groups.begin(), groups.end(), identifier,
                         [](const NodeGroupDefinition& group, std::string_view sought) {
                             return group.identifier < sought;
                         });
    return found == groups.end() || found->identifier != identifier ? nullptr : &*found;
}

NodeGroupDefinition* find_mutable_group(std::span<NodeGroupDefinition> groups,
                                        std::string_view identifier) {
    return const_cast<NodeGroupDefinition*>(
        find_group(std::span<const NodeGroupDefinition>{groups.data(), groups.size()}, identifier));
}

std::vector<std::string> dependencies(const NodeGroupDefinition& group) {
    std::vector<std::string> result;
    for (const GraphNode& node : group.graph.nodes()) {
        if (const auto identifier = instance_group_id(node)) {
            result.emplace_back(*identifier);
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

std::vector<std::string> reconstruct_path(const std::map<std::string, std::string>& parent,
                                          std::string start, std::string target) {
    std::vector<std::string> path{std::move(target)};
    while (path.back() != start) {
        path.push_back(parent.at(path.back()));
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::optional<std::vector<std::string>> dependency_path(std::span<const NodeGroupDefinition> groups,
                                                        std::string_view start,
                                                        std::string_view target) {
    if (start == target) {
        return std::vector<std::string>{std::string(start)};
    }
    std::map<std::string, std::string> parent;
    std::vector<std::string> queue{std::string(start)};
    parent.emplace(start, std::string{});
    for (std::size_t cursor = 0; cursor < queue.size(); ++cursor) {
        const NodeGroupDefinition* group = find_group(groups, queue[cursor]);
        if (group == nullptr) {
            continue;
        }
        for (std::string dependency : dependencies(*group)) {
            if (parent.contains(dependency)) {
                continue;
            }
            parent.emplace(dependency, queue[cursor]);
            if (dependency == target) {
                return reconstruct_path(parent, std::string(start), std::move(dependency));
            }
            queue.push_back(std::move(dependency));
        }
    }
    return std::nullopt;
}

std::string recursion_message(std::span<const std::string> path) {
    std::string message = "group instance would create recursion: ";
    for (std::size_t index = 0; index < path.size(); ++index) {
        if (index != 0) {
            message.append(" -> ");
        }
        message.append(path[index]);
    }
    return message;
}

void record_removed_links(GroupInterfaceUpdate& result, GraphOwnerKind owner_kind,
                          std::string_view owner_identifier, NodeInterfaceUpdate update) {
    for (GraphLink& link : update.removed_links) {
        result.removed_links.push_back(
            {owner_kind, std::string(owner_identifier), std::move(link)});
    }
}

void update_instances(GraphDocument& graph, GraphOwnerKind owner_kind,
                      std::string_view owner_identifier, const NodeGroupDefinition& group,
                      GroupInterfaceUpdate& result) {
    std::vector<NodeId> instances;
    for (const GraphNode& node : graph.nodes()) {
        if (instance_group_id(node) == group.identifier) {
            instances.push_back(node.id);
        }
    }
    for (NodeId instance : instances) {
        record_removed_links(
            result, owner_kind, owner_identifier,
            graph.update_node_interface(instance, group.version, group.inputs, group.outputs));
        ++result.instances_updated;
    }
}

}  // namespace

GroupRecursionError::GroupRecursionError(std::vector<std::string> cycle_path)
    : std::invalid_argument(recursion_message(cycle_path)), cycle_path_(std::move(cycle_path)) {}

void GraphWorkspace::add_material(std::string identifier, GraphDocument graph) {
    if (identifier.empty()) {
        throw std::invalid_argument("material graph identifier cannot be empty");
    }
    const auto position = std::lower_bound(materials_.begin(), materials_.end(), identifier,
                                           [](const OwnedGraph& material, std::string_view sought) {
                                               return material.identifier < sought;
                                           });
    if (position != materials_.end() && position->identifier == identifier) {
        throw std::invalid_argument("material graph identifier already exists");
    }
    materials_.insert(position, {std::move(identifier), std::move(graph)});
}

void GraphWorkspace::create_group(std::string identifier, std::string display_name,
                                  std::vector<NodeSocket> inputs, std::vector<NodeSocket> outputs) {
    if (identifier.empty() || display_name.empty()) {
        throw std::invalid_argument("node group requires an identifier and display name");
    }
    const auto position =
        std::lower_bound(groups_.begin(), groups_.end(), identifier,
                         [](const NodeGroupDefinition& group, std::string_view sought) {
                             return group.identifier < sought;
                         });
    if (position != groups_.end() && position->identifier == identifier) {
        throw std::invalid_argument("node group identifier already exists");
    }
    validate_interface(inputs, outputs);
    outputs = without_values(outputs);
    GraphDocument graph(group_output_node(display_name, 1, outputs));
    const NodeId input_node_id = graph.add_node(group_input_node(display_name, 1, inputs));
    groups_.insert(position, {.identifier = std::move(identifier),
                              .display_name = std::move(display_name),
                              .version = 1,
                              .inputs = std::move(inputs),
                              .outputs = std::move(outputs),
                              .input_node_id = input_node_id,
                              .graph = std::move(graph)});
}

NodeId GraphWorkspace::instantiate_group_in_material(std::string_view group_identifier,
                                                     std::string_view material_identifier,
                                                     NodePosition position) {
    const NodeGroupDefinition& definition = group(group_identifier);
    return material(material_identifier).add_node(group_instance_node(definition, position));
}

NodeId GraphWorkspace::instantiate_group_in_group(std::string_view group_identifier,
                                                  std::string_view containing_group_identifier,
                                                  NodePosition position) {
    const NodeGroupDefinition& definition = group(group_identifier);
    if (auto path = dependency_path(groups_, group_identifier, containing_group_identifier)) {
        path->insert(path->begin(), std::string(containing_group_identifier));
        throw GroupRecursionError(std::move(*path));
    }
    return mutable_group(containing_group_identifier)
        .graph.add_node(group_instance_node(definition, position));
}

GroupInterfaceUpdate GraphWorkspace::update_group_interface(std::string_view group_identifier,
                                                            std::vector<NodeSocket> inputs,
                                                            std::vector<NodeSocket> outputs) {
    validate_interface(inputs, outputs);
    outputs = without_values(outputs);
    std::vector<OwnedGraph> updated_materials = materials_;
    std::vector<NodeGroupDefinition> updated_groups = groups_;
    NodeGroupDefinition* target = find_mutable_group(updated_groups, group_identifier);
    if (target == nullptr) {
        throw std::out_of_range("node group does not exist");
    }
    if (target->version == std::numeric_limits<std::uint32_t>::max()) {
        throw std::overflow_error("node group version space is exhausted");
    }
    ++target->version;

    GroupInterfaceUpdate result;
    record_removed_links(result, GraphOwnerKind::group, target->identifier,
                         target->graph.update_node_interface(target->input_node_id, target->version,
                                                             {}, without_values(inputs)));
    record_removed_links(
        result, GraphOwnerKind::group, target->identifier,
        target->graph.update_node_interface(target->graph.output_node_id(), target->version,
                                            without_values(outputs), {}));
    target->inputs = std::move(inputs);
    target->outputs = std::move(outputs);

    for (OwnedGraph& material_entry : updated_materials) {
        update_instances(material_entry.graph, GraphOwnerKind::material, material_entry.identifier,
                         *target, result);
    }
    for (NodeGroupDefinition& group_entry : updated_groups) {
        update_instances(group_entry.graph, GraphOwnerKind::group, group_entry.identifier, *target,
                         result);
    }
    materials_.swap(updated_materials);
    groups_.swap(updated_groups);
    return result;
}

GraphDocument& GraphWorkspace::material(std::string_view identifier) {
    return const_cast<GraphDocument&>(std::as_const(*this).material(identifier));
}

const GraphDocument& GraphWorkspace::material(std::string_view identifier) const {
    const auto found =
        std::lower_bound(materials_.begin(), materials_.end(), identifier,
                         [](const OwnedGraph& material_entry, std::string_view sought) {
                             return material_entry.identifier < sought;
                         });
    if (found == materials_.end() || found->identifier != identifier) {
        throw std::out_of_range("material graph does not exist");
    }
    return found->graph;
}

GraphDocument& GraphWorkspace::group_graph(std::string_view identifier) {
    return mutable_group(identifier).graph;
}

const GraphDocument& GraphWorkspace::group_graph(std::string_view identifier) const {
    return group(identifier).graph;
}

const NodeGroupDefinition& GraphWorkspace::group(std::string_view identifier) const {
    const NodeGroupDefinition* found = find_group(groups_, identifier);
    if (found == nullptr) {
        throw std::out_of_range("node group does not exist");
    }
    return *found;
}

NodeGroupDefinition& GraphWorkspace::mutable_group(std::string_view identifier) {
    return const_cast<NodeGroupDefinition&>(std::as_const(*this).group(identifier));
}

}  // namespace ctex::graph
