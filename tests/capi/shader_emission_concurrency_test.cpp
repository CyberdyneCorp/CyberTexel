#include <ctex/capi.h>

#include <array>
#include <barrier>
#include <cstddef>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

struct GraphBytes {
    std::vector<std::byte> bytes;
};

struct EmissionBytes {
    ctex_result result{CTEX_RESULT_INTERNAL_ERROR};
    std::vector<std::byte> vertex;
    std::vector<std::byte> fragment;
    std::string plan;

    friend bool operator==(const EmissionBytes&, const EmissionBytes&) = default;
};

GraphBytes make_graph(float red) {
    ctex_material_graph_info graph_info{};
    graph_info.size = CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE;
    if (ctex_material_graph_create_default(&graph_info, nullptr, 0, nullptr, 0) !=
        CTEX_RESULT_SUCCESS) {
        return {};
    }
    std::vector<std::byte> original(graph_info.canonical_size);
    if (ctex_material_graph_create_default(&graph_info, original.data(), original.size(), nullptr,
                                           0) != CTEX_RESULT_SUCCESS) {
        return {};
    }
    ctex_smart_material_value_descriptor colour{};
    colour.size = CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE;
    colour.type = CTEX_SMART_MATERIAL_VALUE_COLOUR;
    colour.colour = {red, 0.25F, 0.75F, 1.0F};
    if (ctex_material_graph_set_input_value(original.data(), original.size(),
                                            graph_info.output_node_id, "pbr.base_color", &colour,
                                            &graph_info, nullptr, 0) != CTEX_RESULT_SUCCESS) {
        return {};
    }
    GraphBytes result{std::vector<std::byte>(graph_info.canonical_size)};
    if (ctex_material_graph_set_input_value(
            original.data(), original.size(), graph_info.output_node_id, "pbr.base_color", &colour,
            &graph_info, result.bytes.data(), result.bytes.size()) != CTEX_RESULT_SUCCESS) {
        return {};
    }
    return result;
}

ctex_shader_material_request request_for(std::uint32_t target) {
    static constexpr std::array formats{std::uint32_t{CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM}};
    return {
        .size = CTEX_SHADER_MATERIAL_REQUEST_CURRENT_SIZE,
        .stable_identity = "concurrency/material",
        .target = target,
        .features = {.size = CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_CURRENT_SIZE,
                     .binding_budget = 16,
                     .maximum_texture_dimension = 8192,
                     .supported_texture_formats = formats.data(),
                     .supported_texture_format_count = formats.size(),
                     .floating_point_filtering = 1,
                     .compute_available = 0},
        .resources = nullptr,
        .resource_count = 0,
        .output = {.size = CTEX_SHADER_TEXTURE_DESCRIPTOR_CURRENT_SIZE,
                   .logical_id = "concurrency/output",
                   .generation = 3,
                   .role = "concurrent material output",
                   .format = CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM,
                   .width = 32,
                   .height = 32,
                   .layers = 1,
                   .mip_levels = 1,
                   .tile_width = 16,
                   .tile_height = 16,
                   .externally_initialized = 0},
        .requested_filter = CTEX_SHADER_FILTER_LINEAR,
        .vertex_count = 3,
    };
}

EmissionBytes emit(const GraphBytes& graph, std::uint32_t target) {
    const ctex_shader_material_request request = request_for(target);
    ctex_shader_material_info info{};
    info.size = CTEX_SHADER_MATERIAL_INFO_CURRENT_SIZE;
    EmissionBytes result;
    result.result =
        ctex_shader_emit_material(nullptr, graph.bytes.data(), graph.bytes.size(), &request, &info,
                                  nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0);
    if (result.result != CTEX_RESULT_SUCCESS) {
        return result;
    }
    result.vertex.resize(info.vertex_artifact_size);
    result.fragment.resize(info.fragment_artifact_size);
    result.plan.resize(info.pass_plan_size);
    std::vector<char> workarounds(info.workaround_report_size);
    result.result = ctex_shader_emit_material(
        nullptr, graph.bytes.data(), graph.bytes.size(), &request, &info, result.vertex.data(),
        result.vertex.size(), result.fragment.data(), result.fragment.size(), result.plan.data(),
        result.plan.size(), workarounds.data(), workarounds.size());
    return result;
}

}  // namespace

int main() {
    constexpr std::array targets{std::uint32_t{CTEX_MATERIAL_GRAPH_TARGET_WGSL},
                                 std::uint32_t{CTEX_MATERIAL_GRAPH_TARGET_MSL},
                                 std::uint32_t{CTEX_MATERIAL_GRAPH_TARGET_SPIRV},
                                 std::uint32_t{CTEX_MATERIAL_GRAPH_TARGET_HLSL}};
    std::array<GraphBytes, targets.size()> graphs;
    std::array<EmissionBytes, targets.size()> serial;
    for (std::size_t index = 0; index < graphs.size(); ++index) {
        graphs[index] = make_graph(static_cast<float>(index + 1) / 8.0F);
        if (graphs[index].bytes.empty()) {
            return 1;
        }
        serial[index] = emit(graphs[index], targets[index]);
        if (serial[index].result != CTEX_RESULT_SUCCESS) {
            return 1;
        }
    }

    std::barrier start(static_cast<std::ptrdiff_t>(targets.size() + 1));
    std::array<EmissionBytes, targets.size()> concurrent;
    std::array<std::thread, targets.size()> workers;
    for (std::size_t index = 0; index < workers.size(); ++index) {
        workers[index] = std::thread([&, index] {
            start.arrive_and_wait();
            concurrent[index] = emit(graphs[index], targets[index]);
        });
    }
    start.arrive_and_wait();
    for (std::thread& worker : workers) {
        worker.join();
    }
    return concurrent == serial ? 0 : 1;
}
