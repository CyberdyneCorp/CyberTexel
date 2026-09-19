#include <array>
#include <chrono>
#include <cstddef>
#include <ctex/io/project_autosave.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::io;
using namespace std::chrono_literals;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, ProjectContainerErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const ProjectContainerError& error) {
        return expect(error.code() == code, "autosave returned the wrong error code");
    } catch (...) {
    }
    return expect(false, message);
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return {};
    }
    const std::streampos end = stream.tellg();
    std::vector<std::byte> bytes(static_cast<std::size_t>(end));
    stream.seekg(0);
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return stream ? bytes : std::vector<std::byte>{};
}

ProjectSaveSnapshot capture(ProjectRevision revision, const image::TiledImage& image) {
    const ProjectSnapshotImageSource source{.resource_id = "layers/base-color", .image = &image};
    return capture_project_snapshot(revision, {}, std::span(&source, 1));
}

bool pinned_snapshot_remains_consistent_while_painting_continues() {
    constexpr std::uint32_t canvas_size = 16'384;
    const std::array clear{std::byte{0}};
    image::TiledImage image(canvas_size, canvas_size,
                            {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1},
                            64, clear);
    const std::array before{std::byte{17}};
    const std::array after{std::byte{203}};
    image.write_pixel(canvas_size - 1, canvas_size - 1, before);
    image.write_pixel(0, 0, before);

    ProjectSnapshotMetadata metadata;
    metadata.assets.push_back({.identifier = "materials/snapshot",
                               .kind = "material",
                               .format_version = 1,
                               .resource_dependencies = {},
                               .tiled_image_dependencies = {"layers/base-color"},
                               .payload = {std::byte{'m'}}});
    const ProjectSnapshotImageSource source{.resource_id = "layers/base-color", .image = &image};
    const ProjectSaveSnapshot snapshot =
        capture_project_snapshot(41, std::move(metadata), std::span(&source, 1));
    image.write_pixel(canvas_size - 1, canvas_size - 1, after);
    const ProjectContainer materialized = materialize_project_snapshot(snapshot);
    const image::TiledImage restored = restore_tiled_image(materialized.tiled_images.front());

    return expect(snapshot.revision() == 41 && snapshot.image_count() == 1 &&
                      snapshot.retained_pixel_bytes() == image.tile_bytes() * 2 &&
                      materialized.tiled_images.front().occupied_tiles.size() == 2 &&
                      materialized.assets.size() == 1 &&
                      materialized.assets.front().identifier == "materials/snapshot" &&
                      materialized.tiled_images.front().occupied_tiles.front().coordinate ==
                          image::TileCoordinate{0, 0} &&
                      materialized.tiled_images.front().occupied_tiles.back().coordinate ==
                          image::TileCoordinate{255, 255},
                  "snapshot did not pin and order only the occupied image tiles") &&
           expect(restored.read_pixel(canvas_size - 1, canvas_size - 1)[0] == before[0] &&
                      image.read_pixel(canvas_size - 1, canvas_size - 1)[0] == after[0],
                  "painting after capture changed the committed snapshot");
}

bool periodic_autosave_coalesces_and_enumerates_recovery() {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "ctex-project-autosave-test";
    std::error_code filesystem_error;
    std::filesystem::remove_all(directory, filesystem_error);

    const std::array clear{std::byte{0}};
    image::TiledImage image(
        64, 64, {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1}, 64, clear);
    const std::array first{std::byte{10}};
    const std::array second{std::byte{20}};
    image.write_pixel(0, 0, first);
    ProjectSaveSnapshot revision_one = capture(1, image);
    image.write_pixel(0, 0, second);
    ProjectSaveSnapshot revision_two = capture(2, image);
    ProjectSaveSnapshot stale = capture(1, image);

    ProjectAutosaveStatus saved_status;
    std::filesystem::path recovery_path;
    {
        ProjectAutosaveSession autosave(
            {.recovery_directory = directory, .recovery_key = "document-7", .interval = 250ms});
        recovery_path = autosave.recovery_path();
        const AutosaveSubmissionStatus first_status = autosave.submit(std::move(revision_one));
        const AutosaveSubmissionStatus second_status = autosave.submit(std::move(revision_two));
        const AutosaveSubmissionStatus stale_status = autosave.submit(std::move(stale));
        const ProjectAutosaveStatus queued_status = autosave.status();
        const bool deferred = !std::filesystem::exists(recovery_path) &&
                              queued_status.pending_revision == 2 &&
                              !queued_status.saving_revision.has_value();
        const bool completed = autosave.wait_until_idle(30s);
        saved_status = autosave.status();
        if (!expect(first_status == AutosaveSubmissionStatus::queued &&
                        second_status == AutosaveSubmissionStatus::queued &&
                        stale_status == AutosaveSubmissionStatus::stale_revision && deferred &&
                        completed,
                    "autosave did not defer, coalesce, or reject a stale revision")) {
            return false;
        }
    }

    const std::vector<std::byte> bytes = read_binary_file(recovery_path);
    const ProjectContainerReadResult opened = read_project_container(bytes);
    const image::TiledImage restored = restore_tiled_image(opened.container.tiled_images.front());
    {
        std::ofstream malformed(directory / "broken.ctex-recovery", std::ios::binary);
        malformed << "bad";
    }
    {
        std::ofstream temporary(directory / ".document.ctex-recovery.tmp.1", std::ios::binary);
        temporary << "partial";
    }
    const RecoveryEnumeration recovery = enumerate_recoverable_projects(directory);
    std::filesystem::remove_all(directory, filesystem_error);

    return expect(saved_status.last_saved_revision == 2 &&
                      !saved_status.pending_revision.has_value() &&
                      !saved_status.saving_revision.has_value() &&
                      saved_status.successful_writes == 1 && saved_status.last_error.empty(),
                  "autosave status did not report the coalesced revision") &&
           expect(restored.read_pixel(0, 0)[0] == second[0],
                  "autosave did not persist the latest consistent snapshot") &&
           expect(recovery.recoverable.size() == 1 && recovery.rejected.size() == 1 &&
                      recovery.recoverable.front().recovery_key == "document-7" &&
                      recovery.recoverable.front().schema_version == current_container_schema &&
                      recovery.recoverable.front().file_bytes == bytes.size(),
                  "recovery enumeration did not report valid and malformed autosaves");
}

bool flush_publishes_immediately_and_configuration_is_validated() {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "ctex-project-autosave-flush-test";
    std::error_code filesystem_error;
    std::filesystem::remove_all(directory, filesystem_error);
    const std::array clear{std::byte{0}};
    image::TiledImage image(
        1, 1, {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1}, 1, clear);
    const std::array painted{std::byte{99}};
    image.write_pixel(0, 0, painted);

    bool flushed = false;
    {
        ProjectAutosaveSession autosave(
            {.recovery_directory = directory, .recovery_key = "manual", .interval = 1h});
        static_cast<void>(autosave.submit(capture(9, image)));
        autosave.flush();
        flushed = std::filesystem::is_regular_file(autosave.recovery_path()) &&
                  autosave.status().last_saved_revision == 9;
    }
    const bool invalid_key = expect_error(
        [&] {
            ProjectAutosaveSession autosave(
                {.recovery_directory = directory, .recovery_key = "../escape", .interval = 1s});
        },
        ProjectContainerErrorCode::invalid_snapshot, "unsafe recovery key was accepted");
    const bool invalid_interval = expect_error(
        [&] {
            ProjectAutosaveSession autosave(
                {.recovery_directory = directory, .recovery_key = "valid", .interval = 0ms});
        },
        ProjectContainerErrorCode::invalid_snapshot, "zero autosave interval was accepted");
    std::filesystem::remove_all(directory, filesystem_error);
    return expect(flushed, "explicit autosave flush did not publish the pending snapshot") &&
           invalid_key && invalid_interval;
}

}  // namespace

int main() {
    return pinned_snapshot_remains_consistent_while_painting_continues() &&
                   periodic_autosave_coalesces_and_enumerates_recovery() &&
                   flush_publishes_immediately_and_configuration_is_validated()
               ? 0
               : 1;
}
