#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctex/emit/feature_emission.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>

namespace {

using namespace ctex::emit;

constexpr std::array supported_formats{TextureFormat::rgba8_unorm, TextureFormat::rgba16_float};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

LogicalTexture texture(std::string identifier, TextureFormat format, bool initialized = true) {
    return {.version = {std::move(identifier), 1},
            .role = "layer-stack texture",
            .format = format,
            .extent = {256, 128, 1},
            .mip_levels = 1,
            .tile_shape = {64, 64},
            .externally_initialized = initialized};
}

DeviceFeatureSet features(std::uint32_t binding_budget, bool float_filtering = true) {
    return {.binding_budget = binding_budget,
            .maximum_texture_dimension = 4096,
            .supported_texture_formats = {supported_formats.begin(), supported_formats.end()},
            .floating_point_filtering = float_filtering,
            .compute_available = false};
}

LayerStackEmissionRequest request(std::size_t layer_count, std::uint32_t binding_budget,
                                  TextureFormat format = TextureFormat::rgba8_unorm,
                                  ShaderTarget target = ShaderTarget::wgsl,
                                  bool float_filtering = true) {
    std::vector<LayerStackInput> layers;
    for (std::size_t index = 0; index < layer_count; ++index) {
        layers.push_back(
            {"layer-" + std::to_string(index), texture("source-" + std::to_string(index), format)});
    }
    return {.stable_identity = "material/fixture/layer-stack-v1",
            .target = target,
            .features = features(binding_budget, float_filtering),
            .layers = std::move(layers),
            .output = texture("composite", format, false),
            .requested_filter = FilterMode::linear};
}

bool eight_layers_fit_one_pass_when_budget_allows() {
    const LayerStackEmission emitted = emit_layer_stack(request(8, 9));
    const PassDescriptor& pass = emitted.pass_plan.passes().front();
    const auto& shader = std::get<SplitTextShaderProgram>(emitted.shaders.front().shader.payload);
    return expect(emitted.pass_plan.passes().size() == 1 && emitted.shaders.size() == 1,
                  "eight layers that fit the binding budget were split") &&
           expect(pass.texture_bindings.size() == 8 && pass.sampler_bindings.size() == 1 &&
                      pass.sampler_bindings.front().binding == 8,
                  "single-pass binding layout does not contain eight layers and one sampler") &&
           expect(shader.fragment_source.find("@group(0) @binding(8)") != std::string::npos &&
                      shader.fragment_source.find("textureSample") != std::string::npos,
                  "single-pass WGSL does not expose the planned bindings or sampling code");
}

bool binding_budget_splits_and_carries_in_order() {
    const LayerStackEmission emitted = emit_layer_stack(request(8, 4));
    const auto passes = emitted.pass_plan.passes();
    if (!expect(passes.size() == 4 && emitted.shaders.size() == 4,
                "eight layers at budget four did not split into four passes")) {
        return false;
    }
    for (std::size_t index = 0; index < passes.size(); ++index) {
        const PassDescriptor& pass = passes[index];
        if (!expect(pass.texture_bindings.size() + pass.sampler_bindings.size() <= 4,
                    "a split pass exceeded the declared binding budget")) {
            return false;
        }
        if (index > 0 &&
            !expect(pass.dependencies ==
                            std::vector<std::string>{"layer-stack-" + std::to_string(index - 1)} &&
                        pass.texture_bindings.front().role == "carried composite",
                    "split pass did not depend on and bind the preceding intermediate")) {
            return false;
        }
    }
    return expect(passes.front().texture_bindings.front().role == "layer layer-0" &&
                      passes.back().texture_bindings.back().role == "layer layer-7" &&
                      emitted.pass_plan.lifetimes().size() == emitted.pass_plan.resources().size(),
                  "layer order or intermediate lifetimes were not preserved");
}

bool missing_float_filtering_uses_reported_nearest_workaround() {
    const LayerStackEmission emitted =
        emit_layer_stack(request(4, 4, TextureFormat::rgba16_float, ShaderTarget::wgsl, false));
    return expect(emitted.workarounds ==
                      std::vector<CapabilityWorkaround>{
                          {"nearest_float_sampling",
                           "linear floating-point texture filtering is unavailable; emitted "
                           "samplers use nearest filtering"}},
                  "float-filtering workaround was not reported exactly once") &&
           expect(std::ranges::all_of(
                      emitted.pass_plan.passes(),
                      [](const PassDescriptor& pass) {
                          return pass.sampler_bindings.front().min_filter == FilterMode::nearest &&
                                 pass.sampler_bindings.front().mag_filter == FilterMode::nearest;
                      }),
                  "float-filtering workaround left a linear sampler in the plan") &&
           expect(!emitted.compute_used,
                  "render layer-stack emission incorrectly reported compute use");
}

bool every_target_compiles_layer_stack_source() {
    for (const ShaderTarget target : supported_shader_targets()) {
        const LayerStackEmission emitted =
            emit_layer_stack(request(2, 4, TextureFormat::rgba8_unorm, target));
        const bool uses_named_msl_entries = target == ShaderTarget::msl;
        const PassDescriptor& pass = emitted.pass_plan.passes().front();
        if (!expect(
                emitted.shaders.front().shader.target == target &&
                    pass.vertex_entry_point == (uses_named_msl_entries ? "ctex_vertex" : "main") &&
                    pass.fragment_entry_point ==
                        (uses_named_msl_entries ? "ctex_fragment" : "main"),
                "layer-stack target or exported entry points were not preserved")) {
            return false;
        }
    }
    return true;
}

bool feature_limit_refusals_are_named() {
    LayerStackEmissionRequest unsupported = request(1, 4);
    unsupported.layers.front().texture.format = TextureFormat::rgba32_float;
    bool unsupported_named = false;
    try {
        static_cast<void>(emit_layer_stack(unsupported));
    } catch (const FeatureEmissionError& error) {
        unsupported_named = std::string_view(error.what()).find("unsupported texture format") !=
                            std::string_view::npos;
    }

    LayerStackEmissionRequest oversized = request(1, 4);
    oversized.features.maximum_texture_dimension = 64;
    bool dimension_named = false;
    try {
        static_cast<void>(emit_layer_stack(oversized));
    } catch (const FeatureEmissionError& error) {
        dimension_named = std::string_view(error.what()).find("maximum texture dimension 64") !=
                          std::string_view::npos;
    }

    bool budget_named = false;
    try {
        static_cast<void>(emit_layer_stack(request(2, 2)));
    } catch (const FeatureEmissionError& error) {
        budget_named =
            std::string_view(error.what()).find("binding budget 2") != std::string_view::npos;
    }

    LayerStackEmissionRequest conflicting = request(2, 4);
    conflicting.layers[1].texture.version = conflicting.layers[0].texture.version;
    conflicting.layers[1].texture.format = TextureFormat::rgba16_float;
    bool conflict_named = false;
    try {
        static_cast<void>(emit_layer_stack(conflicting));
    } catch (const FeatureEmissionError& error) {
        conflict_named = std::string_view(error.what()).find("conflicting layer declarations") !=
                         std::string_view::npos;
    }
    return expect(unsupported_named && dimension_named && budget_named && conflict_named,
                  "feature-limit refusal did not name the violated capability");
}

bool identical_feature_emission_is_deterministic() {
    const LayerStackEmissionRequest fixture = request(8, 4);
    return expect(emit_layer_stack(fixture) == emit_layer_stack(fixture),
                  "identical layer-stack feature emission changed source or pass plan");
}

bool write_binary(std::string_view path, std::span<const std::uint32_t> words) {
    std::ofstream stream(std::string(path), std::ios::binary);
    stream.write(reinterpret_cast<const char*>(words.data()),
                 static_cast<std::streamsize>(words.size_bytes()));
    return stream.good();
}

bool write_spirv(std::string_view vertex_path, std::string_view fragment_path) {
    const LayerStackEmission emitted =
        emit_layer_stack(request(2, 4, TextureFormat::rgba8_unorm, ShaderTarget::spirv));
    const auto& shader = std::get<SpirvShaderProgram>(emitted.shaders.front().shader.payload);
    return expect(write_binary(vertex_path, shader.vertex_module) &&
                      write_binary(fragment_path, shader.fragment_module),
                  "could not write layer-stack SPIR-V validation fixtures");
}

bool write_determinism_artifacts() {
    const char* directory = std::getenv("CTEX_DETERMINISM_OUTPUT_DIR");
    if (directory == nullptr) {
        return expect(false, "CTEX_DETERMINISM_OUTPUT_DIR is required");
    }
    const LayerStackEmission emitted = emit_layer_stack(request(8, 4));
    for (std::size_t index = 0; index < emitted.shaders.size(); ++index) {
        const auto& shader =
            std::get<SplitTextShaderProgram>(emitted.shaders[index].shader.payload);
        for (const auto& [suffix, text] :
             std::array<std::pair<std::string_view, std::string_view>, 2>{
                 std::pair{"vert", std::string_view(shader.vertex_source)},
                 std::pair{"frag", std::string_view(shader.fragment_source)}}) {
            const std::filesystem::path path =
                std::filesystem::path(directory) /
                ("layer-stack-" + std::to_string(index) + "." + std::string(suffix) + ".wgsl");
            std::ofstream stream(path, std::ios::binary);
            stream.write(text.data(), static_cast<std::streamsize>(text.size()));
            if (!expect(stream.good(), "could not write layer-stack determinism artifact")) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 4 && std::string_view(argv[1]) == "--write-spirv") {
        return write_spirv(argv[2], argv[3]) ? 0 : 1;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--write-determinism") {
        return write_determinism_artifacts() ? 0 : 1;
    }
    return eight_layers_fit_one_pass_when_budget_allows() &&
                   binding_budget_splits_and_carries_in_order() &&
                   missing_float_filtering_uses_reported_nearest_workaround() &&
                   every_target_compiles_layer_stack_source() &&
                   feature_limit_refusals_are_named() &&
                   identical_feature_emission_is_deterministic()
               ? 0
               : 1;
}
