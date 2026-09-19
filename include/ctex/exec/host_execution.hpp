#ifndef CTEX_EXEC_HOST_EXECUTION_HPP
#define CTEX_EXEC_HOST_EXECUTION_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/emit/pass_plan.hpp>
#include <ctex/exec/executor.hpp>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::exec {

using DocumentRevision = std::uint64_t;
using CompletionToken = std::uint64_t;

enum class HostResourceOwner : std::uint8_t { library, host };
enum class HostResourceState : std::uint8_t {
    shader_read,
    storage_read,
    storage_write,
    render_target,
    depth_target,
};

struct HostResourceHandoff {
    emit::LogicalTexture texture;
    HostResourceOwner owner{};
    HostResourceState required_state{};
    bool output{};
    friend bool operator==(const HostResourceHandoff&, const HostResourceHandoff&) = default;
};

enum class HostReplaySemantics : std::uint8_t { deterministic, checkpoint_only };

struct HostSubmissionRequest {
    std::string operation;
    DocumentRevision base_revision{};
    std::vector<HostResourceHandoff> resources;
    HostReplaySemantics replay_semantics{};
};

struct HostSubmission {
    CompletionToken token{};
    DocumentRevision base_revision{};
    std::string operation;
    std::vector<HostResourceHandoff> resources;
};

enum class HostExecutionStatus : std::uint8_t { succeeded, failed, cancelled };

struct HostCompletedResource {
    emit::ResourceVersion version;
    emit::TextureFormat format{};
    emit::TextureExtent extent;
    friend bool operator==(const HostCompletedResource&, const HostCompletedResource&) = default;
};

enum class RecoveryEvidenceKind : std::uint8_t { result_checkpoint, deterministic_record };

struct RecoveryEvidence {
    RecoveryEvidenceKind kind{};
    bool checkpoint_complete{};
    DocumentRevision checkpoint_revision{};
    std::string operation_record_version;
    bool inputs_pinned{};
    std::size_t retained_bytes{};
    friend bool operator==(const RecoveryEvidence&, const RecoveryEvidence&) = default;
};

struct HostCompletion {
    CompletionToken token{};
    HostExecutionStatus status{};
    std::vector<HostCompletedResource> outputs;
    std::optional<RecoveryEvidence> recovery;
    std::string detail;
};

enum class HostCompletionDisposition : std::uint8_t {
    published,
    awaiting_recovery,
    stale,
    cancelled,
    failed,
    rejected,
    duplicate,
    unknown_token,
};

struct HostCompletionResult {
    HostCompletionDisposition disposition{};
    CompletionToken token{};
    std::optional<DocumentRevision> published_revision;
    std::vector<emit::ResourceVersion> released_resources;
    std::string message;
};

struct HostRecoveryReport {
    DocumentRevision recovered_revision{};
    std::vector<CompletionToken> cancelled_submissions;
    std::vector<emit::ResourceVersion> released_resources;
    std::size_t retained_recovery_bytes{};
    bool restored{};
};

class HostExecutionError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

class HostExecutedExecutor final : public Executor {
public:
    HostExecutedExecutor(std::string device_name, emit::DeviceFeatureSet features, bool attached);

    [[nodiscard]] const ExecutorDescriptor& descriptor() const noexcept override;
    void set_attached(bool attached) noexcept;

private:
    ExecutorDescriptor descriptor_;
};

class HostExecutionSession {
public:
    explicit HostExecutionSession(DocumentRevision initial_revision = 0);
    ~HostExecutionSession();
    HostExecutionSession(HostExecutionSession&&) noexcept;
    HostExecutionSession& operator=(HostExecutionSession&&) noexcept;
    HostExecutionSession(const HostExecutionSession&) = delete;
    HostExecutionSession& operator=(const HostExecutionSession&) = delete;

    [[nodiscard]] DocumentRevision revision() const noexcept;
    [[nodiscard]] std::size_t active_submission_count() const noexcept;
    [[nodiscard]] std::size_t retained_recovery_bytes() const noexcept;
    [[nodiscard]] bool resource_is_held(const emit::ResourceVersion& resource) const;
    [[nodiscard]] std::optional<emit::ResourceVersion> committed_resource(
        std::string_view logical_id) const;

    [[nodiscard]] HostSubmission submit(HostSubmissionRequest request);
    [[nodiscard]] bool cancel(CompletionToken token);
    [[nodiscard]] HostCompletionResult complete(HostCompletion completion);
    [[nodiscard]] HostCompletionResult establish_recovery(CompletionToken token,
                                                          RecoveryEvidence recovery);
    [[nodiscard]] HostRecoveryReport report_device_loss();

private:
    struct State;
    std::unique_ptr<State> state_;
};

}  // namespace ctex::exec

#endif
