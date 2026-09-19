#include <algorithm>
#include <cmath>
#include <ctex/graph/document.hpp>
#include <limits>
#include <optional>
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

bool socket_type_is_preserved(std::span<const NodeSocket> before, std::span<const NodeSocket> after,
                              std::string_view identifier) {
    const NodeSocket* old_socket = find_socket(before, identifier);
    const NodeSocket* new_socket = find_socket(after, identifier);
    return old_socket != nullptr && new_socket != nullptr && old_socket->type == new_socket->type;
}

void preserve_input_values(std::span<const NodeSocket> previous,
                           std::span<NodeSocket> replacement) {
    for (NodeSocket& new_socket : replacement) {
        const NodeSocket* old_socket = find_socket(previous, new_socket.identifier);
        if (old_socket != nullptr && old_socket->type == new_socket.type) {
            new_socket.value = old_socket->value;
        }
    }
}

std::size_t node_index(std::span<const GraphNode> nodes, NodeId id) {
    const auto found = std::lower_bound(
        nodes.begin(), nodes.end(), id,
        [](const GraphNode& node_value, NodeId sought) { return node_value.id < sought; });
    return found == nodes.end() || found->id != id
               ? std::numeric_limits<std::size_t>::max()
               : static_cast<std::size_t>(found - nodes.begin());
}

std::optional<std::vector<NodeId>> path_between(std::span<const GraphNode> nodes,
                                                std::span<const GraphLink> links, NodeId start,
                                                NodeId target) {
    if (start == target) {
        return std::vector<NodeId>{start};
    }
    std::vector<NodeId> parent(nodes.size(), 0);
    std::vector<NodeId> queue;
    queue.reserve(nodes.size());
    parent[node_index(nodes, start)] = start;
    queue.push_back(start);
    for (std::size_t cursor = 0; cursor < queue.size(); ++cursor) {
        const NodeId current = queue[cursor];
        auto link = std::lower_bound(
            links.begin(), links.end(), current,
            [](const GraphLink& value, NodeId source) { return value.source_node < source; });
        for (; link != links.end() && link->source_node == current; ++link) {
            const std::size_t next_index = node_index(nodes, link->target_node);
            if (parent[next_index] != 0) {
                continue;
            }
            parent[next_index] = current;
            queue.push_back(link->target_node);
            if (link->target_node != target) {
                continue;
            }
            std::vector<NodeId> path{target};
            while (path.back() != start) {
                path.push_back(parent[node_index(nodes, path.back())]);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
    }
    return std::nullopt;
}

void validate_acyclic(std::span<const GraphNode> nodes, std::span<const GraphLink> links) {
    std::vector<std::size_t> offsets(nodes.size() + 1, 0);
    std::vector<std::size_t> incoming(nodes.size(), 0);
    for (const GraphLink& link : links) {
        ++offsets[node_index(nodes, link.source_node) + 1];
        ++incoming[node_index(nodes, link.target_node)];
    }
    for (std::size_t index = 1; index < offsets.size(); ++index) {
        offsets[index] += offsets[index - 1];
    }
    std::vector<std::size_t> cursor = offsets;
    std::vector<std::size_t> adjacency(links.size());
    for (const GraphLink& link : links) {
        const std::size_t source = node_index(nodes, link.source_node);
        adjacency[cursor[source]++] = node_index(nodes, link.target_node);
    }

    std::vector<std::size_t> queue;
    queue.reserve(nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        if (incoming[index] == 0) {
            queue.push_back(index);
        }
    }
    for (std::size_t position = 0; position < queue.size(); ++position) {
        const std::size_t source = queue[position];
        for (std::size_t adjacent = offsets[source]; adjacent < offsets[source + 1]; ++adjacent) {
            const std::size_t target_index = adjacency[adjacent];
            if (--incoming[target_index] == 0) {
                queue.push_back(target_index);
            }
        }
    }
    if (queue.size() != nodes.size()) {
        throw std::invalid_argument("serialized graph contains a directed cycle");
    }
}

std::string cycle_message(std::span<const NodeId> path) {
    std::string result = "graph link would create cycle: ";
    for (std::size_t index = 0; index < path.size(); ++index) {
        if (index != 0) {
            result.append(" -> ");
        }
        result.append(std::to_string(path[index]));
    }
    return result;
}

std::string socket_type_message(SocketType source, SocketType target) {
    return "cannot connect " + std::string(socket_type_name(source)) + " output to " +
           std::string(socket_type_name(target)) + " input";
}

}  // namespace

std::string_view socket_type_name(SocketType type) noexcept {
    switch (type) {
        case SocketType::scalar:
            return "scalar";
        case SocketType::vector:
            return "vector";
        case SocketType::colour:
            return "colour";
        case SocketType::string:
            return "string";
        case SocketType::image:
            return "image";
        case SocketType::boolean:
            return "boolean";
    }
    return "unknown";
}

std::optional<SocketCoercion> socket_coercion(SocketType source, SocketType target) noexcept {
    if (source == target) {
        return SocketCoercion::identity;
    }
    if (source == SocketType::scalar && target == SocketType::vector) {
        return SocketCoercion::scalar_to_vector;
    }
    if (source == SocketType::vector && target == SocketType::scalar) {
        return SocketCoercion::vector_to_scalar;
    }
    if (source == SocketType::colour && target == SocketType::vector) {
        return SocketCoercion::colour_to_vector;
    }
    if (source == SocketType::colour && target == SocketType::scalar) {
        return SocketCoercion::colour_to_scalar;
    }
    return std::nullopt;
}

SocketTypeError::SocketTypeError(SocketType source, SocketType target)
    : std::invalid_argument(socket_type_message(source, target)),
      source_type_(source),
      target_type_(target) {}

GraphCycleError::GraphCycleError(std::vector<NodeId> cycle_path)
    : std::invalid_argument(cycle_message(cycle_path)), cycle_path_(std::move(cycle_path)) {}

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

void GraphDocument::set_property_value(NodeId id, std::string_view property_key,
                                       SocketValue value) {
    GraphNode& node_value = mutable_node(id);
    const auto found =
        std::find_if(node_value.properties.begin(), node_value.properties.end(),
                     [&](const NodeProperty& property) { return property.key == property_key; });
    if (found == node_value.properties.end()) {
        throw std::out_of_range("graph node property does not exist");
    }
    if (!value_is_valid(value) || value.index() != found->value.index()) {
        throw std::invalid_argument("graph property value is non-finite or has the wrong type");
    }
    found->value = std::move(value);
}

NodeInterfaceUpdate GraphDocument::update_node_interface(NodeId id, std::uint32_t type_version,
                                                         std::vector<NodeSocket> inputs,
                                                         std::vector<NodeSocket> outputs) {
    if (type_version == 0) {
        throw std::invalid_argument("graph node interface requires a non-zero type version");
    }
    validate_sockets(inputs, "input");
    validate_sockets(outputs, "output");
    const GraphNode& existing = node(id);
    preserve_input_values(existing.inputs, inputs);

    GraphDocument updated = *this;
    GraphNode& replacement = updated.mutable_node(id);
    replacement.type_version = type_version;
    replacement.inputs = std::move(inputs);
    replacement.outputs = std::move(outputs);
    validate_node(replacement);

    NodeInterfaceUpdate result;
    std::erase_if(updated.links_, [&](const GraphLink& link) {
        const bool source_removed =
            link.source_node == id &&
            !socket_type_is_preserved(existing.outputs, replacement.outputs, link.source_socket);
        const bool target_removed =
            link.target_node == id &&
            !socket_type_is_preserved(existing.inputs, replacement.inputs, link.target_socket);
        if (source_removed || target_removed) {
            result.removed_links.push_back(link);
            return true;
        }
        return false;
    });
    updated.validate();
    nodes_.swap(updated.nodes_);
    links_.swap(updated.links_);
    return result;
}

AddLinkResult GraphDocument::add_link(GraphLink link) {
    const GraphNode& source = node(link.source_node);
    const GraphNode& target = node(link.target_node);
    const NodeSocket* source_socket = find_socket(source.outputs, link.source_socket);
    const NodeSocket* target_socket = find_socket(target.inputs, link.target_socket);
    if (source_socket == nullptr) {
        throw std::invalid_argument("graph link source socket does not exist");
    }
    if (target_socket == nullptr) {
        throw std::invalid_argument("graph link target socket does not exist");
    }
    const auto coercion = socket_coercion(source_socket->type, target_socket->type);
    if (!coercion) {
        throw SocketTypeError(source_socket->type, target_socket->type);
    }
    if (std::find(links_.begin(), links_.end(), link) != links_.end()) {
        throw std::invalid_argument("graph link already exists");
    }
    if (auto path = path_between(nodes_, links_, link.target_node, link.source_node)) {
        path->insert(path->begin(), link.source_node);
        throw GraphCycleError(std::move(*path));
    }
    const auto replaced = std::find_if(links_.begin(), links_.end(), [&](const GraphLink& current) {
        return current.target_node == link.target_node &&
               current.target_socket == link.target_socket;
    });
    AddLinkResult result{.coercion = *coercion, .replaced_link = std::nullopt};
    std::vector<GraphLink> updated = links_;
    if (replaced != links_.end()) {
        result.replaced_link = *replaced;
        updated.erase(updated.begin() + (replaced - links_.begin()));
    }
    updated.push_back(std::move(link));
    std::sort(updated.begin(), updated.end(), link_less);
    links_.swap(updated);
    return result;
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
    std::set<std::pair<NodeId, std::string_view>> occupied_inputs;
    for (const GraphLink& link : links_) {
        const GraphNode& source = node(link.source_node);
        const GraphNode& target = node(link.target_node);
        const NodeSocket* source_socket = find_socket(source.outputs, link.source_socket);
        const NodeSocket* target_socket = find_socket(target.inputs, link.target_socket);
        if (source_socket == nullptr || target_socket == nullptr) {
            throw std::invalid_argument("graph link references a missing socket");
        }
        if (!socket_coercion(source_socket->type, target_socket->type)) {
            throw SocketTypeError(source_socket->type, target_socket->type);
        }
        if (!occupied_inputs.emplace(link.target_node, link.target_socket).second) {
            throw std::invalid_argument("serialized graph has more than one link to an input");
        }
    }
    validate_acyclic(nodes_, links_);
}

}  // namespace ctex::graph
