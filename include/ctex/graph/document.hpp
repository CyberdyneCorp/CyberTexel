#ifndef CTEX_GRAPH_DOCUMENT_HPP
#define CTEX_GRAPH_DOCUMENT_HPP

#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ctex::graph {

using NodeId = std::uint64_t;

struct VectorValue {
    float x;
    float y;
    float z;
    friend constexpr bool operator==(VectorValue, VectorValue) noexcept = default;
};

struct ColourValue {
    float r;
    float g;
    float b;
    float a;
    friend constexpr bool operator==(ColourValue, ColourValue) noexcept = default;
};

struct ImageValue {
    std::string resource_id;
    friend bool operator==(const ImageValue&, const ImageValue&) = default;
};

using SocketValue =
    std::variant<std::monostate, bool, double, VectorValue, ColourValue, std::string, ImageValue>;

enum class SocketType : std::uint8_t { scalar, vector, colour, string, image, boolean };
enum class SocketCoercion : std::uint8_t {
    identity,
    scalar_to_vector,
    vector_to_scalar,
    colour_to_vector,
    colour_to_scalar,
};
enum class NodeRole : std::uint8_t { regular, output };

inline constexpr VectorValue linear_rec709_luminance_weights{0.2126F, 0.7152F, 0.0722F};

[[nodiscard]] std::string_view socket_type_name(SocketType type) noexcept;
[[nodiscard]] std::optional<SocketCoercion> socket_coercion(SocketType source,
                                                            SocketType target) noexcept;

struct NodePosition {
    float x;
    float y;
    friend constexpr bool operator==(NodePosition, NodePosition) noexcept = default;
};

struct NodeSocket {
    std::string identifier;
    std::string display_name;
    SocketType type;
    SocketValue value;
    friend bool operator==(const NodeSocket&, const NodeSocket&) = default;
};

struct NodeProperty {
    std::string key;
    SocketValue value;
    friend bool operator==(const NodeProperty&, const NodeProperty&) = default;
};

struct GraphNode {
    NodeId id{};
    NodeRole role{NodeRole::regular};
    std::string type_id;
    std::uint32_t type_version{1};
    std::string display_name;
    NodePosition position{};
    std::vector<NodeSocket> inputs;
    std::vector<NodeSocket> outputs;
    std::vector<NodeProperty> properties;
    friend bool operator==(const GraphNode&, const GraphNode&) = default;
};

struct GraphLink {
    NodeId source_node;
    std::string source_socket;
    NodeId target_node;
    std::string target_socket;
    friend bool operator==(const GraphLink&, const GraphLink&) = default;
};

struct AddLinkResult {
    SocketCoercion coercion;
    std::optional<GraphLink> replaced_link;
    friend bool operator==(const AddLinkResult&, const AddLinkResult&) = default;
};

struct NodeInterfaceUpdate {
    std::vector<GraphLink> removed_links;
    friend bool operator==(const NodeInterfaceUpdate&, const NodeInterfaceUpdate&) = default;
};

class SocketTypeError final : public std::invalid_argument {
public:
    SocketTypeError(SocketType source, SocketType target);

    [[nodiscard]] SocketType source_type() const noexcept { return source_type_; }
    [[nodiscard]] SocketType target_type() const noexcept { return target_type_; }

private:
    SocketType source_type_;
    SocketType target_type_;
};

class GraphCycleError final : public std::invalid_argument {
public:
    explicit GraphCycleError(std::vector<NodeId> cycle_path);

    [[nodiscard]] std::span<const NodeId> cycle_path() const noexcept { return cycle_path_; }

private:
    std::vector<NodeId> cycle_path_;
};

class GraphDocument {
public:
    // The supplied output node must have role=output and id=0; it becomes node 1.
    explicit GraphDocument(GraphNode output_node);

    [[nodiscard]] NodeId add_node(GraphNode node);
    void remove_node(NodeId id);
    void set_node_position(NodeId id, NodePosition position);
    void set_input_value(NodeId id, std::string_view socket_identifier, SocketValue value);
    [[nodiscard]] NodeInterfaceUpdate update_node_interface(NodeId id, std::uint32_t type_version,
                                                            std::vector<NodeSocket> inputs,
                                                            std::vector<NodeSocket> outputs);

    [[nodiscard]] AddLinkResult add_link(GraphLink link);
    [[nodiscard]] bool remove_link(const GraphLink& link) noexcept;

    [[nodiscard]] const GraphNode& node(NodeId id) const;
    [[nodiscard]] NodeId output_node_id() const noexcept;
    [[nodiscard]] std::span<const GraphNode> nodes() const noexcept { return nodes_; }
    [[nodiscard]] std::span<const GraphLink> links() const noexcept { return links_; }
    [[nodiscard]] GraphDocument clone() const { return *this; }

    friend bool operator==(const GraphDocument&, const GraphDocument&) = default;

private:
    struct SerializedTag {};
    GraphDocument(std::vector<GraphNode> nodes, std::vector<GraphLink> links, NodeId next_node_id,
                  SerializedTag);

    [[nodiscard]] GraphNode& mutable_node(NodeId id);
    void validate() const;

    std::vector<GraphNode> nodes_;
    std::vector<GraphLink> links_;
    NodeId next_node_id_{};

    friend std::string serialize_graph(const GraphDocument& graph);
    friend GraphDocument deserialize_graph(std::string_view serialized);
};

// Canonical, versioned, device-independent text representation.
[[nodiscard]] std::string serialize_graph(const GraphDocument& graph);
[[nodiscard]] GraphDocument deserialize_graph(std::string_view serialized);

}  // namespace ctex::graph

#endif
