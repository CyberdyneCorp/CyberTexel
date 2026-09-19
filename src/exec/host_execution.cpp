#include <algorithm>
#include <ctex/exec/host_execution.hpp>
#include <limits>
#include <map>
#include <set>
#include <string_view>
#include <utility>

namespace ctex::exec {
namespace {

enum class SubmissionPhase : std::uint8_t { executing, cancelled, awaiting_recovery };

struct ResourceKey {
    std::string logical_id;
    std::uint64_t generation{};
    friend bool operator<(const ResourceKey& left, const ResourceKey& right) noexcept {
        return left.logical_id < right.logical_id ||
               (left.logical_id == right.logical_id && left.generation < right.generation);
    }
};

ResourceKey key(const emit::ResourceVersion& resource) {
    return {.logical_id = resource.logical_id, .generation = resource.generation};
}

std::string resource_name(const emit::ResourceVersion& resource) {
    return resource.logical_id + "@" + std::to_string(resource.generation);
}

struct PendingSubmission {
    HostSubmission submission;
    HostReplaySemantics replay_semantics{};
    SubmissionPhase phase{};
};

bool same_version(const emit::ResourceVersion& left, const emit::ResourceVersion& right) {
    return left == right;
}

void validate_texture(const emit::LogicalTexture& texture) {
    if (texture.version.logical_id.empty() || texture.extent.width == 0 ||
        texture.extent.height == 0 || texture.extent.layers == 0 || texture.mip_levels == 0) {
        throw HostExecutionError("host resource requires an identity and non-zero dimensions");
    }
}

void validate_submission(const HostSubmissionRequest& request) {
    if (request.operation.empty() || request.resources.empty()) {
        throw HostExecutionError("host submission requires an operation and resources");
    }
    std::set<ResourceKey> identities;
    std::set<std::string_view> output_identifiers;
    bool has_output = false;
    for (const HostResourceHandoff& resource : request.resources) {
        validate_texture(resource.texture);
        if (!identities.insert(key(resource.texture.version)).second) {
            throw HostExecutionError("host submission repeats resource " +
                                     resource_name(resource.texture.version));
        }
        if (resource.output &&
            !output_identifiers.insert(resource.texture.version.logical_id).second) {
            throw HostExecutionError("host submission repeats output logical identity " +
                                     resource.texture.version.logical_id);
        }
        has_output |= resource.output;
    }
    if (!has_output) {
        throw HostExecutionError("host submission requires at least one output resource");
    }
}

const HostResourceHandoff* expected_output(const PendingSubmission& pending,
                                           const emit::ResourceVersion& version) {
    const auto found =
        std::find_if(pending.submission.resources.begin(), pending.submission.resources.end(),
                     [&](const auto& resource) {
                         return resource.output && same_version(resource.texture.version, version);
                     });
    return found == pending.submission.resources.end() ? nullptr : &*found;
}

std::optional<std::string> output_mismatch(const PendingSubmission& pending,
                                           std::span<const HostCompletedResource> completed) {
    const std::size_t expected_count = static_cast<std::size_t>(
        std::count_if(pending.submission.resources.begin(), pending.submission.resources.end(),
                      [](const HostResourceHandoff& resource) { return resource.output; }));
    if (completed.size() != expected_count) {
        return "host returned " + std::to_string(completed.size()) + " outputs; expected " +
               std::to_string(expected_count);
    }
    std::set<ResourceKey> seen;
    for (const HostCompletedResource& output : completed) {
        const HostResourceHandoff* expected = expected_output(pending, output.version);
        if (expected == nullptr || !seen.insert(key(output.version)).second) {
            return "host returned an undeclared or duplicate output " +
                   resource_name(output.version);
        }
        if (output.format != expected->texture.format) {
            return "host returned the wrong format for " + resource_name(output.version);
        }
        if (output.extent != expected->texture.extent) {
            return "host returned " + std::to_string(output.extent.width) + "x" +
                   std::to_string(output.extent.height) + "x" +
                   std::to_string(output.extent.layers) + " for " + resource_name(output.version) +
                   "; expected " + std::to_string(expected->texture.extent.width) + "x" +
                   std::to_string(expected->texture.extent.height) + "x" +
                   std::to_string(expected->texture.extent.layers);
        }
    }
    return std::nullopt;
}

bool valid_recovery(const RecoveryEvidence& recovery, HostReplaySemantics semantics,
                    DocumentRevision base_revision) {
    if (!recovery.checkpoint_complete) {
        return false;
    }
    if (recovery.kind == RecoveryEvidenceKind::result_checkpoint) {
        return true;
    }
    return semantics == HostReplaySemantics::deterministic &&
           recovery.checkpoint_revision == base_revision &&
           !recovery.operation_record_version.empty() && recovery.inputs_pinned;
}

std::string recovery_requirement(HostReplaySemantics semantics) {
    if (semantics == HostReplaySemantics::checkpoint_only) {
        return "checkpoint-only operation requires a completed result checkpoint";
    }
    return "operation requires a completed result checkpoint or a base checkpoint with a "
           "versioned deterministic record and pinned inputs";
}

HostCompletionResult result(HostCompletionDisposition disposition, CompletionToken token,
                            std::string message, std::vector<emit::ResourceVersion> released = {},
                            std::optional<DocumentRevision> revision = std::nullopt) {
    return {.disposition = disposition,
            .token = token,
            .published_revision = revision,
            .released_resources = std::move(released),
            .message = std::move(message)};
}

}  // namespace

struct HostExecutionSession::State {
    DocumentRevision revision{};
    CompletionToken next_token{1};
    std::map<CompletionToken, PendingSubmission> active;
    std::map<ResourceKey, std::size_t> holds;
    std::map<std::string, emit::ResourceVersion> committed_resources;
    RecoveryEvidence committed_recovery{.kind = RecoveryEvidenceKind::result_checkpoint,
                                        .checkpoint_complete = true,
                                        .checkpoint_revision = 0,
                                        .operation_record_version = {},
                                        .inputs_pinned = false,
                                        .retained_bytes = 0};
};

namespace {

void hold_resources(auto& state, const HostSubmission& submission) {
    for (const HostResourceHandoff& resource : submission.resources) {
        ++state.holds[key(resource.texture.version)];
    }
}

std::vector<emit::ResourceVersion> release_resources(auto& state,
                                                     const HostSubmission& submission) {
    std::vector<emit::ResourceVersion> released;
    for (const HostResourceHandoff& resource : submission.resources) {
        const ResourceKey identity = key(resource.texture.version);
        auto found = state.holds.find(identity);
        if (found == state.holds.end()) {
            continue;
        }
        if (--found->second == 0) {
            released.push_back(resource.texture.version);
            state.holds.erase(found);
        }
    }
    return released;
}

HostCompletionResult finish(auto& state,
                            std::map<CompletionToken, PendingSubmission>::iterator position,
                            HostCompletionDisposition disposition, std::string message,
                            std::optional<DocumentRevision> published = std::nullopt,
                            std::vector<emit::ResourceVersion> released = {}) {
    const CompletionToken token = position->first;
    auto submission_released = release_resources(state, position->second.submission);
    released.insert(released.end(), submission_released.begin(), submission_released.end());
    state.active.erase(position);
    return result(disposition, token, std::move(message), std::move(released), published);
}

std::vector<emit::ResourceVersion> adopt_outputs(auto& state, const PendingSubmission& pending) {
    std::vector<emit::ResourceVersion> released;
    for (const HostResourceHandoff& resource : pending.submission.resources) {
        if (!resource.output) {
            continue;
        }
        auto old = state.committed_resources.find(resource.texture.version.logical_id);
        if (old != state.committed_resources.end() && old->second != resource.texture.version) {
            auto hold = state.holds.find(key(old->second));
            if (hold != state.holds.end() && --hold->second == 0) {
                released.push_back(old->second);
                state.holds.erase(hold);
            }
        }
        if (old == state.committed_resources.end() || old->second != resource.texture.version) {
            ++state.holds[key(resource.texture.version)];
            state.committed_resources[resource.texture.version.logical_id] =
                resource.texture.version;
        }
    }
    return released;
}

HostCompletionResult publish(auto& state,
                             std::map<CompletionToken, PendingSubmission>::iterator position,
                             RecoveryEvidence recovery) {
    if (position->second.submission.base_revision != state.revision) {
        return finish(state, position, HostCompletionDisposition::stale,
                      "completion base revision is stale; no revision was published");
    }
    if (state.revision == std::numeric_limits<DocumentRevision>::max()) {
        return finish(state, position, HostCompletionDisposition::rejected,
                      "document revision space is exhausted");
    }
    auto released = adopt_outputs(state, position->second);
    ++state.revision;
    state.committed_recovery = std::move(recovery);
    return finish(state, position, HostCompletionDisposition::published,
                  "host result published atomically", state.revision, std::move(released));
}

}  // namespace

HostExecutedExecutor::HostExecutedExecutor(std::string device_name, emit::DeviceFeatureSet features,
                                           bool attached)
    : descriptor_{.identifier = "host",
                  .display_name = "Host executed",
                  .device_name = std::move(device_name),
                  .route = ExecutorRoute::host_executed,
                  .availability = attached ? ExecutorAvailability::available
                                           : ExecutorAvailability::host_not_attached,
                  .features = std::move(features)} {
    if (descriptor_.device_name.empty() || descriptor_.features.binding_budget < 2 ||
        descriptor_.features.maximum_texture_dimension == 0 ||
        descriptor_.features.supported_texture_formats.empty()) {
        throw HostExecutionError("host attachment requires a device name and valid features");
    }
}

const ExecutorDescriptor& HostExecutedExecutor::descriptor() const noexcept { return descriptor_; }

void HostExecutedExecutor::set_attached(bool attached) noexcept {
    descriptor_.availability =
        attached ? ExecutorAvailability::available : ExecutorAvailability::host_not_attached;
}

HostExecutionSession::HostExecutionSession(DocumentRevision initial_revision)
    : state_(std::make_unique<State>()) {
    state_->revision = initial_revision;
    state_->committed_recovery.checkpoint_revision = initial_revision;
}

HostExecutionSession::~HostExecutionSession() = default;
HostExecutionSession::HostExecutionSession(HostExecutionSession&&) noexcept = default;
HostExecutionSession& HostExecutionSession::operator=(HostExecutionSession&&) noexcept = default;

DocumentRevision HostExecutionSession::revision() const noexcept { return state_->revision; }

std::size_t HostExecutionSession::active_submission_count() const noexcept {
    return state_->active.size();
}

std::size_t HostExecutionSession::retained_recovery_bytes() const noexcept {
    return state_->committed_recovery.retained_bytes;
}

bool HostExecutionSession::resource_is_held(const emit::ResourceVersion& resource) const {
    return state_->holds.contains(key(resource));
}

std::optional<emit::ResourceVersion> HostExecutionSession::committed_resource(
    std::string_view logical_id) const {
    const auto found = state_->committed_resources.find(std::string(logical_id));
    return found == state_->committed_resources.end()
               ? std::nullopt
               : std::optional<emit::ResourceVersion>{found->second};
}

HostSubmission HostExecutionSession::submit(HostSubmissionRequest request) {
    validate_submission(request);
    if (request.base_revision != state_->revision) {
        throw HostExecutionError(
            "host submission base revision " + std::to_string(request.base_revision) +
            " does not match current revision " + std::to_string(state_->revision));
    }
    for (const HostResourceHandoff& resource : request.resources) {
        if (resource.output && state_->holds.contains(key(resource.texture.version))) {
            throw HostExecutionError("host output resource generation is already retained: " +
                                     resource_name(resource.texture.version));
        }
    }
    if (state_->next_token == std::numeric_limits<CompletionToken>::max()) {
        throw HostExecutionError("host completion token space is exhausted");
    }
    HostSubmission submission{.token = state_->next_token++,
                              .base_revision = request.base_revision,
                              .operation = std::move(request.operation),
                              .resources = std::move(request.resources)};
    hold_resources(*state_, submission);
    const CompletionToken token = submission.token;
    state_->active.emplace(token, PendingSubmission{.submission = submission,
                                                    .replay_semantics = request.replay_semantics,
                                                    .phase = SubmissionPhase::executing});
    return submission;
}

bool HostExecutionSession::cancel(CompletionToken token) {
    auto position = state_->active.find(token);
    if (position == state_->active.end()) {
        return false;
    }
    if (position->second.phase == SubmissionPhase::awaiting_recovery) {
        static_cast<void>(finish(*state_, position, HostCompletionDisposition::cancelled,
                                 "completed result cancelled before recovery publication"));
        return true;
    }
    position->second.phase = SubmissionPhase::cancelled;
    return true;
}

HostCompletionResult HostExecutionSession::complete(HostCompletion completion) {
    auto position = state_->active.find(completion.token);
    if (position == state_->active.end()) {
        const auto disposition = completion.token != 0 && completion.token < state_->next_token
                                     ? HostCompletionDisposition::duplicate
                                     : HostCompletionDisposition::unknown_token;
        return result(disposition, completion.token,
                      disposition == HostCompletionDisposition::duplicate
                          ? "completion token is already terminal"
                          : "completion token is unknown");
    }
    if (position->second.phase == SubmissionPhase::awaiting_recovery) {
        return result(HostCompletionDisposition::duplicate, completion.token,
                      "execution completion was already recorded; recovery is pending");
    }
    if (position->second.phase == SubmissionPhase::cancelled ||
        completion.status == HostExecutionStatus::cancelled) {
        return finish(*state_, position, HostCompletionDisposition::cancelled,
                      "cancelled host result discarded without publication");
    }
    if (completion.status == HostExecutionStatus::failed) {
        const std::string detail =
            completion.detail.empty() ? "host execution failed" : std::move(completion.detail);
        return finish(*state_, position, HostCompletionDisposition::failed, detail);
    }
    if (const auto mismatch = output_mismatch(position->second, completion.outputs)) {
        return finish(*state_, position, HostCompletionDisposition::rejected, *mismatch);
    }
    if (position->second.submission.base_revision != state_->revision) {
        return finish(*state_, position, HostCompletionDisposition::stale,
                      "completion base revision is stale; no revision was published");
    }
    if (!completion.recovery ||
        !valid_recovery(*completion.recovery, position->second.replay_semantics,
                        position->second.submission.base_revision)) {
        position->second.phase = SubmissionPhase::awaiting_recovery;
        return result(HostCompletionDisposition::awaiting_recovery, completion.token,
                      recovery_requirement(position->second.replay_semantics));
    }
    return publish(*state_, position, std::move(*completion.recovery));
}

HostCompletionResult HostExecutionSession::establish_recovery(CompletionToken token,
                                                              RecoveryEvidence recovery) {
    auto position = state_->active.find(token);
    if (position == state_->active.end()) {
        const auto disposition = token != 0 && token < state_->next_token
                                     ? HostCompletionDisposition::duplicate
                                     : HostCompletionDisposition::unknown_token;
        return result(disposition, token,
                      disposition == HostCompletionDisposition::duplicate
                          ? "completion token is already terminal"
                          : "completion token is unknown");
    }
    if (position->second.phase != SubmissionPhase::awaiting_recovery) {
        return result(HostCompletionDisposition::rejected, token,
                      "execution has not completed successfully");
    }
    if (!valid_recovery(recovery, position->second.replay_semantics,
                        position->second.submission.base_revision)) {
        return result(HostCompletionDisposition::awaiting_recovery, token,
                      recovery_requirement(position->second.replay_semantics));
    }
    return publish(*state_, position, std::move(recovery));
}

HostRecoveryReport HostExecutionSession::report_device_loss() {
    HostRecoveryReport report{.recovered_revision = state_->revision,
                              .cancelled_submissions = {},
                              .released_resources = {},
                              .retained_recovery_bytes = state_->committed_recovery.retained_bytes,
                              .restored = state_->committed_recovery.checkpoint_complete};
    while (!state_->active.empty()) {
        auto position = state_->active.begin();
        report.cancelled_submissions.push_back(position->first);
        auto released = release_resources(*state_, position->second.submission);
        report.released_resources.insert(report.released_resources.end(), released.begin(),
                                         released.end());
        state_->active.erase(position);
    }
    return report;
}

}  // namespace ctex::exec
