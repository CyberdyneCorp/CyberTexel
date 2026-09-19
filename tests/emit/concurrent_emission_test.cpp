#include <array>
#include <barrier>
#include <ctex/emit/emission_cache.hpp>
#include <ctex/graph/catalogue.hpp>
#include <future>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ctex::emit;
using ctex::graph::GraphDocument;
using ctex::graph::GraphNode;
using ctex::graph::NodeRole;
using ctex::graph::NodeSocket;
using ctex::graph::SocketType;

constexpr std::size_t worker_count = 4;
constexpr std::array targets{ShaderTarget::wgsl, ShaderTarget::msl, ShaderTarget::spirv,
                             ShaderTarget::hlsl};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

NodeSocket scalar_socket(std::string identifier, double value) {
    return {.identifier = std::move(identifier),
            .display_name = "Value",
            .type = SocketType::scalar,
            .value = value};
}

GraphDocument graph(double value) {
    GraphDocument document({.role = NodeRole::output,
                            .type_id = "ctex.material-output",
                            .type_version = 1,
                            .display_name = "Material Output",
                            .position = {},
                            .inputs = {scalar_socket("roughness", 0.0)},
                            .outputs = {},
                            .properties = {}});
    GraphNode constant = ctex::graph::make_builtin_node("ctex.input.constant-value");
    constant.properties.front().value = value;
    const auto constant_id = document.add_node(std::move(constant));
    static_cast<void>(
        document.add_link({constant_id, "value", document.output_node_id(), "roughness"}));
    return document;
}

DeviceFeatureSet features() {
    return {.binding_budget = 5,
            .maximum_texture_dimension = 4096,
            .supported_texture_formats = {TextureFormat::rgba8_unorm, TextureFormat::rgba16_float},
            .floating_point_filtering = true,
            .compute_available = false};
}

LogicalTexture texture(std::string identifier, bool initialized = true) {
    return {.version = {std::move(identifier), 1},
            .role = "concurrent emission fixture",
            .format = TextureFormat::rgba8_unorm,
            .extent = {128, 128, 1},
            .mip_levels = 1,
            .tile_shape = {64, 64},
            .externally_initialized = initialized};
}

LayerStackEmissionRequest layer_request(std::size_t index) {
    const std::string prefix = "concurrent-" + std::to_string(index);
    return {.stable_identity = prefix,
            .target = targets[index],
            .features = features(),
            .layers = {{"bottom", texture(prefix + "-bottom")}, {"top", texture(prefix + "-top")}},
            .output = texture(prefix + "-output", false),
            .requested_filter = FilterMode::linear};
}

bool four_graphs_match_serial_emission() {
    std::array graphs{graph(0.125), graph(0.25), graph(0.5), graph(0.875)};
    std::array<WgslExpressionProgram, worker_count> serial;
    for (std::size_t index = 0; index < worker_count; ++index) {
        serial[index] = emit_wgsl_expressions(graphs[index]);
    }

    GraphEmissionCache cache;
    std::barrier start(static_cast<std::ptrdiff_t>(worker_count + 1));
    std::array<std::future<CachedGraphEmission>, worker_count> futures;
    for (std::size_t index = 0; index < worker_count; ++index) {
        futures[index] = std::async(std::launch::async, [&, index] {
            start.arrive_and_wait();
            return cache.emit(graphs[index], ShaderTarget::wgsl, features());
        });
    }
    start.arrive_and_wait();

    for (std::size_t index = 0; index < worker_count; ++index) {
        const CachedGraphEmission concurrent = futures[index].get();
        if (!expect(!concurrent.cache_hit && *concurrent.emission == serial[index],
                    "concurrent graph emission differed from serial output")) {
            return false;
        }
    }
    return expect(cache.statistics() == EmissionCacheStatistics{4, 0, 4},
                  "concurrent graph cache did not retain four independent results");
}

bool four_target_layer_stacks_match_serial_emission() {
    const std::array requests{layer_request(0), layer_request(1), layer_request(2),
                              layer_request(3)};
    std::vector<LayerStackEmission> serial;
    serial.reserve(worker_count);
    for (const LayerStackEmissionRequest& request : requests) {
        serial.push_back(emit_layer_stack(request));
    }

    LayerStackEmissionCache cache;
    std::barrier start(static_cast<std::ptrdiff_t>(worker_count + 1));
    std::array<std::future<CachedLayerStackEmission>, worker_count> futures;
    for (std::size_t index = 0; index < worker_count; ++index) {
        futures[index] = std::async(std::launch::async, [&, index] {
            start.arrive_and_wait();
            return cache.emit(requests[index]);
        });
    }
    start.arrive_and_wait();

    for (std::size_t index = 0; index < worker_count; ++index) {
        const CachedLayerStackEmission concurrent = futures[index].get();
        if (!expect(!concurrent.cache_hit && *concurrent.emission == serial[index],
                    "concurrent target emission differed from serial shader and pass plan")) {
            return false;
        }
    }
    return expect(cache.statistics() == EmissionCacheStatistics{4, 0, 4},
                  "concurrent layer-stack cache did not retain four independent results");
}

}  // namespace

int main() {
    return four_graphs_match_serial_emission() && four_target_layer_stacks_match_serial_emission()
               ? 0
               : 1;
}
