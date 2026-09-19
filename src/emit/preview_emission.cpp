#include <algorithm>
#include <array>
#include <ctex/emit/preview_emission.hpp>
#include <map>
#include <set>
#include <span>
#include <sstream>
#include <utility>

namespace ctex::emit {
namespace {

using ResourceKey = std::pair<std::string, std::uint64_t>;

constexpr std::array<std::string_view, 8> lit_semantics{
    "pbr.base_color", "pbr.opacity",   "pbr.roughness", "pbr.metallic",
    "pbr.normal",     "pbr.occlusion", "pbr.emission",  "pbr.subsurface"};

struct PreviewInputs {
    std::vector<const PreviewChannelInput*> channels;
    std::map<std::string_view, std::size_t, std::less<>> binding_by_semantic;
};

TextureSubresourceRange whole_texture(const LogicalTexture& texture) {
    return {.first_mip = 0,
            .mip_count = texture.mip_levels,
            .first_layer = 0,
            .layer_count = texture.extent.layers,
            .tiles = std::nullopt};
}

bool supports_format(const DeviceFeatureSet& features, TextureFormat format) {
    return std::ranges::find(features.supported_texture_formats, format) !=
           features.supported_texture_formats.end();
}

bool is_floating_point(TextureFormat format) {
    switch (format) {
        case TextureFormat::r16_float:
        case TextureFormat::rg16_float:
        case TextureFormat::rgba16_float:
        case TextureFormat::r32_float:
        case TextureFormat::rg32_float:
        case TextureFormat::rgba32_float:
        case TextureFormat::depth32_float:
            return true;
        case TextureFormat::r8_unorm:
        case TextureFormat::rg8_unorm:
        case TextureFormat::rgba8_unorm:
        case TextureFormat::r16_unorm:
        case TextureFormat::rg16_unorm:
        case TextureFormat::rgba16_unorm:
            return false;
    }
    return false;
}

void validate_texture(const LogicalTexture& texture, const DeviceFeatureSet& features,
                      std::string_view role, std::uint32_t required_layers) {
    if (texture.version.logical_id.empty() || texture.role.empty() || texture.extent.width == 0 ||
        texture.extent.height == 0 || texture.extent.layers != required_layers ||
        texture.mip_levels == 0 || texture.tile_shape.width == 0 ||
        texture.tile_shape.height == 0) {
        throw PreviewEmissionError(std::string(role) + " texture declaration is incomplete");
    }
    if (texture.extent.width > features.maximum_texture_dimension ||
        texture.extent.height > features.maximum_texture_dimension) {
        throw PreviewEmissionError(std::string(role) + " texture " + texture.version.logical_id +
                                   " exceeds the maximum texture dimension");
    }
    if (!supports_format(features, texture.format) ||
        texture.format == TextureFormat::depth32_float) {
        throw PreviewEmissionError(std::string(role) + " texture " + texture.version.logical_id +
                                   " uses an unsupported preview format");
    }
}

void validate_environment(const PreviewEnvironmentInput& environment,
                          const DeviceFeatureSet& features) {
    validate_texture(environment.radiance, features, "environment radiance", 6);
    validate_texture(environment.diffuse_irradiance, features, "diffuse irradiance", 6);
    validate_texture(environment.specular_brdf_lookup, features, "specular BRDF lookup", 1);
    if (environment.radiance.extent.width != environment.radiance.extent.height ||
        environment.radiance.format != TextureFormat::rgba16_float) {
        throw PreviewEmissionError(
            "environment radiance must be a square six-face RGBA16-float cube");
    }
    if (environment.diffuse_irradiance.extent.width !=
            environment.diffuse_irradiance.extent.height ||
        environment.diffuse_irradiance.format != TextureFormat::rgba16_float ||
        environment.diffuse_irradiance.mip_levels != 1) {
        throw PreviewEmissionError(
            "diffuse irradiance must be a single-mip square six-face RGBA16-float cube");
    }
    if (environment.specular_brdf_lookup.format != TextureFormat::rg16_float ||
        environment.specular_brdf_lookup.mip_levels != 1) {
        throw PreviewEmissionError("specular BRDF lookup must be a single-mip RG16-float texture");
    }
}

void validate_request(const PreviewEmissionRequest& request) {
    if (request.stable_identity.empty() || request.features.binding_budget < 2 ||
        request.features.maximum_texture_dimension == 0 ||
        request.features.supported_texture_formats.empty() || request.vertex_count == 0) {
        throw PreviewEmissionError(
            "preview emission requires an identity, vertices, and a usable device feature set");
    }
    if (request.analytic_light_count > maximum_preview_analytic_lights) {
        throw PreviewEmissionError("preview emission supports at most four analytic lights");
    }
    validate_texture(request.output, request.features, "preview output", 1);
    std::set<std::string, std::less<>> semantics;
    for (const PreviewChannelInput& channel : request.channels) {
        if (channel.semantic_id.empty() || channel.component_count == 0 ||
            channel.component_count > 4 || !semantics.insert(channel.semantic_id).second) {
            throw PreviewEmissionError(
                "preview channels require unique semantics and one to four components");
        }
        validate_texture(channel.texture, request.features, "preview channel", 1);
        if (!channel.texture.externally_initialized) {
            throw PreviewEmissionError("preview channel " + channel.semantic_id +
                                       " has no externally initialized contents");
        }
    }
    if (request.environment) {
        validate_environment(*request.environment, request.features);
        if (!request.environment->radiance.externally_initialized ||
            !request.environment->diffuse_irradiance.externally_initialized ||
            !request.environment->specular_brdf_lookup.externally_initialized) {
            throw PreviewEmissionError("environment lighting resources are not initialized");
        }
    }
}

const PreviewChannelInput& require_channel(const PreviewEmissionRequest& request,
                                           std::string_view semantic_id) {
    const auto found =
        std::ranges::find(request.channels, semantic_id, &PreviewChannelInput::semantic_id);
    if (found == request.channels.end()) {
        throw PreviewEmissionError("inspection channel is unavailable: " +
                                   std::string(semantic_id));
    }
    return *found;
}

PreviewInputs lit_inputs(const PreviewEmissionRequest& request) {
    PreviewInputs result;
    for (std::string_view semantic : lit_semantics) {
        const auto found =
            std::ranges::find(request.channels, semantic, &PreviewChannelInput::semantic_id);
        if (found != request.channels.end()) {
            result.binding_by_semantic.emplace(semantic, result.channels.size());
            result.channels.push_back(&*found);
        }
    }
    return result;
}

std::string channel_sample(const PreviewInputs& inputs, std::string_view semantic,
                           std::string_view components, std::string_view fallback) {
    const auto found = inputs.binding_by_semantic.find(semantic);
    if (found == inputs.binding_by_semantic.end()) {
        return std::string(fallback);
    }
    return "sample(ctex_channel_" + std::to_string(found->second) + ", ctex_sampler, input.uv)." +
           std::string(components);
}

void write_common_shading_functions(std::ostringstream& source) {
    source << R"(
fun ctex_saturate(value: float): float {
    return clamp(value, 0.0, 1.0);
}

fun ctex_distribution_ggx(n_dot_h: float, roughness: float): float {
    var alpha: float = roughness * roughness;
    var alpha_squared: float = alpha * alpha;
    var denominator: float = n_dot_h * n_dot_h * (alpha_squared - 1.0) + 1.0;
    return alpha_squared / (3.141592653589793 * denominator * denominator);
}

fun ctex_geometry_schlick(n_dot_x: float, roughness: float): float {
    var radius: float = roughness + 1.0;
    var k: float = radius * radius * 0.125;
    return n_dot_x / (n_dot_x * (1.0 - k) + k);
}

fun ctex_fresnel_schlick(cos_theta: float, f0: float3): float3 {
    var factor: float = pow(1.0 - ctex_saturate(cos_theta), 5.0);
    return f0 + (float3(1.0, 1.0, 1.0) - f0) * factor;
}

fun ctex_rotate_environment(direction: float3, angle: float): float3 {
    var sine: float = sin(angle);
    var cosine: float = cos(angle);
    return float3(cosine * direction.x + sine * direction.z, direction.y,
                  cosine * direction.z - sine * direction.x);
}
)";
}

void write_lit_vertex_source(std::ostringstream& source) {
    source << R"(
struct ctex_vertex_in {
    clip_position: float4;
    world_position: float3;
    normal: float3;
    tangent: float4;
    uv: float2;
}

struct ctex_vertex_out {
    pos: float4;
    world_position: float3;
    normal: float3;
    tangent: float4;
    uv: float2;
}

fun ctex_vertex(input: ctex_vertex_in): ctex_vertex_out {
    var output: ctex_vertex_out;
    output.pos = input.clip_position;
    output.world_position = input.world_position;
    output.normal = input.normal;
    output.tangent = input.tangent;
    output.uv = input.uv;
    return output;
}
)";
}

std::string make_lit_source(const PreviewEmissionRequest& request, const PreviewInputs& inputs) {
    std::ostringstream source;
    write_lit_vertex_source(source);
    write_common_shading_functions(source);
    for (std::size_t index = 0; index < inputs.channels.size(); ++index) {
        source << "#[set(material)]\nconst ctex_channel_" << index << ": tex2d;\n\n";
    }
    if (request.environment) {
        source << "#[set(material)]\nconst ctex_environment_radiance: texcube;\n\n"
                  "#[set(material)]\nconst ctex_diffuse_irradiance: texcube;\n\n"
                  "#[set(material)]\nconst ctex_specular_brdf: tex2d;\n\n";
    }
    source << "#[set(material)]\nconst ctex_sampler: sampler;\n\n"
              "#[set(material)]\nconst ctex_parameters: {\n"
              "    camera_position: float4;\n"
              "    environment_rotation_intensity: float4;\n";
    for (std::size_t index = 0; index < request.analytic_light_count; ++index) {
        source << "    light_" << index << "_direction_intensity: float4;\n"
               << "    light_" << index << "_color: float4;\n";
    }
    source << "};\n\n"
              "fun ctex_fragment(input: ctex_vertex_out): float4 {\n"
           << "    var base_color: float3 = "
           << channel_sample(inputs, "pbr.base_color", "xyz", "float3(0.5, 0.5, 0.5)")
           << ";\n    var opacity: float = " << channel_sample(inputs, "pbr.opacity", "x", "1.0")
           << ";\n    var roughness: float = ctex_saturate("
           << channel_sample(inputs, "pbr.roughness", "x", "0.5")
           << ");\n    roughness = max(roughness, 0.045);\n    var metallic: float = ctex_saturate("
           << channel_sample(inputs, "pbr.metallic", "x", "0.0")
           << ");\n    var normal_sample: float3 = "
           << channel_sample(inputs, "pbr.normal", "xyz", "float3(0.5, 0.5, 1.0)")
           << " * 2.0 - float3(1.0, 1.0, 1.0);\n"
              "    var occlusion: float = ctex_saturate("
           << channel_sample(inputs, "pbr.occlusion", "x", "1.0")
           << ");\n    var emission: float3 = "
           << channel_sample(inputs, "pbr.emission", "xyz", "float3(0.0, 0.0, 0.0)")
           << ";\n    var subsurface: float = ctex_saturate("
           << channel_sample(inputs, "pbr.subsurface", "x", "0.0")
           << ");\n"
              "    var geometric_normal: float3 = normalize(input.normal);\n"
              "    var tangent: float3 = normalize(input.tangent.xyz);\n"
              "    var bitangent: float3 = normalize(cross(geometric_normal, tangent)) * "
              "input.tangent.w;\n"
              "    var normal: float3 = normalize(tangent * normal_sample.x + bitangent * "
              "normal_sample.y + geometric_normal * normal_sample.z);\n"
              "    var view: float3 = normalize(ctex_parameters.camera_position.xyz - "
              "input.world_position);\n"
              "    var n_dot_v: float = max(dot(normal, view), 0.0001);\n"
              "    var dielectric_f0: float3 = float3(0.04, 0.04, 0.04);\n"
              "    var f0: float3 = dielectric_f0 + (base_color - dielectric_f0) * metallic;\n"
              "    var direct: float3 = float3(0.0, 0.0, 0.0);\n";
    for (std::size_t index = 0; index < request.analytic_light_count; ++index) {
        source << "    var light_" << index << ": float3 = normalize(ctex_parameters.light_"
               << index << "_direction_intensity.xyz);\n"
               << "    var half_" << index << ": float3 = normalize(view + light_" << index
               << ");\n"
               << "    var n_dot_l_" << index << ": float = ctex_saturate(dot(normal, light_"
               << index << "));\n"
               << "    var n_dot_h_" << index << ": float = ctex_saturate(dot(normal, half_"
               << index << "));\n"
               << "    var h_dot_v_" << index << ": float = ctex_saturate(dot(half_" << index
               << ", view));\n"
               << "    var fresnel_" << index << ": float3 = ctex_fresnel_schlick(h_dot_v_" << index
               << ", f0);\n"
               << "    var distribution_" << index << ": float = ctex_distribution_ggx(n_dot_h_"
               << index << ", roughness);\n"
               << "    var geometry_" << index
               << ": float = ctex_geometry_schlick(n_dot_v, roughness) * "
                  "ctex_geometry_schlick(n_dot_l_"
               << index << ", roughness);\n"
               << "    var specular_" << index << ": float3 = fresnel_" << index
               << " * (distribution_" << index << " * geometry_" << index
               << " / max(4.0 * n_dot_v * n_dot_l_" << index << ", 0.0001));\n"
               << "    var diffuse_" << index << ": float3 = (float3(1.0, 1.0, 1.0) - fresnel_"
               << index << ") * (1.0 - metallic) * base_color * 0.3183098861837907;\n"
               << "    var wrapped_" << index << ": float = ctex_saturate((n_dot_l_" << index
               << " + subsurface) / (1.0 + subsurface));\n"
               << "    var radiance_" << index << ": float3 = ctex_parameters.light_" << index
               << "_color.xyz * ctex_parameters.light_" << index << "_direction_intensity.w;\n"
               << "    direct = direct + (diffuse_" << index << " * wrapped_" << index
               << " + specular_" << index << " * n_dot_l_" << index << ") * radiance_" << index
               << ";\n";
    }
    source << "    var environment: float3;\n";
    if (request.environment) {
        source
            << "    var rotation: float = ctex_parameters.environment_rotation_intensity.x;\n"
               "    var intensity: float = ctex_parameters.environment_rotation_intensity.y;\n"
               "    var environment_normal: float3 = ctex_rotate_environment(normal, rotation);\n"
               "    var reflection: float3 = reflect(view * -1.0, normal);\n"
               "    var environment_reflection: float3 = ctex_rotate_environment(reflection, "
               "rotation);\n"
               "    var irradiance: float3 = sample(ctex_diffuse_irradiance, ctex_sampler, "
               "environment_normal).xyz;\n"
               "    var diffuse_environment: float3 = irradiance * base_color * (1.0 - "
               "metallic) * 0.3183098861837907;\n"
               "    var radiance_lod: float = roughness * "
            << (request.environment->radiance.mip_levels - 1U)
            << ".0;\n"
               "    var reflected_radiance: float3 = sample_lod(ctex_environment_radiance, "
               "ctex_sampler, environment_reflection, radiance_lod).xyz;\n"
               "    var brdf: float2 = sample(ctex_specular_brdf, ctex_sampler, "
               "float2(n_dot_v, roughness)).xy;\n"
               "    var specular_environment: float3 = reflected_radiance * (f0 * brdf.x + "
               "float3(brdf.y, brdf.y, brdf.y));\n"
               "    environment = (diffuse_environment + specular_environment) * intensity;\n";
    } else {
        source << "    var intensity: float = ctex_parameters.environment_rotation_intensity.y;\n"
                  "    var hemisphere: float = ctex_saturate(normal.y * 0.5 + 0.5);\n"
                  "    var ground: float3 = float3(0.025, 0.025, 0.03);\n"
                  "    var sky: float3 = float3(0.12, 0.16, 0.24);\n"
                  "    var fallback_irradiance: float3 = ground + (sky - ground) * hemisphere;\n"
                  "    var diffuse_environment: float3 = fallback_irradiance * base_color * "
                  "(1.0 - metallic);\n"
                  "    var fallback_fresnel: float3 = ctex_fresnel_schlick(n_dot_v, f0);\n"
                  "    var specular_environment: float3 = sky * fallback_fresnel * (1.0 - "
                  "roughness * 0.75);\n"
                  "    environment = (diffuse_environment + specular_environment) * intensity;\n";
    }
    source << "    var colour: float3 = direct + environment * occlusion + emission;\n"
              "    return float4(colour, opacity);\n"
              "}\n\n#[pipe]\nstruct ctex_preview_pipe {\n"
              "    vertex = ctex_vertex;\n"
              "    fragment = ctex_fragment;\n"
              "}\n";
    return source.str();
}

std::string make_inspection_source(std::uint8_t component_count) {
    std::ostringstream source;
    source << R"(
struct ctex_vertex_in {
    clip_position: float4;
    world_position: float3;
    normal: float3;
    tangent: float4;
    uv: float2;
}

struct ctex_vertex_out {
    pos: float4;
    uv: float2;
}

fun ctex_vertex(input: ctex_vertex_in): ctex_vertex_out {
    var output: ctex_vertex_out;
    output.pos = input.clip_position;
    output.uv = input.uv;
    return output;
}

#[set(material)]
const ctex_channel: tex2d;

#[set(material)]
const ctex_sampler: sampler;

fun ctex_fragment(input: ctex_vertex_out): float4 {
    var value: float4 = sample(ctex_channel, ctex_sampler, input.uv);
)";
    if (component_count == 1) {
        source << "    return float4(value.x, value.x, value.x, 1.0);\n";
    } else if (component_count == 2) {
        source << "    return float4(value.x, value.y, 0.0, 1.0);\n";
    } else if (component_count == 3) {
        source << "    return float4(value.xyz, 1.0);\n";
    } else {
        source << "    return value;\n";
    }
    source << "}\n\n#[pipe]\nstruct ctex_inspection_pipe {\n"
              "    vertex = ctex_vertex;\n"
              "    fragment = ctex_fragment;\n"
              "}\n";
    return source.str();
}

std::pair<std::string, std::string> exported_entry_points(ShaderTarget target) {
    return target == ShaderTarget::msl ? std::pair{"ctex_vertex", "ctex_fragment"}
                                       : std::pair{"main", "main"};
}

VertexBufferLayout preview_vertex_layout() {
    return {.slot = 0,
            .stride = 64,
            .step_mode = VertexStepMode::vertex,
            .attributes = {{"clip_position", 0, VertexFormat::float32x4, 0},
                           {"world_position", 1, VertexFormat::float32x3, 16},
                           {"normal", 2, VertexFormat::float32x3, 28},
                           {"tangent", 3, VertexFormat::float32x4, 40},
                           {"uv", 4, VertexFormat::float32x2, 56}}};
}

UniformBlockLayout preview_uniforms(std::uint32_t binding, std::size_t light_count) {
    UniformBlockLayout block{
        .group = 0,
        .binding = binding,
        .role = "preview camera, environment, and analytic lights",
        .visibility = ShaderVisibility::fragment,
        .size = static_cast<std::uint32_t>((2 + light_count * 2) * 16),
        .fields = {{"camera_position", 0, 16, 16}, {"environment_rotation_intensity", 16, 16, 16}}};
    for (std::size_t index = 0; index < light_count; ++index) {
        const std::uint32_t offset = static_cast<std::uint32_t>((2 + index * 2) * 16);
        block.fields.push_back(
            {"light_" + std::to_string(index) + "_direction_intensity", offset, 16, 16});
        block.fields.push_back({"light_" + std::to_string(index) + "_color", offset + 16, 16, 16});
    }
    return block;
}

void add_resource(std::vector<LogicalTexture>& resources, std::map<ResourceKey, std::size_t>& seen,
                  const LogicalTexture& texture) {
    const ResourceKey key{texture.version.logical_id, texture.version.generation};
    const auto [position, inserted] = seen.emplace(key, resources.size());
    if (inserted) {
        resources.push_back(texture);
    } else if (resources[position->second] != texture) {
        throw PreviewEmissionError("preview resource " + texture.version.logical_id +
                                   " has conflicting declarations");
    }
}

TextureBinding sampled_binding(std::uint32_t binding, std::string role,
                               const LogicalTexture& texture, TextureViewDimension dimension,
                               std::string encoding, std::string mip_convention) {
    return {.group = 0,
            .binding = binding,
            .role = std::move(role),
            .visibility = ShaderVisibility::fragment,
            .kind = TextureBindingKind::sampled,
            .resource = texture.version,
            .subresources = whole_texture(texture),
            .view_dimension = dimension,
            .encoding = std::move(encoding),
            .mip_convention = std::move(mip_convention)};
}

void add_sampled_input(PassDescriptor& pass, const LogicalTexture& texture,
                       TextureBinding binding) {
    pass.accesses.push_back({texture.version, whole_texture(texture), ResourceAccessMode::read,
                             LoadAction::load, StoreAction::not_applicable});
    pass.texture_bindings.push_back(std::move(binding));
}

PassDescriptor base_pass(const PreviewEmissionRequest& request, std::string identifier) {
    const auto [vertex_entry, fragment_entry] = exported_entry_points(request.target);
    return {.identifier = std::move(identifier),
            .kind = PassKind::render,
            .dependencies = {},
            .vertex_entry_point = vertex_entry,
            .fragment_entry_point = fragment_entry,
            .compute_entry_point = {},
            .accesses = {{request.output.version, whole_texture(request.output),
                          ResourceAccessMode::write, LoadAction::clear, StoreAction::store}},
            .texture_bindings = {},
            .sampler_bindings = {},
            .uniform_blocks = {},
            .vertex_buffers = {preview_vertex_layout()},
            .render_targets = {{.role = "preview colour output",
                                .resource = request.output.version,
                                .subresources = whole_texture(request.output),
                                .format = request.output.format,
                                .width = request.output.extent.width,
                                .height = request.output.extent.height,
                                .load = LoadAction::clear,
                                .store = StoreAction::store,
                                .clear_colour = {},
                                .blend = {}}},
            .depth_target = std::nullopt,
            .depth_state = {},
            .command = DrawCommand{.topology = PrimitiveTopology::triangle_list,
                                   .indexed = false,
                                   .vertex_count = request.vertex_count,
                                   .index_count = 0,
                                   .instance_count = 1}};
}

SamplerBinding preview_sampler(std::uint32_t binding, FilterMode filter) {
    return {.group = 0,
            .binding = binding,
            .role = "preview linear sampler",
            .visibility = ShaderVisibility::fragment,
            .min_filter = filter,
            .mag_filter = filter,
            .mip_filter = filter,
            .address_u = AddressMode::repeat,
            .address_v = AddressMode::repeat,
            .address_w = AddressMode::repeat};
}

FilterMode preview_filter(const PreviewEmissionRequest& request,
                          std::span<const PreviewChannelInput* const> channels,
                          bool samples_environment,
                          std::vector<CapabilityWorkaround>& workarounds) {
    bool samples_float = std::ranges::any_of(channels, [](const PreviewChannelInput* channel) {
        return is_floating_point(channel->texture.format);
    });
    if (samples_environment) {
        samples_float = true;
    }
    if (request.features.floating_point_filtering || !samples_float) {
        return FilterMode::linear;
    }
    workarounds.push_back({"nearest_float_sampling",
                           "linear floating-point texture filtering is unavailable; preview "
                           "samplers use nearest filtering"});
    return FilterMode::nearest;
}

}  // namespace

PreviewEmission emit_lit_preview(const PreviewEmissionRequest& request) {
    validate_request(request);
    const PreviewInputs inputs = lit_inputs(request);
    const std::size_t texture_count = inputs.channels.size() + (request.environment ? 3U : 0U);
    const std::size_t binding_count = texture_count + 2U;
    if (binding_count > request.features.binding_budget) {
        throw PreviewEmissionError("lit preview requires " + std::to_string(binding_count) +
                                   " bindings but the device budget is " +
                                   std::to_string(request.features.binding_budget));
    }

    std::vector<LogicalTexture> resources;
    std::map<ResourceKey, std::size_t> seen;
    PassDescriptor pass = base_pass(request, "preview-lit");
    std::uint32_t binding = 0;
    for (const PreviewChannelInput* channel : inputs.channels) {
        add_resource(resources, seen, channel->texture);
        const bool colour =
            channel->semantic_id == "pbr.base_color" || channel->semantic_id == "pbr.emission";
        add_sampled_input(pass, channel->texture,
                          sampled_binding(binding++, "material channel " + channel->semantic_id,
                                          channel->texture, TextureViewDimension::d2,
                                          colour ? "linear Rec. 709 RGB" : "linear channel data",
                                          "base mip with implicit derivatives"));
    }
    if (request.environment) {
        const PreviewEnvironmentInput& environment = *request.environment;
        add_resource(resources, seen, environment.radiance);
        add_sampled_input(
            pass, environment.radiance,
            sampled_binding(binding++, "prefiltered environment radiance", environment.radiance,
                            TextureViewDimension::cube, "linear Rec. 709 radiance RGB",
                            "mip 0 is sharp; increasing mips encode GGX roughness, selected as "
                            "roughness * (mip_count - 1)"));
        add_resource(resources, seen, environment.diffuse_irradiance);
        add_sampled_input(
            pass, environment.diffuse_irradiance,
            sampled_binding(binding++, "diffuse environment irradiance",
                            environment.diffuse_irradiance, TextureViewDimension::cube,
                            "linear Rec. 709 irradiance RGB; cosine integral is not divided by pi",
                            "single mip"));
        add_resource(resources, seen, environment.specular_brdf_lookup);
        add_sampled_input(
            pass, environment.specular_brdf_lookup,
            sampled_binding(binding++, "split-sum specular BRDF lookup",
                            environment.specular_brdf_lookup, TextureViewDimension::d2,
                            "linear RG: Fresnel scale in R and bias in G",
                            "single mip; coordinates are NdotV and perceptual roughness"));
    }
    std::vector<CapabilityWorkaround> workarounds;
    pass.sampler_bindings.push_back(preview_sampler(
        binding++,
        preview_filter(request, inputs.channels, request.environment.has_value(), workarounds)));
    pass.uniform_blocks.push_back(preview_uniforms(binding, request.analytic_light_count));
    add_resource(resources, seen, request.output);

    KongContext compiler;
    KongShaderProgram shader = compiler.compile(make_lit_source(request, inputs), request.target);
    return {PassPlan(request.stable_identity + "/lit", std::move(resources), {std::move(pass)}),
            std::move(shader),
            std::move(workarounds),
            PreviewEmissionKind::lit,
            !request.environment,
            {}};
}

PreviewEmission emit_channel_inspection(const PreviewEmissionRequest& request,
                                        std::string_view semantic_id) {
    validate_request(request);
    const PreviewChannelInput& channel = require_channel(request, semantic_id);
    PassDescriptor pass = base_pass(request, "preview-inspect-" + std::string(semantic_id));
    add_sampled_input(
        pass, channel.texture,
        sampled_binding(0, "inspection channel " + channel.semantic_id, channel.texture,
                        TextureViewDimension::d2, "raw linear channel values",
                        "base mip with implicit derivatives"));
    std::vector<CapabilityWorkaround> workarounds;
    const std::array selected{&channel};
    pass.sampler_bindings.push_back(
        preview_sampler(1, preview_filter(request, selected, false, workarounds)));
    std::vector resources{channel.texture, request.output};
    if (channel.texture.version == request.output.version) {
        throw PreviewEmissionError("inspection channel aliases the preview output");
    }

    KongContext compiler;
    KongShaderProgram shader =
        compiler.compile(make_inspection_source(channel.component_count), request.target);
    return {PassPlan(request.stable_identity + "/inspect/" + std::string(semantic_id),
                     std::move(resources), {std::move(pass)}),
            std::move(shader),
            std::move(workarounds),
            PreviewEmissionKind::channel_inspection,
            false,
            std::string(semantic_id)};
}

}  // namespace ctex::emit
