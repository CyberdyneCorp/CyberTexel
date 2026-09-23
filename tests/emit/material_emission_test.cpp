#include <array>
#include <cstdlib>
#include <ctex/emit/emission_cache.hpp>
#include <ctex/emit/material_emission.hpp>
#include <ctex/graph/catalogue.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>

namespace {

using ctex::emit::DeviceFeatureSet;
using ctex::emit::LogicalTexture;
using ctex::emit::MaterialResourceInput;
using ctex::emit::MaterialShaderEmissionRequest;
using ctex::emit::ShaderTarget;
using ctex::emit::TextureFormat;
using ctex::graph::ColourValue;
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

NodeSocket socket(std::string identifier, SocketType type, ctex::graph::SocketValue value) {
    return {identifier, std::move(identifier), type, std::move(value)};
}

GraphDocument constant_material(double red = 0.25) {
    GraphNode output{.role = NodeRole::output,
                     .type_id = "test.material-output",
                     .type_version = 1,
                     .display_name = "Material Output",
                     .position = {},
                     .inputs = {socket("pbr.base_color", SocketType::colour,
                                       ColourValue{0.5F, 0.5F, 0.5F, 1.0F}),
                                socket("pbr.opacity", SocketType::scalar, 1.0)},
                     .outputs = {},
                     .properties = {}};
    GraphDocument graph(std::move(output));
    auto colour = ctex::graph::make_builtin_node("ctex.input.constant-colour");
    colour.properties[0].value = ColourValue{static_cast<float>(red), 0.5F, 0.75F, 0.8F};
    const auto colour_id = graph.add_node(std::move(colour));
    static_cast<void>(
        graph.add_link({colour_id, "colour", graph.output_node_id(), "pbr.base_color"}));
    return graph;
}

LogicalTexture texture(std::string id, std::uint64_t generation, std::string role,
                       TextureFormat format = TextureFormat::rgba8_unorm,
                       bool initialized = false) {
    return {.version = {std::move(id), generation},
            .role = std::move(role),
            .format = format,
            .extent = {64, 32, 1},
            .mip_levels = 1,
            .tile_shape = {16, 16},
            .externally_initialized = initialized};
}

DeviceFeatureSet features(bool float_filtering = true) {
    return {.binding_budget = 16,
            .maximum_texture_dimension = 8192,
            .supported_texture_formats = {TextureFormat::rgba8_unorm, TextureFormat::rgba16_float},
            .floating_point_filtering = float_filtering,
            .compute_available = false};
}

MaterialShaderEmissionRequest request(ShaderTarget target = ShaderTarget::wgsl) {
    return {.stable_identity = "material/scenario",
            .target = target,
            .features = features(),
            .resources = {},
            .output = texture("material/output", 7, "material graph output"),
            .requested_filter = ctex::emit::FilterMode::linear,
            .vertex_count = 3};
}

bool every_target_receives_a_complete_headless_artifact() {
    const GraphDocument graph = constant_material();
    for (const ShaderTarget target : ctex::emit::supported_shader_targets()) {
        const auto emitted = ctex::emit::emit_material_shader(graph, request(target));
        const auto passes = emitted.pass_plan.passes();
        if (!expect(emitted.shader.target == target && passes.size() == 1 &&
                        passes[0].identifier == "material-graph" &&
                        passes[0].render_targets.size() == 1 &&
                        passes[0].render_targets[0].resource ==
                            ctex::emit::ResourceVersion{"material/output", 7} &&
                        std::get<ctex::emit::DrawCommand>(passes[0].command).vertex_count == 3,
                    "target artifact or host pass plan was incomplete")) {
            return false;
        }
        if (target == ShaderTarget::spirv) {
            const auto& binary = std::get<ctex::emit::SpirvShaderProgram>(emitted.shader.payload);
            if (!expect(!binary.vertex_module.empty() && !binary.fragment_module.empty(),
                        "SPIR-V material emission returned an empty module")) {
                return false;
            }
        }
    }
    return true;
}

bool constant_changes_preserve_the_binding_layout() {
    const auto first = ctex::emit::emit_material_shader(constant_material(0.25), request());
    const auto changed = ctex::emit::emit_material_shader(constant_material(0.75), request());
    const auto& first_text =
        std::get<ctex::emit::SplitTextShaderProgram>(first.shader.payload).fragment_source;
    const auto& changed_text =
        std::get<ctex::emit::SplitTextShaderProgram>(changed.shader.payload).fragment_source;
    return expect(first.pass_plan == changed.pass_plan,
                  "constant-only edit changed the host pipeline layout or pass plan") &&
           expect(first_text != changed_text,
                  "constant-only edit did not update the emitted shader value");
}

ctex::graph::HostNodeTypeRegistration sampled_colour_registration() {
    const std::string resource = "textures/host-albedo.exr";
    return {
        .declaration = {.type_id = "acme.sample-colour",
                        .version = 1,
                        .display_name = "Sample Colour",
                        .category = ctex::graph::NodeCategory::host,
                        .inputs = {},
                        .outputs = {socket("colour", SocketType::colour, {})},
                        .properties = {}},
        .cpu_evaluate =
            [](const ctex::graph::NodeEvaluationRequest&) {
                return std::vector<ctex::graph::SocketValue>{ColourValue{0.2F, 0.4F, 0.6F, 1.0F}};
            },
        .emit =
            [resource](const ctex::graph::NodeEmissionRequest&) {
                return ctex::graph::NodeEmissionResult{
                    .output_expressions = {"textureSample(" +
                                           ctex::emit::material_resource_name(resource) +
                                           ", ctex_material_sampler, input.uv)"},
                    .resource_identifiers = {resource}};
            },
        .deterministic = true,
        .resource_dependencies = {},
        .supported_targets = {ctex::graph::EmissionTarget::wgsl},
        .parity_fixtures = {{.identifier = "reference",
                             .inputs = {},
                             .expected_outputs = {ColourValue{0.2F, 0.4F, 0.6F, 1.0F}},
                             .tolerance = 0.0}},
    };
}

GraphDocument host_material(const ctex::graph::NodeTypeRegistry& registry) {
    GraphDocument graph = constant_material();
    const auto node = graph.add_node(registry.make_node("acme.sample-colour", 1));
    static_cast<void>(graph.add_link({node, "colour", graph.output_node_id(), "pbr.base_color"}));
    return graph;
}

bool host_resources_are_explicit_and_float_filtering_is_gated() {
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(sampled_colour_registration());
    auto fixture = request();
    fixture.features = features(false);
    fixture.resources = {MaterialResourceInput{
        "textures/host-albedo.exr",
        texture("host/albedo", 4, "host albedo", TextureFormat::rgba16_float, true)}};
    const auto emitted =
        ctex::emit::emit_material_shader(host_material(registry), registry, fixture);
    const auto passes = emitted.pass_plan.passes();
    const auto& pass = passes.front();
    const auto& source =
        std::get<ctex::emit::SplitTextShaderProgram>(emitted.shader.payload).fragment_source;
    return expect(
               pass.texture_bindings.size() == 1 && pass.sampler_bindings.size() == 1 &&
                   pass.texture_bindings[0].role == "material resource textures/host-albedo.exr" &&
                   pass.texture_bindings[0].binding == 0 && pass.sampler_bindings[0].binding == 1,
               "host resource creation or binding order was implicit") &&
           expect(emitted.workarounds.size() == 1 &&
                      emitted.workarounds[0].code == "nearest_float_sampling" &&
                      pass.sampler_bindings[0].min_filter == ctex::emit::FilterMode::nearest,
                  "unavailable float filtering was not removed and reported") &&
           expect(source.find(ctex::emit::material_resource_name("textures/host-albedo.exr")) !=
                          std::string::npos &&
                      source.find("// node acme.sample-colour") != std::string::npos,
                  "WGSL source lost its resource declaration or node attribution");
}

bool missing_resources_and_unknown_targets_are_named() {
    ctex::graph::NodeTypeRegistry registry;
    registry.register_type(sampled_colour_registration());
    bool resource_named = false;
    try {
        static_cast<void>(
            ctex::emit::emit_material_shader(host_material(registry), registry, request()));
    } catch (const ctex::emit::MaterialEmissionError& error) {
        resource_named = std::string_view(error.what()).find("textures/host-albedo.exr") !=
                         std::string_view::npos;
    }

    auto unsupported_host_target = request(ShaderTarget::msl);
    unsupported_host_target.resources = {MaterialResourceInput{
        "textures/host-albedo.exr",
        texture("host/albedo", 4, "host albedo", TextureFormat::rgba16_float, true)}};
    bool host_target_named = false;
    try {
        static_cast<void>(ctex::emit::emit_material_shader(host_material(registry), registry,
                                                           unsupported_host_target));
    } catch (const ctex::emit::MaterialEmissionError& error) {
        const std::string_view message(error.what());
        host_target_named = message.find("acme.sample-colour") != std::string_view::npos &&
                            message.find("msl") != std::string_view::npos;
    }

    auto unsupported = request(static_cast<ShaderTarget>(99));
    bool target_named = false;
    try {
        static_cast<void>(ctex::emit::emit_material_shader(constant_material(), unsupported));
    } catch (const ctex::emit::KongCompilationError& error) {
        const std::string_view message(error.what());
        target_named = message.find("unknown(99)") != std::string_view::npos &&
                       message.find("WGSL, MSL, SPIR-V, HLSL") != std::string_view::npos;
    }
    return expect(resource_named, "missing material resource diagnostic omitted its identity") &&
           expect(host_target_named,
                  "unsupported host-node target diagnostic omitted its type or target") &&
           expect(target_named, "unknown shader target diagnostic omitted the target inventory");
}

bool complete_material_results_are_cached_by_content_target_and_features() {
    ctex::emit::MaterialShaderEmissionCache cache;
    const GraphDocument graph = constant_material();
    const auto first = cache.emit(graph, request());
    const auto second = cache.emit(graph, request());
    auto other_target = request(ShaderTarget::msl);
    const auto msl = cache.emit(graph, other_target);
    auto other_features = request();
    other_features.features.compute_available = true;
    const auto feature_variant = cache.emit(graph, other_features);
    return expect(!first.cache_hit && second.cache_hit && !msl.cache_hit &&
                      !feature_variant.cache_hit && first.emission == second.emission &&
                      *first.emission == *second.emission,
                  "complete material cache did not reuse one immutable identical result") &&
           expect(cache.statistics() == ctex::emit::EmissionCacheStatistics{3, 1, 3},
                  "material cache did not partition target or feature-set entries");
}

bool write_binary(std::string_view path, std::span<const std::uint32_t> words) {
    std::ofstream stream(std::string(path), std::ios::binary);
    stream.write(reinterpret_cast<const char*>(words.data()),
                 static_cast<std::streamsize>(words.size_bytes()));
    return stream.good();
}

bool write_spirv(std::string_view vertex_path, std::string_view fragment_path) {
    const auto emitted =
        ctex::emit::emit_material_shader(constant_material(), request(ShaderTarget::spirv));
    const auto& shader = std::get<ctex::emit::SpirvShaderProgram>(emitted.shader.payload);
    return expect(write_binary(vertex_path, shader.vertex_module) &&
                      write_binary(fragment_path, shader.fragment_module),
                  "could not write material SPIR-V validation fixtures");
}

bool write_text(const std::filesystem::path& path, std::string_view value) {
    std::ofstream stream(path, std::ios::binary);
    stream.write(value.data(), static_cast<std::streamsize>(value.size()));
    return stream.good();
}

bool write_determinism_artifacts() {
    const char* directory = std::getenv("CTEX_DETERMINISM_OUTPUT_DIR");
    if (directory == nullptr) {
        return expect(false, "CTEX_DETERMINISM_OUTPUT_DIR is required");
    }
    const std::filesystem::path root(directory);
    const auto wgsl = ctex::emit::emit_material_shader(constant_material(), request());
    const auto msl =
        ctex::emit::emit_material_shader(constant_material(), request(ShaderTarget::msl));
    const auto spirv =
        ctex::emit::emit_material_shader(constant_material(), request(ShaderTarget::spirv));
    const auto hlsl =
        ctex::emit::emit_material_shader(constant_material(), request(ShaderTarget::hlsl));
    const auto& wgsl_text = std::get<ctex::emit::SplitTextShaderProgram>(wgsl.shader.payload);
    const auto& msl_text = std::get<ctex::emit::UnifiedTextShaderProgram>(msl.shader.payload);
    const auto& spirv_binary = std::get<ctex::emit::SpirvShaderProgram>(spirv.shader.payload);
    const auto& hlsl_text = std::get<ctex::emit::SplitTextShaderProgram>(hlsl.shader.payload);
    return expect(
        write_text(root / "material.vert.wgsl", wgsl_text.vertex_source) &&
            write_text(root / "material.frag.wgsl", wgsl_text.fragment_source) &&
            write_text(root / "material.msl", msl_text.source) &&
            write_binary((root / "material.vert.spv").string(), spirv_binary.vertex_module) &&
            write_binary((root / "material.frag.spv").string(), spirv_binary.fragment_module) &&
            write_text(root / "material.vert.hlsl", hlsl_text.vertex_source) &&
            write_text(root / "material.frag.hlsl", hlsl_text.fragment_source),
        "could not write material determinism artifacts");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 4 && std::string_view(argv[1]) == "--write-spirv") {
        return write_spirv(argv[2], argv[3]) ? 0 : 1;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--write-determinism") {
        return write_determinism_artifacts() ? 0 : 1;
    }
    return every_target_receives_a_complete_headless_artifact() &&
                   constant_changes_preserve_the_binding_layout() &&
                   host_resources_are_explicit_and_float_filtering_is_gated() &&
                   missing_resources_and_unknown_targets_are_named() &&
                   complete_material_results_are_cached_by_content_target_and_features()
               ? 0
               : 1;
}
