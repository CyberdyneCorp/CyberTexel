#ifndef CTEX_GRAPH_CATALOGUE_HPP
#define CTEX_GRAPH_CATALOGUE_HPP

#include <ctex/graph/document.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::graph {

enum class NodeCategory : std::uint8_t { input, texture, colour_filter, vector_math };

struct NodePropertyDeclaration {
    std::string identifier;
    std::string display_name;
    SocketValue default_value;
    std::vector<std::string> allowed_values;
    friend bool operator==(const NodePropertyDeclaration&,
                           const NodePropertyDeclaration&) = default;
};

struct NodeTypeDeclaration {
    std::string type_id;
    std::uint32_t version;
    std::string display_name;
    NodeCategory category;
    std::vector<NodeSocket> inputs;
    std::vector<NodeSocket> outputs;
    std::vector<NodePropertyDeclaration> properties;
    friend bool operator==(const NodeTypeDeclaration&, const NodeTypeDeclaration&) = default;
};

struct OperationDefinition {
    std::string_view identifier;
    std::string_view display_name;
    std::string_view formula;
};

[[nodiscard]] std::span<const OperationDefinition> math_operations() noexcept;
[[nodiscard]] std::span<const OperationDefinition> vector_math_operations() noexcept;

[[nodiscard]] std::span<const NodeTypeDeclaration> builtin_node_types();
[[nodiscard]] const NodeTypeDeclaration* find_builtin_node_type(std::string_view type_id);
[[nodiscard]] GraphNode make_builtin_node(std::string_view type_id, NodePosition position = {});

}  // namespace ctex::graph

#endif
