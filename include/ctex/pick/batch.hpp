#ifndef CTEX_PICK_BATCH_HPP
#define CTEX_PICK_BATCH_HPP

#include <cstddef>
#include <ctex/pick/hit.hpp>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace ctex::pick {

class SpatialIndex;

struct BatchPickProgress {
    std::size_t completed_rays;
    std::size_t total_rays;
};

using BatchPickCancellation = bool (*)(void* user_data) noexcept;
using BatchPickProgressCallback = void (*)(void* user_data, BatchPickProgress progress) noexcept;

struct BatchPickControl {
    std::size_t memory_ceiling_bytes{std::numeric_limits<std::size_t>::max()};
    std::size_t progress_interval{256};
    void* user_data{};
    BatchPickCancellation is_cancelled{};
    BatchPickProgressCallback report_progress{};
};

enum class BatchPickStatus { complete, cancelled, memory_ceiling_exceeded };

struct BatchPickResult {
    BatchPickStatus status;
    // Populated with exactly one entry per input ray only when status is complete.
    std::vector<std::optional<HitRecord>> hits;
    std::size_t processed_rays;
    // Conservative logical heap payload required before processing starts.
    std::size_t required_memory_bytes;
};

[[nodiscard]] BatchPickResult pick_nearest_batch(
    SpatialIndex& index, const mesh::MeshBinding& mesh, std::span<const Ray> rays,
    float maximum_distance, std::span<const TextureSetBindingView> texture_sets,
    BatchPickControl control = {}, BackfacePolicy backfaces = BackfacePolicy::accept);

}  // namespace ctex::pick

#endif
