#include <algorithm>
#include <array>
#include <condition_variable>
#include <ctex/io/project_autosave.hpp>
#include <fstream>
#include <limits>
#include <mutex>
#include <set>
#include <thread>
#include <utility>

namespace ctex::io {
namespace {

constexpr std::string_view recovery_suffix = ".ctex-recovery";

struct PinnedTile {
    image::TileCoordinate coordinate;
    image::TileExtent extent;
    image::TileStorageHandle storage;
};

struct PinnedImage {
    StoredTiledImage metadata;
    std::vector<PinnedTile> tiles;
};

void add_size(std::size_t& total, std::size_t addition, std::string_view description) {
    if (addition > std::numeric_limits<std::size_t>::max() - total) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    std::string(description) + " byte count overflows");
    }
    total += addition;
}

PinnedImage pin_image(const ProjectSnapshotImageSource& source, std::size_t& retained_bytes,
                      std::set<const void*>& retained_allocations) {
    if (source.image == nullptr || source.resource_id.empty()) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_snapshot,
                                    "project snapshot image source is invalid");
    }
    const image::TiledImage& image = *source.image;
    const image::RevisionCursor cursor = image.revision_cursor();
    PinnedImage pinned{
        .metadata = {.resource_id = source.resource_id,
                     .width = image.width(),
                     .height = image.height(),
                     .tile_size = image.tile_size(),
                     .format = image.format(),
                     .clear_pixel = {image.clear_pixel().begin(), image.clear_pixel().end()},
                     .occupied_tiles = {}},
        .tiles = {}};
    const std::vector<image::TileCoordinate> coordinates = image.allocated_tiles();
    pinned.tiles.reserve(coordinates.size());
    for (const image::TileCoordinate coordinate : coordinates) {
        image::TileStorageHandle storage = image.pin_tile_storage(coordinate);
        if (!storage) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_snapshot,
                                        "project image changed while its snapshot was captured");
        }
        if (retained_allocations.insert(storage.get()).second) {
            add_size(retained_bytes, storage->size(), "snapshot retained pixel");
        }
        pinned.tiles.push_back({.coordinate = coordinate,
                                .extent = image.tile_extent(coordinate),
                                .storage = std::move(storage)});
    }
    if (image.revision_cursor() != cursor) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_snapshot,
                                    "project image changed while its snapshot was captured");
    }
    return pinned;
}

StoredTile materialize_tile(const PinnedImage& image, const PinnedTile& pinned) {
    StoredTile tile{.coordinate = pinned.coordinate,
                    .extent = pinned.extent,
                    .compression = TileCompression::zlib_deflate,
                    .pixels = {}};
    const std::size_t pixel_bytes = image.metadata.format.bytes_per_pixel();
    const std::size_t row_bytes = pinned.extent.width * pixel_bytes;
    tile.pixels.reserve(row_bytes * pinned.extent.height);
    for (std::uint32_t row = 0; row < pinned.extent.height; ++row) {
        const auto begin = pinned.storage->begin() +
                           static_cast<std::ptrdiff_t>(static_cast<std::size_t>(row) *
                                                       image.metadata.tile_size * pixel_bytes);
        tile.pixels.insert(tile.pixels.end(), begin,
                           begin + static_cast<std::ptrdiff_t>(row_bytes));
    }
    return tile;
}

bool valid_recovery_key(std::string_view key) {
    if (key.empty() || key.size() > 128 || key == "." || key == "..") {
        return false;
    }
    return std::all_of(key.begin(), key.end(), [](char character) {
        const bool ascii_letter =
            (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
        const bool ascii_digit = character >= '0' && character <= '9';
        return ascii_letter || ascii_digit || character == '-' || character == '_' ||
               character == '.';
    });
}

std::filesystem::path recovery_path(const ProjectAutosaveConfig& config) {
    if (!valid_recovery_key(config.recovery_key)) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_snapshot,
                                    "autosave recovery key is not portable");
    }
    if (config.recovery_directory.empty() || config.interval <= std::chrono::milliseconds::zero()) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_snapshot,
                                    "autosave directory or interval is invalid");
    }
    return config.recovery_directory / (config.recovery_key + std::string(recovery_suffix));
}

}  // namespace

struct ProjectSaveSnapshot::Impl {
    ProjectRevision revision{};
    ProjectSnapshotMetadata metadata;
    std::vector<PinnedImage> images;
    std::size_t retained_pixel_bytes{};
};

ProjectSaveSnapshot::ProjectSaveSnapshot(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}
ProjectSaveSnapshot::ProjectSaveSnapshot(ProjectSaveSnapshot&&) noexcept = default;
ProjectSaveSnapshot& ProjectSaveSnapshot::operator=(ProjectSaveSnapshot&&) noexcept = default;
ProjectSaveSnapshot::~ProjectSaveSnapshot() = default;

ProjectRevision ProjectSaveSnapshot::revision() const noexcept {
    return impl_ ? impl_->revision : 0;
}

std::size_t ProjectSaveSnapshot::image_count() const noexcept {
    return impl_ ? impl_->images.size() : 0;
}

std::size_t ProjectSaveSnapshot::retained_pixel_bytes() const noexcept {
    return impl_ ? impl_->retained_pixel_bytes : 0;
}

ProjectSaveSnapshot capture_project_snapshot(ProjectRevision revision,
                                             ProjectSnapshotMetadata metadata,
                                             std::span<const ProjectSnapshotImageSource> images) {
    if (revision == 0) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_snapshot,
                                    "project snapshot revision must be non-zero");
    }
    auto snapshot = std::make_unique<ProjectSaveSnapshot::Impl>();
    snapshot->revision = revision;
    snapshot->metadata = std::move(metadata);
    snapshot->images.reserve(images.size());
    std::set<std::string> identities;
    std::set<const void*> retained_allocations;
    for (const ProjectSnapshotImageSource& source : images) {
        if (!identities.insert(source.resource_id).second) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_snapshot,
                                        "project snapshot repeats an image identity");
        }
        snapshot->images.push_back(
            pin_image(source, snapshot->retained_pixel_bytes, retained_allocations));
    }
    return ProjectSaveSnapshot(std::move(snapshot));
}

ProjectContainer materialize_project_snapshot(const ProjectSaveSnapshot& snapshot) {
    if (!snapshot.impl_) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_snapshot,
                                    "project snapshot has been moved from");
    }
    ProjectContainer container{.schema_version = snapshot.impl_->metadata.schema_version,
                               .tiled_images = {},
                               .resources = snapshot.impl_->metadata.resources,
                               .assets = snapshot.impl_->metadata.assets,
                               .opaque_sections = snapshot.impl_->metadata.opaque_sections,
                               .recovery_checkpoint_revision = snapshot.impl_->revision};
    container.tiled_images.reserve(snapshot.impl_->images.size());
    for (const PinnedImage& pinned : snapshot.impl_->images) {
        StoredTiledImage image = pinned.metadata;
        image.occupied_tiles.reserve(pinned.tiles.size());
        for (const PinnedTile& tile : pinned.tiles) {
            image.occupied_tiles.push_back(materialize_tile(pinned, tile));
        }
        container.tiled_images.push_back(std::move(image));
    }
    return container;
}

void save_project_snapshot_atomic(const std::filesystem::path& path,
                                  const ProjectSaveSnapshot& snapshot) {
    save_project_container_atomic(path, materialize_project_snapshot(snapshot));
}

struct ProjectAutosaveSession::Impl {
    explicit Impl(ProjectAutosaveConfig input)
        : config(std::move(input)), path(io::recovery_path(config)) {
        std::error_code error;
        std::filesystem::create_directories(config.recovery_directory, error);
        if (error) {
            throw ProjectContainerError(
                ProjectContainerErrorCode::filesystem_failure,
                "could not create autosave recovery directory: " + error.message());
        }
        next_write = std::chrono::steady_clock::now() + config.interval;
        worker = std::thread([this] { run(); });
    }

    ~Impl() {
        {
            std::scoped_lock lock(mutex);
            stopping = true;
            pending.reset();
        }
        changed.notify_all();
        worker.join();
    }

    [[nodiscard]] ProjectRevision newest_revision() const noexcept {
        ProjectRevision newest = status.last_saved_revision.value_or(0);
        newest = std::max(newest, status.saving_revision.value_or(0));
        newest = std::max(newest, status.pending_revision.value_or(0));
        return newest;
    }

    void run() {
        std::unique_lock lock(mutex);
        while (!stopping) {
            changed.wait(lock, [&] { return stopping || pending.has_value(); });
            if (stopping) {
                break;
            }
            if (!flush_requested && std::chrono::steady_clock::now() < next_write) {
                changed.wait_until(lock, next_write, [&] { return stopping || flush_requested; });
                continue;
            }
            ProjectSaveSnapshot snapshot = std::move(*pending);
            pending.reset();
            status.pending_revision.reset();
            status.saving_revision = snapshot.revision();
            flush_requested = false;
            lock.unlock();
            std::string error;
            try {
                save_project_snapshot_atomic(path, snapshot);
            } catch (const std::exception& failure) {
                error = failure.what();
            }
            lock.lock();
            if (error.empty()) {
                status.last_saved_revision = snapshot.revision();
                ++status.successful_writes;
                status.last_error.clear();
            } else {
                status.last_error = std::move(error);
            }
            status.saving_revision.reset();
            next_write = std::chrono::steady_clock::now() + config.interval;
            changed.notify_all();
        }
    }

    ProjectAutosaveConfig config;
    std::filesystem::path path;
    mutable std::mutex mutex;
    std::condition_variable changed;
    std::thread worker;
    std::optional<ProjectSaveSnapshot> pending;
    ProjectAutosaveStatus status;
    std::chrono::steady_clock::time_point next_write;
    bool flush_requested{};
    bool stopping{};
};

ProjectAutosaveSession::ProjectAutosaveSession(ProjectAutosaveConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

ProjectAutosaveSession::~ProjectAutosaveSession() = default;

AutosaveSubmissionStatus ProjectAutosaveSession::submit(ProjectSaveSnapshot snapshot) {
    std::scoped_lock lock(impl_->mutex);
    if (snapshot.revision() <= impl_->newest_revision() &&
        (impl_->status.last_saved_revision.has_value() ||
         impl_->status.saving_revision.has_value() || impl_->status.pending_revision.has_value())) {
        return AutosaveSubmissionStatus::stale_revision;
    }
    impl_->status.pending_revision = snapshot.revision();
    impl_->pending = std::move(snapshot);
    impl_->changed.notify_all();
    return AutosaveSubmissionStatus::queued;
}

bool ProjectAutosaveSession::wait_until_idle(std::chrono::milliseconds timeout) {
    std::unique_lock lock(impl_->mutex);
    return impl_->changed.wait_for(lock, timeout, [&] {
        return !impl_->pending.has_value() && !impl_->status.saving_revision.has_value();
    });
}

void ProjectAutosaveSession::request_flush() noexcept {
    std::scoped_lock lock(impl_->mutex);
    if (impl_->pending.has_value()) {
        impl_->flush_requested = true;
        impl_->changed.notify_all();
    }
}

bool ProjectAutosaveSession::wait_until_saved(ProjectRevision revision,
                                              std::chrono::milliseconds timeout) {
    std::unique_lock lock(impl_->mutex);
    impl_->changed.wait_for(lock, timeout, [&] {
        return impl_->status.last_saved_revision.value_or(0) >= revision ||
               (!impl_->pending.has_value() && !impl_->status.saving_revision.has_value() &&
                !impl_->status.last_error.empty());
    });
    return impl_->status.last_saved_revision.value_or(0) >= revision;
}

void ProjectAutosaveSession::flush() {
    std::unique_lock lock(impl_->mutex);
    impl_->flush_requested = true;
    impl_->changed.notify_all();
    impl_->changed.wait(lock, [&] {
        return !impl_->pending.has_value() && !impl_->status.saving_revision.has_value();
    });
    impl_->flush_requested = false;
    if (!impl_->status.last_error.empty()) {
        throw ProjectContainerError(ProjectContainerErrorCode::filesystem_failure,
                                    "autosave failed: " + impl_->status.last_error);
    }
}

ProjectAutosaveStatus ProjectAutosaveSession::status() const {
    std::scoped_lock lock(impl_->mutex);
    return impl_->status;
}

const std::filesystem::path& ProjectAutosaveSession::recovery_path() const noexcept {
    return impl_->path;
}

RecoveryEnumeration enumerate_recoverable_projects(
    const std::filesystem::path& recovery_directory) {
    RecoveryEnumeration result;
    std::error_code error;
    if (!std::filesystem::exists(recovery_directory, error)) {
        if (error) {
            throw ProjectContainerError(ProjectContainerErrorCode::filesystem_failure,
                                        "could not inspect recovery directory: " + error.message());
        }
        return result;
    }
    std::filesystem::directory_iterator entries(recovery_directory, error);
    if (error) {
        throw ProjectContainerError(ProjectContainerErrorCode::filesystem_failure,
                                    "could not enumerate recovery directory: " + error.message());
    }
    for (const auto& entry : entries) {
        const std::string filename = entry.path().filename().string();
        if (!filename.ends_with(recovery_suffix) || !entry.is_regular_file(error)) {
            error.clear();
            continue;
        }
        const std::string key = filename.substr(0, filename.size() - recovery_suffix.size());
        try {
            if (!valid_recovery_key(key)) {
                throw ProjectContainerError(ProjectContainerErrorCode::malformed_header,
                                            "recovery filename has an invalid key");
            }
            std::array<std::byte, 40> header{};
            std::ifstream stream(entry.path(), std::ios::binary);
            stream.read(reinterpret_cast<char*>(header.data()),
                        static_cast<std::streamsize>(header.size()));
            if (!stream) {
                throw ProjectContainerError(ProjectContainerErrorCode::malformed_header,
                                            "recovery file has a truncated header");
            }
            result.recoverable.push_back({
                .recovery_key = key,
                .path = entry.path(),
                .schema_version = probe_project_container_version(header),
                .last_write_time = entry.last_write_time(),
                .file_bytes = entry.file_size(),
            });
        } catch (const std::exception& failure) {
            result.rejected.push_back({.path = entry.path(), .message = failure.what()});
        }
    }
    std::sort(result.recoverable.begin(), result.recoverable.end(),
              [](const RecoverableProject& left, const RecoverableProject& right) {
                  if (left.last_write_time != right.last_write_time) {
                      return left.last_write_time > right.last_write_time;
                  }
                  return left.recovery_key < right.recovery_key;
              });
    return result;
}

}  // namespace ctex::io
