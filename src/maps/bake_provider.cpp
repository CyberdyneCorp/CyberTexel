#include <atomic>
#include <cmath>
#include <ctex/maps/bake_provider.hpp>
#include <limits>
#include <map>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "pixel_buffer_copy.hpp"

namespace ctex::maps {
namespace {

std::atomic<BakeSessionIdentity> next_bake_session_identity{1};

BakeSessionIdentity issue_bake_session_identity() {
    BakeSessionIdentity identity = next_bake_session_identity.load(std::memory_order_relaxed);
    while (identity != std::numeric_limits<BakeSessionIdentity>::max()) {
        if (next_bake_session_identity.compare_exchange_weak(identity, identity + 1,
                                                             std::memory_order_relaxed)) {
            return identity;
        }
    }
    throw std::overflow_error("bake session identity space is exhausted");
}

void validate_provider(const BakeProvider& provider) {
    if (provider.name == nullptr || std::string_view(provider.name).empty() ||
        provider.can_produce == nullptr || provider.request == nullptr) {
        throw std::invalid_argument("bake provider requires a name, capability query and request");
    }
}

void validate_request_dimensions(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("bake request resolution must be non-zero");
    }
}

void validate_request_version(BakeRequestVersion version) {
    if (version.bake_settings_revision == 0 || version.request_generation == 0) {
        throw std::invalid_argument("bake request revisions must not be zero");
    }
}

std::string advertised_maps(const BakeProvider& provider) {
    std::string result;
    for (const MeshMapKind kind : all_mesh_map_kinds) {
        if (!provider.can_produce(provider.user_data, kind)) {
            continue;
        }
        if (!result.empty()) {
            result += ", ";
        }
        result += mesh_map_name(kind);
    }
    return result.empty() ? "none" : result;
}

bool cancelled(const BakeControl& control) {
    return control.is_cancelled != nullptr && control.is_cancelled(control.user_data);
}

void report(const BakeControl& control, double fraction) {
    if (control.report_progress != nullptr) {
        control.report_progress(control.user_data, {.fraction = fraction});
    }
}

struct ProviderControlState {
    BakeControl caller;
    double latest_fraction{};
    bool invalid_progress{};
};

bool provider_cancelled(void* user_data) noexcept {
    auto& state = *static_cast<ProviderControlState*>(user_data);
    return cancelled(state.caller);
}

void provider_progress(void* user_data, BakeProgress progress) noexcept {
    auto& state = *static_cast<ProviderControlState*>(user_data);
    if (!std::isfinite(progress.fraction) || progress.fraction < state.latest_fraction ||
        progress.fraction < 0.0 || progress.fraction > 1.0) {
        state.invalid_progress = true;
        return;
    }
    if (progress.fraction == state.latest_fraction) {
        return;
    }
    state.latest_fraction = progress.fraction;
    if (progress.fraction < 1.0) {
        report(state.caller, progress.fraction);
    }
}

void validate_output_extent(const BakeImageView& source, std::uint32_t requested_width,
                            std::uint32_t requested_height) {
    if (source.width != requested_width || source.height != requested_height) {
        throw std::invalid_argument(
            "bake provider returned a resolution different from the request");
    }
}

BakeRequestResult provider_failure(const BakeProvider& provider, const BakeProviderOutput& output,
                                   std::string fallback) {
    const std::string detail = output.detail == nullptr || std::string_view(output.detail).empty()
                                   ? std::move(fallback)
                                   : std::string(output.detail);
    return {.status = BakeRequestStatus::provider_failed,
            .binding = std::nullopt,
            .message = "bake provider '" + std::string(provider.name) + "' failed: " + detail};
}

enum class AsyncRequestPhase : std::uint8_t { active, cancelled, stale };

struct PendingBakeRequest {
    BakeRevisionToken token;
    AsyncRequestPhase phase{AsyncRequestPhase::active};
};

struct BakeSettingsHistoryEntry {
    BakeSettingsRevision revision{};
    MeshMapBindingSnapshot bindings;
};

std::vector<BakeRequestGeneration> invalidate_active_requests(auto& requests) {
    std::vector<BakeRequestGeneration> invalidated;
    for (auto& [generation, request] : requests) {
        if (request.phase == AsyncRequestPhase::active) {
            request.phase = AsyncRequestPhase::stale;
            invalidated.push_back(generation);
        }
    }
    return invalidated;
}

AsyncBakeCompletionResult async_result(AsyncBakeCompletionDisposition disposition,
                                       const BakeRevisionToken& token, std::string message) {
    return {.disposition = disposition,
            .token = token,
            .binding = std::nullopt,
            .message = std::move(message)};
}

}  // namespace

struct AsyncBakeSession::State {
    MeshMapSet* target{};
    BakeSessionIdentity identity{};
    BakeSettingsRevision settings_revision{};
    BakeRequestGeneration next_generation{1};
    std::map<BakeRequestGeneration, PendingBakeRequest> requests;
    std::map<MeshMapKind, BakeRequestGeneration> latest_by_kind;
    std::vector<BakeSettingsHistoryEntry> history;
};

AsyncBakeSession::AsyncBakeSession(MeshMapSet& target,
                                   BakeSettingsRevision initial_settings_revision)
    : state_(std::make_unique<State>()) {
    if (initial_settings_revision == 0) {
        throw std::invalid_argument("bake settings revision must not be zero");
    }
    state_->target = &target;
    state_->identity = issue_bake_session_identity();
    state_->settings_revision = initial_settings_revision;
}

AsyncBakeSession::~AsyncBakeSession() = default;
AsyncBakeSession::AsyncBakeSession(AsyncBakeSession&&) noexcept = default;
AsyncBakeSession& AsyncBakeSession::operator=(AsyncBakeSession&&) noexcept = default;

BakeRequest bake_request_view(const BakeRevisionToken& token) noexcept {
    return {.kind = token.kind,
            .texture_set_id = token.texture_set_id.c_str(),
            .uv_set = token.uv_set.c_str(),
            .mesh_revision = token.mesh_revision,
            .bake_settings_revision = token.bake_settings_revision,
            .request_generation = token.request_generation,
            .tangent_frame = token.tangent_frame ? &token.tangent_frame.value() : nullptr,
            .width = token.width,
            .height = token.height};
}

BakeSettingsRevision AsyncBakeSession::settings_revision() const noexcept {
    return state_->settings_revision;
}

std::size_t AsyncBakeSession::pending_request_count() const noexcept {
    return state_->requests.size();
}

std::size_t AsyncBakeSession::undo_step_count() const noexcept { return state_->history.size(); }

BakeRevisionToken AsyncBakeSession::begin_request(MeshMapKind kind, std::uint32_t width,
                                                  std::uint32_t height) {
    validate_request_dimensions(width, height);
    static_cast<void>(mesh_map_name(kind));
    if (state_->next_generation == std::numeric_limits<BakeRequestGeneration>::max()) {
        throw std::overflow_error("bake request generation space is exhausted");
    }
    if (const auto latest = state_->latest_by_kind.find(kind);
        latest != state_->latest_by_kind.end()) {
        if (const auto pending = state_->requests.find(latest->second);
            pending != state_->requests.end()) {
            pending->second.phase = AsyncRequestPhase::stale;
        }
    }
    BakeRevisionToken token{.session_identity = state_->identity,
                            .kind = kind,
                            .texture_set_id = state_->target->texture_set_id(),
                            .uv_set = state_->target->uv_set(),
                            .mesh_revision = state_->target->mesh_revision(),
                            .bake_settings_revision = state_->settings_revision,
                            .request_generation = state_->next_generation++,
                            .tangent_frame = state_->target->tangent_frame(),
                            .width = width,
                            .height = height};
    state_->latest_by_kind.insert_or_assign(kind, token.request_generation);
    state_->requests.emplace(
        token.request_generation,
        PendingBakeRequest{.token = token, .phase = AsyncRequestPhase::active});
    return token;
}

bool AsyncBakeSession::cancel(const BakeRevisionToken& token) {
    const auto found = state_->requests.find(token.request_generation);
    if (found == state_->requests.end() || found->second.token != token ||
        found->second.phase != AsyncRequestPhase::active) {
        return false;
    }
    found->second.phase = AsyncRequestPhase::cancelled;
    return true;
}

AsyncBakeCompletionResult AsyncBakeSession::complete(const BakeRevisionToken& token,
                                                     const BakeProviderOutput& output) {
    const auto found = state_->requests.find(token.request_generation);
    if (found == state_->requests.end() || found->second.token != token) {
        return async_result(AsyncBakeCompletionDisposition::unknown_token, token,
                            "bake completion used an unknown or altered request token");
    }
    const PendingBakeRequest pending = found->second;
    state_->requests.erase(found);
    if (pending.phase == AsyncRequestPhase::cancelled) {
        return async_result(AsyncBakeCompletionDisposition::cancelled, token,
                            "cancelled bake result was discarded");
    }
    const bool identity_is_current = pending.phase == AsyncRequestPhase::active &&
                                     token.texture_set_id == state_->target->texture_set_id() &&
                                     token.uv_set == state_->target->uv_set() &&
                                     token.mesh_revision == state_->target->mesh_revision() &&
                                     token.bake_settings_revision == state_->settings_revision &&
                                     token.tangent_frame == state_->target->tangent_frame() &&
                                     state_->latest_by_kind[token.kind] == token.request_generation;
    if (!identity_is_current) {
        return async_result(AsyncBakeCompletionDisposition::stale, token,
                            "stale bake result was discarded without changing map bindings");
    }
    try {
        validate_output_extent(output.image, token.width, token.height);
        auto pixels = detail::copy_mesh_map_pixel_buffer(output.image);
        MeshMapBindResult binding =
            state_->target->bind({.kind = token.kind,
                                  .texture_set_id = token.texture_set_id,
                                  .uv_set = token.uv_set,
                                  .mesh_revision = token.mesh_revision,
                                  .normal_convention = output.normal_convention,
                                  .tangent_frame = output.tangent_frame,
                                  .pixels = std::move(pixels)});
        return {.disposition = AsyncBakeCompletionDisposition::bound,
                .token = token,
                .binding = std::move(binding),
                .message = "current bake result bound atomically"};
    } catch (const std::exception& error) {
        return async_result(AsyncBakeCompletionDisposition::invalid_output, token, error.what());
    }
}

BakeSettingsEditResult AsyncBakeSession::edit_settings(BakeSettingsRevision revision) {
    if (revision == 0 || revision == state_->settings_revision) {
        throw std::invalid_argument("bake settings edit requires a new non-zero revision");
    }
    const BakeSettingsRevision previous = state_->settings_revision;
    state_->history.push_back(
        {.revision = previous, .bindings = state_->target->snapshot_bindings()});
    state_->settings_revision = revision;
    return {.previous_revision = previous,
            .current_revision = revision,
            .invalidated_requests = invalidate_active_requests(state_->requests)};
}

BakeSettingsUndoResult AsyncBakeSession::undo_settings_edit() {
    if (state_->history.empty()) {
        return {.restored = false,
                .previous_revision = state_->settings_revision,
                .restored_revision = state_->settings_revision,
                .restored_maps = {},
                .invalidated_requests = {}};
    }
    BakeSettingsHistoryEntry& history = state_->history.back();
    std::vector<MeshMapKind> restored_maps;
    restored_maps.reserve(history.bindings.maps.size());
    for (const MeshMapDescriptor& descriptor : history.bindings.maps) {
        restored_maps.push_back(descriptor.kind);
    }
    state_->target->restore_bindings(history.bindings);
    const BakeSettingsRevision previous = state_->settings_revision;
    state_->settings_revision = history.revision;
    state_->history.pop_back();
    return {.restored = true,
            .previous_revision = previous,
            .restored_revision = state_->settings_revision,
            .restored_maps = std::move(restored_maps),
            .invalidated_requests = invalidate_active_requests(state_->requests)};
}

BakeRequestResult request_bake(const BakeProvider& provider, MeshMapSet& target, MeshMapKind kind,
                               std::uint32_t width, std::uint32_t height, BakeControl control,
                               BakeRequestVersion version) {
    validate_provider(provider);
    validate_request_dimensions(width, height);
    validate_request_version(version);
    const std::string map_name(mesh_map_name(kind));
    if (!provider.can_produce(provider.user_data, kind)) {
        return {.status = BakeRequestStatus::unsupported,
                .binding = std::nullopt,
                .message = "bake provider '" + std::string(provider.name) + "' cannot produce '" +
                           map_name + "'; advertised maps: " + advertised_maps(provider)};
    }

    report(control, 0.0);
    if (cancelled(control)) {
        return {.status = BakeRequestStatus::cancelled,
                .binding = std::nullopt,
                .message = "bake request for '" + map_name + "' cancelled before provider work"};
    }

    ProviderControlState state{.caller = control};
    const BakeControl provider_control{.user_data = &state,
                                       .is_cancelled = provider_cancelled,
                                       .report_progress = provider_progress};
    const mesh::MeshRevision requested_mesh_revision = target.mesh_revision();
    const BakeRequest request{
        .kind = kind,
        .texture_set_id = target.texture_set_id().c_str(),
        .uv_set = target.uv_set().c_str(),
        .mesh_revision = requested_mesh_revision,
        .bake_settings_revision = version.bake_settings_revision,
        .request_generation = version.request_generation,
        .tangent_frame = target.tangent_frame() ? &target.tangent_frame().value() : nullptr,
        .width = width,
        .height = height};
    BakeProviderOutput output{};
    const BakeProviderStatus status =
        provider.request(provider.user_data, &request, &provider_control, &output);
    if (status == BakeProviderStatus::cancelled || cancelled(control)) {
        return {.status = BakeRequestStatus::cancelled,
                .binding = std::nullopt,
                .message = "bake request for '" + map_name + "' cancelled without binding output"};
    }
    if (status == BakeProviderStatus::failed) {
        return provider_failure(provider, output, "provider reported failure");
    }
    if (status != BakeProviderStatus::completed) {
        return provider_failure(provider, output, "provider returned an invalid status");
    }
    if (state.invalid_progress) {
        return provider_failure(provider, output, "provider reported invalid progress");
    }

    try {
        validate_output_extent(output.image, width, height);
        auto pixels = detail::copy_mesh_map_pixel_buffer(output.image);
        MeshMapBindResult binding = target.bind({.kind = kind,
                                                 .texture_set_id = target.texture_set_id(),
                                                 .uv_set = target.uv_set(),
                                                 .mesh_revision = requested_mesh_revision,
                                                 .normal_convention = output.normal_convention,
                                                 .tangent_frame = output.tangent_frame,
                                                 .pixels = std::move(pixels)});
        report(control, 1.0);
        return {.status = BakeRequestStatus::completed,
                .binding = std::move(binding),
                .message = "bake provider '" + std::string(provider.name) + "' produced '" +
                           map_name + "'"};
    } catch (const std::exception& error) {
        return provider_failure(provider, output, error.what());
    }
}

}  // namespace ctex::maps
