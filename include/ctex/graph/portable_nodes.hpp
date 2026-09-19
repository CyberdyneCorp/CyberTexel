#ifndef CTEX_GRAPH_PORTABLE_NODES_HPP
#define CTEX_GRAPH_PORTABLE_NODES_HPP

#include <ctex/graph/document.hpp>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace ctex::graph {

class BuiltinNodeEvaluationError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Canonical straight-alpha Blend-node formula. Layer-stack implementations use
// the same mode names and reference values rather than maintaining another CPU
// formula table.
[[nodiscard]] ColourValue blend_colour(std::string_view mode, ColourValue base, ColourValue blend,
                                       double factor);

// Portable CPU semantics for built-ins that define material-graph conformance
// fixtures. Inputs are in declaration order and outputs are returned in
// declaration order.
[[nodiscard]] std::vector<SocketValue> evaluate_builtin_node(const GraphNode& node,
                                                             std::span<const SocketValue> inputs);

}  // namespace ctex::graph

#endif
