#ifndef CTEX_MAPS_BAKE_PROVIDER_HPP
#define CTEX_MAPS_BAKE_PROVIDER_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/pixel_format.hpp>
#include <ctex/maps/mesh_maps.hpp>
#include <optional>
#include <string>

namespace ctex::maps {

struct BakeProgress {
    double fraction{};
    friend bool operator==(BakeProgress, BakeProgress) noexcept = default;
};

using BakeCancellationCallback = bool (*)(void* user_data) noexcept;
using BakeProgressCallback = void (*)(void* user_data, BakeProgress progress) noexcept;

struct BakeControl {
    void* user_data{};
    BakeCancellationCallback is_cancelled{};
    BakeProgressCallback report_progress{};
};

struct BakeRequest {
    MeshMapKind kind{};
    const char* texture_set_id{};
    const char* uv_set{};
    mesh::MeshRevision mesh_revision{};
    std::uint32_t width{};
    std::uint32_t height{};
};

struct BakeImageView {
    std::uint32_t width{};
    std::uint32_t height{};
    image::PixelFormat format{};
    std::size_t row_stride_bytes{};
    const void* pixels{};
    std::size_t pixel_bytes{};
};

enum class BakeProviderStatus : std::uint8_t { completed, cancelled, failed };

struct BakeProviderOutput {
    BakeImageView image;
    const char* detail{};
};

using BakeCapabilityCallback = bool (*)(void* user_data, MeshMapKind kind) noexcept;
using BakeRequestCallback = BakeProviderStatus (*)(void* user_data, const BakeRequest* request,
                                                   const BakeControl* control,
                                                   BakeProviderOutput* output) noexcept;

struct BakeProvider {
    const char* name{};
    void* user_data{};
    BakeCapabilityCallback can_produce{};
    BakeRequestCallback request{};
};

enum class BakeRequestStatus : std::uint8_t {
    completed,
    cancelled,
    unsupported,
    provider_failed,
};

struct BakeRequestResult {
    BakeRequestStatus status{};
    std::optional<MeshMapBindResult> binding;
    std::string message;
};

[[nodiscard]] BakeRequestResult request_bake(const BakeProvider& provider, MeshMapSet& target,
                                             MeshMapKind kind, std::uint32_t width,
                                             std::uint32_t height, BakeControl control = {});

}  // namespace ctex::maps

#endif
