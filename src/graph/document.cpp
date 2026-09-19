#include <algorithm>
#include <cmath>
#include <ctex/graph/document.hpp>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace ctex::graph {
namespace {

bool finite(NodePosition value) { return std::isfinite(value.x) && std::isfinite(value.y); }

bool finite(VectorValue value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finite(ColourValue value) {
    return std::isfinite(value.r) && std::isfinite(value.g) && std::isfinite(value.b) &&
           std::isfinite(value.a);
}

bool value_is_valid(const SocketValue& value) {
    if (const auto* scalar = std::get_if<double>(&value)) {
        return std::isfinite(*scalar);
    }
    if (const auto* vector = std::get_if<VectorValue>(&value)) {
        return finite(*vector);
    }
    if (const auto* colour = std::get_if<ColourValue>(&value)) {
        return finite(*colour);
    }
    return true;
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

void validate_sockets(std::span<const NodeSocket> sockets, std::string_view direction) {
    std::set<std::string_view> identifiers;
    for (const NodeSocket& socket : sockets) {
        if (socket.identifier.empty() || !identifiers.insert(socket.identifier).second) {
            throw std::invalid_argument("graph node has an empty or duplicate " +
                                        std::string(direction) + " socket identifier");
        }
        if (!value_is_valid(socket.value) || !value_matches(socket.type, socket.value)) {
            throw std::invalid_argument("graph socket value is non-finite or has the wrong type");
        }
    }
}

void validate_node(const GraphNode& node) {
    if (node.type_id.empty() || node.type_version == 0 || !finite(node.position)) {
        throw std::invalid_argument("graph node requires a type, version, and finite position");
    }
    validate_sockets(node.inputs, "input");
    validate_sockets(node.outputs, "output");
    std::set<std::string_view> property_keys;
    for (const NodeProperty& property : node.properties) {
        if (property.key.empty() || !property_keys.insert(property.key).second ||
            !value_is_valid(property.value)) {
            throw std::invalid_argument("graph node has an invalid or duplicate property");
        }
    }
}

bool link_less(const GraphLink& left, const GraphLink& right) {
    if (left.source_node != right.source_node) {
        return left.source_node < right.source_node;
    }
    if (left.source_socket != right.source_socket) {
        return left.source_socket < right.source_socket;
    }
    if (left.target_node != right.target_node) {
        return left.target_node < right.target_node;
    }
    return left.target_socket < right.target_socket;
}

const NodeSocket* find_socket(std::span<const NodeSocket> sockets, std::string_view identifier) {
    const auto found = std::find_if(sockets.begin(), sockets.end(), [&](const NodeSocket& socket) {
        return socket.identifier == identifier;
    });
    return found == sockets.end() ? nullptr : &*found;
}

}  // namespace

GraphDocument::GraphDocument(GraphNode output_node) {
    if (output_node.id != 0 || output_node.role != NodeRole::output) {
        throw std::invalid_argument("graph construction requires one unassigned output node");
    }
    output_node.id = 1;
    nodes_.push_back(std::move(output_node));
    next_node_id_ = 2;
    validate();
}

GraphDocument::GraphDocument(std::vector<GraphNode> nodes, std::vector<GraphLink> links,
                             NodeId next_node_id, SerializedTag)
    : nodes_(std::move(nodes)), links_(std::move(links)), next_node_id_(next_node_id) {
    std::sort(nodes_.begin(), nodes_.end(),
              [](const GraphNode& left, const GraphNode& right) { return left.id < right.id; });
    std::sort(links_.begin(), links_.end(), link_less);
    validate();
}

NodeId GraphDocument::add_node(GraphNode node_value) {
    if (node_value.id != 0 || node_value.role != NodeRole::regular) {
        throw std::invalid_argument("added graph nodes must be unassigned regular nodes");
    }
    validate_node(node_value);
    if (next_node_id_ == std::numeric_limits<NodeId>::max()) {
        throw std::overflow_error("graph node identity space is exhausted");
    }
    node_value.id = next_node_id_++;
    nodes_.push_back(std::move(node_value));
    return nodes_.back().id;
}

void GraphDocument::remove_node(NodeId id) {
    const auto found = std::find_if(nodes_.begin(), nodes_.end(), [&](const GraphNode& node_value) {
        return node_value.id == id;
    });
    if (found == nodes_.end()) {
        throw std::out_of_range("graph node does not exist");
    }
    if (found->role == NodeRole::output) {
        throw std::invalid_argument("graph output node cannot be removed");
    }
    nodes_.erase(found);
    std::erase_if(links_, [&](const GraphLink& link) {
        return link.source_node == id || link.target_node == id;
    });
}

void GraphDocument::set_node_position(NodeId id, NodePosition position) {
    if (!finite(position)) {
        throw std::invalid_argument("graph node position must be finite");
    }
    mutable_node(id).position = position;
}

void GraphDocument::set_input_value(NodeId id, std::string_view socket_identifier,
                                    SocketValue value) {
    GraphNode& node_value = mutable_node(id);
    const auto found = std::find_if(
        node_value.inputs.begin(), node_value.inputs.end(),
        [&](const NodeSocket& socket) { return socket.identifier == socket_identifier; });
    if (found == node_value.inputs.end()) {
        throw std::out_of_range("graph input socket does not exist");
    }
    if (!value_is_valid(value) || !value_matches(found->type, value)) {
        throw std::invalid_argument("graph input value is non-finite or has the wrong type");
    }
    found->value = std::move(value);
}

void GraphDocument::add_link(GraphLink link) {
    const GraphNode& source = node(link.source_node);
    const GraphNode& target = node(link.target_node);
    if (find_socket(source.outputs, link.source_socket) == nullptr) {
        throw std::invalid_argument("graph link source socket does not exist");
    }
    if (find_socket(target.inputs, link.target_socket) == nullptr) {
        throw std::invalid_argument("graph link target socket does not exist");
    }
    if (std::find(links_.begin(), links_.end(), link) != links_.end()) {
        throw std::invalid_argument("graph link already exists");
    }
    links_.push_back(std::move(link));
    std::sort(links_.begin(), links_.end(), link_less);
}

bool GraphDocument::remove_link(const GraphLink& link) noexcept {
    const auto found = std::find(links_.begin(), links_.end(), link);
    if (found == links_.end()) {
        return false;
    }
    links_.erase(found);
    return true;
}

const GraphNode& GraphDocument::node(NodeId id) const {
    const auto found = std::lower_bound(
        nodes_.begin(), nodes_.end(), id,
        [](const GraphNode& node_value, NodeId sought) { return node_value.id < sought; });
    if (found == nodes_.end() || found->id != id) {
        throw std::out_of_range("graph node does not exist");
    }
    return *found;
}

GraphNode& GraphDocument::mutable_node(NodeId id) {
    return const_cast<GraphNode&>(std::as_const(*this).node(id));
}

NodeId GraphDocument::output_node_id() const noexcept {
    const auto found = std::find_if(nodes_.begin(), nodes_.end(), [](const GraphNode& node_value) {
        return node_value.role == NodeRole::output;
    });
    return found == nodes_.end() ? 0 : found->id;
}

void GraphDocument::validate() const {
    if (nodes_.empty() || next_node_id_ == 0) {
        throw std::invalid_argument("graph document requires nodes and a next identity");
    }
    std::size_t output_count = 0;
    NodeId previous_id = 0;
    for (const GraphNode& node_value : nodes_) {
        if (node_value.id == 0 || node_value.id <= previous_id) {
            throw std::invalid_argument("graph node identities must be non-zero and unique");
        }
        validate_node(node_value);
        output_count += node_value.role == NodeRole::output ? 1 : 0;
        previous_id = node_value.id;
    }
    if (output_count != 1 || next_node_id_ <= previous_id) {
        throw std::invalid_argument("graph requires one output and a monotonic next identity");
    }
    if (!std::is_sorted(links_.begin(), links_.end(), link_less) ||
        std::adjacent_find(links_.begin(), links_.end()) != links_.end()) {
        throw std::invalid_argument("graph links must be unique and canonically ordered");
    }
    for (const GraphLink& link : links_) {
        const GraphNode& source = node(link.source_node);
        const GraphNode& target = node(link.target_node);
        if (find_socket(source.outputs, link.source_socket) == nullptr ||
            find_socket(target.inputs, link.target_socket) == nullptr) {
            throw std::invalid_argument("graph link references a missing socket");
        }
    }
}

}  // namespace ctex::graph
