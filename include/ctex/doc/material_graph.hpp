#ifndef CTEX_DOC_MATERIAL_GRAPH_HPP
#define CTEX_DOC_MATERIAL_GRAPH_HPP

#include <ctex/doc/channels.hpp>
#include <ctex/graph/document.hpp>
#include <span>

namespace ctex::doc {

// Creates the graph's single output node from the registered channel order.
[[nodiscard]] graph::GraphDocument make_material_graph(std::span<const ChannelDescriptor> channels);

}  // namespace ctex::doc

#endif
