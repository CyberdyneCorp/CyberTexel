#ifndef CTEX_PAINT_SURFACE_CACHE_HPP
#define CTEX_PAINT_SURFACE_CACHE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <ctex/paint/coverage.hpp>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

inline constexpr std::uint32_t no_uv_island = std::numeric_limits<std::uint32_t>::max();

struct SurfaceMapRequest {
    std::size_t partition_index{};
    std::string_view uv_set;
    TextureSpaceRasterRequest raster;
};

struct SurfaceMapStatistics {
    std::size_t entries{};
    std::size_t hits{};
    std::size_t misses{};
    std::size_t invalidated_entries{};
    friend bool operator==(SurfaceMapStatistics, SurfaceMapStatistics) noexcept = default;
};

struct CachedSurfaceMaps {
    std::string texture_set_id;
    std::string uv_set;
    mesh::MeshRevision mesh_revision{};
    TextureSpaceRaster surface;
    std::vector<std::uint8_t> coverage;
    std::vector<std::uint32_t> triangle_identity;
    std::vector<std::uint32_t> uv_island_identity;
};

struct SurfaceMapLookup {
    std::shared_ptr<const CachedSurfaceMaps> maps;
    bool cache_hit{};
};

class SurfaceMapCache {
public:
    SurfaceMapCache();
    ~SurfaceMapCache();
    SurfaceMapCache(SurfaceMapCache&&) noexcept;
    SurfaceMapCache& operator=(SurfaceMapCache&&) noexcept;
    SurfaceMapCache(const SurfaceMapCache&) = delete;
    SurfaceMapCache& operator=(const SurfaceMapCache&) = delete;

    [[nodiscard]] SurfaceMapLookup lookup(const mesh::MeshBinding& mesh,
                                          const SurfaceMapRequest& request);
    [[nodiscard]] SurfaceMapStatistics statistics() const;
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ctex::paint

#endif
