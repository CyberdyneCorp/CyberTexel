#include <cmath>
#include <ctex/doc/material_graph.hpp>
#include <stdexcept>

namespace ctex::doc {
namespace {

void validate_channel(const ChannelDescriptor& channel) {
    if (channel.semantic_id.empty() || channel.component_count == 0 ||
        channel.component_count > 4 || channel.default_value.size() != channel.component_count) {
        throw std::invalid_argument("material graph output channel descriptor is malformed");
    }
    for (double value : channel.default_value) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("material graph output channel default is non-finite");
        }
    }
}

graph::NodeSocket output_socket(const ChannelDescriptor& channel) {
    validate_channel(channel);
    graph::SocketType type = graph::SocketType::scalar;
    graph::SocketValue value = channel.default_value.front();
    if (channel.classification == ChannelClassification::color && channel.component_count >= 3) {
        type = graph::SocketType::colour;
        value = graph::ColourValue{
            static_cast<float>(channel.default_value[0]),
            static_cast<float>(channel.default_value[1]),
            static_cast<float>(channel.default_value[2]),
            channel.component_count == 4 ? static_cast<float>(channel.default_value[3]) : 1.0F,
        };
    } else if (channel.component_count == 2 || channel.component_count == 3) {
        type = graph::SocketType::vector;
        value = graph::VectorValue{
            static_cast<float>(channel.default_value[0]),
            static_cast<float>(channel.default_value[1]),
            channel.component_count == 3 ? static_cast<float>(channel.default_value[2]) : 0.0F,
        };
    } else if (channel.component_count == 4) {
        type = graph::SocketType::colour;
        value = graph::ColourValue{
            static_cast<float>(channel.default_value[0]),
            static_cast<float>(channel.default_value[1]),
            static_cast<float>(channel.default_value[2]),
            static_cast<float>(channel.default_value[3]),
        };
    }
    return {
        .identifier = {channel.semantic_id.begin(), channel.semantic_id.end()},
        .display_name = {channel.semantic_id.begin(), channel.semantic_id.end()},
        .type = type,
        .value = std::move(value),
    };
}

}  // namespace

graph::GraphDocument make_material_graph(std::span<const ChannelDescriptor> channels) {
    if (channels.empty()) {
        throw std::invalid_argument("material graph requires at least one registered channel");
    }
    std::vector<graph::NodeSocket> inputs;
    inputs.reserve(channels.size());
    for (const ChannelDescriptor& channel : channels) {
        inputs.push_back(output_socket(channel));
    }
    return graph::GraphDocument({
        .role = graph::NodeRole::output,
        .type_id = "ctex.output",
        .type_version = 1,
        .display_name = "Material Output",
        .position = {},
        .inputs = std::move(inputs),
        .outputs = {},
        .properties = {},
    });
}

}  // namespace ctex::doc
