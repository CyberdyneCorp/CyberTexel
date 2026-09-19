#include <algorithm>
#include <ctex/emit/feature_emission.hpp>
#include <map>
#include <sstream>
#include <utility>

namespace ctex::emit {
namespace {

using ResourceKey = std::pair<std::string, std::uint64_t>;

struct PassSlice {
    std::size_t first_layer{};
    std::size_t layer_count{};
    bool reads_carried_result{};
};

struct PlannedResources {
    std::vector<LogicalTexture> declarations;
    std::vector<ResourceVersion> intermediates;
};

TextureSubresourceRange whole_texture() {
    return {
        .first_mip = 0, .mip_count = 1, .first_layer = 0, .layer_count = 1, .tiles = std::nullopt};
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

bool supports_format(const DeviceFeatureSet& features, TextureFormat format) {
    return std::ranges::find(features.supported_texture_formats, format) !=
           features.supported_texture_formats.end();
}

void validate_texture(const LogicalTexture& texture, const DeviceFeatureSet& features,
                      std::string_view role) {
    if (texture.version.logical_id.empty() || texture.role.empty() || texture.extent.width == 0 ||
        texture.extent.height == 0 || texture.extent.layers != 1 || texture.mip_levels == 0 ||
        texture.tile_shape.width == 0 || texture.tile_shape.height == 0) {
        throw FeatureEmissionError(std::string(role) + " texture declaration is incomplete");
    }
    if (texture.extent.width > features.maximum_texture_dimension ||
        texture.extent.height > features.maximum_texture_dimension) {
        throw FeatureEmissionError(std::string(role) + " texture " + texture.version.logical_id +
                                   " exceeds maximum texture dimension " +
                                   std::to_string(features.maximum_texture_dimension));
    }
    if (!supports_format(features, texture.format)) {
        throw FeatureEmissionError(std::string(role) + " texture " + texture.version.logical_id +
                                   " uses an unsupported texture format");
    }
    if (texture.format == TextureFormat::depth32_float) {
        throw FeatureEmissionError(std::string(role) + " texture " + texture.version.logical_id +
                                   " uses a depth format for colour compositing");
    }
}

void validate_request(const LayerStackEmissionRequest& request) {
    if (request.stable_identity.empty() || request.layers.empty()) {
        throw FeatureEmissionError(
            "layer-stack emission requires an identity and at least one layer");
    }
    if (request.features.binding_budget < 2 || request.features.maximum_texture_dimension == 0 ||
        request.features.supported_texture_formats.empty()) {
        throw FeatureEmissionError(
            "device feature set has no usable binding, dimension, or format budget");
    }
    validate_texture(request.output, request.features, "output");
    std::map<std::string, bool> identifiers;
    for (const LayerStackInput& layer : request.layers) {
        if (layer.identifier.empty() || !identifiers.emplace(layer.identifier, true).second) {
            throw FeatureEmissionError("layer identifiers must be non-empty and unique");
        }
        validate_texture(layer.texture, request.features, "layer");
        if (!layer.texture.externally_initialized) {
            throw FeatureEmissionError("layer " + layer.identifier +
                                       " has no externally initialized contents");
        }
        if (layer.texture.extent.width != request.output.extent.width ||
            layer.texture.extent.height != request.output.extent.height) {
            throw FeatureEmissionError("layer " + layer.identifier +
                                       " dimensions differ from the output texture");
        }
        if (layer.texture.version == request.output.version) {
            throw FeatureEmissionError("layer " + layer.identifier +
                                       " aliases the output resource generation");
        }
    }
}

std::vector<PassSlice> split_layers(std::size_t layer_count, std::uint32_t binding_budget) {
    const std::size_t first_capacity = binding_budget - 1U;
    std::vector<PassSlice> result;
    std::size_t cursor = 0;
    const std::size_t first_count = std::min(layer_count, first_capacity);
    result.push_back({0, first_count, false});
    cursor = first_count;
    if (cursor == layer_count) {
        return result;
    }
    if (binding_budget < 3) {
        throw FeatureEmissionError(
            "binding budget 2 cannot carry an intermediate and add another layer");
    }
    const std::size_t continued_capacity = binding_budget - 2U;
    while (cursor < layer_count) {
        const std::size_t count = std::min(layer_count - cursor, continued_capacity);
        result.push_back({cursor, count, true});
        cursor += count;
    }
    return result;
}

ResourceKey resource_key(const ResourceVersion& version) {
    return {version.logical_id, version.generation};
}

PlannedResources plan_resources(const LayerStackEmissionRequest& request, std::size_t pass_count) {
    PlannedResources result;
    std::map<ResourceKey, std::size_t> declared;
    for (const LayerStackInput& layer : request.layers) {
        const auto [position, inserted] =
            declared.emplace(resource_key(layer.texture.version), result.declarations.size());
        if (inserted) {
            result.declarations.push_back(layer.texture);
        } else if (result.declarations[position->second] != layer.texture) {
            throw FeatureEmissionError("resource " + layer.texture.version.logical_id +
                                       " has conflicting layer declarations");
        }
    }
    for (std::size_t index = 0; index + 1 < pass_count; ++index) {
        LogicalTexture intermediate = request.output;
        intermediate.version = {request.stable_identity + "/intermediate/" + std::to_string(index),
                                1};
        intermediate.role = "layer-stack intermediate " + std::to_string(index);
        intermediate.externally_initialized = false;
        if (!declared.emplace(resource_key(intermediate.version), result.declarations.size())
                 .second) {
            throw FeatureEmissionError("generated intermediate resource identity collides");
        }
        result.intermediates.push_back(intermediate.version);
        result.declarations.push_back(std::move(intermediate));
    }
    if (!declared.emplace(resource_key(request.output.version), result.declarations.size())
             .second) {
        throw FeatureEmissionError("output resource generation aliases an input or intermediate");
    }
    result.declarations.push_back(request.output);
    return result;
}

bool samples_float(const LayerStackEmissionRequest& request, std::size_t pass_count) {
    if (std::ranges::any_of(request.layers, [](const LayerStackInput& layer) {
            return is_floating_point(layer.texture.format);
        })) {
        return true;
    }
    return pass_count > 1 && is_floating_point(request.output.format);
}

FilterMode select_filter(const LayerStackEmissionRequest& request, std::size_t pass_count,
                         std::vector<CapabilityWorkaround>& workarounds) {
    if (request.requested_filter != FilterMode::linear ||
        request.features.floating_point_filtering || !samples_float(request, pass_count)) {
        return request.requested_filter;
    }
    workarounds.push_back({"nearest_float_sampling",
                           "linear floating-point texture filtering is unavailable; emitted "
                           "samplers use nearest filtering"});
    return FilterMode::nearest;
}

std::string pass_identifier(std::size_t index) { return "layer-stack-" + std::to_string(index); }

std::pair<std::string, std::string> exported_entry_points(ShaderTarget target) {
    if (target == ShaderTarget::msl) {
        return {"ctex_vertex", "ctex_fragment"};
    }
    return {"main", "main"};
}

std::string make_kong_source(std::size_t input_count, std::size_t pass_index) {
    std::ostringstream source;
    source << "struct ctex_vertex_in {\n"
              "    pos: float2;\n"
              "}\n\n"
              "struct ctex_vertex_out {\n"
              "    pos: float4;\n"
              "    uv: float2;\n"
              "}\n\n"
              "fun ctex_vertex(input: ctex_vertex_in): ctex_vertex_out {\n"
              "    var output: ctex_vertex_out;\n"
              "    output.pos = float4(input.pos.x, input.pos.y, 0.0, 1.0);\n"
              "    output.uv = input.pos * float2(0.5, 0.5) + float2(0.5, 0.5);\n"
              "    return output;\n"
              "}\n\n";
    for (std::size_t index = 0; index < input_count; ++index) {
        source << "#[set(material)]\nconst ctex_input_" << index << ": tex2d;\n\n";
    }
    source << "#[set(material)]\nconst ctex_sampler: sampler;\n\n"
              "fun ctex_fragment(input: ctex_vertex_out): float4 {\n"
              "    var result: float4 = sample(ctex_input_0, ctex_sampler, input.uv);\n";
    for (std::size_t index = 1; index < input_count; ++index) {
        source << "    var layer_" << index << ": float4 = sample(ctex_input_" << index
               << ", ctex_sampler, input.uv);\n"
               << "    var inverse_alpha_" << index << ": float = 1.0 - layer_" << index << ".a;\n"
               << "    result = float4(layer_" << index << ".r + result.r * inverse_alpha_" << index
               << ", layer_" << index << ".g + result.g * inverse_alpha_" << index << ", layer_"
               << index << ".b + result.b * inverse_alpha_" << index << ", layer_" << index
               << ".a + result.a * inverse_alpha_" << index << ");\n";
    }
    source << "    return result;\n"
              "}\n\n"
              "#[pipe]\nstruct ctex_layer_stack_"
           << pass_index
           << " {\n"
              "    vertex = ctex_vertex;\n"
              "    fragment = ctex_fragment;\n"
              "}\n";
    return source.str();
}

TextureBinding texture_binding(std::uint32_t binding, std::string role,
                               const ResourceVersion& resource) {
    return {.group = 0,
            .binding = binding,
            .role = std::move(role),
            .visibility = ShaderVisibility::fragment,
            .kind = TextureBindingKind::sampled,
            .resource = resource,
            .subresources = whole_texture()};
}

void add_read(PassDescriptor& pass, std::string role, const ResourceVersion& resource) {
    pass.accesses.push_back({resource, whole_texture(), ResourceAccessMode::read, LoadAction::load,
                             StoreAction::not_applicable});
    pass.texture_bindings.push_back(texture_binding(
        static_cast<std::uint32_t>(pass.texture_bindings.size()), std::move(role), resource));
}

PassDescriptor make_pass(const LayerStackEmissionRequest& request,
                         const std::vector<ResourceVersion>& intermediates, const PassSlice& slice,
                         std::size_t pass_index, std::size_t pass_count, FilterMode filter) {
    PassDescriptor pass;
    pass.identifier = pass_identifier(pass_index);
    pass.kind = PassKind::render;
    if (pass_index > 0) {
        pass.dependencies.push_back(pass_identifier(pass_index - 1));
        add_read(pass, "carried composite", intermediates[pass_index - 1]);
    }
    for (std::size_t index = slice.first_layer; index < slice.first_layer + slice.layer_count;
         ++index) {
        add_read(pass, "layer " + request.layers[index].identifier,
                 request.layers[index].texture.version);
    }
    const ResourceVersion output =
        pass_index + 1 == pass_count ? request.output.version : intermediates[pass_index];
    pass.accesses.push_back({output, whole_texture(), ResourceAccessMode::write, LoadAction::clear,
                             StoreAction::store});
    const std::uint32_t sampler_slot = static_cast<std::uint32_t>(pass.texture_bindings.size());
    pass.sampler_bindings.push_back({.group = 0,
                                     .binding = sampler_slot,
                                     .role = "layer-stack sampler",
                                     .visibility = ShaderVisibility::fragment,
                                     .min_filter = filter,
                                     .mag_filter = filter,
                                     .mip_filter = FilterMode::nearest,
                                     .address_u = AddressMode::clamp_to_edge,
                                     .address_v = AddressMode::clamp_to_edge,
                                     .address_w = AddressMode::clamp_to_edge});
    const auto [vertex_entry_point, fragment_entry_point] = exported_entry_points(request.target);
    pass.vertex_entry_point = vertex_entry_point;
    pass.fragment_entry_point = fragment_entry_point;
    pass.vertex_buffers = {{.slot = 0,
                            .stride = 8,
                            .step_mode = VertexStepMode::vertex,
                            .attributes = {{"position", 0, VertexFormat::float32x2, 0}}}};
    pass.render_targets = {
        {.role = pass_index + 1 == pass_count ? "layer-stack output" : "layer-stack intermediate",
         .resource = output,
         .subresources = whole_texture(),
         .format = request.output.format,
         .width = request.output.extent.width,
         .height = request.output.extent.height,
         .load = LoadAction::clear,
         .store = StoreAction::store,
         .clear_colour = {},
         .blend = {}}};
    pass.command = DrawCommand{.topology = PrimitiveTopology::triangle_list,
                               .indexed = false,
                               .vertex_count = 3,
                               .index_count = 0,
                               .instance_count = 1};
    return pass;
}

}  // namespace

LayerStackEmission emit_layer_stack(const LayerStackEmissionRequest& request) {
    validate_request(request);
    const std::vector<PassSlice> slices =
        split_layers(request.layers.size(), request.features.binding_budget);
    PlannedResources resources = plan_resources(request, slices.size());
    std::vector<CapabilityWorkaround> workarounds;
    const FilterMode filter = select_filter(request, slices.size(), workarounds);

    std::vector<PassDescriptor> passes;
    std::vector<EmittedLayerStackPass> shaders;
    passes.reserve(slices.size());
    shaders.reserve(slices.size());
    for (std::size_t index = 0; index < slices.size(); ++index) {
        passes.push_back(make_pass(request, resources.intermediates, slices[index], index,
                                   slices.size(), filter));
        const std::size_t input_count =
            slices[index].layer_count +
            static_cast<std::size_t>(slices[index].reads_carried_result);
        KongContext compiler;
        shaders.push_back({pass_identifier(index),
                           compiler.compile(make_kong_source(input_count, index), request.target)});
    }
    return {PassPlan(request.stable_identity, std::move(resources.declarations), std::move(passes)),
            std::move(shaders), std::move(workarounds), false};
}

}  // namespace ctex::emit
