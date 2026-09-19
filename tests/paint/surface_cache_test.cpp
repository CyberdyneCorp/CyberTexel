#include <algorithm>
#include <array>
#include <ctex/paint/surface_cache.hpp>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

struct MeshFixture {
    std::vector<mesh::Vec3f> positions{
        {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
        {1.0F, 1.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 1.0F},
        {1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 2.0F}, {1.0F, 0.0F, 2.0F}, {0.0F, 1.0F, 2.0F},
    };
    std::vector<mesh::Vec3f> normals = std::vector<mesh::Vec3f>(12, {0.0F, 0.0F, 1.0F});
    std::vector<mesh::Vec2f> primary{
        {0.0F, 0.0F}, {0.5F, 0.0F}, {0.0F, 1.0F}, {0.5F, 0.0F}, {0.5F, 1.0F}, {0.0F, 1.0F},
        {0.5F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F},
    };
    std::vector<mesh::Vec2f> alternate{
        {0.0F, 0.0F}, {0.0F, 0.5F}, {1.0F, 0.0F}, {0.0F, 0.5F}, {1.0F, 0.5F}, {1.0F, 0.0F},
        {0.0F, 0.5F}, {0.0F, 1.0F}, {1.0F, 1.0F}, {0.0F, 0.0F}, {0.0F, 1.0F}, {1.0F, 0.0F},
    };
    std::vector<std::uint32_t> indices{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    std::array<mesh::UvSetView, 2> uv_sets;
    std::array<mesh::MeshPartition, 2> partitions{
        mesh::MeshPartition{mesh::PartitionKind::material, "body", "Body"},
        mesh::MeshPartition{mesh::PartitionKind::material, "trim", "Trim"}};
    std::array<std::uint32_t, 4> face_partitions{0, 0, 0, 1};
    std::array<std::uint32_t, 4> materials{7, 7, 7, 9};

    MeshFixture()
        : uv_sets{mesh::UvSetView{"primary", primary}, mesh::UvSetView{"alternate", alternate}} {}

    mesh::MeshDescriptor descriptor() const {
        return {.positions = positions,
                .normals = normals,
                .vertex_colors = {},
                .triangle_indices = indices,
                .uv_sets = uv_sets,
                .default_uv_set = "primary",
                .partitions = partitions,
                .face_partition_indices = face_partitions,
                .face_material_ids = materials};
    }
};

paint::SurfaceMapRequest request(std::size_t partition = 0, std::string_view uv_set = "primary") {
    return {.partition_index = partition,
            .uv_set = uv_set,
            .raster = {.width = 16, .height = 8, .tile_origin = {0.0, 0.0}}};
}

bool maps_have_exact_source_identities_and_uv_islands() {
    MeshFixture fixture;
    mesh::MeshBinding binding(fixture.descriptor());
    paint::SurfaceMapCache cache;
    const auto lookup = cache.lookup(binding, request());
    const auto& maps = *lookup.maps;
    bool found_first = false;
    bool found_second = false;
    bool found_disconnected = false;
    std::uint32_t connected_island = paint::no_uv_island;
    std::uint32_t disconnected_island = paint::no_uv_island;
    bool maps_agree = maps.coverage.size() == maps.surface.texels.size() &&
                      maps.triangle_identity.size() == maps.coverage.size() &&
                      maps.uv_island_identity.size() == maps.coverage.size();
    for (std::size_t texel = 0; texel < maps.coverage.size(); ++texel) {
        const std::uint32_t triangle = maps.triangle_identity[texel];
        maps_agree &= (maps.coverage[texel] != 0) == (triangle != paint::no_surface_triangle);
        maps_agree &= maps.surface.texels[texel].triangle == triangle;
        maps_agree &= (triangle == paint::no_surface_triangle) ==
                      (maps.uv_island_identity[texel] == paint::no_uv_island);
        if (triangle == 0 || triangle == 1) {
            found_first |= triangle == 0;
            found_second |= triangle == 1;
            if (connected_island == paint::no_uv_island) {
                connected_island = maps.uv_island_identity[texel];
            }
            maps_agree &= connected_island == maps.uv_island_identity[texel];
        } else if (triangle == 2) {
            found_disconnected = true;
            disconnected_island = maps.uv_island_identity[texel];
        }
        maps_agree &= triangle != 3;
    }
    return expect(!lookup.cache_hit, "first surface-map lookup was reported as a hit") &&
           expect(maps.texture_set_id ==
                      mesh::texture_set_stable_id(mesh::PartitionKind::material, "body", "primary"),
                  "cache did not retain the texture-set stable identity") &&
           expect(maps.mesh_revision == binding.revision() && maps.uv_set == "primary",
                  "cache bundle did not retain its revision and UV identity") &&
           expect(maps_agree && found_first && found_second && found_disconnected,
                  "coverage, source triangle and island maps disagree") &&
           expect(connected_island != disconnected_island,
                  "disconnected geometry sharing a UV edge was merged into one island");
}

bool face_and_island_operations_reuse_one_bundle() {
    MeshFixture fixture;
    mesh::MeshBinding binding(fixture.descriptor());
    paint::SurfaceMapCache cache;
    const auto face_fill = cache.lookup(binding, request());
    const auto island_fill = cache.lookup(binding, request());
    return expect(!face_fill.cache_hit && island_fill.cache_hit,
                  "second operation did not report a surface-map cache hit") &&
           expect(face_fill.maps == island_fill.maps,
                  "face and island operations did not share the immutable map bundle") &&
           expect(cache.statistics() ==
                      paint::SurfaceMapStatistics{.entries = 1, .hits = 1, .misses = 1},
                  "surface-map reuse statistics are incorrect");
}

bool texture_sets_are_cached_independently() {
    MeshFixture fixture;
    mesh::MeshBinding binding(fixture.descriptor());
    paint::SurfaceMapCache cache;
    const auto body = cache.lookup(binding, request(0));
    const auto trim = cache.lookup(binding, request(1));
    const bool trim_is_source_triangle_three =
        std::all_of(trim.maps->triangle_identity.begin(), trim.maps->triangle_identity.end(),
                    [](std::uint32_t triangle) {
                        return triangle == paint::no_surface_triangle || triangle == 3;
                    });
    return expect(body.maps->texture_set_id != trim.maps->texture_set_id,
                  "distinct texture sets received one cache identity") &&
           expect(trim_is_source_triangle_three,
                  "texture-set filtering lost the source triangle identity") &&
           expect(cache.statistics().entries == 2 && cache.statistics().misses == 2,
                  "independent texture sets were not retained together");
}

bool mesh_replacement_invalidates_every_cached_map() {
    MeshFixture original;
    MeshFixture replacement;
    replacement.primary[0] = {0.25F, 0.25F};
    mesh::MeshBinding binding(original.descriptor());
    paint::SurfaceMapCache cache;
    const auto before = cache.lookup(binding, request());
    const mesh::MeshRevision previous_revision = binding.revision();
    binding.replace(replacement.descriptor());
    const auto after = cache.lookup(binding, request());
    const auto statistics = cache.statistics();
    return expect(binding.revision() != previous_revision && !after.cache_hit,
                  "mesh replacement did not force a cache miss") &&
           expect(after.maps != before.maps && after.maps->mesh_revision == binding.revision(),
                  "mesh replacement returned the stale map bundle") &&
           expect(statistics.entries == 1 && statistics.misses == 2 &&
                      statistics.invalidated_entries == 1,
                  "mesh replacement did not atomically invalidate all cached maps");
}

bool uv_change_invalidates_the_texture_set_tiles() {
    MeshFixture fixture;
    mesh::MeshBinding binding(fixture.descriptor());
    paint::SurfaceMapCache cache;
    const auto primary = cache.lookup(binding, request(0, "primary"));
    const auto alternate = cache.lookup(binding, request(0, "alternate"));
    const auto alternate_again = cache.lookup(binding, request(0, "alternate"));
    const auto statistics = cache.statistics();
    return expect(!primary.cache_hit && !alternate.cache_hit && alternate_again.cache_hit,
                  "UV selection did not miss once and then become reusable") &&
           expect(primary.maps != alternate.maps && alternate.maps == alternate_again.maps,
                  "UV selection reused the wrong immutable bundle") &&
           expect(statistics.entries == 1 && statistics.hits == 1 && statistics.misses == 2 &&
                      statistics.invalidated_entries == 1,
                  "UV selection did not invalidate the prior texture-set maps");
}

bool invalid_requests_leave_the_cache_unchanged() {
    MeshFixture fixture;
    mesh::MeshBinding binding(fixture.descriptor());
    paint::SurfaceMapCache cache;
    static_cast<void>(cache.lookup(binding, request()));
    bool refused = false;
    try {
        static_cast<void>(cache.lookup(binding, request(99)));
    } catch (const std::out_of_range&) {
        refused = true;
    }
    return expect(refused, "invalid surface-map partition was accepted") &&
           expect(cache.statistics() ==
                      paint::SurfaceMapStatistics{.entries = 1, .hits = 0, .misses = 1},
                  "invalid lookup mutated cache contents or statistics");
}

}  // namespace

int main() {
    return maps_have_exact_source_identities_and_uv_islands() &&
                   face_and_island_operations_reuse_one_bundle() &&
                   texture_sets_are_cached_independently() &&
                   mesh_replacement_invalidates_every_cached_map() &&
                   uv_change_invalidates_the_texture_set_tiles() &&
                   invalid_requests_leave_the_cache_unchanged()
               ? 0
               : 1;
}
