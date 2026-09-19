#include <algorithm>
#include <cmath>
#include <ctex/pick/batch.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <limits>
#include <stdexcept>

namespace ctex::pick {
namespace {

struct MemoryBound {
    std::size_t bytes;
    bool overflowed;
};

bool finite(mesh::Vec3f value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void validate_inputs(std::span<const Ray> rays, float maximum_distance,
                     const BatchPickControl& control) {
    if (std::isnan(maximum_distance) || maximum_distance < 0.0F) {
        throw std::invalid_argument("batch pick requires a non-negative maximum distance");
    }
    if (control.progress_interval == 0) {
        throw std::invalid_argument("batch pick progress interval must be non-zero");
    }
    for (const Ray& ray : rays) {
        if (!finite(ray.origin) || !finite(ray.direction) ||
            (ray.direction.x == 0.0F && ray.direction.y == 0.0F && ray.direction.z == 0.0F)) {
            throw std::invalid_argument("batch pick rays must be finite with non-zero directions");
        }
    }
}

std::size_t decimal_digits(std::size_t value) {
    std::size_t result = 1;
    while (value >= 10) {
        value /= 10;
        ++result;
    }
    return result;
}

std::size_t partition_kind_length(mesh::PartitionKind kind) {
    switch (kind) {
        case mesh::PartitionKind::material:
            return 8;
        case mesh::PartitionKind::object:
            return 6;
        case mesh::PartitionKind::submesh:
            return 7;
        case mesh::PartitionKind::explicit_faces:
            return 5;
    }
    throw std::invalid_argument("batch pick received an unsupported partition kind");
}

std::size_t texture_id_length(const mesh::MeshPartition& partition, std::string_view uv_set) {
    return partition_kind_length(partition.kind) + 1 + decimal_digits(partition.stable_key.size()) +
           1 + partition.stable_key.size() + 4 + decimal_digits(uv_set.size()) + 1 + uv_set.size();
}

std::size_t maximum_texture_id_length(const mesh::MeshBinding& mesh,
                                      std::span<const TextureSetBindingView> texture_sets) {
    const auto& descriptor = mesh.view().descriptor();
    for (const TextureSetBindingView& binding : texture_sets) {
        if (binding.partition_index >= descriptor.partitions.size()) {
            throw std::invalid_argument("batch pick texture-set partition is not present");
        }
    }

    std::size_t maximum_length = 0;
    for (std::size_t partition_index = 0; partition_index < descriptor.partitions.size();
         ++partition_index) {
        const TextureSetBindingView* selected = nullptr;
        for (const TextureSetBindingView& binding : texture_sets) {
            if (binding.partition_index != partition_index) {
                continue;
            }
            if (selected != nullptr) {
                throw std::invalid_argument("batch pick has duplicate bindings for a partition");
            }
            selected = &binding;
        }
        if (selected == nullptr) {
            throw std::invalid_argument("batch pick requires one binding for every partition");
        }
        static_cast<void>(mesh.view().uv_set(selected->uv_set));
        maximum_length =
            std::max(maximum_length,
                     texture_id_length(descriptor.partitions[partition_index], selected->uv_set));
    }
    return maximum_length;
}

bool add_bytes(std::size_t& total, std::size_t count, std::size_t element_size) {
    constexpr std::size_t maximum = std::numeric_limits<std::size_t>::max();
    if (count != 0 && element_size > maximum / count) {
        return false;
    }
    const std::size_t amount = count * element_size;
    if (amount > maximum - total) {
        return false;
    }
    total += amount;
    return true;
}

MemoryBound batch_memory_bound(std::size_t ray_count, std::size_t triangle_count,
                               std::size_t maximum_texture_id_size) {
    if (ray_count == 0) {
        return {0, false};
    }
    std::size_t total = 0;
    const std::size_t candidate_capacity = std::max<std::size_t>(4, triangle_count * 2);
    const bool complete = add_bytes(total, ray_count, sizeof(std::optional<HitRecord>)) &&
                          add_bytes(total, ray_count, maximum_texture_id_size + 1) &&
                          add_bytes(total, candidate_capacity, sizeof(std::uint32_t)) &&
                          add_bytes(total, 3, sizeof(HitRecord)) &&
                          add_bytes(total, 1, maximum_texture_id_size + 1);
    return {complete ? total : std::numeric_limits<std::size_t>::max(), !complete};
}

void report_progress(const BatchPickControl& control, std::size_t completed, std::size_t total) {
    if (control.report_progress != nullptr) {
        control.report_progress(control.user_data, {completed, total});
    }
}

bool cancelled(const BatchPickControl& control) {
    return control.is_cancelled != nullptr && control.is_cancelled(control.user_data);
}

}  // namespace

BatchPickResult pick_nearest_batch(SpatialIndex& index, const mesh::MeshBinding& mesh,
                                   std::span<const Ray> rays, float maximum_distance,
                                   std::span<const TextureSetBindingView> texture_sets,
                                   BatchPickControl control, BackfacePolicy backfaces) {
    validate_inputs(rays, maximum_distance, control);
    const std::size_t maximum_id_size = maximum_texture_id_length(mesh, texture_sets);
    const MemoryBound memory =
        batch_memory_bound(rays.size(), mesh.view().triangle_count(), maximum_id_size);
    if (memory.overflowed || memory.bytes > control.memory_ceiling_bytes) {
        return {BatchPickStatus::memory_ceiling_exceeded, {}, 0, memory.bytes};
    }
    if (cancelled(control)) {
        report_progress(control, 0, rays.size());
        return {BatchPickStatus::cancelled, {}, 0, memory.bytes};
    }

    std::vector<std::optional<HitRecord>> hits(rays.size());
    std::size_t completed = 0;
    std::size_t last_reported = 0;
    report_progress(control, 0, rays.size());
    for (std::size_t index_in_batch = 0; index_in_batch < rays.size(); ++index_in_batch) {
        if (cancelled(control)) {
            if (last_reported != completed) {
                report_progress(control, completed, rays.size());
            }
            return {BatchPickStatus::cancelled, {}, completed, memory.bytes};
        }
        hits[index_in_batch] = pick_nearest(index, mesh, rays[index_in_batch], maximum_distance,
                                            texture_sets, backfaces);
        completed = index_in_batch + 1;
        if (completed % control.progress_interval == 0 || completed == rays.size()) {
            report_progress(control, completed, rays.size());
            last_reported = completed;
        }
    }
    return {BatchPickStatus::complete, std::move(hits), completed, memory.bytes};
}

}  // namespace ctex::pick
