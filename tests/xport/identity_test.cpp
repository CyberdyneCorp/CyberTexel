#include <ctex/emit/pass_plan.hpp>
#include <exception>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace ctex::emit;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

LogicalTexture output(std::string identifier) {
    return {
        .version = {std::move(identifier), 1},
        .role = "host pipeline output",
        .format = TextureFormat::r8_unorm,
        .extent = {4, 4, 1},
        .mip_levels = 1,
        .tile_shape = {4, 4},
        .externally_initialized = false,
    };
}

PassDescriptor compute_pass(std::string identifier, const ResourceVersion& output_version) {
    const TextureSubresourceRange whole_texture{};
    return {
        .identifier = std::move(identifier),
        .kind = PassKind::compute,
        .dependencies = {},
        .vertex_entry_point = {},
        .fragment_entry_point = {},
        .compute_entry_point = "main",
        .accesses = {{output_version, whole_texture, ResourceAccessMode::write, LoadAction::discard,
                      StoreAction::store}},
        .texture_bindings = {{.group = 0,
                              .binding = 0,
                              .role = "output",
                              .visibility = ShaderVisibility::compute,
                              .kind = TextureBindingKind::storage_write,
                              .resource = output_version,
                              .subresources = whole_texture}},
        .sampler_bindings = {},
        .uniform_blocks = {},
        .vertex_buffers = {},
        .render_targets = {},
        .depth_target = std::nullopt,
        .depth_state = {},
        .command = DispatchCommand{1, 1, 1},
    };
}

PassPlan plan(std::string identity, std::string first_identifier = "first", bool reverse = false) {
    std::vector resources{output("first-output"), output("second-output")};
    std::vector passes{
        compute_pass(std::move(first_identifier), resources[0].version),
        compute_pass("second", resources[1].version),
    };
    if (reverse) {
        std::swap(passes[0], passes[1]);
    }
    return PassPlan(std::move(identity), std::move(resources), std::move(passes));
}

bool reconstructed_plan_finds_the_cached_pipeline() {
    const PassPlan first = plan("material/paint");
    std::map<HostCacheIdentity, std::string> pipelines;
    pipelines.emplace(first.pipeline_identity("first"), "compiled pipeline");

    const PassPlan reconstructed = plan("material/paint");
    const HostCacheIdentity identity = reconstructed.pipeline_identity("first");
    const auto found = pipelines.find(identity);
    return expect(found != pipelines.end() && found->second == "compiled pipeline" &&
                      identity.kind == HostCacheResourceKind::compute_pipeline &&
                      identity.scope == "material/paint" && identity.resource == "first",
                  "a reconstructed pass plan could not address the host pipeline cache");
}

bool identity_does_not_depend_on_pass_order_or_delimiters() {
    const HostCacheIdentity original = plan("material/paint").pipeline_identity("first");
    const HostCacheIdentity reordered =
        plan("material/paint", "first", true).pipeline_identity("first");
    const HostCacheIdentity delimiter_left =
        plan("material/paint/first").pipeline_identity("second");
    const HostCacheIdentity delimiter_right =
        plan("material/paint", "first/second").pipeline_identity("first/second");
    return expect(original == reordered, "pipeline identity depended on pass ordinal") &&
           expect(delimiter_left != delimiter_right,
                  "structured pipeline identity was ambiguous across components");
}

bool unknown_pass_identity_is_refused() {
    try {
        static_cast<void>(plan("material/paint").pipeline_identity("missing"));
    } catch (const PassPlanError& error) {
        return expect(std::string_view(error.what()).find("missing") != std::string_view::npos,
                      "unknown-pass refusal omitted the requested identity");
    } catch (const std::exception&) {
    }
    return expect(false, "an unknown pass received a cache identity");
}

}  // namespace

int main() {
    return reconstructed_plan_finds_the_cached_pipeline() &&
                   identity_does_not_depend_on_pass_order_or_delimiters() &&
                   unknown_pass_identity_is_refused()
               ? 0
               : 1;
}
