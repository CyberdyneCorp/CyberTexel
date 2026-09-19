#include <algorithm>
#include <bit>
#include <cstdint>
#include <ctex/emit/emission_cache.hpp>
#include <map>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ctex::emit {
namespace {

class KeyWriter {
public:
    void unsigned_value(std::uint64_t value) {
        for (std::size_t shift = 0; shift < sizeof(value); ++shift) {
            bytes_.push_back(static_cast<char>((value >> (shift * 8U)) & 0xffU));
        }
    }

    template <typename Enum>
        requires std::is_enum_v<Enum>
    void enum_value(Enum value) {
        unsigned_value(static_cast<std::make_unsigned_t<std::underlying_type_t<Enum>>>(value));
    }

    void boolean(bool value) { bytes_.push_back(value ? '\1' : '\0'); }

    void text(std::string_view value) {
        unsigned_value(value.size());
        bytes_.append(value);
    }

    [[nodiscard]] std::string take() { return std::move(bytes_); }

private:
    std::string bytes_;
};

void write_features(KeyWriter& key, const DeviceFeatureSet& features) {
    key.unsigned_value(features.binding_budget);
    key.unsigned_value(features.maximum_texture_dimension);
    std::vector<TextureFormat> formats = features.supported_texture_formats;
    std::sort(formats.begin(), formats.end());
    formats.erase(std::unique(formats.begin(), formats.end()), formats.end());
    key.unsigned_value(formats.size());
    for (TextureFormat format : formats) {
        key.enum_value(format);
    }
    key.boolean(features.floating_point_filtering);
    key.boolean(features.compute_available);
}

void write_socket_value(KeyWriter& key, const graph::SocketValue& value) {
    key.unsigned_value(value.index());
    if (const auto* boolean = std::get_if<bool>(&value)) {
        key.boolean(*boolean);
    } else if (const auto* scalar = std::get_if<double>(&value)) {
        key.unsigned_value(std::bit_cast<std::uint64_t>(*scalar));
    } else if (const auto* vector = std::get_if<graph::VectorValue>(&value)) {
        key.unsigned_value(std::bit_cast<std::uint32_t>(vector->x));
        key.unsigned_value(std::bit_cast<std::uint32_t>(vector->y));
        key.unsigned_value(std::bit_cast<std::uint32_t>(vector->z));
    } else if (const auto* colour = std::get_if<graph::ColourValue>(&value)) {
        key.unsigned_value(std::bit_cast<std::uint32_t>(colour->r));
        key.unsigned_value(std::bit_cast<std::uint32_t>(colour->g));
        key.unsigned_value(std::bit_cast<std::uint32_t>(colour->b));
        key.unsigned_value(std::bit_cast<std::uint32_t>(colour->a));
    } else if (const auto* text = std::get_if<std::string>(&value)) {
        key.text(*text);
    } else if (const auto* image = std::get_if<graph::ImageValue>(&value)) {
        key.text(image->resource_id);
    }
}

void write_socket(KeyWriter& key, const graph::NodeSocket& socket) {
    key.text(socket.identifier);
    key.text(socket.display_name);
    key.enum_value(socket.type);
    write_socket_value(key, socket.value);
}

void write_sockets(KeyWriter& key, std::span<const graph::NodeSocket> sockets) {
    key.unsigned_value(sockets.size());
    for (const graph::NodeSocket& socket : sockets) {
        write_socket(key, socket);
    }
}

std::string registry_content(const graph::NodeTypeRegistry& registry) {
    KeyWriter key;
    key.unsigned_value(registry.registrations().size());
    for (const graph::HostNodeTypeRegistration& registration : registry.registrations()) {
        const graph::NodeTypeDeclaration& declaration = registration.declaration;
        key.text(declaration.type_id);
        key.unsigned_value(declaration.version);
        key.text(declaration.display_name);
        key.enum_value(declaration.category);
        write_sockets(key, declaration.inputs);
        write_sockets(key, declaration.outputs);
        key.unsigned_value(declaration.properties.size());
        for (const graph::NodePropertyDeclaration& property : declaration.properties) {
            key.text(property.identifier);
            key.text(property.display_name);
            write_socket_value(key, property.default_value);
            key.unsigned_value(property.allowed_values.size());
            for (const std::string& allowed : property.allowed_values) {
                key.text(allowed);
            }
        }
        key.boolean(registration.deterministic);
        key.unsigned_value(registration.resource_dependencies.size());
        for (const graph::NodeResourceDependency& dependency : registration.resource_dependencies) {
            key.text(dependency.identifier);
        }
        key.unsigned_value(registration.supported_targets.size());
        for (graph::EmissionTarget target : registration.supported_targets) {
            key.enum_value(target);
        }
        key.unsigned_value(registration.parity_fixtures.size());
        for (const graph::NodeParityFixture& fixture : registration.parity_fixtures) {
            key.text(fixture.identifier);
            key.unsigned_value(fixture.inputs.size());
            for (const graph::SocketValue& fixture_input : fixture.inputs) {
                write_socket_value(key, fixture_input);
            }
            key.unsigned_value(fixture.expected_outputs.size());
            for (const graph::SocketValue& expected : fixture.expected_outputs) {
                write_socket_value(key, expected);
            }
            key.unsigned_value(std::bit_cast<std::uint64_t>(fixture.tolerance));
        }
    }
    return key.take();
}

std::string graph_content(const graph::GraphDocument& graph,
                          const graph::NodeTypeRegistry& registry) {
    KeyWriter key;
    key.text(graph::serialize_graph(graph));
    key.text(registry_content(registry));
    return key.take();
}

std::string cache_key(std::string_view content, ShaderTarget target,
                      const DeviceFeatureSet& features) {
    KeyWriter key;
    key.text(content);
    key.enum_value(target);
    write_features(key, features);
    return key.take();
}

void write_resource_version(KeyWriter& key, const ResourceVersion& version) {
    key.text(version.logical_id);
    key.unsigned_value(version.generation);
}

void write_texture(KeyWriter& key, const LogicalTexture& texture) {
    write_resource_version(key, texture.version);
    key.text(texture.role);
    key.enum_value(texture.format);
    key.unsigned_value(texture.extent.width);
    key.unsigned_value(texture.extent.height);
    key.unsigned_value(texture.extent.layers);
    key.unsigned_value(texture.mip_levels);
    key.unsigned_value(texture.tile_shape.width);
    key.unsigned_value(texture.tile_shape.height);
    key.boolean(texture.externally_initialized);
}

std::string layer_stack_content(const LayerStackEmissionRequest& request) {
    KeyWriter key;
    key.text(request.stable_identity);
    key.unsigned_value(request.layers.size());
    for (const LayerStackInput& layer : request.layers) {
        key.text(layer.identifier);
        write_texture(key, layer.texture);
    }
    write_texture(key, request.output);
    key.enum_value(request.requested_filter);
    return key.take();
}

std::string material_request_content(const MaterialShaderEmissionRequest& request) {
    KeyWriter key;
    key.text(request.stable_identity);
    key.unsigned_value(request.resources.size());
    for (const MaterialResourceInput& resource : request.resources) {
        key.text(resource.identifier);
        write_texture(key, resource.texture);
    }
    write_texture(key, request.output);
    key.enum_value(request.requested_filter);
    key.unsigned_value(request.vertex_count);
    return key.take();
}

std::string material_cache_content(std::string_view graph,
                                   const MaterialShaderEmissionRequest& request) {
    KeyWriter key;
    key.text(graph);
    key.text(material_request_content(request));
    return key.take();
}

std::string preview_content(const PreviewEmissionRequest& request, std::string_view mode,
                            std::string_view inspected_channel = {}) {
    KeyWriter key;
    key.text(mode);
    key.text(inspected_channel);
    key.text(request.stable_identity);
    key.unsigned_value(request.channels.size());
    for (const PreviewChannelInput& channel : request.channels) {
        key.text(channel.semantic_id);
        key.unsigned_value(channel.component_count);
        write_texture(key, channel.texture);
    }
    write_texture(key, request.output);
    key.boolean(request.environment.has_value());
    if (request.environment) {
        write_texture(key, request.environment->radiance);
        write_texture(key, request.environment->diffuse_irradiance);
        write_texture(key, request.environment->specular_brdf_lookup);
    }
    key.unsigned_value(request.analytic_light_count);
    key.unsigned_value(request.vertex_count);
    return key.take();
}

std::string workspace_content(const graph::GraphWorkspace& workspace,
                              std::string_view material_identifier,
                              const graph::NodeTypeRegistry& registry) {
    KeyWriter key;
    key.text(material_identifier);
    key.text(graph::serialize_graph(workspace.material(material_identifier)));
    key.unsigned_value(workspace.groups().size());
    for (const graph::NodeGroupDefinition& group : workspace.groups()) {
        key.text(group.identifier);
        key.text(group.display_name);
        key.unsigned_value(group.version);
        write_sockets(key, group.inputs);
        write_sockets(key, group.outputs);
        key.unsigned_value(group.input_node_id);
        key.text(graph::serialize_graph(group.graph));
    }
    key.text(registry_content(registry));
    return key.take();
}

template <typename Value>
class ConcurrentCache {
public:
    template <typename Producer>
    std::pair<std::shared_ptr<const Value>, bool> get(std::string key, Producer&& producer) {
        {
            const std::scoped_lock lock(mutex_);
            if (const auto found = entries_.find(key); found != entries_.end()) {
                ++hits_;
                return {found->second, true};
            }
            ++misses_;
        }

        auto produced = std::make_shared<const Value>(std::forward<Producer>(producer)());
        const std::scoped_lock lock(mutex_);
        const auto [position, inserted] = entries_.emplace(std::move(key), produced);
        return {inserted ? std::move(produced) : position->second, false};
    }

    [[nodiscard]] EmissionCacheStatistics statistics() const {
        const std::scoped_lock lock(mutex_);
        return {entries_.size(), hits_, misses_};
    }

    void clear() {
        const std::scoped_lock lock(mutex_);
        entries_.clear();
        hits_ = 0;
        misses_ = 0;
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<const Value>> entries_;
    std::size_t hits_{};
    std::size_t misses_{};
};

void require_graph_target(ShaderTarget target) {
    if (target != ShaderTarget::wgsl) {
        throw GraphEmissionError("graph expression emission currently supports only WGSL, not " +
                                 std::string(shader_target_name(target)));
    }
}

}  // namespace

class GraphEmissionCache::Impl {
public:
    ConcurrentCache<WgslExpressionProgram> cache;
};

GraphEmissionCache::GraphEmissionCache() : impl_(std::make_unique<Impl>()) {}
GraphEmissionCache::~GraphEmissionCache() = default;
GraphEmissionCache::GraphEmissionCache(GraphEmissionCache&&) noexcept = default;
GraphEmissionCache& GraphEmissionCache::operator=(GraphEmissionCache&&) noexcept = default;

CachedGraphEmission GraphEmissionCache::emit(const graph::GraphDocument& graph,
                                             const graph::NodeTypeRegistry& registry,
                                             ShaderTarget target,
                                             const DeviceFeatureSet& features) {
    if (impl_ == nullptr) {
        throw std::logic_error("cannot use a moved-from graph emission cache");
    }
    require_graph_target(target);
    auto [emission, hit] =
        impl_->cache.get(cache_key(graph_content(graph, registry), target, features),
                         [&] { return emit_wgsl_expressions(graph, registry); });
    return {std::move(emission), hit};
}

CachedGraphEmission GraphEmissionCache::emit(const graph::GraphDocument& graph, ShaderTarget target,
                                             const DeviceFeatureSet& features) {
    const graph::NodeTypeRegistry registry;
    return emit(graph, registry, target, features);
}

CachedGraphEmission GraphEmissionCache::emit_material(const graph::GraphWorkspace& workspace,
                                                      std::string_view material_identifier,
                                                      const graph::NodeTypeRegistry& registry,
                                                      ShaderTarget target,
                                                      const DeviceFeatureSet& features) {
    if (impl_ == nullptr) {
        throw std::logic_error("cannot use a moved-from graph emission cache");
    }
    require_graph_target(target);
    auto [emission, hit] = impl_->cache.get(
        cache_key(workspace_content(workspace, material_identifier, registry), target, features),
        [&] { return emit_material_wgsl_expressions(workspace, material_identifier, registry); });
    return {std::move(emission), hit};
}

CachedGraphEmission GraphEmissionCache::emit_material(const graph::GraphWorkspace& workspace,
                                                      std::string_view material_identifier,
                                                      ShaderTarget target,
                                                      const DeviceFeatureSet& features) {
    const graph::NodeTypeRegistry registry;
    return emit_material(workspace, material_identifier, registry, target, features);
}

EmissionCacheStatistics GraphEmissionCache::statistics() const {
    if (impl_ == nullptr) {
        return {};
    }
    return impl_->cache.statistics();
}

void GraphEmissionCache::clear() {
    if (impl_ != nullptr) {
        impl_->cache.clear();
    }
}

class LayerStackEmissionCache::Impl {
public:
    ConcurrentCache<LayerStackEmission> cache;
};

LayerStackEmissionCache::LayerStackEmissionCache() : impl_(std::make_unique<Impl>()) {}
LayerStackEmissionCache::~LayerStackEmissionCache() = default;
LayerStackEmissionCache::LayerStackEmissionCache(LayerStackEmissionCache&&) noexcept = default;
LayerStackEmissionCache& LayerStackEmissionCache::operator=(LayerStackEmissionCache&&) noexcept =
    default;

CachedLayerStackEmission LayerStackEmissionCache::emit(const LayerStackEmissionRequest& request) {
    if (impl_ == nullptr) {
        throw std::logic_error("cannot use a moved-from layer-stack emission cache");
    }
    auto [emission, hit] =
        impl_->cache.get(cache_key(layer_stack_content(request), request.target, request.features),
                         [&] { return emit_layer_stack(request); });
    return {std::move(emission), hit};
}

EmissionCacheStatistics LayerStackEmissionCache::statistics() const {
    if (impl_ == nullptr) {
        return {};
    }
    return impl_->cache.statistics();
}

void LayerStackEmissionCache::clear() {
    if (impl_ != nullptr) {
        impl_->cache.clear();
    }
}

class MaterialShaderEmissionCache::Impl {
public:
    ConcurrentCache<MaterialShaderEmission> cache;
};

MaterialShaderEmissionCache::MaterialShaderEmissionCache() : impl_(std::make_unique<Impl>()) {}
MaterialShaderEmissionCache::~MaterialShaderEmissionCache() = default;
MaterialShaderEmissionCache::MaterialShaderEmissionCache(MaterialShaderEmissionCache&&) noexcept =
    default;
MaterialShaderEmissionCache& MaterialShaderEmissionCache::operator=(
    MaterialShaderEmissionCache&&) noexcept = default;

CachedMaterialShaderEmission MaterialShaderEmissionCache::emit(
    const graph::GraphDocument& graph, const graph::NodeTypeRegistry& registry,
    const MaterialShaderEmissionRequest& request) {
    if (impl_ == nullptr) {
        throw std::logic_error("cannot use a moved-from material shader emission cache");
    }
    auto [emission, hit] =
        impl_->cache.get(cache_key(material_cache_content(graph_content(graph, registry), request),
                                   request.target, request.features),
                         [&] { return emit_material_shader(graph, registry, request); });
    return {std::move(emission), hit};
}

CachedMaterialShaderEmission MaterialShaderEmissionCache::emit(
    const graph::GraphDocument& graph, const MaterialShaderEmissionRequest& request) {
    const graph::NodeTypeRegistry registry;
    return emit(graph, registry, request);
}

CachedMaterialShaderEmission MaterialShaderEmissionCache::emit_material(
    const graph::GraphWorkspace& workspace, std::string_view material_identifier,
    const graph::NodeTypeRegistry& registry, const MaterialShaderEmissionRequest& request) {
    if (impl_ == nullptr) {
        throw std::logic_error("cannot use a moved-from material shader emission cache");
    }
    auto [emission, hit] = impl_->cache.get(
        cache_key(material_cache_content(
                      workspace_content(workspace, material_identifier, registry), request),
                  request.target, request.features),
        [&] {
            return emit_workspace_material_shader(workspace, material_identifier, registry,
                                                  request);
        });
    return {std::move(emission), hit};
}

CachedMaterialShaderEmission MaterialShaderEmissionCache::emit_material(
    const graph::GraphWorkspace& workspace, std::string_view material_identifier,
    const MaterialShaderEmissionRequest& request) {
    const graph::NodeTypeRegistry registry;
    return emit_material(workspace, material_identifier, registry, request);
}

EmissionCacheStatistics MaterialShaderEmissionCache::statistics() const {
    if (impl_ == nullptr) {
        return {};
    }
    return impl_->cache.statistics();
}

void MaterialShaderEmissionCache::clear() {
    if (impl_ != nullptr) {
        impl_->cache.clear();
    }
}

class PreviewEmissionCache::Impl {
public:
    ConcurrentCache<PreviewEmission> cache;
};

PreviewEmissionCache::PreviewEmissionCache() : impl_(std::make_unique<Impl>()) {}
PreviewEmissionCache::~PreviewEmissionCache() = default;
PreviewEmissionCache::PreviewEmissionCache(PreviewEmissionCache&&) noexcept = default;
PreviewEmissionCache& PreviewEmissionCache::operator=(PreviewEmissionCache&&) noexcept = default;

CachedPreviewEmission PreviewEmissionCache::emit_lit(const PreviewEmissionRequest& request) {
    if (impl_ == nullptr) {
        throw std::logic_error("cannot use a moved-from preview emission cache");
    }
    auto [emission, hit] = impl_->cache.get(
        cache_key(preview_content(request, "lit"), request.target, request.features),
        [&request] { return emit_lit_preview(request); });
    return {std::move(emission), hit};
}

CachedPreviewEmission PreviewEmissionCache::emit_inspection(const PreviewEmissionRequest& request,
                                                            std::string_view semantic_id) {
    if (impl_ == nullptr) {
        throw std::logic_error("cannot use a moved-from preview emission cache");
    }
    auto [emission, hit] = impl_->cache.get(
        cache_key(preview_content(request, "inspection", semantic_id), request.target,
                  request.features),
        [&request, semantic_id] { return emit_channel_inspection(request, semantic_id); });
    return {std::move(emission), hit};
}

EmissionCacheStatistics PreviewEmissionCache::statistics() const {
    if (impl_ == nullptr) {
        return {};
    }
    return impl_->cache.statistics();
}

void PreviewEmissionCache::clear() {
    if (impl_ != nullptr) {
        impl_->cache.clear();
    }
}

}  // namespace ctex::emit
