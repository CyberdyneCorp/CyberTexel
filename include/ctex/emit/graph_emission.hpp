#ifndef CTEX_EMIT_GRAPH_EMISSION_HPP
#define CTEX_EMIT_GRAPH_EMISSION_HPP

#include <ctex/graph/groups.hpp>
#include <ctex/graph/host_nodes.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::emit {

struct WgslOutputExpression {
    std::string identifier;
    graph::SocketType type;
    std::string variable_name;
    friend bool operator==(const WgslOutputExpression&, const WgslOutputExpression&) = default;
};

struct ShaderNodeAttribution {
    std::string variable_name;
    std::string node_path;
    std::string type_id;
    graph::NodeId node_id{};
    std::string output_socket;
    friend bool operator==(const ShaderNodeAttribution&, const ShaderNodeAttribution&) = default;
};

struct WgslExpressionProgram {
    // A deterministic sequence of WGSL statements intended for a function body.
    std::string source;
    std::vector<WgslOutputExpression> outputs;
    std::vector<std::string> resource_identifiers;
    std::vector<ShaderNodeAttribution> node_attributions;
    friend bool operator==(const WgslExpressionProgram&, const WgslExpressionProgram&) = default;
};

class GraphEmissionError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] WgslExpressionProgram emit_wgsl_expressions(const graph::GraphDocument& graph,
                                                          const graph::NodeTypeRegistry& registry);
[[nodiscard]] WgslExpressionProgram emit_wgsl_expressions(const graph::GraphDocument& graph);

[[nodiscard]] WgslExpressionProgram emit_material_wgsl_expressions(
    const graph::GraphWorkspace& workspace, std::string_view material_identifier,
    const graph::NodeTypeRegistry& registry);
[[nodiscard]] WgslExpressionProgram emit_material_wgsl_expressions(
    const graph::GraphWorkspace& workspace, std::string_view material_identifier);

}  // namespace ctex::emit

#endif
