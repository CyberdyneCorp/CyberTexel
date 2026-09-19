#include <array>
#include <cstddef>
#include <ctex/mesh/mesh.hpp>
#include <ctex/pick/batch.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {

struct BatchMesh {
    std::array<ctex::mesh::Vec3f, 3> positions{
        ctex::mesh::Vec3f{0.0F, 0.0F, 0.0F},
        ctex::mesh::Vec3f{1.0F, 0.0F, 0.0F},
        ctex::mesh::Vec3f{0.0F, 1.0F, 0.0F},
    };
    std::array<ctex::mesh::Vec3f, 3> normals{
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
        ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
    };
    std::array<ctex::mesh::Vec2f, 3> uv{
        ctex::mesh::Vec2f{0.0F, 0.0F},
        ctex::mesh::Vec2f{1.0F, 0.0F},
        ctex::mesh::Vec2f{0.0F, 1.0F},
    };
    std::array<std::uint32_t, 3> indices{0, 1, 2};
    std::array<ctex::mesh::UvSetView, 1> uv_sets{ctex::mesh::UvSetView{"paint", uv}};
    std::array<ctex::mesh::MeshPartition, 1> partitions{
        ctex::mesh::MeshPartition{ctex::mesh::PartitionKind::object, "surface", "Surface"},
    };
    std::array<std::uint32_t, 1> face_partitions{0};
    std::array<std::uint32_t, 1> face_materials{42};

    [[nodiscard]] ctex::mesh::MeshDescriptor descriptor() const {
        return {
            .positions = positions,
            .normals = normals,
            .vertex_colors = {},
            .triangle_indices = indices,
            .uv_sets = uv_sets,
            .default_uv_set = "paint",
            .partitions = partitions,
            .face_partition_indices = face_partitions,
            .face_material_ids = face_materials,
        };
    }
};

struct CallbackState {
    std::vector<ctex::pick::BatchPickProgress> reports;
    std::size_t latest_completed{};
    std::size_t cancel_after{std::numeric_limits<std::size_t>::max()};
};

void record_progress(void* user_data, ctex::pick::BatchPickProgress progress) noexcept {
    auto& state = *static_cast<CallbackState*>(user_data);
    state.latest_completed = progress.completed_rays;
    state.reports.push_back(progress);
}

bool cancel_at_threshold(void* user_data) noexcept {
    const auto& state = *static_cast<const CallbackState*>(user_data);
    return state.latest_completed >= state.cancel_after;
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ctex::pick::Ray hit_ray() { return {{0.25F, 0.25F, 1.0F}, {0.0F, 0.0F, -1.0F}}; }

ctex::pick::Ray miss_ray() { return {{2.0F, 2.0F, 1.0F}, {0.0F, 0.0F, -1.0F}}; }

bool returns_one_ordered_result_per_ray() {
    BatchMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const std::array rays{hit_ray(), miss_ray(), hit_ray()};
    CallbackState callbacks;
    callbacks.reports.reserve(3);
    const auto result = ctex::pick::pick_nearest_batch(
        index, mesh, rays, 2.0F, bindings,
        {.progress_interval = 2, .user_data = &callbacks, .report_progress = record_progress});
    return expect(result.status == ctex::pick::BatchPickStatus::complete,
                  "valid batch did not complete") &&
           expect(result.hits.size() == rays.size() && result.hits[0].has_value() &&
                      !result.hits[1].has_value() && result.hits[2].has_value(),
                  "batch results did not preserve ray order and misses") &&
           expect(result.processed_rays == rays.size() && result.required_memory_bytes != 0,
                  "completed batch did not report its work and memory bound") &&
           expect(callbacks.reports.size() == 3 && callbacks.reports[0].completed_rays == 0 &&
                      callbacks.reports[1].completed_rays == 2 &&
                      callbacks.reports[2].completed_rays == 3,
                  "batch progress did not report initial, interval, and final states");
}

bool cancellation_discards_partial_records() {
    BatchMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    std::vector<ctex::pick::Ray> rays(5000, hit_ray());
    CallbackState callbacks;
    callbacks.cancel_after = 32;
    callbacks.reports.reserve(4);
    const auto result = ctex::pick::pick_nearest_batch(index, mesh, rays, 2.0F, bindings,
                                                       {.progress_interval = 16,
                                                        .user_data = &callbacks,
                                                        .is_cancelled = cancel_at_threshold,
                                                        .report_progress = record_progress});
    return expect(result.status == ctex::pick::BatchPickStatus::cancelled,
                  "cancelled batch reported completion") &&
           expect(result.hits.empty(), "cancelled batch exposed partial hit records") &&
           expect(result.processed_rays == 32 && result.processed_rays < rays.size(),
                  "batch cancellation was not observed at the next ray boundary") &&
           expect(callbacks.latest_completed == result.processed_rays,
                  "cancelled batch progress did not match processed work");
}

bool memory_ceiling_is_preflighted() {
    BatchMesh buffers;
    const ctex::mesh::MeshBinding mesh(buffers.descriptor());
    ctex::pick::SpatialIndex index(mesh);
    constexpr std::array bindings{ctex::pick::TextureSetBindingView{0, "paint"}};
    const std::array rays{hit_ray(), hit_ray()};
    const auto measured = ctex::pick::pick_nearest_batch(index, mesh, rays, 2.0F, bindings);
    const auto refused = ctex::pick::pick_nearest_batch(
        index, mesh, rays, 2.0F, bindings,
        {.memory_ceiling_bytes = measured.required_memory_bytes - 1});
    const auto admitted =
        ctex::pick::pick_nearest_batch(index, mesh, rays, 2.0F, bindings,
                                       {.memory_ceiling_bytes = measured.required_memory_bytes});
    return expect(refused.status == ctex::pick::BatchPickStatus::memory_ceiling_exceeded &&
                      refused.hits.empty() && refused.processed_rays == 0,
                  "undersized memory ceiling did not refuse before work") &&
           expect(admitted.status == ctex::pick::BatchPickStatus::complete,
                  "exact reported memory ceiling did not admit the batch");
}

}  // namespace

int main() {
    return returns_one_ordered_result_per_ray() && cancellation_discards_partial_records() &&
                   memory_ceiling_is_preflighted()
               ? 0
               : 1;
}
