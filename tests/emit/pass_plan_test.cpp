#include <ctex/emit/pass_plan.hpp>
#include <iostream>
#include <string_view>

namespace {

using namespace ctex::emit;

TextureSubresourceRange tile_range() {
    return {.first_mip = 0,
            .mip_count = 1,
            .first_layer = 0,
            .layer_count = 1,
            .tiles = TileRange{1, 0, 2, 2}};
}

LogicalTexture texture(std::string id, std::uint64_t generation, std::string role,
                       TextureFormat format, bool initialized) {
    return {.version = {std::move(id), generation},
            .role = std::move(role),
            .format = format,
            .extent = {128, 64, 1},
            .mip_levels = 3,
            .tile_shape = {32, 32},
            .externally_initialized = initialized};
}

TextureBinding sampled_binding(std::uint32_t binding, std::string role, ResourceVersion resource) {
    return {.group = 0,
            .binding = binding,
            .role = std::move(role),
            .visibility = ShaderVisibility::fragment,
            .kind = TextureBindingKind::sampled,
            .resource = std::move(resource),
            .subresources = tile_range()};
}

SamplerBinding sampler_binding(std::uint32_t binding) {
    return {.group = 0,
            .binding = binding,
            .role = "material sampler",
            .visibility = ShaderVisibility::fragment,
            .min_filter = FilterMode::linear,
            .mag_filter = FilterMode::linear,
            .mip_filter = FilterMode::nearest,
            .address_u = AddressMode::clamp_to_edge,
            .address_v = AddressMode::clamp_to_edge,
            .address_w = AddressMode::clamp_to_edge};
}

UniformBlockLayout uniform_block(std::uint32_t binding) {
    return {.group = 0,
            .binding = binding,
            .role = "material parameters",
            .visibility = ShaderVisibility::fragment,
            .size = 32,
            .fields = {{"opacity", 0, 4, 4}, {"transform", 16, 16, 16}}};
}

RenderTarget colour_target(ResourceVersion resource, TextureFormat format) {
    return {.role = "paint colour",
            .resource = std::move(resource),
            .subresources = tile_range(),
            .format = format,
            .width = 64,
            .height = 64,
            .load = LoadAction::clear,
            .store = StoreAction::store,
            .clear_colour = {0.0F, 0.0F, 0.0F, 0.0F},
            .blend = {.enabled = true,
                      .colour = {BlendFactor::source_alpha, BlendFactor::one_minus_source_alpha,
                                 BlendOperation::add},
                      .alpha = {BlendFactor::one, BlendFactor::one_minus_source_alpha,
                                BlendOperation::add},
                      .write_mask = 0x0f}};
}

PassDescriptor compose_pass() {
    const ResourceVersion source{"canvas", 7};
    const ResourceVersion scratch{"scratch", 1};
    const ResourceVersion depth{"depth", 1};
    return {.identifier = "compose",
            .kind = PassKind::render,
            .dependencies = {},
            .vertex_entry_point = "material_vertex",
            .fragment_entry_point = "material_fragment",
            .compute_entry_point = {},
            .accesses = {{source, tile_range(), ResourceAccessMode::read, LoadAction::load,
                          StoreAction::not_applicable},
                         {scratch, tile_range(), ResourceAccessMode::write, LoadAction::clear,
                          StoreAction::store},
                         {depth, tile_range(), ResourceAccessMode::write, LoadAction::clear,
                          StoreAction::store}},
            .texture_bindings = {sampled_binding(0, "source colour", source)},
            .sampler_bindings = {sampler_binding(1)},
            .uniform_blocks = {uniform_block(2)},
            .vertex_buffers = {{.slot = 0,
                                .stride = 20,
                                .step_mode = VertexStepMode::vertex,
                                .attributes = {{"position", 0, VertexFormat::float32x3, 0},
                                               {"uv", 1, VertexFormat::float32x2, 12}}}},
            .render_targets = {colour_target(scratch, TextureFormat::rgba8_unorm)},
            .depth_target = DepthTarget{.role = "mesh depth",
                                        .resource = depth,
                                        .subresources = tile_range(),
                                        .format = TextureFormat::depth32_float,
                                        .width = 64,
                                        .height = 64,
                                        .load = LoadAction::clear,
                                        .store = StoreAction::store,
                                        .clear_depth = 1.0F},
            .depth_state = {.test_enabled = true,
                            .write_enabled = true,
                            .compare = CompareFunction::less_equal},
            .command = DrawCommand{.topology = PrimitiveTopology::triangle_list,
                                   .indexed = false,
                                   .vertex_count = 6,
                                   .index_count = 0,
                                   .instance_count = 1}};
}

PassDescriptor resolve_pass() {
    const ResourceVersion scratch{"scratch", 1};
    const ResourceVersion destination{"canvas", 8};
    return {.identifier = "resolve",
            .kind = PassKind::render,
            .dependencies = {"compose"},
            .vertex_entry_point = "fullscreen_vertex",
            .fragment_entry_point = "resolve_fragment",
            .compute_entry_point = {},
            .accesses = {{scratch, tile_range(), ResourceAccessMode::read, LoadAction::load,
                          StoreAction::not_applicable},
                         {destination, tile_range(), ResourceAccessMode::write, LoadAction::clear,
                          StoreAction::store}},
            .texture_bindings = {sampled_binding(0, "composited colour", scratch)},
            .sampler_bindings = {sampler_binding(1)},
            .uniform_blocks = {uniform_block(2)},
            .vertex_buffers = {},
            .render_targets = {colour_target(destination, TextureFormat::rgba8_unorm)},
            .depth_target = std::nullopt,
            .depth_state = {},
            .command = DrawCommand{.topology = PrimitiveTopology::triangle_list,
                                   .indexed = false,
                                   .vertex_count = 3,
                                   .index_count = 0,
                                   .instance_count = 1}};
}

std::vector<LogicalTexture> resources() {
    return {texture("canvas", 7, "committed source", TextureFormat::rgba8_unorm, true),
            texture("scratch", 1, "composite intermediate", TextureFormat::rgba8_unorm, false),
            texture("canvas", 8, "next committed generation", TextureFormat::rgba8_unorm, false),
            texture("depth", 1, "temporary mesh depth", TextureFormat::depth32_float, false)};
}

PassPlan complete_plan() {
    return PassPlan("material/fixture/pass-plan-v1", resources(), {compose_pass(), resolve_pass()});
}

PassDescriptor storage_write_pass(std::string identifier, std::vector<std::string> dependencies,
                                  TileRange tiles) {
    const ResourceVersion resource{"shared", 3};
    const TextureSubresourceRange range{
        .first_mip = 0, .mip_count = 1, .first_layer = 0, .layer_count = 1, .tiles = tiles};
    return {.identifier = std::move(identifier),
            .kind = PassKind::compute,
            .dependencies = std::move(dependencies),
            .vertex_entry_point = {},
            .fragment_entry_point = {},
            .compute_entry_point = "write_main",
            .accesses = {{resource, range, ResourceAccessMode::write, LoadAction::discard,
                          StoreAction::store}},
            .texture_bindings = {{.group = 0,
                                  .binding = 0,
                                  .role = "output",
                                  .visibility = ShaderVisibility::compute,
                                  .kind = TextureBindingKind::storage_write,
                                  .resource = resource,
                                  .subresources = range}},
            .sampler_bindings = {},
            .uniform_blocks = {},
            .vertex_buffers = {},
            .render_targets = {},
            .depth_target = std::nullopt,
            .depth_state = {},
            .command = DispatchCommand{1, 1, 1}};
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool complete_plan_names_every_host_input() {
    const PassPlan plan = complete_plan();
    const auto passes = plan.passes();
    const auto lifetimes = plan.lifetimes();
    return expect(plan.stable_identity() == "material/fixture/pass-plan-v1" &&
                      plan.resources().size() == 4 && passes.size() == 2,
                  "pass plan lost its stable identity, resources, or ordered passes") &&
           expect(passes[0].vertex_entry_point == "material_vertex" &&
                      passes[0].fragment_entry_point == "material_fragment" &&
                      passes[0].texture_bindings[0].role == "source colour" &&
                      passes[0].sampler_bindings[0].binding == 1 &&
                      passes[0].uniform_blocks[0].fields[1].offset == 16 &&
                      passes[0].vertex_buffers[0].attributes[1].semantic == "uv",
                  "render entry points, bindings, uniform offsets, or vertex layout were lost") &&
           expect(passes[0].render_targets[0].width == 64 &&
                      passes[0].depth_target->format == TextureFormat::depth32_float &&
                      passes[0].depth_state.compare == CompareFunction::less_equal &&
                      std::get<DrawCommand>(passes[0].command).vertex_count == 6,
                  "render targets, depth state, or draw dimensions were lost") &&
           expect(passes[1].dependencies == std::vector<std::string>{"compose"},
                  "ordered pass dependency was lost") &&
           expect(lifetimes.size() == 4 && lifetimes[0].resource.generation == 7 &&
                      lifetimes[0].first_pass == 0 && lifetimes[0].last_pass == 0 &&
                      lifetimes[1].first_pass == 0 && lifetimes[1].last_pass == 1 &&
                      lifetimes[2].resource.generation == 8 && lifetimes[2].first_pass == 1,
                  "resource-generation lifetimes were not derived from pass use") &&
           expect(plan == complete_plan(), "identical pass-plan inputs were not deterministic");
}

bool unsynchronized_hazards_are_refused() {
    PassDescriptor resolve = resolve_pass();
    resolve.dependencies.clear();
    try {
        static_cast<void>(
            PassPlan("missing-dependency", resources(), {compose_pass(), std::move(resolve)}));
    } catch (const PassPlanError& error) {
        return expect(
            std::string_view(error.what()).find("unsynchronized hazard") != std::string_view::npos,
            "missing dependency did not name its resource hazard");
    }
    return expect(false, "overlapping read-after-write was accepted without a dependency");
}

bool target_size_mismatch_is_named() {
    PassDescriptor compose = compose_pass();
    compose.render_targets[0].width = 63;
    try {
        static_cast<void>(PassPlan("bad-size", resources(), {std::move(compose)}));
    } catch (const PassPlanError& error) {
        const std::string_view message(error.what());
        return expect(message.find("63x64") != std::string_view::npos &&
                          message.find("64x64") != std::string_view::npos,
                      "target-size refusal did not name actual and declared dimensions");
    }
    return expect(false, "render target with the wrong dimensions was accepted");
}

bool compute_dispatch_and_storage_bindings_are_complete() {
    const ResourceVersion source{"compute-source", 1};
    const ResourceVersion destination{"compute-destination", 2};
    const TextureSubresourceRange range{
        .first_mip = 0, .mip_count = 1, .first_layer = 0, .layer_count = 1, .tiles = std::nullopt};
    PassDescriptor compute{.identifier = "generate",
                           .kind = PassKind::compute,
                           .dependencies = {},
                           .vertex_entry_point = {},
                           .fragment_entry_point = {},
                           .compute_entry_point = "generate_main",
                           .accesses = {{source, range, ResourceAccessMode::read, LoadAction::load,
                                         StoreAction::not_applicable},
                                        {destination, range, ResourceAccessMode::write,
                                         LoadAction::discard, StoreAction::store}},
                           .texture_bindings = {{.group = 0,
                                                 .binding = 0,
                                                 .role = "generator input",
                                                 .visibility = ShaderVisibility::compute,
                                                 .kind = TextureBindingKind::storage_read,
                                                 .resource = source,
                                                 .subresources = range},
                                                {.group = 0,
                                                 .binding = 1,
                                                 .role = "generator output",
                                                 .visibility = ShaderVisibility::compute,
                                                 .kind = TextureBindingKind::storage_write,
                                                 .resource = destination,
                                                 .subresources = range}},
                           .sampler_bindings = {},
                           .uniform_blocks = {{.group = 0,
                                               .binding = 2,
                                               .role = "generator parameters",
                                               .visibility = ShaderVisibility::compute,
                                               .size = 16,
                                               .fields = {{"seed", 0, 4, 4}}}},
                           .vertex_buffers = {},
                           .render_targets = {},
                           .depth_target = std::nullopt,
                           .depth_state = {},
                           .command = DispatchCommand{4, 2, 1}};
    const PassPlan plan(
        "compute-generator",
        {texture("compute-source", 1, "generator input", TextureFormat::rgba16_float, true),
         texture("compute-destination", 2, "generator output", TextureFormat::rgba16_float, false)},
        {std::move(compute)});
    const auto& pass = plan.passes().front();
    const auto dispatch = std::get<DispatchCommand>(pass.command);
    return expect(pass.compute_entry_point == "generate_main" &&
                      pass.texture_bindings[1].kind == TextureBindingKind::storage_write &&
                      dispatch == DispatchCommand{4, 2, 1},
                  "compute entry point, storage binding, or dispatch dimensions were lost");
}

bool invalid_uniform_and_subresource_layouts_are_refused() {
    PassDescriptor bad_uniform = compose_pass();
    bad_uniform.uniform_blocks[0].fields[1].offset = 14;
    bool uniform_refused = false;
    try {
        static_cast<void>(PassPlan("bad-uniform", resources(), {std::move(bad_uniform)}));
    } catch (const PassPlanError&) {
        uniform_refused = true;
    }

    PassDescriptor bad_tiles = compose_pass();
    bad_tiles.accesses[0].subresources.tiles->x = 4;
    bool tiles_refused = false;
    try {
        static_cast<void>(PassPlan("bad-tiles", resources(), {std::move(bad_tiles)}));
    } catch (const PassPlanError&) {
        tiles_refused = true;
    }
    return expect(uniform_refused && tiles_refused,
                  "invalid uniform offsets or tile subresources were accepted");
}

bool uninitialized_and_discarded_reads_are_refused() {
    PassDescriptor uninitialized = compose_pass();
    auto uninitialized_resources = resources();
    uninitialized_resources[0].externally_initialized = false;
    bool uninitialized_refused = false;
    try {
        static_cast<void>(PassPlan("uninitialized", std::move(uninitialized_resources),
                                   {std::move(uninitialized)}));
    } catch (const PassPlanError& error) {
        uninitialized_refused =
            std::string_view(error.what()).find("reads uninitialized") != std::string_view::npos;
    }

    PassDescriptor compose = compose_pass();
    compose.accesses[1].store = StoreAction::discard;
    compose.render_targets[0].store = StoreAction::discard;
    try {
        static_cast<void>(PassPlan("discarded", resources(), {std::move(compose), resolve_pass()}));
    } catch (const PassPlanError& error) {
        return expect(
            uninitialized_refused && std::string_view(error.what()).find("reads uninitialized") !=
                                         std::string_view::npos,
            "uninitialized or discarded resource contents were not diagnosed");
    }
    return expect(false, "a read after a discarded store was accepted");
}

bool duplicate_binding_slots_are_refused() {
    PassDescriptor compose = compose_pass();
    compose.sampler_bindings[0].binding = compose.texture_bindings[0].binding;
    try {
        static_cast<void>(PassPlan("duplicate-binding", resources(), {std::move(compose)}));
    } catch (const PassPlanError& error) {
        return expect(
            std::string_view(error.what()).find("reuses binding") != std::string_view::npos,
            "duplicate binding diagnostic did not name the collision");
    }
    return expect(false, "duplicate texture and sampler binding slots were accepted");
}

bool dependency_analysis_is_tile_aware_and_transitive() {
    const std::vector<LogicalTexture> shared = {
        texture("shared", 3, "shared output", TextureFormat::rgba16_float, false)};
    const PassPlan disjoint("disjoint-tiles", shared,
                            {storage_write_pass("left", {}, TileRange{0, 0, 1, 1}),
                             storage_write_pass("right", {}, TileRange{1, 0, 1, 1})});
    const PassPlan transitive("transitive-dependencies", shared,
                              {storage_write_pass("first", {}, TileRange{0, 0, 1, 1}),
                               storage_write_pass("second", {"first"}, TileRange{0, 0, 1, 1}),
                               storage_write_pass("third", {"second"}, TileRange{0, 0, 1, 1})});
    return expect(disjoint.lifetimes()[0].last_pass == 1 &&
                      transitive.passes()[2].dependencies == std::vector<std::string>{"second"},
                  "disjoint tiles or transitive dependencies were rejected");
}

}  // namespace

int main() {
    return complete_plan_names_every_host_input() && unsynchronized_hazards_are_refused() &&
                   target_size_mismatch_is_named() &&
                   compute_dispatch_and_storage_bindings_are_complete() &&
                   invalid_uniform_and_subresource_layouts_are_refused() &&
                   uninitialized_and_discarded_reads_are_refused() &&
                   duplicate_binding_slots_are_refused() &&
                   dependency_analysis_is_tile_aware_and_transitive()
               ? 0
               : 1;
}
