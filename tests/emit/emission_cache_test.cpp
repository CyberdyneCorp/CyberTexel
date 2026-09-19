#include <ctex/emit/emission_cache.hpp>
#include <iostream>
#include <string_view>

namespace {

using namespace ctex::emit;
using ctex::graph::GraphDocument;
using ctex::graph::GraphNode;
using ctex::graph::NodeRole;
using ctex::graph::NodeSocket;
using ctex::graph::SocketType;

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

GraphDocument graph(double value = 0.25) {
    return GraphDocument({.role = NodeRole::output,
                          .type_id = "ctex.material-output",
                          .type_version = 1,
                          .display_name = "Material Output",
                          .position = {},
                          .inputs = {scalar_socket("roughness", value)},
                          .outputs = {},
                          .properties = {}});
}

ctex::graph::HostNodeTypeRegistration counted_registration(int& calls) {
    return {
        .declaration = {.type_id = "test.cache-value",
                        .version = 1,
                        .display_name = "Cache Value",
                        .category = ctex::graph::NodeCategory::host,
                        .inputs = {},
                        .outputs = {scalar_socket("value", 0.0)},
                        .properties = {}},
        .cpu_evaluate =
            [](const ctex::graph::NodeEvaluationRequest&) {
                return std::vector<ctex::graph::SocketValue>{0.5};
            },
        .emit =
            [&calls](const ctex::graph::NodeEmissionRequest&) {
                ++calls;
                return ctex::graph::NodeEmissionResult{
                    .output_expressions = {"5.00000000000000000e-01"}, .resource_identifiers = {}};
            },
        .deterministic = true,
        .resource_dependencies = {},
        .supported_targets = {ctex::graph::EmissionTarget::wgsl},
        .parity_fixtures =
            {{.identifier = "half", .inputs = {}, .expected_outputs = {0.5}, .tolerance = 0.0}}};
}

DeviceFeatureSet features(bool compute = false) {
    return {.binding_budget = 9,
            .maximum_texture_dimension = 4096,
            .supported_texture_formats = {TextureFormat::rgba8_unorm, TextureFormat::rgba16_float},
            .floating_point_filtering = true,
            .compute_available = compute};
}

LogicalTexture texture(std::string identifier, bool initialized = true) {
    return {.version = {std::move(identifier), 1},
            .role = "cache fixture",
            .format = TextureFormat::rgba8_unorm,
            .extent = {128, 128, 1},
            .mip_levels = 1,
            .tile_shape = {64, 64},
            .externally_initialized = initialized};
}

LayerStackEmissionRequest layer_request(ShaderTarget target = ShaderTarget::wgsl) {
    return {.stable_identity = "cache/layer-stack",
            .target = target,
            .features = features(),
            .layers = {{"bottom", texture("bottom")}, {"top", texture("top")}},
            .output = texture("output", false),
            .requested_filter = FilterMode::linear};
}

bool unchanged_graph_is_served_from_cache() {
    GraphEmissionCache cache;
    int emission_calls = 0;
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(counted_registration(emission_calls));
    GraphDocument fixture = graph();
    const auto value = fixture.add_node(registry.make_node("test.cache-value", 1));
    static_cast<void>(fixture.add_link({value, "value", fixture.output_node_id(), "roughness"}));
    const CachedGraphEmission first = cache.emit(fixture, registry, ShaderTarget::wgsl, features());
    const CachedGraphEmission second =
        cache.emit(fixture, registry, ShaderTarget::wgsl, features());
    return expect(!first.cache_hit && second.cache_hit,
                  "unchanged graph did not transition from miss to hit") &&
           expect(first.emission == second.emission && *first.emission == *second.emission,
                  "graph cache hit did not return the identical immutable emission") &&
           expect(emission_calls == 1, "graph cache hit invoked code generation again") &&
           expect(cache.statistics() == EmissionCacheStatistics{1, 1, 1},
                  "graph cache statistics did not record one entry, hit, and miss");
}

bool graph_content_and_features_are_key_material() {
    GraphEmissionCache cache;
    GraphDocument fixture = graph();
    const CachedGraphEmission original = cache.emit(fixture, ShaderTarget::wgsl, features());

    fixture.set_input_value(fixture.output_node_id(), "roughness", 0.75);
    const CachedGraphEmission changed_graph = cache.emit(fixture, ShaderTarget::wgsl, features());
    const CachedGraphEmission changed_features =
        cache.emit(fixture, ShaderTarget::wgsl, features(true));

    DeviceFeatureSet equivalent = features(true);
    equivalent.supported_texture_formats = {TextureFormat::rgba16_float, TextureFormat::rgba8_unorm,
                                            TextureFormat::rgba8_unorm};
    const CachedGraphEmission normalized = cache.emit(fixture, ShaderTarget::wgsl, equivalent);

    return expect(!original.cache_hit && !changed_graph.cache_hit && !changed_features.cache_hit,
                  "changed graph content or feature set reused a prior entry") &&
           expect(original.emission->source != changed_graph.emission->source,
                  "graph content change did not alter emitted source") &&
           expect(changed_graph.emission->source == changed_features.emission->source,
                  "feature-only cache split unexpectedly changed graph source") &&
           expect(normalized.cache_hit && normalized.emission == changed_features.emission,
                  "equivalent supported-format sets produced distinct keys") &&
           expect(cache.statistics() == EmissionCacheStatistics{3, 1, 3},
                  "graph content/feature cache accounting is incorrect");
}

bool target_is_checked_before_graph_cache_lookup() {
    GraphEmissionCache cache;
    try {
        static_cast<void>(cache.emit(graph(), ShaderTarget::msl, features()));
    } catch (const GraphEmissionError& error) {
        return expect(
            std::string_view(error.what()).find("only WGSL, not MSL") != std::string_view::npos &&
                cache.statistics() == EmissionCacheStatistics{},
            "unsupported graph target changed the cache or lacked a diagnostic");
    }
    return expect(false, "graph expression cache accepted a non-WGSL target");
}

bool host_registry_semantics_partition_graph_entries() {
    GraphEmissionCache cache;
    int emission_calls = 0;
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(counted_registration(emission_calls));
    GraphDocument fixture = graph();
    const auto value = fixture.add_node(registry.make_node("test.cache-value", 1));
    static_cast<void>(fixture.add_link({value, "value", fixture.output_node_id(), "roughness"}));
    static_cast<void>(cache.emit(fixture, registry, ShaderTarget::wgsl, features()));
    try {
        static_cast<void>(cache.emit(fixture, ShaderTarget::wgsl, features()));
    } catch (const GraphEmissionError&) {
        return expect(cache.statistics() == EmissionCacheStatistics{1, 0, 2} && emission_calls == 1,
                      "an empty registry reused a host-registry graph entry");
    }
    return expect(false, "graph cache key ignored host registry semantics");
}

bool workspace_graph_content_is_cached_canonically() {
    GraphEmissionCache cache;
    ctex::graph::GraphWorkspace workspace;
    workspace.add_material("fixture", graph());
    const CachedGraphEmission first =
        cache.emit_material(workspace, "fixture", ShaderTarget::wgsl, features());
    const CachedGraphEmission second =
        cache.emit_material(workspace, "fixture", ShaderTarget::wgsl, features());
    workspace.material("fixture").set_input_value(workspace.material("fixture").output_node_id(),
                                                  "roughness", 0.625);
    const CachedGraphEmission changed =
        cache.emit_material(workspace, "fixture", ShaderTarget::wgsl, features());
    return expect(!first.cache_hit && second.cache_hit && !changed.cache_hit,
                  "workspace graph content did not drive cache hits and invalidation") &&
           expect(first.emission == second.emission &&
                      first.emission->source != changed.emission->source,
                  "workspace cache returned stale graph source") &&
           expect(cache.statistics() == EmissionCacheStatistics{2, 1, 2},
                  "workspace cache accounting is incorrect");
}

bool layer_stack_hit_returns_identical_shader_and_plan() {
    LayerStackEmissionCache cache;
    const LayerStackEmissionRequest fixture = layer_request();
    const CachedLayerStackEmission first = cache.emit(fixture);
    const CachedLayerStackEmission second = cache.emit(fixture);
    return expect(!first.cache_hit && second.cache_hit && first.emission == second.emission,
                  "unchanged layer stack was not served from one immutable cache entry") &&
           expect(first.emission->shaders == second.emission->shaders &&
                      first.emission->pass_plan == second.emission->pass_plan,
                  "layer-stack hit changed shader source or pass plan") &&
           expect(cache.statistics() == EmissionCacheStatistics{1, 1, 1},
                  "layer-stack hit/miss statistics are incorrect");
}

bool target_features_and_content_split_layer_entries() {
    LayerStackEmissionCache cache;
    LayerStackEmissionRequest fixture = layer_request();
    const CachedLayerStackEmission wgsl = cache.emit(fixture);

    fixture.target = ShaderTarget::hlsl;
    const CachedLayerStackEmission hlsl = cache.emit(fixture);
    fixture.target = ShaderTarget::wgsl;
    fixture.features.compute_available = true;
    const CachedLayerStackEmission compute_feature = cache.emit(fixture);
    fixture.layers.front().identifier = "renamed-bottom";
    const CachedLayerStackEmission changed_content = cache.emit(fixture);

    return expect(!wgsl.cache_hit && !hlsl.cache_hit && !compute_feature.cache_hit &&
                      !changed_content.cache_hit,
                  "target, feature, or content change reused a layer-stack entry") &&
           expect(wgsl.emission->shaders.front().shader.target == ShaderTarget::wgsl &&
                      hlsl.emission->shaders.front().shader.target == ShaderTarget::hlsl,
                  "target-specific cache entries returned the wrong shader artifact") &&
           expect(compute_feature.emission->pass_plan != changed_content.emission->pass_plan,
                  "changed layer content did not produce a distinct pass plan") &&
           expect(cache.statistics() == EmissionCacheStatistics{4, 0, 4},
                  "layer-stack key dimensions did not create four entries");
}

bool failed_emission_is_not_cached_and_clear_resets_state() {
    LayerStackEmissionCache cache;
    LayerStackEmissionRequest invalid = layer_request();
    invalid.features.binding_budget = 1;
    for (int attempt = 0; attempt < 2; ++attempt) {
        try {
            static_cast<void>(cache.emit(invalid));
            return expect(false, "invalid layer-stack emission unexpectedly succeeded");
        } catch (const FeatureEmissionError&) {
        }
    }
    if (!expect(cache.statistics() == EmissionCacheStatistics{0, 0, 2},
                "failed emissions were stored as reusable entries")) {
        return false;
    }
    static_cast<void>(cache.emit(layer_request()));
    cache.clear();
    return expect(cache.statistics() == EmissionCacheStatistics{},
                  "clearing the cache did not reset entries and counters");
}

}  // namespace

int main() {
    return unchanged_graph_is_served_from_cache() &&
                   graph_content_and_features_are_key_material() &&
                   target_is_checked_before_graph_cache_lookup() &&
                   host_registry_semantics_partition_graph_entries() &&
                   workspace_graph_content_is_cached_canonically() &&
                   layer_stack_hit_returns_identical_shader_and_plan() &&
                   target_features_and_content_split_layer_entries() &&
                   failed_emission_is_not_cached_and_clear_resets_state()
               ? 0
               : 1;
}
