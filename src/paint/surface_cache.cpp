#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <compare>
#include <ctex/paint/surface_cache.hpp>
#include <map>
#include <mutex>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace ctex::paint {
namespace {

struct CacheKey {
    mesh::PartitionKind partition_kind;
    std::string partition_key;
    std::string uv_set;
    std::uint32_t width;
    std::uint32_t height;
    std::uint64_t tile_x;
    std::uint64_t tile_y;
    auto operator<=>(const CacheKey&) const = default;
};

struct SourceData {
    std::string texture_set_id;
    std::vector<Vec3d> positions;
    std::vector<Vec3d> normals;
    std::vector<Vec2d> uv;
    std::vector<std::uint32_t> indices;
    std::vector<std::uint32_t> source_triangles;
};

std::uint64_t coordinate_bits(double value) {
    return std::bit_cast<std::uint64_t>(value == 0.0 ? 0.0 : value);
}

CacheKey cache_key(const mesh::MeshBinding& binding, const SurfaceMapRequest& request) {
    const mesh::MeshDescriptor& descriptor = binding.view().descriptor();
    if (request.partition_index >= descriptor.partitions.size()) {
        throw std::out_of_range("surface-map partition index is outside the mesh partitions");
    }
    static_cast<void>(binding.view().uv_set(request.uv_set));
    if (request.raster.width == 0 || request.raster.height == 0 ||
        !std::isfinite(request.raster.tile_origin.x) ||
        !std::isfinite(request.raster.tile_origin.y) ||
        static_cast<std::size_t>(request.raster.width) >
            std::numeric_limits<std::size_t>::max() / request.raster.height) {
        throw std::invalid_argument("surface-map raster request is invalid");
    }
    if (std::find(descriptor.face_partition_indices.begin(),
                  descriptor.face_partition_indices.end(),
                  request.partition_index) == descriptor.face_partition_indices.end()) {
        throw std::invalid_argument("surface-map texture set contains no faces");
    }
    const mesh::MeshPartition& partition = descriptor.partitions[request.partition_index];
    return {.partition_kind = partition.kind,
            .partition_key = std::string(partition.stable_key),
            .uv_set = std::string(request.uv_set),
            .width = request.raster.width,
            .height = request.raster.height,
            .tile_x = coordinate_bits(request.raster.tile_origin.x),
            .tile_y = coordinate_bits(request.raster.tile_origin.y)};
}

SourceData collect_source(const mesh::MeshBinding& binding, const SurfaceMapRequest& request) {
    const mesh::MeshDescriptor& descriptor = binding.view().descriptor();
    const mesh::MeshPartition& partition = descriptor.partitions[request.partition_index];
    const mesh::UvSetView& uv_set = binding.view().uv_set(request.uv_set);
    SourceData source{.texture_set_id = mesh::texture_set_stable_id(
                          partition.kind, partition.stable_key, request.uv_set),
                      .positions = {},
                      .normals = {},
                      .uv = {},
                      .indices = {},
                      .source_triangles = {}};
    source.positions.reserve(descriptor.positions.size());
    source.normals.reserve(descriptor.normals.size());
    source.uv.reserve(uv_set.values.size());
    for (const mesh::Vec3f value : descriptor.positions) {
        source.positions.push_back({value.x, value.y, value.z});
    }
    for (const mesh::Vec3f value : descriptor.normals) {
        source.normals.push_back({value.x, value.y, value.z});
    }
    for (const mesh::Vec2f value : uv_set.values) {
        source.uv.push_back({value.x, value.y});
    }
    for (std::size_t triangle = 0; triangle < binding.view().triangle_count(); ++triangle) {
        if (descriptor.face_partition_indices[triangle] != request.partition_index) {
            continue;
        }
        source.source_triangles.push_back(static_cast<std::uint32_t>(triangle));
        const std::size_t first = triangle * 3;
        source.indices.insert(source.indices.end(), descriptor.triangle_indices.begin() + first,
                              descriptor.triangle_indices.begin() + first + 3);
    }
    return source;
}

struct EndpointKey {
    std::array<std::uint32_t, 5> components;
    auto operator<=>(const EndpointKey&) const = default;
};

struct EdgeKey {
    EndpointKey first;
    EndpointKey second;
    auto operator<=>(const EdgeKey&) const = default;
};

std::uint32_t component_bits(float value) {
    return std::bit_cast<std::uint32_t>(value == 0.0F ? 0.0F : value);
}

EndpointKey endpoint_key(const mesh::MeshDescriptor& descriptor, const mesh::UvSetView& uv_set,
                         std::uint32_t vertex) {
    const mesh::Vec3f position = descriptor.positions[vertex];
    const mesh::Vec2f uv = uv_set.values[vertex];
    return {{component_bits(position.x), component_bits(position.y), component_bits(position.z),
             component_bits(uv.x), component_bits(uv.y)}};
}

EdgeKey edge_key(const mesh::MeshDescriptor& descriptor, const mesh::UvSetView& uv_set,
                 std::uint32_t first, std::uint32_t second) {
    EndpointKey a = endpoint_key(descriptor, uv_set, first);
    EndpointKey b = endpoint_key(descriptor, uv_set, second);
    if (b < a) {
        std::swap(a, b);
    }
    return {a, b};
}

class DisjointSet {
public:
    explicit DisjointSet(std::size_t count) : parents_(count) {
        for (std::size_t index = 0; index < count; ++index) {
            parents_[index] = index;
        }
    }

    std::size_t root(std::size_t value) {
        while (parents_[value] != value) {
            parents_[value] = parents_[parents_[value]];
            value = parents_[value];
        }
        return value;
    }

    void unite(std::size_t first, std::size_t second) {
        first = root(first);
        second = root(second);
        if (first != second) {
            parents_[std::max(first, second)] = std::min(first, second);
        }
    }

private:
    std::vector<std::size_t> parents_;
};

std::vector<std::uint32_t> triangle_islands(const mesh::MeshDescriptor& descriptor,
                                            const mesh::UvSetView& uv_set,
                                            const SourceData& source) {
    DisjointSet sets(source.source_triangles.size());
    std::map<EdgeKey, std::size_t> edge_owners;
    for (std::size_t triangle = 0; triangle < source.source_triangles.size(); ++triangle) {
        const std::array<std::uint32_t, 3> vertices{source.indices[triangle * 3],
                                                    source.indices[triangle * 3 + 1],
                                                    source.indices[triangle * 3 + 2]};
        for (std::size_t edge = 0; edge < 3; ++edge) {
            const EdgeKey key =
                edge_key(descriptor, uv_set, vertices[edge], vertices[(edge + 1) % 3]);
            if (key.first == key.second) {
                continue;
            }
            const auto [owner, inserted] = edge_owners.emplace(key, triangle);
            if (!inserted) {
                sets.unite(owner->second, triangle);
            }
        }
    }
    std::map<std::size_t, std::uint32_t> island_by_root;
    std::vector<std::uint32_t> islands(source.source_triangles.size());
    for (std::size_t triangle = 0; triangle < islands.size(); ++triangle) {
        const std::size_t root = sets.root(triangle);
        const auto [island, inserted] =
            island_by_root.emplace(root, static_cast<std::uint32_t>(island_by_root.size()));
        static_cast<void>(inserted);
        islands[triangle] = island->second;
    }
    return islands;
}

CachedSurfaceMaps build_maps(const mesh::MeshBinding& binding, const SurfaceMapRequest& request,
                             SourceData source) {
    const mesh::UvSetView& uv_set = binding.view().uv_set(request.uv_set);
    const auto islands = triangle_islands(binding.view().descriptor(), uv_set, source);
    TextureSpaceRaster surface = rasterize_texture_space(
        {source.positions, source.normals, source.uv, source.indices}, request.raster);
    const std::size_t texel_count = surface.texels.size();
    CachedSurfaceMaps maps{
        .texture_set_id = std::move(source.texture_set_id),
        .uv_set = std::string(request.uv_set),
        .mesh_revision = binding.revision(),
        .surface = std::move(surface),
        .coverage = std::vector<std::uint8_t>(texel_count, 0),
        .triangle_identity = std::vector<std::uint32_t>(texel_count, no_surface_triangle),
        .uv_island_identity = std::vector<std::uint32_t>(texel_count, no_uv_island)};
    for (std::size_t texel = 0; texel < texel_count; ++texel) {
        const std::uint32_t local_triangle = maps.surface.texels[texel].triangle;
        if (local_triangle == no_surface_triangle) {
            continue;
        }
        maps.coverage[texel] = 1;
        maps.triangle_identity[texel] = source.source_triangles[local_triangle];
        maps.uv_island_identity[texel] = islands[local_triangle];
        maps.surface.texels[texel].triangle = source.source_triangles[local_triangle];
    }
    return maps;
}

bool same_partition(const CacheKey& first, const CacheKey& second) {
    return first.partition_kind == second.partition_kind &&
           first.partition_key == second.partition_key;
}

}  // namespace

class SurfaceMapCache::Impl {
public:
    SurfaceMapLookup lookup(const mesh::MeshBinding& binding, const SurfaceMapRequest& request) {
        const CacheKey key = cache_key(binding, request);
        const std::scoped_lock lock(mutex_);
        synchronize_mesh(binding.revision());
        invalidate_changed_uv(key);
        if (const auto found = entries_.find(key); found != entries_.end()) {
            ++statistics_.hits;
            return {.maps = found->second, .cache_hit = true};
        }
        ++statistics_.misses;
        SourceData source = collect_source(binding, request);
        auto maps = std::make_shared<const CachedSurfaceMaps>(
            build_maps(binding, request, std::move(source)));
        entries_.emplace(key, maps);
        statistics_.entries = entries_.size();
        return {.maps = std::move(maps), .cache_hit = false};
    }

    SurfaceMapStatistics statistics() const {
        const std::scoped_lock lock(mutex_);
        return statistics_;
    }

    void clear() {
        const std::scoped_lock lock(mutex_);
        entries_.clear();
        mesh_revision_ = 0;
        statistics_ = {};
    }

private:
    void synchronize_mesh(mesh::MeshRevision revision) {
        if (mesh_revision_ != 0 && mesh_revision_ != revision) {
            statistics_.invalidated_entries += entries_.size();
            entries_.clear();
            statistics_.entries = 0;
        }
        mesh_revision_ = revision;
    }

    void invalidate_changed_uv(const CacheKey& key) {
        for (auto entry = entries_.begin(); entry != entries_.end();) {
            if (same_partition(entry->first, key) && entry->first.uv_set != key.uv_set) {
                entry = entries_.erase(entry);
                ++statistics_.invalidated_entries;
            } else {
                ++entry;
            }
        }
        statistics_.entries = entries_.size();
    }

    mutable std::mutex mutex_;
    mesh::MeshRevision mesh_revision_{};
    std::map<CacheKey, std::shared_ptr<const CachedSurfaceMaps>> entries_;
    SurfaceMapStatistics statistics_;
};

SurfaceMapCache::SurfaceMapCache() : impl_(std::make_unique<Impl>()) {}
SurfaceMapCache::~SurfaceMapCache() = default;
SurfaceMapCache::SurfaceMapCache(SurfaceMapCache&&) noexcept = default;
SurfaceMapCache& SurfaceMapCache::operator=(SurfaceMapCache&&) noexcept = default;

SurfaceMapLookup SurfaceMapCache::lookup(const mesh::MeshBinding& mesh,
                                         const SurfaceMapRequest& request) {
    return impl_->lookup(mesh, request);
}

SurfaceMapStatistics SurfaceMapCache::statistics() const { return impl_->statistics(); }

void SurfaceMapCache::clear() { impl_->clear(); }

}  // namespace ctex::paint
