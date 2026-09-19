#ifndef CTEX_EMIT_EMISSION_CACHE_HPP
#define CTEX_EMIT_EMISSION_CACHE_HPP

#include <cstddef>
#include <ctex/emit/feature_emission.hpp>
#include <ctex/emit/graph_emission.hpp>
#include <memory>
#include <string_view>

namespace ctex::emit {

struct EmissionCacheStatistics {
    std::size_t entries{};
    std::size_t hits{};
    std::size_t misses{};
    friend bool operator==(EmissionCacheStatistics, EmissionCacheStatistics) noexcept = default;
};

struct CachedGraphEmission {
    std::shared_ptr<const WgslExpressionProgram> emission;
    bool cache_hit{};
};

struct CachedLayerStackEmission {
    std::shared_ptr<const LayerStackEmission> emission;
    bool cache_hit{};
};

class GraphEmissionCache {
public:
    GraphEmissionCache();
    ~GraphEmissionCache();
    GraphEmissionCache(GraphEmissionCache&&) noexcept;
    GraphEmissionCache& operator=(GraphEmissionCache&&) noexcept;
    GraphEmissionCache(const GraphEmissionCache&) = delete;
    GraphEmissionCache& operator=(const GraphEmissionCache&) = delete;

    [[nodiscard]] CachedGraphEmission emit(const graph::GraphDocument& graph,
                                           const graph::NodeTypeRegistry& registry,
                                           ShaderTarget target, const DeviceFeatureSet& features);
    [[nodiscard]] CachedGraphEmission emit(const graph::GraphDocument& graph, ShaderTarget target,
                                           const DeviceFeatureSet& features);
    [[nodiscard]] CachedGraphEmission emit_material(const graph::GraphWorkspace& workspace,
                                                    std::string_view material_identifier,
                                                    const graph::NodeTypeRegistry& registry,
                                                    ShaderTarget target,
                                                    const DeviceFeatureSet& features);
    [[nodiscard]] CachedGraphEmission emit_material(const graph::GraphWorkspace& workspace,
                                                    std::string_view material_identifier,
                                                    ShaderTarget target,
                                                    const DeviceFeatureSet& features);

    [[nodiscard]] EmissionCacheStatistics statistics() const;
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class LayerStackEmissionCache {
public:
    LayerStackEmissionCache();
    ~LayerStackEmissionCache();
    LayerStackEmissionCache(LayerStackEmissionCache&&) noexcept;
    LayerStackEmissionCache& operator=(LayerStackEmissionCache&&) noexcept;
    LayerStackEmissionCache(const LayerStackEmissionCache&) = delete;
    LayerStackEmissionCache& operator=(const LayerStackEmissionCache&) = delete;

    [[nodiscard]] CachedLayerStackEmission emit(const LayerStackEmissionRequest& request);
    [[nodiscard]] EmissionCacheStatistics statistics() const;
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ctex::emit

#endif
