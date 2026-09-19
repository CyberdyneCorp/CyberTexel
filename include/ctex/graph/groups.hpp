#ifndef CTEX_GRAPH_GROUPS_HPP
#define CTEX_GRAPH_GROUPS_HPP

#include <cstddef>
#include <ctex/graph/document.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::graph {

enum class GraphOwnerKind : std::uint8_t { material, group };

struct OwnedGraph {
    std::string identifier;
    GraphDocument graph;
    friend bool operator==(const OwnedGraph&, const OwnedGraph&) = default;
};

struct NodeGroupDefinition {
    std::string identifier;
    std::string display_name;
    std::uint32_t version;
    std::vector<NodeSocket> inputs;
    std::vector<NodeSocket> outputs;
    NodeId input_node_id;
    GraphDocument graph;
    friend bool operator==(const NodeGroupDefinition&, const NodeGroupDefinition&) = default;
};

struct RemovedGroupLink {
    GraphOwnerKind owner_kind;
    std::string owner_identifier;
    GraphLink link;
    friend bool operator==(const RemovedGroupLink&, const RemovedGroupLink&) = default;
};

struct GroupInterfaceUpdate {
    std::size_t instances_updated{};
    std::vector<RemovedGroupLink> removed_links;
    friend bool operator==(const GroupInterfaceUpdate&, const GroupInterfaceUpdate&) = default;
};

class GroupRecursionError final : public std::invalid_argument {
public:
    explicit GroupRecursionError(std::vector<std::string> cycle_path);

    [[nodiscard]] std::span<const std::string> cycle_path() const noexcept { return cycle_path_; }

private:
    std::vector<std::string> cycle_path_;
};

class GraphWorkspace {
public:
    void add_material(std::string identifier, GraphDocument graph);
    void create_group(std::string identifier, std::string display_name,
                      std::vector<NodeSocket> inputs, std::vector<NodeSocket> outputs);

    [[nodiscard]] NodeId instantiate_group_in_material(std::string_view group_identifier,
                                                       std::string_view material_identifier,
                                                       NodePosition position = {});
    [[nodiscard]] NodeId instantiate_group_in_group(std::string_view group_identifier,
                                                    std::string_view containing_group_identifier,
                                                    NodePosition position = {});
    [[nodiscard]] GroupInterfaceUpdate update_group_interface(std::string_view group_identifier,
                                                              std::vector<NodeSocket> inputs,
                                                              std::vector<NodeSocket> outputs);

    [[nodiscard]] GraphDocument& material(std::string_view identifier);
    [[nodiscard]] const GraphDocument& material(std::string_view identifier) const;
    [[nodiscard]] GraphDocument& group_graph(std::string_view identifier);
    [[nodiscard]] const GraphDocument& group_graph(std::string_view identifier) const;
    [[nodiscard]] const NodeGroupDefinition& group(std::string_view identifier) const;
    [[nodiscard]] std::span<const OwnedGraph> materials() const noexcept { return materials_; }
    [[nodiscard]] std::span<const NodeGroupDefinition> groups() const noexcept { return groups_; }

    friend bool operator==(const GraphWorkspace&, const GraphWorkspace&) = default;

private:
    [[nodiscard]] NodeGroupDefinition& mutable_group(std::string_view identifier);

    std::vector<OwnedGraph> materials_;
    std::vector<NodeGroupDefinition> groups_;
};

}  // namespace ctex::graph

#endif
