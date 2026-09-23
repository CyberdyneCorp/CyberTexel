#include <algorithm>
#include <ctex/doc/resolution_change.hpp>
#include <limits>
#include <map>
#include <set>
#include <tuple>
#include <utility>

namespace ctex::doc {
namespace {

using RasterKey = std::pair<std::uint32_t, std::string>;

[[noreturn]] void fail(ResolutionChangeErrorCode code, std::string message) {
    throw ResolutionChangeError(code, std::move(message));
}

std::size_t checked_add(std::size_t left, std::size_t right, const char* description) {
    if (right > std::numeric_limits<std::size_t>::max() - left) {
        throw std::overflow_error(description);
    }
    return left + right;
}

std::size_t checked_multiply(std::size_t left, std::size_t right, const char* description) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw std::overflow_error(description);
    }
    return left * right;
}

std::uint8_t storage_bit_depth(image::ChannelType type) {
    switch (type) {
        case image::ChannelType::uint8_unorm:
            return 8;
        case image::ChannelType::uint16_unorm:
            return 16;
        case image::ChannelType::float32:
            return 32;
    }
    fail(ResolutionChangeErrorCode::invalid_request, "resolution channel type is invalid");
}

bool valid_policy(ResolutionChangePolicy policy) noexcept {
    return policy == ResolutionChangePolicy::replay_eligible ||
           policy == ResolutionChangePolicy::resample_all ||
           policy == ResolutionChangePolicy::cancel;
}

bool valid_checkpoint_policy(CheckpointResamplePolicy policy) noexcept {
    return policy == CheckpointResamplePolicy::refuse ||
           policy == CheckpointResamplePolicy::nearest ||
           policy == CheckpointResamplePolicy::bilinear;
}

bool valid_replay_disposition(ResolutionReplayDisposition disposition) noexcept {
    return disposition == ResolutionReplayDisposition::resolution_independent ||
           disposition == ResolutionReplayDisposition::checkpoint_only ||
           disposition == ResolutionReplayDisposition::resample_required ||
           disposition == ResolutionReplayDisposition::unsupported_algorithm;
}

image::ImageResampleFilter image_filter(CheckpointResamplePolicy policy) {
    if (policy == CheckpointResamplePolicy::nearest) {
        return image::ImageResampleFilter::nearest;
    }
    return image::ImageResampleFilter::bilinear;
}

std::vector<std::byte> dense_pixels(const image::TiledImage& source) {
    const std::size_t row_bytes =
        checked_multiply(source.width(), source.pixel_bytes(), "resolution source row overflow");
    std::vector<std::byte> result(
        checked_multiply(row_bytes, source.height(), "resolution source image overflow"));
    for (std::uint32_t y = 0; y < source.height(); ++y) {
        for (std::uint32_t x = 0; x < source.width(); ++x) {
            const auto pixel = source.read_pixel(x, y);
            const std::size_t offset = static_cast<std::size_t>(y) * row_bytes +
                                       static_cast<std::size_t>(x) * source.pixel_bytes();
            std::ranges::copy(pixel, result.begin() + static_cast<std::ptrdiff_t>(offset));
        }
    }
    return result;
}

image::TiledImage tiled_image_from_dense(const image::TiledImage& source, std::uint32_t width,
                                         std::uint32_t height, std::span<const std::byte> pixels,
                                         std::pmr::memory_resource* memory_resource) {
    const std::size_t expected =
        checked_multiply(checked_multiply(width, height, "resolution target texel count overflow"),
                         source.pixel_bytes(), "resolution target image overflow");
    if (pixels.size() != expected) {
        fail(ResolutionChangeErrorCode::missing_replay_output,
             "resolution replay raster has an invalid byte count");
    }
    image::TiledImage result(width, height, source.format(), source.tile_size(),
                             source.clear_pixel(), memory_resource);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t offset =
                (static_cast<std::size_t>(y) * width + x) * source.pixel_bytes();
            const auto pixel = pixels.subspan(offset, source.pixel_bytes());
            if (!std::ranges::equal(pixel, source.clear_pixel())) {
                result.write_pixel(x, y, pixel);
            }
        }
    }
    return result;
}

struct RasterInputs {
    std::map<RasterKey, std::span<const std::byte>> values;
};

RasterInputs index_replay_rasters(std::span<const ResolutionReplayRaster> rasters) {
    RasterInputs result;
    for (const ResolutionReplayRaster& raster : rasters) {
        if (raster.semantic_id.empty() ||
            !result.values
                 .emplace(RasterKey{raster.udim_tile_number, raster.semantic_id}, raster.pixels)
                 .second) {
            fail(ResolutionChangeErrorCode::invalid_request,
                 "resolution replay raster identities must be non-empty and unique");
        }
    }
    return result;
}

std::span<const std::byte> require_replay_raster(const RasterInputs& inputs,
                                                 std::uint32_t udim_tile,
                                                 std::string_view semantic_id) {
    const auto found = inputs.values.find({udim_tile, std::string(semantic_id)});
    if (found == inputs.values.end()) {
        fail(ResolutionChangeErrorCode::missing_replay_output,
             "resolution replay omitted channel raster: " + std::string(semantic_id));
    }
    return found->second;
}

std::vector<std::byte> resampled_pixels(const image::TiledImage& source, std::uint32_t width,
                                        std::uint32_t height,
                                        CheckpointResamplePolicy checkpoint_policy,
                                        std::size_t maximum_output_bytes) {
    const std::vector<std::byte> source_pixels = dense_pixels(source);
    return image::resample_image({.pixels = source_pixels,
                                  .source_width = source.width(),
                                  .source_height = source.height(),
                                  .format = source.format(),
                                  .source_row_stride_bytes = 0,
                                  .output_width = width,
                                  .output_height = height,
                                  .filter = image_filter(checkpoint_policy),
                                  .maximum_output_bytes = maximum_output_bytes})
        .pixels;
}

TextureChannels stage_channels(const TextureChannels& source, std::uint32_t udim_tile,
                               const ResolutionChangeRequest& request, const RasterInputs& inputs,
                               std::pmr::memory_resource* memory_resource,
                               std::size_t& consumed_rasters) {
    TextureChannels result(request.width, request.height, source.default_bit_depth(), {},
                           memory_resource);
    for (const std::string& semantic_id : source.semantic_ids()) {
        result.register_descriptor(source.descriptor(semantic_id));
        if (!source.is_enabled(semantic_id)) {
            continue;
        }
        const image::TiledImage& source_image = source.pixels(semantic_id);
        result.enable(semantic_id, storage_bit_depth(source_image.format().channel_type));
        std::vector<std::byte> resampled;
        std::span<const std::byte> pixels;
        if (request.policy == ResolutionChangePolicy::resample_all) {
            resampled = resampled_pixels(source_image, request.width, request.height,
                                         request.checkpoint_policy, request.maximum_working_bytes);
            pixels = resampled;
        } else {
            pixels = require_replay_raster(inputs, udim_tile, semantic_id);
            ++consumed_rasters;
        }
        result.replace_pixels(
            semantic_id, tiled_image_from_dense(source_image, request.width, request.height, pixels,
                                                memory_resource));
    }
    return result;
}

std::size_t dense_target_bytes(const TextureChannels& channels, std::uint32_t width,
                               std::uint32_t height) {
    std::size_t result = 0;
    for (const std::string& semantic_id : channels.semantic_ids()) {
        if (channels.is_enabled(semantic_id)) {
            result = checked_add(
                result,
                checked_multiply(checked_multiply(width, height, "target texel count overflow"),
                                 channels.pixels(semantic_id).pixel_bytes(),
                                 "target channel size overflow"),
                "target raster set size overflow");
        }
    }
    return result;
}

std::size_t dense_source_bytes(const TextureChannels& channels) {
    std::size_t result = 0;
    for (const std::string& semantic_id : channels.semantic_ids()) {
        if (channels.is_enabled(semantic_id)) {
            const image::TiledImage& pixels = channels.pixels(semantic_id);
            result =
                checked_add(result,
                            checked_multiply(checked_multiply(pixels.width(), pixels.height(),
                                                              "source texel count overflow"),
                                             pixels.pixel_bytes(), "source channel size overflow"),
                            "source raster set size overflow");
        }
    }
    return result;
}

std::size_t enabled_raster_count(const TextureChannels& channels) noexcept {
    return channels.enabled_channel_count();
}

std::size_t retained_channel_bytes(const TextureChannels& channels) {
    std::size_t result = 0;
    for (const std::string& semantic_id : channels.semantic_ids()) {
        if (!channels.is_enabled(semantic_id)) {
            continue;
        }
        const image::TiledImage& pixels = channels.pixels(semantic_id);
        result = checked_add(result,
                             checked_multiply(pixels.allocated_tiles().size(), pixels.tile_bytes(),
                                              "resolution history channel size overflow"),
                             "resolution history channel size overflow");
    }
    return result;
}

std::size_t retained_snapshot_bytes(const TextureSet& texture_set) {
    std::size_t result = retained_channel_bytes(texture_set.channels());
    for (const std::uint32_t number : texture_set.occupied_udim_tiles()) {
        result = checked_add(result, retained_channel_bytes(texture_set.udim_channels(number)),
                             "resolution history byte count overflow");
    }
    return checked_add(result, texture_set.tile_history_budget_report().retained_bytes,
                       "resolution history byte count overflow");
}

std::size_t procedural_entry_count(const LayerStack& stack,
                                   const EditableAuthoringStore& editable) {
    const std::size_t graph_entries = static_cast<std::size_t>(
        std::ranges::count_if(stack.entries(), [](const LayerEntry& entry) {
            return entry.enabled && (entry.kind == LayerEntryKind::fill_layer ||
                                     entry.kind == LayerEntryKind::filter ||
                                     entry.kind == LayerEntryKind::editable_decal ||
                                     entry.kind == LayerEntryKind::editable_text ||
                                     entry.kind == LayerEntryKind::surface_path);
        }));
    return checked_add(graph_entries, editable.entries().size(), "procedural entry count overflow");
}

}  // namespace

struct TextureSetResolutionStaged {
    TextureSetResolutionStaged(const TextureSet& texture_set,
                               const ResolutionChangeRequest& request, const RasterInputs& inputs)
        : channels(stage_channels(texture_set.channels_, 0, request, inputs,
                                  texture_set.memory_resource_, consumed_rasters)),
          udim_tiles(texture_set.memory_resource_),
          tile_history(texture_set.tile_history_.budget_report().budget_bytes) {
        for (const auto& [number, source] : texture_set.udim_tiles_) {
            udim_tiles.emplace(number,
                               stage_channels(source, number, request, inputs,
                                              texture_set.memory_resource_, consumed_rasters));
        }
    }

    std::size_t consumed_rasters{};
    TextureChannels channels;
    std::pmr::map<std::uint32_t, TextureChannels> udim_tiles;
    TileHistory tile_history;
};

struct TextureSetResolutionSnapshot {
    explicit TextureSetResolutionSnapshot(const TextureSet& texture_set)
        : width(texture_set.width_),
          height(texture_set.height_),
          channels(texture_set.channels_),
          tile_history(texture_set.tile_history_) {
        udim_tiles.reserve(texture_set.udim_tiles_.size());
        for (const auto& [number, value] : texture_set.udim_tiles_) {
            udim_tiles.emplace_back(number, value);
        }
        retained_bytes = channels.resident_pixel_bytes();
        for (const auto& [unused, value] : udim_tiles) {
            static_cast<void>(unused);
            retained_bytes = checked_add(retained_bytes, value.resident_pixel_bytes(),
                                         "resolution history byte count overflow");
        }
        retained_bytes =
            checked_add(retained_bytes, texture_set.tile_history_.budget_report().retained_bytes,
                        "resolution history byte count overflow");
    }

    std::uint32_t width{};
    std::uint32_t height{};
    TextureChannels channels;
    std::vector<std::pair<std::uint32_t, TextureChannels>> udim_tiles;
    TileHistory tile_history;
    std::size_t retained_bytes{};
};

struct TextureSetResolutionHistory {
    std::vector<TextureSetResolutionSnapshot> undo;
    std::vector<TextureSetResolutionSnapshot> redo;
};

namespace {

std::size_t snapshot_bytes(const std::vector<TextureSetResolutionSnapshot>& snapshots) {
    std::size_t result = 0;
    for (const TextureSetResolutionSnapshot& snapshot : snapshots) {
        result =
            checked_add(result, snapshot.retained_bytes, "resolution history byte count overflow");
    }
    return result;
}

}  // namespace

ResolutionChangeError::ResolutionChangeError(ResolutionChangeErrorCode code, std::string message)
    : std::runtime_error(std::move(message)), code_(code) {}

ResolutionChangeReport change_texture_set_resolution(TextureSet& texture_set,
                                                     const ResolutionChangeRequest& request) {
    if (!valid_policy(request.policy) || !valid_checkpoint_policy(request.checkpoint_policy) ||
        request.width == 0 || request.height == 0 || request.maximum_working_bytes == 0 ||
        request.maximum_history_bytes == 0) {
        fail(ResolutionChangeErrorCode::invalid_request,
             "resolution change request contains an invalid policy, dimension or budget");
    }
    const TextureSetDescriptor source_descriptor = texture_set.descriptor();
    ResolutionChangeReport report{.committed = false,
                                  .policy = request.policy,
                                  .source_width = source_descriptor.width,
                                  .source_height = source_descriptor.height,
                                  .target_width = request.width,
                                  .target_height = request.height};
    if (request.policy == ResolutionChangePolicy::cancel) {
        return report;
    }
    if (request.width == source_descriptor.width && request.height == source_descriptor.height) {
        fail(ResolutionChangeErrorCode::invalid_request,
             "resolution change target must differ from the current dimensions");
    }
    if (request.policy == ResolutionChangePolicy::resample_all && !request.replay_rasters.empty()) {
        fail(ResolutionChangeErrorCode::invalid_request,
             "resample-all does not accept host replay rasters");
    }
    if (request.policy == ResolutionChangePolicy::resample_all &&
        request.checkpoint_policy == CheckpointResamplePolicy::refuse) {
        fail(ResolutionChangeErrorCode::invalid_request,
             "resample-all requires an explicit nearest or bilinear filter");
    }

    std::set<std::string_view> source_ids;
    for (const ResolutionReplaySource& source : request.replay_sources) {
        if (source.identifier.empty() || !source_ids.insert(source.identifier).second) {
            fail(ResolutionChangeErrorCode::invalid_request,
                 "resolution replay source identities must be non-empty and unique");
        }
        if (!valid_replay_disposition(source.disposition)) {
            fail(ResolutionChangeErrorCode::invalid_request,
                 "resolution replay source has an invalid disposition");
        }
        if (request.policy == ResolutionChangePolicy::resample_all) {
            ++report.resampled_source_count;
            continue;
        }
        if (source.disposition == ResolutionReplayDisposition::resolution_independent) {
            ++report.replayed_source_count;
        } else {
            if (!source.checkpoint_available ||
                request.checkpoint_policy == CheckpointResamplePolicy::refuse) {
                fail(
                    ResolutionChangeErrorCode::replay_unavailable,
                    "resolution replay source requires an explicit checkpoint resampling policy: " +
                        source.identifier);
            }
            ++report.resampled_source_count;
        }
    }
    report.procedural_entry_count =
        procedural_entry_count(texture_set.layer_stack_, texture_set.editable_authoring_);

    std::size_t target_bytes =
        dense_target_bytes(texture_set.channels_, request.width, request.height);
    std::size_t source_bytes = dense_source_bytes(texture_set.channels_);
    for (const auto& [unused, channels] : texture_set.udim_tiles_) {
        static_cast<void>(unused);
        target_bytes =
            checked_add(target_bytes, dense_target_bytes(channels, request.width, request.height),
                        "resolution target byte count overflow");
        source_bytes = checked_add(source_bytes, dense_source_bytes(channels),
                                   "resolution source byte count overflow");
    }
    const std::size_t working_bytes =
        request.policy == ResolutionChangePolicy::resample_all
            ? checked_add(target_bytes, source_bytes, "resolution working byte count overflow")
            : target_bytes;
    if (working_bytes > request.maximum_working_bytes) {
        fail(ResolutionChangeErrorCode::over_budget,
             "resolution change exceeds the configured working-byte budget");
    }

    const std::size_t snapshot_estimate = retained_snapshot_bytes(texture_set);
    const std::size_t retained_after = checked_add(
        texture_set.resolution_history_ ? snapshot_bytes(texture_set.resolution_history_->undo) : 0,
        snapshot_estimate, "resolution history byte count overflow");
    if (retained_after > request.maximum_history_bytes) {
        fail(ResolutionChangeErrorCode::over_budget,
             "resolution change exceeds the configured undo-history byte budget");
    }

    const RasterInputs inputs = index_replay_rasters(request.replay_rasters);
    TextureSetResolutionStaged staged(texture_set, request, inputs);
    if (staged.consumed_rasters != inputs.values.size()) {
        fail(ResolutionChangeErrorCode::invalid_request,
             "resolution replay supplied a raster for an unknown or disabled channel");
    }
    TextureSetResolutionSnapshot before(texture_set);
    if (!texture_set.resolution_history_) {
        texture_set.resolution_history_ = std::make_shared<TextureSetResolutionHistory>();
    } else if (texture_set.resolution_history_.use_count() != 1) {
        texture_set.resolution_history_ =
            std::make_shared<TextureSetResolutionHistory>(*texture_set.resolution_history_);
    }
    if (before.retained_bytes != snapshot_estimate) {
        throw std::logic_error("resolution history byte estimate changed during staging");
    }
    texture_set.resolution_history_->undo.push_back(std::move(before));
    texture_set.resolution_history_->redo.clear();
    texture_set.channels_.swap(staged.channels);
    texture_set.udim_tiles_.swap(staged.udim_tiles);
    texture_set.tile_history_ = std::move(staged.tile_history);
    texture_set.width_ = request.width;
    texture_set.height_ = request.height;
    texture_set.resolution_history_bytes_ = retained_after;

    report.committed = true;
    report.raster_count = request.policy == ResolutionChangePolicy::replay_eligible
                              ? staged.consumed_rasters
                              : enabled_raster_count(texture_set.channels_);
    if (request.policy == ResolutionChangePolicy::resample_all) {
        for (const auto& [unused, channels] : texture_set.udim_tiles_) {
            static_cast<void>(unused);
            report.raster_count = checked_add(report.raster_count, enabled_raster_count(channels),
                                              "resolution raster count overflow");
        }
    }
    report.staged_pixel_bytes = target_bytes;
    report.retained_history_bytes = retained_after;
    return report;
}

ResolutionRestoreReport undo_texture_set_resolution(TextureSet& texture_set) {
    return restore_texture_set_resolution(texture_set, true);
}

ResolutionRestoreReport redo_texture_set_resolution(TextureSet& texture_set) {
    return restore_texture_set_resolution(texture_set, false);
}

ResolutionRestoreReport restore_texture_set_resolution(TextureSet& texture_set, bool undoing) {
    if (!texture_set.resolution_history_) {
        fail(undoing ? ResolutionChangeErrorCode::no_undo : ResolutionChangeErrorCode::no_redo,
             undoing ? "texture set has no resolution undo step"
                     : "texture set has no resolution redo step");
    }
    if (texture_set.resolution_history_.use_count() != 1) {
        texture_set.resolution_history_ =
            std::make_shared<TextureSetResolutionHistory>(*texture_set.resolution_history_);
    }
    auto& source =
        undoing ? texture_set.resolution_history_->undo : texture_set.resolution_history_->redo;
    auto& destination =
        undoing ? texture_set.resolution_history_->redo : texture_set.resolution_history_->undo;
    if (source.empty()) {
        fail(undoing ? ResolutionChangeErrorCode::no_undo : ResolutionChangeErrorCode::no_redo,
             undoing ? "texture set has no resolution undo step"
                     : "texture set has no resolution redo step");
    }
    TextureSetResolutionSnapshot target = source.back();
    TextureSetResolutionSnapshot current(texture_set);
    std::pmr::map<std::uint32_t, TextureChannels> restored(texture_set.memory_resource_);
    for (const auto& [number, channels] : target.udim_tiles) {
        restored.emplace(number, channels);
    }
    destination.push_back(std::move(current));
    texture_set.channels_.swap(target.channels);
    texture_set.udim_tiles_.swap(restored);
    texture_set.tile_history_ = std::move(target.tile_history);
    texture_set.width_ = target.width;
    texture_set.height_ = target.height;
    source.pop_back();
    texture_set.resolution_history_bytes_ =
        checked_add(snapshot_bytes(texture_set.resolution_history_->undo),
                    snapshot_bytes(texture_set.resolution_history_->redo),
                    "resolution history byte count overflow");
    return {.width = texture_set.width_,
            .height = texture_set.height_,
            .retained_history_bytes = texture_set.resolution_history_bytes_};
}

}  // namespace ctex::doc
