#include <algorithm>
#include <cmath>
#include <ctex/emit/pass_plan.hpp>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace ctex::emit {
namespace {

using ResourceKey = std::pair<std::string, std::uint64_t>;
using BindingKey = std::pair<std::uint32_t, std::uint32_t>;

ResourceKey key(const ResourceVersion& resource) {
    return {resource.logical_id, resource.generation};
}

std::string label(const ResourceVersion& resource) {
    return resource.logical_id + "@" + std::to_string(resource.generation);
}

bool interval_overlap(std::uint32_t first_a, std::uint32_t count_a, std::uint32_t first_b,
                      std::uint32_t count_b) {
    const std::uint64_t end_a = static_cast<std::uint64_t>(first_a) + count_a;
    const std::uint64_t end_b = static_cast<std::uint64_t>(first_b) + count_b;
    return first_a < end_b && first_b < end_a;
}

bool tile_overlap(const std::optional<TileRange>& left, const std::optional<TileRange>& right) {
    if (!left || !right) {
        return true;
    }
    return interval_overlap(left->x, left->width, right->x, right->width) &&
           interval_overlap(left->y, left->height, right->y, right->height);
}

bool ranges_overlap(const TextureSubresourceRange& left, const TextureSubresourceRange& right) {
    return interval_overlap(left.first_mip, left.mip_count, right.first_mip, right.mip_count) &&
           interval_overlap(left.first_layer, left.layer_count, right.first_layer,
                            right.layer_count) &&
           tile_overlap(left.tiles, right.tiles);
}

bool interval_contains(std::uint32_t outer_first, std::uint32_t outer_count,
                       std::uint32_t inner_first, std::uint32_t inner_count) {
    return outer_first <= inner_first && static_cast<std::uint64_t>(inner_first) + inner_count <=
                                             static_cast<std::uint64_t>(outer_first) + outer_count;
}

bool range_contains(const TextureSubresourceRange& outer, const TextureSubresourceRange& inner) {
    if (!interval_contains(outer.first_mip, outer.mip_count, inner.first_mip, inner.mip_count) ||
        !interval_contains(outer.first_layer, outer.layer_count, inner.first_layer,
                           inner.layer_count)) {
        return false;
    }
    if (!outer.tiles) {
        return true;
    }
    return inner.tiles &&
           interval_contains(outer.tiles->x, outer.tiles->width, inner.tiles->x,
                             inner.tiles->width) &&
           interval_contains(outer.tiles->y, outer.tiles->height, inner.tiles->y,
                             inner.tiles->height);
}

bool writes(ResourceAccessMode mode) { return mode != ResourceAccessMode::read; }

bool reads(ResourceAccessMode mode) { return mode != ResourceAccessMode::write; }

std::uint32_t mip_dimension(std::uint32_t dimension, std::uint32_t mip) {
    return std::max(1U, dimension >> mip);
}

std::uint32_t maximum_mip_levels(std::uint32_t width, std::uint32_t height) {
    std::uint32_t dimension = std::max(width, height);
    std::uint32_t levels = 0;
    while (dimension != 0) {
        ++levels;
        dimension >>= 1U;
    }
    return levels;
}

const LogicalTexture& require_resource(
    const std::map<ResourceKey, const LogicalTexture*>& resources, const ResourceVersion& version,
    std::string_view context) {
    const auto found = resources.find(key(version));
    if (found == resources.end()) {
        throw PassPlanError(std::string(context) + " references undeclared resource " +
                            label(version));
    }
    return *found->second;
}

void validate_subresources(const LogicalTexture& texture, const TextureSubresourceRange& range,
                           std::string_view context) {
    if (range.mip_count == 0 || range.layer_count == 0 ||
        static_cast<std::uint64_t>(range.first_mip) + range.mip_count > texture.mip_levels ||
        static_cast<std::uint64_t>(range.first_layer) + range.layer_count > texture.extent.layers) {
        throw PassPlanError(std::string(context) + " has an out-of-range mip or layer range");
    }
    if (!range.tiles) {
        return;
    }
    if (range.mip_count != 1 || range.tiles->width == 0 || range.tiles->height == 0) {
        throw PassPlanError(std::string(context) +
                            " tile ranges require one mip and non-zero dimensions");
    }
    const std::uint32_t mip_width = mip_dimension(texture.extent.width, range.first_mip);
    const std::uint32_t mip_height = mip_dimension(texture.extent.height, range.first_mip);
    const std::uint32_t columns = 1U + (mip_width - 1U) / texture.tile_shape.width;
    const std::uint32_t rows = 1U + (mip_height - 1U) / texture.tile_shape.height;
    if (static_cast<std::uint64_t>(range.tiles->x) + range.tiles->width > columns ||
        static_cast<std::uint64_t>(range.tiles->y) + range.tiles->height > rows) {
        throw PassPlanError(std::string(context) + " has an out-of-range tile rectangle");
    }
}

void validate_resource_declarations(std::span<const LogicalTexture> resources,
                                    std::map<ResourceKey, const LogicalTexture*>& resource_index) {
    for (const LogicalTexture& resource : resources) {
        if (resource.version.logical_id.empty() || resource.role.empty() ||
            resource.extent.width == 0 || resource.extent.height == 0 ||
            resource.extent.layers == 0 || resource.tile_shape.width == 0 ||
            resource.tile_shape.height == 0 || resource.mip_levels == 0 ||
            resource.mip_levels >
                maximum_mip_levels(resource.extent.width, resource.extent.height)) {
            throw PassPlanError("logical texture declaration is incomplete: " +
                                label(resource.version));
        }
        if (!resource_index.emplace(key(resource.version), &resource).second) {
            throw PassPlanError("logical texture version is declared more than once: " +
                                label(resource.version));
        }
    }
}

void validate_access_actions(const ResourceAccess& access, std::string_view pass_id) {
    const std::string context =
        "pass " + std::string(pass_id) + " access to " + label(access.resource);
    if (access.mode == ResourceAccessMode::read &&
        (access.load != LoadAction::load || access.store != StoreAction::not_applicable)) {
        throw PassPlanError(context + " must load without a store action");
    }
    if (access.mode == ResourceAccessMode::write &&
        (access.load == LoadAction::load || access.store == StoreAction::not_applicable)) {
        throw PassPlanError(context + " must initialize and declare a store action");
    }
    if (access.mode == ResourceAccessMode::read_write &&
        (access.load != LoadAction::load || access.store == StoreAction::not_applicable)) {
        throw PassPlanError(context + " must load and declare a store action");
    }
}

bool initialized_before(std::span<const PassDescriptor> passes, std::size_t pass_index,
                        std::size_t access_index, const LogicalTexture& resource,
                        const ResourceAccess& requested) {
    for (std::size_t pass_cursor = pass_index + 1; pass_cursor > 0; --pass_cursor) {
        const std::size_t prior_pass = pass_cursor - 1;
        const std::size_t access_limit =
            prior_pass == pass_index ? access_index : passes[prior_pass].accesses.size();
        for (std::size_t access_cursor = access_limit; access_cursor > 0; --access_cursor) {
            const ResourceAccess& candidate = passes[prior_pass].accesses[access_cursor - 1];
            if (candidate.resource != requested.resource || !writes(candidate.mode) ||
                !ranges_overlap(candidate.subresources, requested.subresources)) {
                continue;
            }
            if (candidate.store == StoreAction::discard) {
                return false;
            }
            if (range_contains(candidate.subresources, requested.subresources)) {
                return true;
            }
        }
    }
    return resource.externally_initialized;
}

void validate_binding_slot(BindingKey binding, std::string_view role,
                           std::set<BindingKey>& occupied, std::string_view pass_id) {
    if (role.empty()) {
        throw PassPlanError("pass " + std::string(pass_id) + " contains a binding without a role");
    }
    if (!occupied.insert(binding).second) {
        throw PassPlanError("pass " + std::string(pass_id) + " reuses binding " +
                            std::to_string(binding.first) + ":" + std::to_string(binding.second));
    }
}

void validate_visibility(ShaderVisibility visibility, PassKind kind, std::string_view pass_id) {
    const bool valid = kind == PassKind::compute ? visibility == ShaderVisibility::compute
                                                 : visibility != ShaderVisibility::compute;
    if (!valid) {
        throw PassPlanError("pass " + std::string(pass_id) +
                            " has a binding with incompatible shader visibility");
    }
}

const ResourceAccess* find_covering_access(const PassDescriptor& pass,
                                           const ResourceVersion& resource,
                                           const TextureSubresourceRange& range, bool require_read,
                                           bool require_write) {
    const auto found =
        std::find_if(pass.accesses.begin(), pass.accesses.end(), [&](const ResourceAccess& access) {
            return access.resource == resource && range_contains(access.subresources, range) &&
                   (!require_read || reads(access.mode)) && (!require_write || writes(access.mode));
        });
    return found == pass.accesses.end() ? nullptr : &*found;
}

void validate_uniform_block(const UniformBlockLayout& block, std::string_view pass_id) {
    if (block.size == 0 || block.fields.empty()) {
        throw PassPlanError("pass " + std::string(pass_id) + " has an empty uniform block layout");
    }
    std::uint64_t preceding_end = 0;
    std::set<std::string> names;
    for (const UniformField& field : block.fields) {
        if (field.name.empty() || field.size == 0 || field.alignment == 0 ||
            (field.alignment & (field.alignment - 1U)) != 0 ||
            field.offset % field.alignment != 0 || field.offset < preceding_end ||
            static_cast<std::uint64_t>(field.offset) + field.size > block.size ||
            !names.insert(field.name).second) {
            throw PassPlanError("pass " + std::string(pass_id) +
                                " has an invalid uniform field layout");
        }
        preceding_end = static_cast<std::uint64_t>(field.offset) + field.size;
    }
}

std::uint32_t vertex_format_size(VertexFormat format) {
    switch (format) {
        case VertexFormat::float32:
        case VertexFormat::uint32:
            return 4;
        case VertexFormat::float32x2:
            return 8;
        case VertexFormat::float32x3:
            return 12;
        case VertexFormat::float32x4:
            return 16;
    }
    return 0;
}

void validate_vertex_layouts(std::span<const VertexBufferLayout> layouts,
                             std::string_view pass_id) {
    std::set<std::uint32_t> slots;
    std::set<std::uint32_t> locations;
    for (const VertexBufferLayout& layout : layouts) {
        if (layout.stride == 0 || layout.attributes.empty() || !slots.insert(layout.slot).second) {
            throw PassPlanError("pass " + std::string(pass_id) +
                                " has an invalid vertex-buffer layout");
        }
        for (const VertexAttribute& attribute : layout.attributes) {
            if (attribute.semantic.empty() || !locations.insert(attribute.location).second ||
                static_cast<std::uint64_t>(attribute.offset) +
                        vertex_format_size(attribute.format) >
                    layout.stride) {
                throw PassPlanError("pass " + std::string(pass_id) +
                                    " has an invalid vertex attribute layout");
            }
        }
    }
}

std::pair<std::uint32_t, std::uint32_t> target_dimensions(const LogicalTexture& texture,
                                                          const TextureSubresourceRange& range) {
    const std::uint32_t mip_width = mip_dimension(texture.extent.width, range.first_mip);
    const std::uint32_t mip_height = mip_dimension(texture.extent.height, range.first_mip);
    if (!range.tiles) {
        return {mip_width, mip_height};
    }
    const std::uint64_t pixel_x =
        static_cast<std::uint64_t>(range.tiles->x) * texture.tile_shape.width;
    const std::uint64_t pixel_y =
        static_cast<std::uint64_t>(range.tiles->y) * texture.tile_shape.height;
    const std::uint64_t requested_width =
        static_cast<std::uint64_t>(range.tiles->width) * texture.tile_shape.width;
    const std::uint64_t requested_height =
        static_cast<std::uint64_t>(range.tiles->height) * texture.tile_shape.height;
    return {
        static_cast<std::uint32_t>(std::min<std::uint64_t>(requested_width, mip_width - pixel_x)),
        static_cast<std::uint32_t>(
            std::min<std::uint64_t>(requested_height, mip_height - pixel_y))};
}

void validate_target_common(const PassDescriptor& pass, const LogicalTexture& texture,
                            const TextureSubresourceRange& range, TextureFormat format,
                            std::uint32_t width, std::uint32_t height, LoadAction load,
                            StoreAction store, std::string_view role) {
    if (role.empty() || range.mip_count != 1 || range.layer_count != 1 ||
        texture.format != format) {
        throw PassPlanError("pass " + pass.identifier + " has an invalid render target");
    }
    const auto [expected_width, expected_height] = target_dimensions(texture, range);
    if (width != expected_width || height != expected_height) {
        throw PassPlanError("pass " + pass.identifier + " render target size " +
                            std::to_string(width) + "x" + std::to_string(height) +
                            " differs from declared resource size " +
                            std::to_string(expected_width) + "x" + std::to_string(expected_height));
    }
    const ResourceAccess* access =
        find_covering_access(pass, texture.version, range, load == LoadAction::load, true);
    if (access == nullptr || access->subresources != range || access->load != load ||
        access->store != store) {
        throw PassPlanError("pass " + pass.identifier +
                            " render target does not match its declared access");
    }
}

void validate_render_targets(const PassDescriptor& pass,
                             const std::map<ResourceKey, const LogicalTexture*>& resources) {
    for (const RenderTarget& target : pass.render_targets) {
        const LogicalTexture& texture =
            require_resource(resources, target.resource, "render target");
        validate_subresources(texture, target.subresources, "render target");
        validate_target_common(pass, texture, target.subresources, target.format, target.width,
                               target.height, target.load, target.store, target.role);
        if (target.format == TextureFormat::depth32_float || target.blend.write_mask == 0 ||
            (target.blend.write_mask & 0xf0U) != 0 ||
            (target.load == LoadAction::clear &&
             (!std::isfinite(target.clear_colour.r) || !std::isfinite(target.clear_colour.g) ||
              !std::isfinite(target.clear_colour.b) || !std::isfinite(target.clear_colour.a)))) {
            throw PassPlanError("pass " + pass.identifier + " has invalid render-target state");
        }
    }
    if (!pass.depth_target) {
        if (pass.depth_state.test_enabled || pass.depth_state.write_enabled) {
            throw PassPlanError("pass " + pass.identifier +
                                " enables depth state without a depth target");
        }
        return;
    }
    const DepthTarget& target = *pass.depth_target;
    const LogicalTexture& texture = require_resource(resources, target.resource, "depth target");
    validate_subresources(texture, target.subresources, "depth target");
    validate_target_common(pass, texture, target.subresources, target.format, target.width,
                           target.height, target.load, target.store, target.role);
    if (target.format != TextureFormat::depth32_float || !std::isfinite(target.clear_depth) ||
        target.clear_depth < 0.0F || target.clear_depth > 1.0F) {
        throw PassPlanError("pass " + pass.identifier + " has invalid depth-target state");
    }
}

void validate_command(const PassDescriptor& pass) {
    if (pass.kind == PassKind::render) {
        const auto* draw = std::get_if<DrawCommand>(&pass.command);
        if (draw == nullptr || pass.vertex_entry_point.empty() ||
            pass.fragment_entry_point.empty() || !pass.compute_entry_point.empty() ||
            (pass.render_targets.empty() && !pass.depth_target) || draw->instance_count == 0 ||
            (draw->indexed && draw->index_count == 0) ||
            (!draw->indexed && (draw->vertex_count == 0 || draw->index_count != 0))) {
            throw PassPlanError("render pass " + pass.identifier +
                                " has incomplete entry points, targets, or draw dimensions");
        }
        return;
    }
    const auto* dispatch = std::get_if<DispatchCommand>(&pass.command);
    if (dispatch == nullptr || pass.compute_entry_point.empty() ||
        !pass.vertex_entry_point.empty() || !pass.fragment_entry_point.empty() ||
        !pass.render_targets.empty() || pass.depth_target || !pass.vertex_buffers.empty() ||
        pass.depth_state.test_enabled || pass.depth_state.write_enabled || dispatch->x == 0 ||
        dispatch->y == 0 || dispatch->z == 0) {
        throw PassPlanError("compute pass " + pass.identifier +
                            " has incompatible state or dispatch dimensions");
    }
}

void validate_bindings(const PassDescriptor& pass,
                       const std::map<ResourceKey, const LogicalTexture*>& resources) {
    std::set<BindingKey> occupied;
    for (const TextureBinding& binding : pass.texture_bindings) {
        validate_binding_slot({binding.group, binding.binding}, binding.role, occupied,
                              pass.identifier);
        validate_visibility(binding.visibility, pass.kind, pass.identifier);
        const LogicalTexture& texture =
            require_resource(resources, binding.resource, "texture binding");
        validate_subresources(texture, binding.subresources, "texture binding");
        if (binding.view_dimension == TextureViewDimension::cube &&
            (texture.extent.width != texture.extent.height || texture.extent.layers % 6U != 0 ||
             binding.subresources.first_layer % 6U != 0 ||
             binding.subresources.layer_count % 6U != 0)) {
            throw PassPlanError("pass " + pass.identifier +
                                " cube binding requires square six-layer faces");
        }
        const bool require_read = binding.kind != TextureBindingKind::storage_write;
        const bool require_write = binding.kind == TextureBindingKind::storage_write ||
                                   binding.kind == TextureBindingKind::storage_read_write;
        if (find_covering_access(pass, binding.resource, binding.subresources, require_read,
                                 require_write) == nullptr) {
            throw PassPlanError("pass " + pass.identifier +
                                " texture binding has no matching resource access");
        }
    }
    for (const SamplerBinding& binding : pass.sampler_bindings) {
        validate_binding_slot({binding.group, binding.binding}, binding.role, occupied,
                              pass.identifier);
        validate_visibility(binding.visibility, pass.kind, pass.identifier);
    }
    for (const UniformBlockLayout& block : pass.uniform_blocks) {
        validate_binding_slot({block.group, block.binding}, block.role, occupied, pass.identifier);
        validate_visibility(block.visibility, pass.kind, pass.identifier);
        validate_uniform_block(block, pass.identifier);
    }
}

void validate_pass_accesses(std::span<const PassDescriptor> passes, std::size_t pass_index,
                            const std::map<ResourceKey, const LogicalTexture*>& resources) {
    const PassDescriptor& pass = passes[pass_index];
    for (std::size_t index = 0; index < pass.accesses.size(); ++index) {
        const ResourceAccess& access = pass.accesses[index];
        const LogicalTexture& resource = require_resource(resources, access.resource, "access");
        validate_subresources(resource, access.subresources, "resource access");
        validate_access_actions(access, pass.identifier);
        if ((reads(access.mode) || access.load == LoadAction::load) &&
            !initialized_before(passes, pass_index, index, resource, access)) {
            throw PassPlanError("pass " + pass.identifier + " reads uninitialized resource " +
                                label(access.resource));
        }
        for (std::size_t prior = 0; prior < index; ++prior) {
            const ResourceAccess& other = pass.accesses[prior];
            if (other.resource == access.resource &&
                ranges_overlap(other.subresources, access.subresources) &&
                (writes(other.mode) || writes(access.mode))) {
                throw PassPlanError("pass " + pass.identifier +
                                    " declares overlapping mutable accesses; combine them");
            }
        }
    }
}

std::vector<std::set<std::string>> validate_dependencies(std::span<const PassDescriptor> passes) {
    std::map<std::string, std::size_t> positions;
    std::vector<std::set<std::string>> ancestors(passes.size());
    for (std::size_t index = 0; index < passes.size(); ++index) {
        const PassDescriptor& pass = passes[index];
        if (pass.identifier.empty() || !positions.emplace(pass.identifier, index).second) {
            throw PassPlanError("pass identifiers must be non-empty and unique");
        }
        std::set<std::string> direct;
        for (const std::string& dependency : pass.dependencies) {
            const auto found = positions.find(dependency);
            if (dependency.empty() || found == positions.end() || found->second == index ||
                !direct.insert(dependency).second) {
                throw PassPlanError("pass " + pass.identifier +
                                    " has a duplicate, forward, or unknown dependency");
            }
            ancestors[index].insert(dependency);
            ancestors[index].insert(ancestors[found->second].begin(),
                                    ancestors[found->second].end());
        }
    }
    return ancestors;
}

void validate_hazards(std::span<const PassDescriptor> passes,
                      const std::vector<std::set<std::string>>& ancestors) {
    for (std::size_t current = 0; current < passes.size(); ++current) {
        for (const ResourceAccess& access : passes[current].accesses) {
            for (std::size_t prior = 0; prior < current; ++prior) {
                for (const ResourceAccess& preceding : passes[prior].accesses) {
                    if (preceding.resource == access.resource &&
                        ranges_overlap(preceding.subresources, access.subresources) &&
                        (writes(preceding.mode) || writes(access.mode)) &&
                        !ancestors[current].contains(passes[prior].identifier)) {
                        throw PassPlanError("pass " + passes[current].identifier +
                                            " has an unsynchronized hazard after pass " +
                                            passes[prior].identifier + " on " +
                                            label(access.resource));
                    }
                }
            }
        }
    }
}

std::vector<ResourceLifetime> derive_lifetimes(std::span<const LogicalTexture> resources,
                                               std::span<const PassDescriptor> passes) {
    std::vector<ResourceLifetime> result;
    for (const LogicalTexture& resource : resources) {
        std::uint32_t first = std::numeric_limits<std::uint32_t>::max();
        std::uint32_t last = 0;
        for (std::size_t pass_index = 0; pass_index < passes.size(); ++pass_index) {
            const bool used = std::any_of(
                passes[pass_index].accesses.begin(), passes[pass_index].accesses.end(),
                [&](const ResourceAccess& access) { return access.resource == resource.version; });
            if (used) {
                first = std::min(first, static_cast<std::uint32_t>(pass_index));
                last = static_cast<std::uint32_t>(pass_index);
            }
        }
        if (first != std::numeric_limits<std::uint32_t>::max()) {
            result.push_back({resource.version, first, last});
        }
    }
    return result;
}

}  // namespace

PassPlan::PassPlan(std::string stable_identity, std::vector<LogicalTexture> resources,
                   std::vector<PassDescriptor> passes)
    : stable_identity_(std::move(stable_identity)),
      resources_(std::move(resources)),
      passes_(std::move(passes)) {
    if (stable_identity_.empty() || passes_.empty()) {
        throw PassPlanError("pass plan requires a stable identity and at least one pass");
    }
    std::map<ResourceKey, const LogicalTexture*> resource_index;
    validate_resource_declarations(resources_, resource_index);
    const std::vector<std::set<std::string>> ancestors = validate_dependencies(passes_);
    for (std::size_t index = 0; index < passes_.size(); ++index) {
        const PassDescriptor& pass = passes_[index];
        if (pass.accesses.empty()) {
            throw PassPlanError("pass " + pass.identifier + " has no resource accesses");
        }
        validate_pass_accesses(passes_, index, resource_index);
        validate_bindings(pass, resource_index);
        validate_vertex_layouts(pass.vertex_buffers, pass.identifier);
        validate_command(pass);
        if (pass.kind == PassKind::render) {
            validate_render_targets(pass, resource_index);
        }
    }
    validate_hazards(passes_, ancestors);
    lifetimes_ = derive_lifetimes(resources_, passes_);
    if (lifetimes_.size() != resources_.size()) {
        throw PassPlanError("every declared logical texture version must be used by a pass");
    }
}

}  // namespace ctex::emit
