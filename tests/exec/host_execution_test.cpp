#include <algorithm>
#include <ctex/exec/host_execution.hpp>
#include <iostream>
#include <string_view>

namespace {

using ctex::emit::LogicalTexture;
using ctex::emit::ResourceVersion;
using ctex::emit::TextureExtent;
using ctex::emit::TextureFormat;
using ctex::exec::CompletionToken;
using ctex::exec::HostCompletedResource;
using ctex::exec::HostCompletion;
using ctex::exec::HostCompletionDisposition;
using ctex::exec::HostExecutionSession;
using ctex::exec::HostExecutionStatus;
using ctex::exec::HostReplaySemantics;
using ctex::exec::HostResourceHandoff;
using ctex::exec::HostResourceOwner;
using ctex::exec::HostResourceState;
using ctex::exec::HostSubmissionRequest;
using ctex::exec::RecoveryEvidence;
using ctex::exec::RecoveryEvidenceKind;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

LogicalTexture texture(std::string id, std::uint64_t generation) {
    return {.version = {std::move(id), generation},
            .role = "test texture",
            .format = TextureFormat::rgba8_unorm,
            .extent = {.width = 64, .height = 32, .layers = 1},
            .mip_levels = 1,
            .tile_shape = {.width = 32, .height = 32},
            .externally_initialized = true};
}

HostResourceHandoff input(std::string id, std::uint64_t generation) {
    return {.texture = texture(std::move(id), generation),
            .owner = HostResourceOwner::library,
            .required_state = HostResourceState::shader_read,
            .output = false};
}

HostResourceHandoff output(std::string id, std::uint64_t generation) {
    return {.texture = texture(std::move(id), generation),
            .owner = HostResourceOwner::library,
            .required_state = HostResourceState::render_target,
            .output = true};
}

HostSubmissionRequest request(std::uint64_t base, ResourceVersion output_version,
                              HostReplaySemantics semantics = HostReplaySemantics::deterministic) {
    return {.operation = "paint",
            .base_revision = base,
            .resources = {input("source", base + 1),
                          output(std::move(output_version.logical_id), output_version.generation)},
            .replay_semantics = semantics};
}

HostCompletedResource completed(const HostResourceHandoff& resource) {
    return {.version = resource.texture.version,
            .format = resource.texture.format,
            .extent = resource.texture.extent};
}

RecoveryEvidence checkpoint(std::size_t retained_bytes = 64) {
    return {.kind = RecoveryEvidenceKind::result_checkpoint,
            .checkpoint_complete = true,
            .checkpoint_revision = 0,
            .operation_record_version = {},
            .inputs_pinned = false,
            .retained_bytes = retained_bytes};
}

RecoveryEvidence replay(std::uint64_t base, std::size_t retained_bytes = 32) {
    return {.kind = RecoveryEvidenceKind::deterministic_record,
            .checkpoint_complete = true,
            .checkpoint_revision = base,
            .operation_record_version = "paint-v1",
            .inputs_pinned = true,
            .retained_bytes = retained_bytes};
}

ctex::emit::DeviceFeatureSet host_features() {
    return {.binding_budget = 16,
            .maximum_texture_dimension = 8192,
            .supported_texture_formats = {TextureFormat::rgba8_unorm},
            .floating_point_filtering = true,
            .compute_available = true};
}

HostCompletion success(CompletionToken token, const HostResourceHandoff& resource,
                       RecoveryEvidence recovery) {
    return {.token = token,
            .status = HostExecutionStatus::succeeded,
            .outputs = {completed(resource)},
            .recovery = std::move(recovery),
            .detail = {}};
}

bool executor_reports_attachment_without_owning_a_device() {
    ctex::exec::HostExecutedExecutor executor("Test host GPU", host_features(), false);
    const bool detached =
        executor.descriptor().route == ctex::exec::ExecutorRoute::host_executed &&
        executor.descriptor().availability == ctex::exec::ExecutorAvailability::host_not_attached;
    executor.set_attached(true);
    return expect(detached && executor.descriptor().availability ==
                                  ctex::exec::ExecutorAvailability::available,
                  "host executor attachment was not reflected in runtime availability");
}

bool publication_is_atomic_and_retains_current_host_authority() {
    HostExecutionSession session;
    const auto submission = session.submit(request(0, {"paint", 1}));
    const ResourceVersion input_version = submission.resources[0].texture.version;
    const ResourceVersion output_version = submission.resources[1].texture.version;
    const bool held_during_execution =
        session.resource_is_held(input_version) && session.resource_is_held(output_version);
    const auto completion = session.complete(
        success(submission.token, submission.resources[1], replay(submission.base_revision, 96)));
    return expect(held_during_execution &&
                      completion.disposition == HostCompletionDisposition::published &&
                      completion.published_revision == 1 && session.revision() == 1,
                  "successful host completion did not atomically publish one revision") &&
           expect(!session.resource_is_held(input_version) &&
                      session.resource_is_held(output_version) &&
                      session.committed_resource("paint") == output_version &&
                      session.retained_recovery_bytes() == 96,
                  "submission input was not released or committed host output lost authority");
}

bool stale_and_duplicate_completions_never_publish() {
    HostExecutionSession session;
    const auto first = session.submit(request(0, {"first", 1}));
    const auto second = session.submit(request(0, {"second", 1}));
    const auto published = session.complete(success(first.token, first.resources[1], checkpoint()));
    const auto stale = session.complete(success(second.token, second.resources[1], checkpoint()));
    const auto duplicate = session.complete(success(first.token, first.resources[1], checkpoint()));
    return expect(published.disposition == HostCompletionDisposition::published &&
                      stale.disposition == HostCompletionDisposition::stale &&
                      duplicate.disposition == HostCompletionDisposition::duplicate &&
                      session.revision() == 1,
                  "stale or duplicate host completion advanced the document revision");
}

bool wrong_output_shape_is_rejected_with_both_sizes() {
    HostExecutionSession session;
    const auto submission = session.submit(request(0, {"paint", 1}));
    auto wrong = completed(submission.resources[1]);
    wrong.extent = {.width = 16, .height = 8, .layers = 1};
    const auto completion = session.complete({.token = submission.token,
                                              .status = HostExecutionStatus::succeeded,
                                              .outputs = {wrong},
                                              .recovery = checkpoint(),
                                              .detail = {}});
    return expect(completion.disposition == HostCompletionDisposition::rejected &&
                      completion.message.find("16x8x1") != std::string::npos &&
                      completion.message.find("64x32x1") != std::string::npos &&
                      session.revision() == 0,
                  "wrong host output size was not rejected without publication");
}

bool cancellation_waits_for_gpu_completion_before_release() {
    HostExecutionSession session;
    const auto submission = session.submit(request(0, {"paint", 1}));
    const ResourceVersion output_version = submission.resources[1].texture.version;
    const bool cancelled = session.cancel(submission.token);
    const bool held_after_cancel = session.resource_is_held(output_version);
    const auto late =
        session.complete(success(submission.token, submission.resources[1], checkpoint()));
    return expect(cancelled && held_after_cancel &&
                      late.disposition == HostCompletionDisposition::cancelled &&
                      !session.resource_is_held(output_version) && session.revision() == 0,
                  "late cancelled result published or recycled resources before completion");
}

bool checkpoint_only_work_waits_for_async_recovery() {
    HostExecutionSession session;
    const auto submission =
        session.submit(request(0, {"paint", 1}, HostReplaySemantics::checkpoint_only));
    const auto waiting =
        session.complete(success(submission.token, submission.resources[1], replay(0)));
    const bool retained_while_waiting =
        session.resource_is_held(submission.resources[1].texture.version);
    const bool unpublished_while_waiting = session.revision() == 0;
    const auto published = session.establish_recovery(submission.token, checkpoint(128));
    return expect(waiting.disposition == HostCompletionDisposition::awaiting_recovery &&
                      retained_while_waiting && unpublished_while_waiting &&
                      session.revision() == 1 &&
                      published.disposition == HostCompletionDisposition::published &&
                      published.published_revision == 1 && session.retained_recovery_bytes() == 128,
                  "checkpoint-only work published before asynchronous recovery completed");
}

bool replacing_a_generation_retires_only_after_completion() {
    HostExecutionSession session;
    const auto first = session.submit(request(0, {"paint", 1}));
    static_cast<void>(session.complete(success(first.token, first.resources[1], checkpoint())));
    const auto second = session.submit({.operation = "paint-again",
                                        .base_revision = 1,
                                        .resources = {input("paint", 1), output("paint", 2)},
                                        .replay_semantics = HostReplaySemantics::deterministic});
    const auto completion = session.complete(success(second.token, second.resources[1], replay(1)));
    const bool old_released = std::ranges::any_of(
        completion.released_resources,
        [](const ResourceVersion& version) { return version == ResourceVersion{"paint", 1}; });
    return expect(old_released && !session.resource_is_held({"paint", 1}) &&
                      session.resource_is_held({"paint", 2}),
                  "completed generation was not retired when its committed successor published");
}

bool device_loss_restores_commit_and_cancels_uncommitted_work() {
    HostExecutionSession session;
    const auto committed = session.submit(request(0, {"paint", 1}));
    static_cast<void>(
        session.complete(success(committed.token, committed.resources[1], checkpoint(144))));
    const auto pending = session.submit(request(1, {"other", 2}));
    const auto report = session.report_device_loss();
    const auto fallback = ctex::exec::make_fallback_report(
        "host", ctex::exec::ExecutionFailureCode::device_lost, "host device removed",
        ctex::exec::FallbackDisposition::cpu_fallback, "cpu", report.restored);
    return expect(report.restored && report.recovered_revision == 1 &&
                      report.cancelled_submissions == std::vector<CompletionToken>{pending.token} &&
                      report.retained_recovery_bytes == 144 && session.revision() == 1 &&
                      session.active_submission_count() == 0,
                  "device loss did not restore the last commit and cancel uncommitted work") &&
           expect(fallback.disposition == ctex::exec::FallbackDisposition::cpu_fallback,
                  "restored host state could not produce an explicit CPU fallback report");
}

}  // namespace

int main() {
    return executor_reports_attachment_without_owning_a_device() &&
                   publication_is_atomic_and_retains_current_host_authority() &&
                   stale_and_duplicate_completions_never_publish() &&
                   wrong_output_shape_is_rejected_with_both_sizes() &&
                   cancellation_waits_for_gpu_completion_before_release() &&
                   checkpoint_only_work_waits_for_async_recovery() &&
                   replacing_a_generation_retires_only_after_completion() &&
                   device_loss_restores_commit_and_cancels_uncommitted_work()
               ? 0
               : 1;
}
