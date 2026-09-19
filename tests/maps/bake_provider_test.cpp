#include <array>
#include <cmath>
#include <cstddef>
#include <ctex/maps/bake_provider.hpp>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::maps;

constexpr mesh::MeshRevision fixture_mesh_revision = 23;

mesh::TangentFrameDescriptor tangent_frame() { return {.uv_set = "paint"}; }

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

struct ProviderState {
    std::vector<MeshMapKind> advertised{MeshMapKind::ambient_occlusion, MeshMapKind::curvature};
    std::vector<std::byte> pixels{std::byte{0}, std::byte{64}, std::byte{128}, std::byte{255}};
    std::uint8_t channel_count{1};
    std::size_t row_stride_bytes{};
    std::size_t request_count{};
    mesh::MeshRevision requested_mesh_revision{};
    std::uint64_t requested_settings_revision{};
    std::uint64_t requested_generation{};
    std::optional<mesh::TangentFrameDescriptor> requested_tangent_frame;
    bool fail{};
    bool invalid_progress{};
    bool invalid_status{};
    std::optional<NormalMapConvention> normal_convention;
    std::optional<mesh::TangentFrameDescriptor> tangent_frame;
};

bool can_produce(void* user_data, MeshMapKind kind) noexcept {
    const auto& state = *static_cast<ProviderState*>(user_data);
    for (const MeshMapKind available : state.advertised) {
        if (available == kind) {
            return true;
        }
    }
    return false;
}

BakeProviderStatus produce(void* user_data, const BakeRequest* request, const BakeControl* control,
                           BakeProviderOutput* output) noexcept {
    auto& state = *static_cast<ProviderState*>(user_data);
    ++state.request_count;
    state.requested_mesh_revision = request->mesh_revision;
    state.requested_settings_revision = request->bake_settings_revision;
    state.requested_generation = request->request_generation;
    state.requested_tangent_frame =
        request->tangent_frame == nullptr ? std::nullopt : std::optional(*request->tangent_frame);
    control->report_progress(control->user_data, {.fraction = 0.25});
    if (control->is_cancelled(control->user_data)) {
        return BakeProviderStatus::cancelled;
    }
    control->report_progress(
        control->user_data,
        {.fraction = state.invalid_progress ? std::numeric_limits<double>::quiet_NaN() : 0.75});
    if (state.fail) {
        output->detail = "fixture failure";
        return BakeProviderStatus::failed;
    }
    output->image = {
        .width = request->width,
        .height = request->height,
        .format = {.channel_type = image::ChannelType::uint8_unorm,
                   .channel_count = state.channel_count},
        .row_stride_bytes = state.row_stride_bytes == 0
                                ? static_cast<std::size_t>(request->width) * state.channel_count
                                : state.row_stride_bytes,
        .pixels = state.pixels.data(),
        .pixel_bytes = state.pixels.size()};
    output->normal_convention = state.normal_convention;
    output->tangent_frame = state.tangent_frame;
    if (state.invalid_status) {
        return static_cast<BakeProviderStatus>(255);
    }
    return BakeProviderStatus::completed;
}

BakeProvider provider(ProviderState& state) {
    return {.name = "fixture-baker",
            .user_data = &state,
            .can_produce = can_produce,
            .request = produce};
}

doc::TextureSet& texture_set(doc::TextureDocument& document) {
    return document.create_texture_set({.display_name = "Body",
                                        .partition_kind = doc::PartitionSourceKind::material,
                                        .partition_key = "body",
                                        .uv_set = "paint",
                                        .width = 4,
                                        .height = 4,
                                        .default_bit_depth = 8});
}

struct ControlState {
    std::vector<BakeProgress> progress;
    bool cancel{};
    double cancel_at{2.0};
};

void record_progress(void* user_data, BakeProgress progress) noexcept {
    auto& state = *static_cast<ControlState*>(user_data);
    state.progress.push_back(progress);
    if (progress.fraction >= state.cancel_at) {
        state.cancel = true;
    }
}

bool is_cancelled(void* user_data) noexcept {
    return static_cast<ControlState*>(user_data)->cancel;
}

BakeControl control(ControlState& state) {
    return {.user_data = &state, .is_cancelled = is_cancelled, .report_progress = record_progress};
}

BakeProviderOutput output(ProviderState& state, std::uint32_t width = 1, std::uint32_t height = 1) {
    return {.image = {.width = width,
                      .height = height,
                      .format = {.channel_type = image::ChannelType::uint8_unorm,
                                 .channel_count = state.channel_count},
                      .row_stride_bytes = static_cast<std::size_t>(width) * state.channel_count,
                      .pixels = state.pixels.data(),
                      .pixel_bytes = state.pixels.size()},
            .normal_convention = state.normal_convention,
            .tangent_frame = state.tangent_frame,
            .detail = nullptr};
}

bool supported_request_reports_progress_and_binds_output() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision);
    ProviderState provider_state;
    ControlState control_state;
    const BakeRequestResult result =
        request_bake(provider(provider_state), maps, MeshMapKind::ambient_occlusion, 2, 2,
                     control(control_state));
    provider_state.pixels[3] = std::byte{0};
    return expect(result.status == BakeRequestStatus::completed && result.binding &&
                      result.binding->resolution_mismatch && provider_state.request_count == 1 &&
                      provider_state.requested_mesh_revision == fixture_mesh_revision,
                  "supported bake did not bind its lower-resolution result") &&
           expect(control_state.progress == std::vector<BakeProgress>{{0.0}, {0.25}, {0.75}, {1.0}},
                  "bake progress omitted its initial, provider, or terminal report") &&
           expect(maps.contains(MeshMapKind::ambient_occlusion) &&
                      maps.map(MeshMapKind::ambient_occlusion).mesh_revision ==
                          fixture_mesh_revision &&
                      maps.sample(MeshMapKind::ambient_occlusion, 1.0, 0.0).sample.values[0] == 1.0,
                  "provider output was not copied into the requested map binding");
}

bool unsupported_request_names_map_and_advertised_set() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision);
    ProviderState state;
    const BakeRequestResult result =
        request_bake(provider(state), maps, MeshMapKind::thickness, 2, 2);
    return expect(result.status == BakeRequestStatus::unsupported && !result.binding &&
                      result.message.find("thickness") != std::string::npos &&
                      result.message.find("ambient-occlusion") != std::string::npos &&
                      result.message.find("curvature") != std::string::npos,
                  "unsupported bake diagnostic omitted the request or advertised map set") &&
           expect(state.request_count == 0 && maps.size() == 0,
                  "unsupported bake invoked the provider or bound a placeholder");
}

bool strided_output_needs_no_padding_after_the_last_row() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision);
    ProviderState state;
    state.row_stride_bytes = 4;
    state.pixels = {std::byte{0}, std::byte{64},  std::byte{0},
                    std::byte{0}, std::byte{128}, std::byte{255}};
    const BakeRequestResult result =
        request_bake(provider(state), maps, MeshMapKind::curvature, 2, 2);
    return expect(result.status == BakeRequestStatus::completed &&
                      maps.sample(MeshMapKind::curvature, 1.0, 0.0).sample.values[0] == 1.0,
                  "valid strided provider output required nonexistent final-row padding");
}

bool normal_provider_must_declare_and_forwards_its_convention() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision, tangent_frame());
    ProviderState state;
    state.advertised = {MeshMapKind::tangent_space_normal};
    state.channel_count = 3;
    state.pixels = {std::byte{128}, std::byte{64}, std::byte{255}};
    state.tangent_frame = tangent_frame();
    const BakeRequestResult missing =
        request_bake(provider(state), maps, MeshMapKind::tangent_space_normal, 1, 1);
    const bool missing_bound_output = maps.size() != 0;
    state.normal_convention = NormalMapConvention::direct_x;
    const BakeRequestResult declared =
        request_bake(provider(state), maps, MeshMapKind::tangent_space_normal, 1, 1);
    const MeshMapSample sample = maps.sample(MeshMapKind::tangent_space_normal, 0.5, 0.5).sample;
    return expect(missing.status == BakeRequestStatus::provider_failed && !missing_bound_output &&
                      maps.size() == 1 && declared.status == BakeRequestStatus::completed &&
                      declared.binding && declared.binding->resolution_mismatch &&
                      state.requested_tangent_frame == tangent_frame(),
                  "normal provider convention was not required and then accepted") &&
           expect(
               maps.map(MeshMapKind::tangent_space_normal).normal_convention ==
                       NormalMapConvention::direct_x &&
                   maps.map(MeshMapKind::tangent_space_normal).tangent_frame == tangent_frame() &&
                   std::abs(sample.values[1] - (191.0 / 255.0)) < 1.0e-12,
               "provider normal convention was not retained or converted on read");
}

bool cancellation_before_and_during_provider_work_binds_nothing() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision);
    ProviderState state;
    ControlState before{.progress = {}, .cancel = true};
    const BakeRequestResult cancelled_before =
        request_bake(provider(state), maps, MeshMapKind::curvature, 2, 2, control(before));
    ControlState during{.progress = {}, .cancel = false, .cancel_at = 0.25};
    const BakeRequestResult cancelled_during =
        request_bake(provider(state), maps, MeshMapKind::curvature, 2, 2, control(during));
    return expect(cancelled_before.status == BakeRequestStatus::cancelled &&
                      before.progress == std::vector<BakeProgress>{{0.0}} &&
                      cancelled_during.status == BakeRequestStatus::cancelled,
                  "bake cancellation did not produce the cancelled outcome") &&
           expect(state.request_count == 1 && maps.size() == 0,
                  "cancelled bake invoked excess work or published partial output");
}

bool provider_failures_and_invalid_output_are_transactional() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision);
    ProviderState failed;
    failed.fail = true;
    const BakeRequestResult failure =
        request_bake(provider(failed), maps, MeshMapKind::curvature, 2, 2);
    ProviderState invalid_progress;
    invalid_progress.invalid_progress = true;
    const BakeRequestResult invalid =
        request_bake(provider(invalid_progress), maps, MeshMapKind::curvature, 2, 2);
    ProviderState short_buffer;
    short_buffer.pixels.resize(1);
    const BakeRequestResult incomplete =
        request_bake(provider(short_buffer), maps, MeshMapKind::curvature, 2, 2);
    ProviderState invalid_status;
    invalid_status.invalid_status = true;
    const BakeRequestResult unknown =
        request_bake(provider(invalid_status), maps, MeshMapKind::curvature, 2, 2);
    return expect(failure.status == BakeRequestStatus::provider_failed &&
                      failure.message.find("fixture failure") != std::string::npos &&
                      invalid.status == BakeRequestStatus::provider_failed &&
                      incomplete.status == BakeRequestStatus::provider_failed &&
                      unknown.status == BakeRequestStatus::provider_failed,
                  "provider failure, invalid progress, status, or output was accepted") &&
           expect(maps.size() == 0, "failed bake request changed the target map set");
}

bool malformed_provider_and_request_are_refused_before_callbacks() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision);
    ProviderState state;
    bool provider_refused = false;
    try {
        static_cast<void>(request_bake({}, maps, MeshMapKind::curvature, 2, 2));
    } catch (const std::invalid_argument&) {
        provider_refused = true;
    }
    bool resolution_refused = false;
    try {
        static_cast<void>(request_bake(provider(state), maps, MeshMapKind::curvature, 0, 2));
    } catch (const std::invalid_argument&) {
        resolution_refused = true;
    }
    bool zero_version_refused = false;
    try {
        static_cast<void>(request_bake(provider(state), maps, MeshMapKind::curvature, 2, 2, {},
                                       {.bake_settings_revision = 0, .request_generation = 1}));
    } catch (const std::invalid_argument&) {
        zero_version_refused = true;
    }
    return expect(
        provider_refused && resolution_refused && zero_version_refused && state.request_count == 0,
        "malformed provider or bake request reached a callback");
}

bool synchronous_requests_forward_explicit_revisions() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision);
    ProviderState state;
    const BakeRequestResult result =
        request_bake(provider(state), maps, MeshMapKind::curvature, 2, 2, {},
                     {.bake_settings_revision = 17, .request_generation = 23});
    return expect(result.status == BakeRequestStatus::completed &&
                      state.requested_settings_revision == 17 && state.requested_generation == 23,
                  "synchronous provider request omitted its settings revision or generation");
}

bool async_tokens_reject_superseded_cancelled_and_stale_results() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision);
    AsyncBakeSession session(maps, 7);
    const BakeRevisionToken superseded =
        session.begin_request(MeshMapKind::ambient_occlusion, 1, 1);
    const BakeRequest superseded_view = bake_request_view(superseded);
    const BakeRevisionToken current = session.begin_request(MeshMapKind::ambient_occlusion, 1, 1);
    AsyncBakeSession foreign_session(maps, 7);
    const BakeRevisionToken foreign = foreign_session.begin_request(MeshMapKind::object_id, 1, 1);
    ProviderState stale_state;
    stale_state.pixels = {std::byte{32}};
    const AsyncBakeCompletionResult foreign_result = session.complete(foreign, output(stale_state));
    const AsyncBakeCompletionResult stale = session.complete(superseded, output(stale_state));
    ProviderState current_state;
    current_state.pixels = {std::byte{255}};
    BakeRevisionToken altered = current;
    altered.uv_set = "other";
    const AsyncBakeCompletionResult altered_result =
        session.complete(altered, output(current_state));
    const AsyncBakeCompletionResult accepted = session.complete(current, output(current_state));
    const AsyncBakeCompletionResult duplicate = session.complete(current, output(current_state));

    const BakeRevisionToken old_settings = session.begin_request(MeshMapKind::curvature, 1, 1);
    const BakeSettingsEditResult edit = session.edit_settings(8);
    const BakeRevisionToken new_settings = session.begin_request(MeshMapKind::curvature, 1, 1);
    const AsyncBakeCompletionResult settings_accepted =
        session.complete(new_settings, output(current_state));
    const AsyncBakeCompletionResult settings_stale =
        session.complete(old_settings, output(stale_state));

    const BakeRevisionToken cancelled = session.begin_request(MeshMapKind::thickness, 1, 1);
    const bool cancellation_recorded = session.cancel(cancelled);
    const AsyncBakeCompletionResult cancelled_result =
        session.complete(cancelled, output(stale_state));

    const BakeRevisionToken mesh_stale = session.begin_request(MeshMapKind::height, 1, 1);
    static_cast<void>(maps.synchronize_mesh_revision(fixture_mesh_revision + 1));
    const AsyncBakeCompletionResult mesh_stale_result =
        session.complete(mesh_stale, output(stale_state));

    const BakeRevisionToken malformed = session.begin_request(MeshMapKind::uv_density, 1, 1);
    const AsyncBakeCompletionResult invalid =
        session.complete(malformed, output(stale_state, 2, 1));

    return expect(
               superseded.session_identity != 0 &&
                   superseded.texture_set_id == maps.texture_set_id() &&
                   superseded.uv_set == maps.uv_set() &&
                   superseded.mesh_revision == fixture_mesh_revision &&
                   superseded.bake_settings_revision == 7 &&
                   std::string_view(superseded_view.texture_set_id) == superseded.texture_set_id &&
                   std::string_view(superseded_view.uv_set) == superseded.uv_set &&
                   superseded_view.request_generation == superseded.request_generation &&
                   superseded.request_generation != current.request_generation,
               "asynchronous bake token omitted a request identity") &&
           expect(foreign_result.disposition == AsyncBakeCompletionDisposition::unknown_token &&
                      stale.disposition == AsyncBakeCompletionDisposition::stale &&
                      altered_result.disposition == AsyncBakeCompletionDisposition::unknown_token &&
                      accepted.disposition == AsyncBakeCompletionDisposition::bound &&
                      duplicate.disposition == AsyncBakeCompletionDisposition::unknown_token &&
                      maps.sample(MeshMapKind::ambient_occlusion, 0.5, 0.5).sample.values[0] == 1.0,
                  "superseded or duplicate completion changed the current map") &&
           expect(edit.invalidated_requests ==
                          std::vector<BakeRequestGeneration>{old_settings.request_generation} &&
                      settings_accepted.disposition == AsyncBakeCompletionDisposition::bound &&
                      settings_stale.disposition == AsyncBakeCompletionDisposition::stale &&
                      maps.sample(MeshMapKind::curvature, 0.5, 0.5).sample.values[0] == 1.0,
                  "settings-stale bake overwrote the current generation") &&
           expect(cancellation_recorded &&
                      cancelled_result.disposition == AsyncBakeCompletionDisposition::cancelled &&
                      !maps.contains(MeshMapKind::thickness),
                  "cancelled asynchronous bake published output") &&
           expect(mesh_stale_result.disposition == AsyncBakeCompletionDisposition::stale &&
                      !maps.contains(MeshMapKind::height) &&
                      invalid.disposition == AsyncBakeCompletionDisposition::invalid_output &&
                      !maps.contains(MeshMapKind::uv_density) &&
                      session.pending_request_count() == 0,
                  "mesh-stale or malformed asynchronous bake published output");
}

bool settings_edit_and_map_replacements_undo_as_one_step() {
    doc::TextureDocument document;
    MeshMapSet maps(texture_set(document), fixture_mesh_revision);
    ProviderState initial;
    initial.pixels = {std::byte{64}};
    const BakeRequestResult initial_bind =
        request_bake(provider(initial), maps, MeshMapKind::curvature, 1, 1);
    AsyncBakeSession session(maps, 10);
    const BakeSettingsEditResult edit = session.edit_settings(11);

    ProviderState replacement;
    replacement.pixels = {std::byte{255}};
    const BakeRevisionToken replacement_token = session.begin_request(MeshMapKind::curvature, 1, 1);
    const AsyncBakeCompletionResult replacement_result =
        session.complete(replacement_token, output(replacement));
    const BakeRevisionToken pending = session.begin_request(MeshMapKind::ambient_occlusion, 1, 1);
    const BakeSettingsUndoResult undone = session.undo_settings_edit();
    const AsyncBakeCompletionResult late = session.complete(pending, output(replacement));
    const BakeSettingsUndoResult nothing_left = session.undo_settings_edit();

    const double restored = maps.sample(MeshMapKind::curvature, 0.5, 0.5).sample.values[0];
    return expect(initial_bind.status == BakeRequestStatus::completed &&
                      edit.previous_revision == 10 && edit.current_revision == 11 &&
                      replacement_result.disposition == AsyncBakeCompletionDisposition::bound &&
                      undone.restored && undone.previous_revision == 11 &&
                      undone.restored_revision == 10 && session.settings_revision() == 10,
                  "settings revision and accepted replacement were not grouped for undo") &&
           expect(restored == 64.0 / 255.0 && !maps.contains(MeshMapKind::ambient_occlusion) &&
                      undone.restored_maps == std::vector<MeshMapKind>{MeshMapKind::curvature},
                  "settings undo did not restore the exact prior map bindings") &&
           expect(undone.invalidated_requests ==
                          std::vector<BakeRequestGeneration>{pending.request_generation} &&
                      late.disposition == AsyncBakeCompletionDisposition::stale &&
                      !nothing_left.restored && session.undo_step_count() == 0,
                  "settings undo did not invalidate its pending bake result");
}

}  // namespace

int main() {
    return supported_request_reports_progress_and_binds_output() &&
                   unsupported_request_names_map_and_advertised_set() &&
                   strided_output_needs_no_padding_after_the_last_row() &&
                   normal_provider_must_declare_and_forwards_its_convention() &&
                   cancellation_before_and_during_provider_work_binds_nothing() &&
                   provider_failures_and_invalid_output_are_transactional() &&
                   malformed_provider_and_request_are_refused_before_callbacks() &&
                   synchronous_requests_forward_explicit_revisions() &&
                   async_tokens_reject_superseded_cancelled_and_stale_results() &&
                   settings_edit_and_map_replacements_undo_as_one_step()
               ? 0
               : 1;
}
