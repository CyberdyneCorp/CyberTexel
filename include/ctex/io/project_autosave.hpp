#ifndef CTEX_IO_PROJECT_AUTOSAVE_HPP
#define CTEX_IO_PROJECT_AUTOSAVE_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctex/io/project_container.hpp>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ctex::io {

struct ProjectSnapshotImageSource {
    std::string resource_id;
    const image::TiledImage* image{};
};

struct ProjectSnapshotMetadata {
    ContainerSchemaVersion schema_version{current_container_schema};
    std::vector<ProjectResource> resources;
    std::vector<StandaloneAsset> assets;
    std::vector<OpaqueContainerSection> opaque_sections;
};

class ProjectSaveSnapshot {
public:
    ProjectSaveSnapshot(ProjectSaveSnapshot&&) noexcept;
    ProjectSaveSnapshot& operator=(ProjectSaveSnapshot&&) noexcept;
    ProjectSaveSnapshot(const ProjectSaveSnapshot&) = delete;
    ProjectSaveSnapshot& operator=(const ProjectSaveSnapshot&) = delete;
    ~ProjectSaveSnapshot();

    [[nodiscard]] ProjectRevision revision() const noexcept;
    [[nodiscard]] std::size_t image_count() const noexcept;
    [[nodiscard]] std::size_t retained_pixel_bytes() const noexcept;

private:
    struct Impl;
    explicit ProjectSaveSnapshot(std::unique_ptr<Impl> impl) noexcept;
    std::unique_ptr<Impl> impl_;

    friend ProjectSaveSnapshot capture_project_snapshot(
        ProjectRevision, ProjectSnapshotMetadata, std::span<const ProjectSnapshotImageSource>);
    friend ProjectContainer materialize_project_snapshot(const ProjectSaveSnapshot&);
};

[[nodiscard]] ProjectSaveSnapshot capture_project_snapshot(
    ProjectRevision revision, ProjectSnapshotMetadata metadata,
    std::span<const ProjectSnapshotImageSource> images);
[[nodiscard]] ProjectContainer materialize_project_snapshot(const ProjectSaveSnapshot& snapshot);
void save_project_snapshot_atomic(const std::filesystem::path& path,
                                  const ProjectSaveSnapshot& snapshot);

struct ProjectAutosaveConfig {
    std::filesystem::path recovery_directory;
    std::string recovery_key;
    std::chrono::milliseconds interval{std::chrono::minutes(5)};
};

enum class AutosaveSubmissionStatus : std::uint8_t { queued, stale_revision };

struct ProjectAutosaveStatus {
    std::optional<ProjectRevision> last_saved_revision;
    std::optional<ProjectRevision> pending_revision;
    std::optional<ProjectRevision> saving_revision;
    std::uint64_t successful_writes{};
    std::string last_error;
};

class ProjectAutosaveSession {
public:
    explicit ProjectAutosaveSession(ProjectAutosaveConfig config);
    ProjectAutosaveSession(ProjectAutosaveSession&&) = delete;
    ProjectAutosaveSession& operator=(ProjectAutosaveSession&&) = delete;
    ProjectAutosaveSession(const ProjectAutosaveSession&) = delete;
    ProjectAutosaveSession& operator=(const ProjectAutosaveSession&) = delete;
    ~ProjectAutosaveSession();

    [[nodiscard]] AutosaveSubmissionStatus submit(ProjectSaveSnapshot snapshot);
    [[nodiscard]] bool wait_until_idle(std::chrono::milliseconds timeout);
    void request_flush() noexcept;
    [[nodiscard]] bool wait_until_saved(ProjectRevision revision,
                                        std::chrono::milliseconds timeout);
    void flush();
    [[nodiscard]] ProjectAutosaveStatus status() const;
    [[nodiscard]] const std::filesystem::path& recovery_path() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

struct RecoverableProject {
    std::string recovery_key;
    std::filesystem::path path;
    ContainerSchemaVersion schema_version;
    std::filesystem::file_time_type last_write_time;
    std::uintmax_t file_bytes{};
};

struct RejectedRecoveryFile {
    std::filesystem::path path;
    std::string message;
};

struct RecoveryEnumeration {
    std::vector<RecoverableProject> recoverable;
    std::vector<RejectedRecoveryFile> rejected;
};

[[nodiscard]] RecoveryEnumeration enumerate_recoverable_projects(
    const std::filesystem::path& recovery_directory);

}  // namespace ctex::io

#endif
