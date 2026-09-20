#include <algorithm>
#include <charconv>
#include <ctex/emit/graph_emission.hpp>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace ctex::emit {
namespace {

using graph::GraphDocument;
using graph::GraphLink;
using graph::GraphNode;
using graph::NodeGroupDefinition;
using graph::NodeId;
using graph::NodeSocket;
using graph::SocketType;
using graph::SocketValue;

struct QualifiedGroup {
    std::string identifier;
    NodeId instance_id;
};

struct Frame {
    const GraphDocument& graph;
    const Frame* parent{};
    const GraphNode* instance{};
    std::vector<QualifiedGroup> path;
};

struct EmittedValue {
    SocketType type;
    std::string variable_name;
};

struct MemoEntry {
    SocketType type;
    bool ready{};
};

std::string hex_identifier(std::string_view value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(value.size() * 2);
    for (unsigned char character : value) {
        result.push_back(digits[character >> 4U]);
        result.push_back(digits[character & 0x0fU]);
    }
    return result;
}

std::string comment_text(std::string_view value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (unsigned char character : value) {
        if (character >= 0x20U && character <= 0x7eU) {
            result.push_back(static_cast<char>(character));
            continue;
        }
        result.append("\\x");
        result.push_back(digits[character >> 4U]);
        result.push_back(digits[character & 0x0fU]);
    }
    return result;
}

std::string result_name(std::span<const QualifiedGroup> path, NodeId node_id,
                        std::string_view socket_identifier) {
    std::string result = "ctex_";
    for (const QualifiedGroup& group : path) {
        result.append("g");
        result.append(hex_identifier(group.identifier));
        result.append("_i");
        result.append(std::to_string(group.instance_id));
        result.append("_");
    }
    result.append("n");
    result.append(std::to_string(node_id));
    result.append("_s");
    result.append(hex_identifier(socket_identifier));
    return result;
}

const NodeSocket& find_socket(std::span<const NodeSocket> sockets, std::string_view identifier,
                              std::string_view kind) {
    const auto found = std::find_if(sockets.begin(), sockets.end(), [&](const NodeSocket& socket) {
        return socket.identifier == identifier;
    });
    if (found == sockets.end()) {
        throw GraphEmissionError(std::string(kind) +
                                 " socket does not exist: " + std::string(identifier));
    }
    return *found;
}

const graph::NodeProperty& find_property(const GraphNode& node, std::string_view key) {
    const auto found =
        std::find_if(node.properties.begin(), node.properties.end(),
                     [&](const graph::NodeProperty& property) { return property.key == key; });
    if (found == node.properties.end()) {
        throw GraphEmissionError("node " + std::to_string(node.id) + " has no property " +
                                 std::string(key));
    }
    return *found;
}

template <typename Value>
std::string floating_literal(Value value) {
    char buffer[64];
    const auto converted =
        std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::scientific,
                      std::numeric_limits<Value>::max_digits10);
    if (converted.ec != std::errc{}) {
        throw GraphEmissionError("could not format a floating-point graph value");
    }
    return std::string(buffer, converted.ptr);
}

std::string literal(const SocketValue& value, SocketType type) {
    switch (type) {
        case SocketType::scalar:
            if (const auto* scalar = std::get_if<double>(&value)) {
                return floating_literal(*scalar);
            }
            break;
        case SocketType::vector:
            if (const auto* vector = std::get_if<graph::VectorValue>(&value)) {
                return "vec3<f32>(" + floating_literal(vector->x) + ", " +
                       floating_literal(vector->y) + ", " + floating_literal(vector->z) + ")";
            }
            break;
        case SocketType::colour:
            if (const auto* colour = std::get_if<graph::ColourValue>(&value)) {
                return "vec4<f32>(" + floating_literal(colour->r) + ", " +
                       floating_literal(colour->g) + ", " + floating_literal(colour->b) + ", " +
                       floating_literal(colour->a) + ")";
            }
            break;
        case SocketType::boolean:
            if (const auto* boolean = std::get_if<bool>(&value)) {
                return *boolean ? "true" : "false";
            }
            break;
        case SocketType::string:
        case SocketType::image:
            throw GraphEmissionError("socket type " + std::string(graph::socket_type_name(type)) +
                                     " cannot be represented as a WGSL value expression");
    }
    throw GraphEmissionError("stored value does not match its socket type");
}

std::string wgsl_type(SocketType type) {
    switch (type) {
        case SocketType::scalar:
            return "f32";
        case SocketType::vector:
            return "vec3<f32>";
        case SocketType::colour:
            return "vec4<f32>";
        case SocketType::boolean:
            return "bool";
        case SocketType::string:
        case SocketType::image:
            break;
    }
    throw GraphEmissionError("socket type " + std::string(graph::socket_type_name(type)) +
                             " cannot be declared as a WGSL value");
}

std::string coerce_expression(const EmittedValue& source, SocketType target) {
    const auto coercion = graph::socket_coercion(source.type, target);
    if (!coercion) {
        throw GraphEmissionError("cannot coerce emitted " +
                                 std::string(graph::socket_type_name(source.type)) + " to " +
                                 std::string(graph::socket_type_name(target)));
    }
    switch (*coercion) {
        case graph::SocketCoercion::identity:
            return source.variable_name;
        case graph::SocketCoercion::scalar_to_vector:
            return "vec3<f32>(" + source.variable_name + ")";
        case graph::SocketCoercion::vector_to_scalar:
            return "dot(" + source.variable_name +
                   ", vec3<f32>(2.126000000e-01, 7.152000000e-01, 7.220000000e-02))";
        case graph::SocketCoercion::colour_to_vector:
            return "(" + source.variable_name + ").rgb";
        case graph::SocketCoercion::colour_to_scalar:
            return "dot((" + source.variable_name +
                   ").rgb, vec3<f32>(2.126000000e-01, 7.152000000e-01, 7.220000000e-02))";
    }
    throw GraphEmissionError("unknown socket coercion");
}

class WgslEmitter {
public:
    WgslEmitter(const graph::GraphWorkspace* workspace, const graph::NodeTypeRegistry& registry)
        : workspace_(workspace), registry_(registry) {}

    WgslExpressionProgram emit(const GraphDocument& graph) {
        Frame root{.graph = graph, .parent = nullptr, .instance = nullptr, .path = {}};
        const GraphNode& output = graph.node(graph.output_node_id());
        program_.source = "// CyberTexel deterministic WGSL expression program\n";
        for (const NodeSocket& socket : output.inputs) {
            const std::string expression = input_expression(root, output, socket);
            const std::string variable = "ctex_output_s" + hex_identifier(socket.identifier);
            program_.source.append("// material output ");
            program_.source.append(comment_text(socket.identifier));
            program_.source.append("\nlet ");
            program_.source.append(variable);
            program_.source.append(": ");
            program_.source.append(wgsl_type(socket.type));
            program_.source.append(" = ");
            program_.source.append(expression);
            program_.source.append(";\n");
            program_.outputs.push_back({socket.identifier, socket.type, variable});
        }
        return std::move(program_);
    }

private:
    using InputKey = std::pair<NodeId, std::string>;

    const std::map<InputKey, const GraphLink*>& input_links(const GraphDocument& graph) {
        const auto cached = links_.find(&graph);
        if (cached != links_.end()) {
            return cached->second;
        }
        std::map<InputKey, const GraphLink*> index;
        for (const GraphLink& link : graph.links()) {
            index.emplace(InputKey{link.target_node, link.target_socket}, &link);
        }
        return links_.emplace(&graph, std::move(index)).first->second;
    }

    std::optional<const GraphLink*> input_link(const Frame& frame, const GraphNode& node,
                                               const NodeSocket& socket) {
        const auto& index = input_links(frame.graph);
        const auto found = index.find({node.id, socket.identifier});
        return found == index.end() ? std::nullopt : std::optional{found->second};
    }

    std::string input_expression(const Frame& frame, const GraphNode& node,
                                 const NodeSocket& socket) {
        const auto link = input_link(frame, node, socket);
        if (!link) {
            return literal(socket.value, socket.type);
        }
        const GraphNode& source_node = frame.graph.node((*link)->source_node);
        const NodeSocket& source_socket =
            find_socket(source_node.outputs, (*link)->source_socket, "source");
        return coerce_expression(output_expression(frame, source_node, source_socket), socket.type);
    }

    EmittedValue output_expression(const Frame& frame, const GraphNode& node,
                                   const NodeSocket& socket) {
        if (node.type_id == "ctex.group-input") {
            return group_input_expression(frame, socket);
        }
        if (node.type_id == "ctex.group-instance") {
            return group_instance_expression(frame, node, socket);
        }
        return ordinary_node_expression(frame, node, socket);
    }

    EmittedValue group_input_expression(const Frame& frame, const NodeSocket& socket) {
        if (frame.parent == nullptr || frame.instance == nullptr) {
            throw GraphEmissionError("group input node appeared outside a group instance");
        }
        const NodeSocket& instance_input =
            find_socket(frame.instance->inputs, socket.identifier, "group instance input");
        return {instance_input.type,
                input_expression(*frame.parent, *frame.instance, instance_input)};
    }

    std::string group_identifier(const GraphNode& node) const {
        const auto& property = find_property(node, "group_id");
        const auto* identifier = std::get_if<std::string>(&property.value);
        if (identifier == nullptr || identifier->empty()) {
            throw GraphEmissionError("group instance has an invalid group identifier");
        }
        return *identifier;
    }

    EmittedValue group_instance_expression(const Frame& frame, const GraphNode& node,
                                           const NodeSocket& socket) {
        if (workspace_ == nullptr) {
            throw GraphEmissionError("group instances require workspace emission");
        }
        const std::string name = result_name(frame.path, node.id, socket.identifier);
        if (const auto found = memo_.find(name); found != memo_.end()) {
            if (!found->second.ready) {
                throw GraphEmissionError("recursive emission reached " + name);
            }
            return {found->second.type, name};
        }
        memo_.emplace(name, MemoEntry{socket.type, false});

        const std::string identifier = group_identifier(node);
        const NodeGroupDefinition& group = workspace_->group(identifier);
        Frame inner{group.graph, &frame, &node, frame.path};
        inner.path.push_back({identifier, node.id});
        const GraphNode& group_output = group.graph.node(group.graph.output_node_id());
        const NodeSocket& output_input =
            find_socket(group_output.inputs, socket.identifier, "group output");
        const std::string expression = input_expression(inner, group_output, output_input);
        emit_declaration(frame.path, node, socket, name, expression);
        memo_.at(name).ready = true;
        return {socket.type, name};
    }

    std::vector<std::string> builtin_expressions(const GraphNode& node) {
        if (node.type_id == "ctex.input.constant-value") {
            return {literal(find_property(node, "value").value, SocketType::scalar)};
        }
        if (node.type_id == "ctex.input.constant-colour") {
            return {literal(find_property(node, "colour").value, SocketType::colour)};
        }
        throw GraphEmissionError("built-in node emission is not implemented for " + node.type_id);
    }

    std::vector<std::string> node_expressions(const GraphNode& node,
                                              std::span<const std::string> inputs) {
        if (const auto* builtin = graph::find_builtin_node_type(node.type_id); builtin != nullptr) {
            if (builtin->version != node.type_version) {
                throw GraphEmissionError(
                    "built-in node version is not available for WGSL emission: " + node.type_id +
                    "@" + std::to_string(node.type_version));
            }
            return builtin_expressions(node);
        }
        if (registry_.find(node.type_id, node.type_version) == nullptr) {
            throw GraphEmissionError("node type is not available for WGSL emission: " +
                                     node.type_id + "@" + std::to_string(node.type_version));
        }
        try {
            graph::NodeEmissionResult result =
                registry_.emit(node, graph::EmissionTarget::wgsl, inputs);
            for (std::string& resource : result.resource_identifiers) {
                if (std::find(program_.resource_identifiers.begin(),
                              program_.resource_identifiers.end(),
                              resource) == program_.resource_identifiers.end()) {
                    program_.resource_identifiers.push_back(std::move(resource));
                }
            }
            return std::move(result.output_expressions);
        } catch (const graph::NodeInvocationError& error) {
            throw GraphEmissionError(error.what());
        }
    }

    void emit_declaration(std::span<const QualifiedGroup> path, const GraphNode& node,
                          const NodeSocket& socket, std::string_view name,
                          std::string_view expression) {
        std::string node_path;
        for (const QualifiedGroup& group : path) {
            node_path.append(group.identifier);
            node_path.append("[");
            node_path.append(std::to_string(group.instance_id));
            node_path.append("]/ ");
        }
        node_path.append(node.type_id);
        node_path.append("[");
        node_path.append(std::to_string(node.id));
        node_path.append("]");
        program_.node_attributions.push_back(
            {std::string(name), node_path, node.type_id, node.id, socket.identifier});
        program_.source.append("// node ");
        for (const QualifiedGroup& group : path) {
            program_.source.append(comment_text(group.identifier));
            program_.source.append("[");
            program_.source.append(std::to_string(group.instance_id));
            program_.source.append("]/ ");
        }
        program_.source.append(comment_text(node.type_id));
        program_.source.append("[");
        program_.source.append(std::to_string(node.id));
        program_.source.append("] output ");
        program_.source.append(comment_text(socket.identifier));
        program_.source.append("\nlet ");
        program_.source.append(name);
        program_.source.append(": ");
        program_.source.append(wgsl_type(socket.type));
        program_.source.append(" = ");
        program_.source.append(expression);
        program_.source.append(";\n");
    }

    void emit_ordinary_node(const Frame& frame, const GraphNode& node) {
        std::vector<std::string> names;
        names.reserve(node.outputs.size());
        for (const NodeSocket& output : node.outputs) {
            std::string name = result_name(frame.path, node.id, output.identifier);
            const auto [position, inserted] = memo_.emplace(name, MemoEntry{output.type, false});
            if (!inserted && !position->second.ready) {
                throw GraphEmissionError("recursive emission reached " + name);
            }
            names.push_back(std::move(name));
        }

        std::vector<std::string> inputs;
        inputs.reserve(node.inputs.size());
        for (const NodeSocket& input : node.inputs) {
            inputs.push_back(input_expression(frame, node, input));
        }
        const std::vector<std::string> expressions = node_expressions(node, inputs);
        if (expressions.size() != node.outputs.size()) {
            throw GraphEmissionError("node emission returned the wrong output count for " +
                                     node.type_id);
        }
        for (std::size_t index = 0; index < node.outputs.size(); ++index) {
            emit_declaration(frame.path, node, node.outputs[index], names[index],
                             expressions[index]);
            memo_.at(names[index]).ready = true;
        }
    }

    EmittedValue ordinary_node_expression(const Frame& frame, const GraphNode& node,
                                          const NodeSocket& socket) {
        const std::string name = result_name(frame.path, node.id, socket.identifier);
        const auto found = memo_.find(name);
        if (found == memo_.end()) {
            emit_ordinary_node(frame, node);
        } else if (!found->second.ready) {
            throw GraphEmissionError("recursive emission reached " + name);
        }
        return {socket.type, name};
    }

    const graph::GraphWorkspace* workspace_;
    const graph::NodeTypeRegistry& registry_;
    WgslExpressionProgram program_;
    std::map<std::string, MemoEntry> memo_;
    std::map<const GraphDocument*, std::map<InputKey, const GraphLink*>> links_;
};

}  // namespace

WgslExpressionProgram emit_wgsl_expressions(const GraphDocument& graph,
                                            const graph::NodeTypeRegistry& registry) {
    return WgslEmitter(nullptr, registry).emit(graph);
}

WgslExpressionProgram emit_wgsl_expressions(const GraphDocument& graph) {
    const graph::NodeTypeRegistry registry;
    return emit_wgsl_expressions(graph, registry);
}

WgslExpressionProgram emit_material_wgsl_expressions(const graph::GraphWorkspace& workspace,
                                                     std::string_view material_identifier,
                                                     const graph::NodeTypeRegistry& registry) {
    return WgslEmitter(&workspace, registry).emit(workspace.material(material_identifier));
}

WgslExpressionProgram emit_material_wgsl_expressions(const graph::GraphWorkspace& workspace,
                                                     std::string_view material_identifier) {
    const graph::NodeTypeRegistry registry;
    return emit_material_wgsl_expressions(workspace, material_identifier, registry);
}

}  // namespace ctex::emit
