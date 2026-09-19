#ifndef CTEX_MAPS_BAKE_PROVIDER_HPP
#define CTEX_MAPS_BAKE_PROVIDER_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/maps/mesh_maps.hpp>
#include <ctex/maps/pixel_buffer.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ctex::maps {

using BakeSettingsRevision = std::uint64_t;
using BakeRequestGeneration = std::uint64_t;
using BakeSessionIdentity = std::uint64_t;

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
    BakeSettingsRevision bake_settings_revision{1};
    BakeRequestGeneration request_generation{1};
    const mesh::TangentFrameDescriptor* tangent_frame{};
    std::uint32_t width{};
    std::uint32_t height{};
};

using BakeImageView = MeshMapPixelBufferView;

enum class BakeProviderStatus : std::uint8_t { completed, cancelled, failed };

struct BakeProviderOutput {
    BakeImageView image;
    std::optional<NormalMapConvention> normal_convention{};
    std::optional<mesh::TangentFrameDescriptor> tangent_frame{};
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

struct BakeRequestVersion {
    BakeSettingsRevision bake_settings_revision{1};
    BakeRequestGeneration request_generation{1};
};

struct BakeRevisionToken {
    BakeSessionIdentity session_identity{};
    MeshMapKind kind{};
    std::string texture_set_id;
    std::string uv_set;
    mesh::MeshRevision mesh_revision{};
    BakeSettingsRevision bake_settings_revision{};
    BakeRequestGeneration request_generation{};
    std::optional<mesh::TangentFrameDescriptor> tangent_frame{};
    std::uint32_t width{};
    std::uint32_t height{};

    friend bool operator==(const BakeRevisionToken&, const BakeRevisionToken&) = default;
};

[[nodiscard]] BakeRequest bake_request_view(const BakeRevisionToken& token) noexcept;

enum class AsyncBakeCompletionDisposition : std::uint8_t {
    bound,
    stale,
    cancelled,
    invalid_output,
    unknown_token,
};

struct AsyncBakeCompletionResult {
    AsyncBakeCompletionDisposition disposition{};
    BakeRevisionToken token;
    std::optional<MeshMapBindResult> binding;
    std::string message;
};

struct BakeSettingsEditResult {
    BakeSettingsRevision previous_revision{};
    BakeSettingsRevision current_revision{};
    std::vector<BakeRequestGeneration> invalidated_requests;
};

struct BakeSettingsUndoResult {
    bool restored{};
    BakeSettingsRevision previous_revision{};
    BakeSettingsRevision restored_revision{};
    std::vector<MeshMapKind> restored_maps;
    std::vector<BakeRequestGeneration> invalidated_requests;
};

class AsyncBakeSession {
public:
    explicit AsyncBakeSession(MeshMapSet& target,
                              BakeSettingsRevision initial_settings_revision = 1);
    ~AsyncBakeSession();
    AsyncBakeSession(AsyncBakeSession&&) noexcept;
    AsyncBakeSession& operator=(AsyncBakeSession&&) noexcept;
    AsyncBakeSession(const AsyncBakeSession&) = delete;
    AsyncBakeSession& operator=(const AsyncBakeSession&) = delete;

    [[nodiscard]] BakeSettingsRevision settings_revision() const noexcept;
    [[nodiscard]] std::size_t pending_request_count() const noexcept;
    [[nodiscard]] std::size_t undo_step_count() const noexcept;

    [[nodiscard]] BakeRevisionToken begin_request(MeshMapKind kind, std::uint32_t width,
                                                  std::uint32_t height);
    [[nodiscard]] bool cancel(const BakeRevisionToken& token);
    [[nodiscard]] AsyncBakeCompletionResult complete(const BakeRevisionToken& token,
                                                     const BakeProviderOutput& output);
    [[nodiscard]] BakeSettingsEditResult edit_settings(BakeSettingsRevision revision);
    [[nodiscard]] BakeSettingsUndoResult undo_settings_edit();

private:
    struct State;
    std::unique_ptr<State> state_;
};

[[nodiscard]] BakeRequestResult request_bake(const BakeProvider& provider, MeshMapSet& target,
                                             MeshMapKind kind, std::uint32_t width,
                                             std::uint32_t height, BakeControl control = {},
                                             BakeRequestVersion version = {});

}  // namespace ctex::maps

#endif
