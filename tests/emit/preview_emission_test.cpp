#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctex/emit/emission_cache.hpp>
#include <ctex/emit/preview_emission.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>

namespace {

using namespace ctex::emit;

constexpr std::array channel_definitions{
    std::pair{"pbr.base_color", 3U}, std::pair{"pbr.opacity", 1U},
    std::pair{"pbr.roughness", 1U},  std::pair{"pbr.metallic", 1U},
    std::pair{"pbr.normal", 3U},     std::pair{"pbr.height", 1U},
    std::pair{"pbr.occlusion", 1U},  std::pair{"pbr.emission", 3U},
    std::pair{"pbr.subsurface", 1U}};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

TextureFormat channel_format(std::uint32_t component_count) {
    return component_count == 1 ? TextureFormat::r8_unorm : TextureFormat::rgba8_unorm;
}

LogicalTexture texture(std::string identifier, TextureFormat format, bool initialized = true,
                       std::uint32_t size = 256, std::uint32_t layers = 1,
                       std::uint32_t mip_levels = 1) {
    return {.version = {std::move(identifier), 1},
            .role = "preview fixture",
            .format = format,
            .extent = {size, size, layers},
            .mip_levels = mip_levels,
            .tile_shape = {64, 64},
            .externally_initialized = initialized};
}

DeviceFeatureSet features(std::uint32_t binding_budget = 16) {
    return {.binding_budget = binding_budget,
            .maximum_texture_dimension = 4096,
            .supported_texture_formats = {TextureFormat::r8_unorm, TextureFormat::rg16_float,
                                          TextureFormat::rgba8_unorm, TextureFormat::rgba16_float},
            .floating_point_filtering = true,
            .compute_available = false};
}

std::vector<PreviewChannelInput> channels() {
    std::vector<PreviewChannelInput> result;
    for (const auto& [semantic, components] : channel_definitions) {
        result.push_back({semantic, static_cast<std::uint8_t>(components),
                          texture(std::string(semantic), channel_format(components))});
    }
    return result;
}

PreviewEnvironmentInput environment() {
    return {
        .radiance = texture("lighting/radiance", TextureFormat::rgba16_float, true, 256, 6, 8),
        .diffuse_irradiance = texture("lighting/diffuse", TextureFormat::rgba16_float, true, 32, 6),
        .specular_brdf_lookup = texture("lighting/brdf", TextureFormat::rg16_float, true, 256)};
}

PreviewEmissionRequest request(ShaderTarget target = ShaderTarget::wgsl,
                               bool with_environment = true) {
    return {.stable_identity = "material/preview-fixture",
            .target = target,
            .features = features(),
            .channels = channels(),
            .output = texture("preview/output", TextureFormat::rgba8_unorm, false, 512),
            .environment = with_environment ? std::optional<PreviewEnvironmentInput>(environment())
                                            : std::nullopt,
            .analytic_light_count = 2,
            .vertex_count = 36};
}

bool environment_contract_is_complete() {
    const PreviewEmission emitted = emit_lit_preview(request());
    const PassDescriptor& pass = emitted.pass_plan.passes().front();
    const auto radiance = std::ranges::find(
        pass.texture_bindings, "prefiltered environment radiance", &TextureBinding::role);
    const auto irradiance = std::ranges::find(
        pass.texture_bindings, "diffuse environment irradiance", &TextureBinding::role);
    const auto brdf = std::ranges::find(pass.texture_bindings, "split-sum specular BRDF lookup",
                                        &TextureBinding::role);
    const auto& shader = std::get<SplitTextShaderProgram>(emitted.shader.payload);
    return expect(emitted.kind == PreviewEmissionKind::lit && !emitted.fallback_lighting,
                  "environment preview did not report lit environment mode") &&
           expect(radiance != pass.texture_bindings.end() &&
                      radiance->view_dimension == TextureViewDimension::cube &&
                      radiance->encoding.find("radiance") != std::string::npos &&
                      radiance->mip_convention.find("roughness") != std::string::npos,
                  "radiance binding omitted its cube, encoding, or mip contract") &&
           expect(irradiance != pass.texture_bindings.end() &&
                      irradiance->encoding.find("not divided by pi") != std::string::npos &&
                      brdf != pass.texture_bindings.end() &&
                      brdf->encoding.find("Fresnel") != std::string::npos,
                  "irradiance or BRDF lookup encoding was not declared") &&
           expect(pass.uniform_blocks.size() == 1 &&
                      pass.uniform_blocks.front().fields ==
                          std::vector<UniformField>{{"camera_position", 0, 16, 16},
                                                    {"environment_rotation_intensity", 16, 16, 16},
                                                    {"light_0_direction_intensity", 32, 16, 16},
                                                    {"light_0_color", 48, 16, 16},
                                                    {"light_1_direction_intensity", 64, 16, 16},
                                                    {"light_1_color", 80, 16, 16}},
                  "camera, environment, or analytic-light parameters were incomplete") &&
           expect(shader.fragment_source.find("textureSampleLevel") != std::string::npos,
                  "WGSL preview did not select the prefiltered radiance mip explicitly");
}

bool fallback_lighting_compiles_for_every_target() {
    for (const ShaderTarget target : supported_shader_targets()) {
        const PreviewEmission emitted = emit_lit_preview(request(target, false));
        const auto& bindings = emitted.pass_plan.passes().front().texture_bindings;
        if (!expect(emitted.shader.target == target && emitted.fallback_lighting &&
                        std::ranges::none_of(bindings,
                                             [](const TextureBinding& binding) {
                                                 return binding.role.find("environment") !=
                                                            std::string::npos ||
                                                        binding.role.find("BRDF") !=
                                                            std::string::npos;
                                             }),
                    "fallback preview required an environment binding or used the wrong target")) {
            return false;
        }
    }
    return true;
}

bool cube_environment_lowering_matches_each_text_target() {
    const PreviewEmission wgsl_emission = emit_lit_preview(request(ShaderTarget::wgsl));
    const PreviewEmission msl_emission = emit_lit_preview(request(ShaderTarget::msl));
    const PreviewEmission hlsl_emission = emit_lit_preview(request(ShaderTarget::hlsl));
    const auto& wgsl = std::get<SplitTextShaderProgram>(wgsl_emission.shader.payload);
    const auto& msl = std::get<UnifiedTextShaderProgram>(msl_emission.shader.payload);
    const auto& hlsl = std::get<SplitTextShaderProgram>(hlsl_emission.shader.payload);
    return expect(wgsl.fragment_source.find("texture_cube<f32>") != std::string::npos &&
                      msl.source.find("texturecube<float>") != std::string::npos &&
                      hlsl.fragment_source.find("TextureCube<float4>") != std::string::npos,
                  "cube environment resources were lowered as 2D textures");
}

bool every_channel_has_an_unlit_inspection_shader() {
    const PreviewEmissionRequest fixture = request(ShaderTarget::wgsl, false);
    for (const auto& [semantic, unused] : channel_definitions) {
        static_cast<void>(unused);
        const PreviewEmission emitted = emit_channel_inspection(fixture, semantic);
        const PassDescriptor& pass = emitted.pass_plan.passes().front();
        if (!expect(emitted.kind == PreviewEmissionKind::channel_inspection &&
                        emitted.inspected_channel == semantic &&
                        pass.texture_bindings.size() == 1 && pass.uniform_blocks.empty() &&
                        pass.texture_bindings.front().role ==
                            "inspection channel " + std::string(semantic),
                    "channel inspection retained lighting or inspected the wrong channel")) {
            return false;
        }
    }
    for (const ShaderTarget target : supported_shader_targets()) {
        if (!expect(
                emit_channel_inspection(request(target, false), "pbr.roughness").shader.target ==
                    target,
                "roughness inspection did not compile for every target")) {
            return false;
        }
    }
    return true;
}

bool invalid_lighting_contracts_are_refused() {
    PreviewEmissionRequest invalid_environment = request();
    invalid_environment.environment->radiance.format = TextureFormat::rgba8_unorm;
    bool environment_refused = false;
    try {
        static_cast<void>(emit_lit_preview(invalid_environment));
    } catch (const PreviewEmissionError& error) {
        environment_refused =
            std::string_view(error.what()).find("RGBA16-float cube") != std::string_view::npos;
    }

    PreviewEmissionRequest insufficient_budget = request();
    insufficient_budget.features.binding_budget = 12;
    bool budget_refused = false;
    try {
        static_cast<void>(emit_lit_preview(insufficient_budget));
    } catch (const PreviewEmissionError& error) {
        budget_refused =
            std::string_view(error.what()).find("device budget is 12") != std::string_view::npos;
    }
    return expect(environment_refused && budget_refused,
                  "invalid environment format or binding budget was accepted");
}

bool missing_float_filtering_uses_a_reported_workaround() {
    PreviewEmissionRequest fixture = request();
    fixture.features.floating_point_filtering = false;
    const PreviewEmission lit = emit_lit_preview(fixture);
    const PreviewEmission inspection = emit_channel_inspection(fixture, "pbr.roughness");
    return expect(lit.workarounds ==
                          std::vector<CapabilityWorkaround>{
                              {"nearest_float_sampling",
                               "linear floating-point texture filtering is unavailable; preview "
                               "samplers use nearest filtering"}} &&
                      lit.pass_plan.passes().front().sampler_bindings.front().min_filter ==
                          FilterMode::nearest,
                  "float preview textures retained linear filtering without a workaround") &&
           expect(inspection.workarounds.empty() &&
                      inspection.pass_plan.passes().front().sampler_bindings.front().min_filter ==
                          FilterMode::linear,
                  "unorm channel inspection inherited an unused environment workaround");
}

bool identical_preview_emission_is_deterministic() {
    const PreviewEmissionRequest fixture = request();
    return expect(emit_lit_preview(fixture) == emit_lit_preview(fixture),
                  "identical preview requests changed shader source or pass plan");
}

bool preview_reconstructs_bitangent_from_per_corner_handedness() {
    const PreviewEmission emitted = emit_lit_preview(request(ShaderTarget::wgsl));
    const auto& shader = std::get<SplitTextShaderProgram>(emitted.shader.payload);
    return expect(
        shader.fragment_source.find(".tangent.w") != std::string::npos &&
            shader.fragment_source.find("cross(") != std::string::npos,
        "preview normal reconstruction ignored the per-corner mirrored-UV handedness sign");
}

bool preview_results_are_cached_by_mode_and_content() {
    PreviewEmissionCache cache;
    const PreviewEmissionRequest fixture = request();
    const CachedPreviewEmission first = cache.emit_lit(fixture);
    const CachedPreviewEmission second = cache.emit_lit(fixture);
    const CachedPreviewEmission inspection = cache.emit_inspection(fixture, "pbr.roughness");
    return expect(!first.cache_hit && second.cache_hit && !inspection.cache_hit &&
                      first.emission == second.emission && *first.emission == *second.emission,
                  "preview cache did not reuse the identical lit result or isolate inspection") &&
           expect(cache.statistics() == EmissionCacheStatistics{2, 1, 2},
                  "preview cache hit, miss, or entry accounting is incorrect");
}

bool write_binary(std::string_view path, std::span<const std::uint32_t> words) {
    std::ofstream stream(std::string(path), std::ios::binary);
    stream.write(reinterpret_cast<const char*>(words.data()),
                 static_cast<std::streamsize>(words.size_bytes()));
    return stream.good();
}

bool write_spirv(std::string_view vertex_path, std::string_view fragment_path) {
    const PreviewEmission emitted = emit_lit_preview(request(ShaderTarget::spirv));
    const auto& shader = std::get<SpirvShaderProgram>(emitted.shader.payload);
    return expect(write_binary(vertex_path, shader.vertex_module) &&
                      write_binary(fragment_path, shader.fragment_module),
                  "could not write preview SPIR-V validation fixtures");
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
    const PreviewEmission lit_emission = emit_lit_preview(request());
    const PreviewEmission inspection_emission =
        emit_channel_inspection(request(ShaderTarget::wgsl, false), "pbr.roughness");
    const auto& lit = std::get<SplitTextShaderProgram>(lit_emission.shader.payload);
    const auto& inspection = std::get<SplitTextShaderProgram>(inspection_emission.shader.payload);
    const std::filesystem::path root(directory);
    return expect(write_text(root / "preview.vert.wgsl", lit.vertex_source) &&
                      write_text(root / "preview.frag.wgsl", lit.fragment_source) &&
                      write_text(root / "inspect-roughness.vert.wgsl", inspection.vertex_source) &&
                      write_text(root / "inspect-roughness.frag.wgsl", inspection.fragment_source),
                  "could not write preview determinism artifacts");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 4 && std::string_view(argv[1]) == "--write-spirv") {
        return write_spirv(argv[2], argv[3]) ? 0 : 1;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--write-determinism") {
        return write_determinism_artifacts() ? 0 : 1;
    }
    return environment_contract_is_complete() && fallback_lighting_compiles_for_every_target() &&
                   cube_environment_lowering_matches_each_text_target() &&
                   every_channel_has_an_unlit_inspection_shader() &&
                   invalid_lighting_contracts_are_refused() &&
                   missing_float_filtering_uses_a_reported_workaround() &&
                   identical_preview_emission_is_deterministic() &&
                   preview_reconstructs_bitangent_from_per_corner_handedness() &&
                   preview_results_are_cached_by_mode_and_content()
               ? 0
               : 1;
}
