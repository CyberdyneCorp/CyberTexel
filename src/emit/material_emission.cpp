#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <ctex/emit/material_emission.hpp>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <utility>

namespace ctex::emit {
namespace {

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
                      std::string_view role, bool require_initialized) {
    if (texture.version.logical_id.empty() || texture.role.empty() || texture.extent.width == 0 ||
        texture.extent.height == 0 || texture.extent.layers != 1 || texture.mip_levels == 0 ||
        texture.tile_shape.width == 0 || texture.tile_shape.height == 0) {
        throw MaterialEmissionError(std::string(role) + " texture declaration is incomplete");
    }
    if (texture.extent.width > features.maximum_texture_dimension ||
        texture.extent.height > features.maximum_texture_dimension) {
        throw MaterialEmissionError(std::string(role) + " texture '" + texture.version.logical_id +
                                    "' exceeds the maximum texture dimension");
    }
    if (!supports_format(features, texture.format) ||
        texture.format == TextureFormat::depth32_float) {
        throw MaterialEmissionError(std::string(role) + " texture '" + texture.version.logical_id +
                                    "' uses an unsupported format");
    }
    if (require_initialized && !texture.externally_initialized) {
        throw MaterialEmissionError(std::string(role) + " texture '" + texture.version.logical_id +
                                    "' is not initialized");
    }
}

void validate_request(const MaterialShaderEmissionRequest& request) {
    if (request.stable_identity.empty() || request.features.maximum_texture_dimension == 0 ||
        request.features.supported_texture_formats.empty() || request.vertex_count == 0) {
        throw MaterialEmissionError(
            "material emission requires an identity, vertices, and a usable feature set");
    }
    validate_texture(request.output, request.features, "output", false);
    std::set<std::string, std::less<>> identifiers;
    std::set<std::pair<std::string, std::uint64_t>> versions;
    versions.emplace(request.output.version.logical_id, request.output.version.generation);
    for (const MaterialResourceInput& resource : request.resources) {
        if (resource.identifier.empty() || !identifiers.insert(resource.identifier).second) {
            throw MaterialEmissionError("material resources require unique non-empty identifiers");
        }
        validate_texture(resource.texture, request.features, "material resource", true);
        if (!versions
                 .emplace(resource.texture.version.logical_id, resource.texture.version.generation)
                 .second) {
            throw MaterialEmissionError("material resource '" + resource.identifier +
                                        "' aliases another resource generation");
        }
    }
}

const MaterialResourceInput& supplied_resource(const MaterialShaderEmissionRequest& request,
                                               std::string_view identifier) {
    const auto found =
        std::ranges::find(request.resources, identifier, &MaterialResourceInput::identifier);
    if (found == request.resources.end()) {
        throw MaterialEmissionError("emitted graph resource is not supplied: " +
                                    std::string(identifier));
    }
    return *found;
}

std::vector<const MaterialResourceInput*> ordered_resources(
    const WgslExpressionProgram& program, const MaterialShaderEmissionRequest& request) {
    if (program.resource_identifiers.size() != request.resources.size()) {
        for (const MaterialResourceInput& resource : request.resources) {
            if (std::ranges::find(program.resource_identifiers, resource.identifier) ==
                program.resource_identifiers.end()) {
                throw MaterialEmissionError("supplied material resource is unused: " +
                                            resource.identifier);
            }
        }
    }
    std::vector<const MaterialResourceInput*> result;
    result.reserve(program.resource_identifiers.size());
    for (const std::string& identifier : program.resource_identifiers) {
        result.push_back(&supplied_resource(request, identifier));
    }
    return result;
}

const WgslOutputExpression* find_output(const WgslExpressionProgram& program,
                                        std::span<const std::string_view> identifiers,
                                        graph::SocketType type) {
    for (std::string_view identifier : identifiers) {
        const auto found =
            std::ranges::find(program.outputs, identifier, &WgslOutputExpression::identifier);
        if (found != program.outputs.end() && found->type == type) {
            return &*found;
        }
    }
    const auto fallback = std::ranges::find(program.outputs, type, &WgslOutputExpression::type);
    return fallback == program.outputs.end() ? nullptr : &*fallback;
}

struct MaterialOutputs {
    const WgslOutputExpression& colour;
    const WgslOutputExpression* opacity;
};

MaterialOutputs select_outputs(const WgslExpressionProgram& program) {
    constexpr std::array colour_names{std::string_view{"pbr.base_color"},
                                      std::string_view{"base_color"},
                                      std::string_view{"base_colour"}, std::string_view{"colour"}};
    constexpr std::array opacity_names{std::string_view{"pbr.opacity"},
                                       std::string_view{"opacity"}};
    const WgslOutputExpression* colour =
        find_output(program, colour_names, graph::SocketType::colour);
    if (colour == nullptr) {
        throw MaterialEmissionError("material graph has no colour output to render");
    }
    return {*colour, find_output(program, opacity_names, graph::SocketType::scalar)};
}

std::string hex_identifier(std::string_view value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result = "ctex_resource_";
    result.reserve(result.size() + value.size() * 2);
    for (const unsigned char byte : value) {
        result.push_back(digits[byte >> 4U]);
        result.push_back(digits[byte & 0x0fU]);
    }
    return result;
}

std::string indent(std::string_view source) {
    std::string result;
    result.reserve(source.size() + 64);
    bool line_start = true;
    for (char character : source) {
        if (line_start) {
            result.append("    ");
            line_start = false;
        }
        result.push_back(character);
        if (character == '\n') {
            line_start = true;
        }
    }
    return result;
}

void replace_all(std::string& value, std::string_view from, std::string_view to) {
    std::size_t position = 0;
    while ((position = value.find(from, position)) != std::string::npos) {
        value.replace(position, from.size(), to);
        position += to.size();
    }
}

std::string kong_numeric_literal(std::string_view token) {
    double value = 0.0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value,
                                        std::chars_format::scientific);
    if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()) {
        throw MaterialEmissionError("could not translate a graph numeric literal to Kong");
    }
    std::array<char, 128> buffer{};
    const auto formatted =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed,
                      std::numeric_limits<double>::max_digits10);
    if (formatted.ec != std::errc{}) {
        throw MaterialEmissionError("could not format a graph numeric literal for Kong");
    }
    return {buffer.data(), formatted.ptr};
}

bool starts_numeric_token(std::string_view source, std::size_t cursor) {
    if (!std::isdigit(static_cast<unsigned char>(source[cursor]))) {
        return false;
    }
    return cursor == 0 || (!std::isalnum(static_cast<unsigned char>(source[cursor - 1])) &&
                           source[cursor - 1] != '_');
}

std::size_t decimal_end(std::string_view source, std::size_t cursor) {
    while (cursor < source.size() &&
           (std::isdigit(static_cast<unsigned char>(source[cursor])) || source[cursor] == '.')) {
        ++cursor;
    }
    return cursor;
}

std::size_t exponent_end(std::string_view source, std::size_t cursor) {
    if (cursor == source.size() || (source[cursor] != 'e' && source[cursor] != 'E')) {
        return cursor;
    }
    ++cursor;
    if (cursor < source.size() && (source[cursor] == '+' || source[cursor] == '-')) {
        ++cursor;
    }
    while (cursor < source.size() && std::isdigit(static_cast<unsigned char>(source[cursor]))) {
        ++cursor;
    }
    return cursor;
}

std::string normalize_kong_numbers(std::string_view source) {
    std::string result;
    result.reserve(source.size());
    std::size_t cursor = 0;
    while (cursor < source.size()) {
        if (!starts_numeric_token(source, cursor)) {
            result.push_back(source[cursor++]);
            continue;
        }
        const std::size_t decimal = decimal_end(source, cursor);
        const std::size_t end = exponent_end(source, decimal);
        const std::string_view token = source.substr(cursor, end - cursor);
        result.append(end == decimal ? std::string(token) : kong_numeric_literal(token));
        cursor = end;
    }
    return result;
}

std::string return_expression(const MaterialOutputs& outputs, std::string_view constructor) {
    const std::string alpha = outputs.opacity == nullptr ? outputs.colour.variable_name + ".a"
                                                         : outputs.colour.variable_name + ".a * " +
                                                               outputs.opacity->variable_name;
    return std::string(constructor) + "(" + outputs.colour.variable_name + ".r, " +
           outputs.colour.variable_name + ".g, " + outputs.colour.variable_name + ".b, " + alpha +
           ")";
}

std::string wgsl_vertex_source() {
    return R"(struct CtexVertexInput {
    @location(0) position: vec2<f32>,
};

struct CtexVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@vertex
fn main(input: CtexVertexInput) -> CtexVertexOutput {
    var output: CtexVertexOutput;
    output.position = vec4<f32>(input.position, 0.0, 1.0);
    output.uv = input.position * vec2<f32>(0.5, 0.5) + vec2<f32>(0.5, 0.5);
    return output;
}
)";
}

std::string wgsl_fragment_source(const WgslExpressionProgram& program,
                                 std::span<const MaterialResourceInput* const> resources,
                                 const MaterialOutputs& outputs) {
    std::ostringstream source;
    source << "struct CtexFragmentInput {\n    @location(0) uv: vec2<f32>,\n};\n\n";
    for (std::size_t index = 0; index < resources.size(); ++index) {
        source << "@group(0) @binding(" << index << ") var "
               << material_resource_name(resources[index]->identifier) << ": texture_2d<f32>;\n";
    }
    if (!resources.empty()) {
        source << "@group(0) @binding(" << resources.size()
               << ") var ctex_material_sampler: sampler;\n";
    }
    source << "\n@fragment\nfn main(input: CtexFragmentInput) -> @location(0) vec4<f32> {\n"
           << indent(program.source) << "    return " << return_expression(outputs, "vec4<f32>")
           << ";\n}\n";
    return source.str();
}

std::string kong_source(const WgslExpressionProgram& program,
                        std::span<const MaterialResourceInput* const> resources,
                        const MaterialOutputs& outputs) {
    std::string expressions = program.source;
    replace_all(expressions, "vec3<f32>", "float3");
    replace_all(expressions, "vec4<f32>", "float4");
    replace_all(expressions, "f32", "float");
    replace_all(expressions, "let ", "var ");
    expressions = normalize_kong_numbers(expressions);

    std::ostringstream source;
    source << "struct ctex_vertex_in {\n    pos: float2;\n}\n\n"
              "struct ctex_vertex_out {\n    pos: float4;\n    uv: float2;\n}\n\n"
              "fun ctex_vertex(input: ctex_vertex_in): ctex_vertex_out {\n"
              "    var output: ctex_vertex_out;\n"
              "    output.pos = float4(input.pos.x, input.pos.y, 0.0, 1.0);\n"
              "    output.uv = input.pos * float2(0.5, 0.5) + float2(0.5, 0.5);\n"
              "    return output;\n}\n\n";
    for (const MaterialResourceInput* resource : resources) {
        source << "#[set(material)]\nconst " << material_resource_name(resource->identifier)
               << ": tex2d;\n\n";
    }
    if (!resources.empty()) {
        source << "#[set(material)]\nconst ctex_material_sampler: sampler;\n\n";
    }
    source << "fun ctex_fragment(input: ctex_vertex_out): float4 {\n"
           << indent(expressions) << "    return " << return_expression(outputs, "float4")
           << ";\n}\n\n#[pipe]\nstruct ctex_material_pipe {\n"
              "    vertex = ctex_vertex;\n    fragment = ctex_fragment;\n}\n";
    return source.str();
}

FilterMode select_filter(const MaterialShaderEmissionRequest& request,
                         std::span<const MaterialResourceInput* const> resources,
                         std::vector<CapabilityWorkaround>& workarounds) {
    const bool samples_float = std::ranges::any_of(resources, [](const auto* resource) {
        return is_floating_point(resource->texture.format);
    });
    if (request.requested_filter != FilterMode::linear ||
        request.features.floating_point_filtering || !samples_float) {
        return request.requested_filter;
    }
    workarounds.push_back({"nearest_float_sampling",
                           "linear floating-point texture filtering is unavailable; material "
                           "samplers use nearest filtering"});
    return FilterMode::nearest;
}

std::pair<std::string, std::string> exported_entry_points(ShaderTarget target) {
    return target == ShaderTarget::msl ? std::pair{"ctex_vertex", "ctex_fragment"}
                                       : std::pair{"main", "main"};
}

PassPlan make_plan(const MaterialShaderEmissionRequest& request,
                   std::span<const MaterialResourceInput* const> ordered, FilterMode filter) {
    const auto [vertex_entry, fragment_entry] = exported_entry_points(request.target);
    PassDescriptor pass{
        .identifier = "material-graph",
        .kind = PassKind::render,
        .dependencies = {},
        .vertex_entry_point = vertex_entry,
        .fragment_entry_point = fragment_entry,
        .compute_entry_point = {},
        .accesses = {},
        .texture_bindings = {},
        .sampler_bindings = {},
        .uniform_blocks = {},
        .vertex_buffers = {{.slot = 0,
                            .stride = 8,
                            .step_mode = VertexStepMode::vertex,
                            .attributes = {{"position", 0, VertexFormat::float32x2, 0}}}},
        .render_targets = {{.role = "material graph colour output",
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

    std::vector<LogicalTexture> resources;
    resources.reserve(ordered.size() + 1);
    for (std::size_t index = 0; index < ordered.size(); ++index) {
        const MaterialResourceInput& input = *ordered[index];
        resources.push_back(input.texture);
        pass.accesses.push_back({input.texture.version, whole_texture(input.texture),
                                 ResourceAccessMode::read, LoadAction::load,
                                 StoreAction::not_applicable});
        pass.texture_bindings.push_back({.group = 0,
                                         .binding = static_cast<std::uint32_t>(index),
                                         .role = "material resource " + input.identifier,
                                         .visibility = ShaderVisibility::fragment,
                                         .kind = TextureBindingKind::sampled,
                                         .resource = input.texture.version,
                                         .subresources = whole_texture(input.texture),
                                         .view_dimension = TextureViewDimension::d2,
                                         .encoding = "declared graph resource",
                                         .mip_convention = "base mip with implicit derivatives"});
    }
    if (!ordered.empty()) {
        const std::uint32_t binding = static_cast<std::uint32_t>(ordered.size());
        pass.sampler_bindings.push_back({.group = 0,
                                         .binding = binding,
                                         .role = "material graph sampler",
                                         .visibility = ShaderVisibility::fragment,
                                         .min_filter = filter,
                                         .mag_filter = filter,
                                         .mip_filter = filter,
                                         .address_u = AddressMode::repeat,
                                         .address_v = AddressMode::repeat,
                                         .address_w = AddressMode::repeat});
    }
    pass.accesses.push_back({request.output.version, whole_texture(request.output),
                             ResourceAccessMode::write, LoadAction::clear, StoreAction::store});
    resources.push_back(request.output);
    return PassPlan(request.stable_identity, std::move(resources), {std::move(pass)});
}

MaterialShaderEmission finish_emission(const WgslExpressionProgram& program,
                                       const MaterialShaderEmissionRequest& request) {
    validate_request(request);
    const std::vector<const MaterialResourceInput*> resources = ordered_resources(program, request);
    const std::size_t binding_count =
        resources.size() + static_cast<std::size_t>(!resources.empty());
    if (binding_count > request.features.binding_budget) {
        throw MaterialEmissionError("material graph requires " + std::to_string(binding_count) +
                                    " bindings but the device budget is " +
                                    std::to_string(request.features.binding_budget));
    }
    const MaterialOutputs outputs = select_outputs(program);
    std::vector<CapabilityWorkaround> workarounds;
    const FilterMode filter = select_filter(request, resources, workarounds);

    KongShaderProgram shader;
    if (request.target == ShaderTarget::wgsl) {
        shader = {request.target,
                  SplitTextShaderProgram{wgsl_vertex_source(),
                                         wgsl_fragment_source(program, resources, outputs)}};
    } else {
        KongContext compiler;
        shader = compiler.compile(kong_source(program, resources, outputs), request.target);
    }
    return {make_plan(request, resources, filter), std::move(shader), std::move(workarounds),
            program.node_attributions};
}

void require_target_semantics(const graph::GraphDocument& graph,
                              const graph::NodeTypeRegistry& registry, ShaderTarget target) {
    const graph::GraphSemanticReport report =
        graph::inspect_graph_semantics(graph, registry, target);
    if (!report.emittable()) {
        throw MaterialEmissionError(report.issues.front().message);
    }
}

}  // namespace

std::string material_resource_name(std::string_view identifier) {
    return hex_identifier(identifier);
}

MaterialShaderEmission emit_material_shader(const graph::GraphDocument& graph,
                                            const graph::NodeTypeRegistry& registry,
                                            const MaterialShaderEmissionRequest& request) {
    require_target_semantics(graph, registry, request.target);
    return finish_emission(emit_wgsl_expressions(graph, registry), request);
}

MaterialShaderEmission emit_material_shader(const graph::GraphDocument& graph,
                                            const MaterialShaderEmissionRequest& request) {
    const graph::NodeTypeRegistry registry;
    return emit_material_shader(graph, registry, request);
}

MaterialShaderEmission emit_workspace_material_shader(
    const graph::GraphWorkspace& workspace, std::string_view material_identifier,
    const graph::NodeTypeRegistry& registry, const MaterialShaderEmissionRequest& request) {
    require_target_semantics(workspace.material(material_identifier), registry, request.target);
    for (const graph::NodeGroupDefinition& group : workspace.groups()) {
        require_target_semantics(group.graph, registry, request.target);
    }
    return finish_emission(emit_material_wgsl_expressions(workspace, material_identifier, registry),
                           request);
}

MaterialShaderEmission emit_workspace_material_shader(
    const graph::GraphWorkspace& workspace, std::string_view material_identifier,
    const MaterialShaderEmissionRequest& request) {
    const graph::NodeTypeRegistry registry;
    return emit_workspace_material_shader(workspace, material_identifier, registry, request);
}

}  // namespace ctex::emit
