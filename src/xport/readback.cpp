#include <algorithm>
#include <ctex/xport/readback.hpp>
#include <limits>
#include <new>
#include <set>
#include <stdexcept>
#include <utility>

namespace ctex::xport {
namespace {

using CoordinateKey = std::pair<std::uint32_t, std::uint32_t>;

std::uint8_t channel_count(ChannelOrder order) {
    switch (order) {
        case ChannelOrder::r:
            return 1;
        case ChannelOrder::rg:
            return 2;
        case ChannelOrder::rgb:
            return 3;
        case ChannelOrder::rgba:
            return 4;
    }
    return 0;
}

ChannelOrder channel_order(std::uint8_t count) {
    switch (count) {
        case 1:
            return ChannelOrder::r;
        case 2:
            return ChannelOrder::rg;
        case 3:
            return ChannelOrder::rgb;
        case 4:
            return ChannelOrder::rgba;
        default:
            throw std::invalid_argument("tile layout requires one to four channels");
    }
}

bool valid_layout(const TileMemoryLayout& layout) {
    const image::PixelFormat format{layout.component_type, channel_count(layout.channel_order)};
    if (layout.width == 0 || layout.height == 0 || !format.is_valid() ||
        layout.component_byte_order != ComponentByteOrder::native ||
        layout.tile_contiguity != TileContiguity::separate_buffers ||
        layout.pixel_stride_bytes != format.bytes_per_pixel()) {
        return false;
    }
    if (layout.width > std::numeric_limits<std::size_t>::max() / layout.pixel_stride_bytes ||
        layout.row_pitch_bytes != layout.width * layout.pixel_stride_bytes) {
        return false;
    }
    return layout.height <= std::numeric_limits<std::size_t>::max() / layout.row_pitch_bytes;
}

bool validate_destinations(std::span<const TileReadbackDestination> destinations,
                           TileResidency expected_residency, std::string& error) {
    std::set<CoordinateKey> coordinates;
    for (const TileReadbackDestination& destination : destinations) {
        if (destination.version.residency != expected_residency) {
            error = "tile readback residency does not match its route";
            return false;
        }
        if (destination.output.empty()) {
            error = "tile readback output buffer must not be empty";
            return false;
        }
        if (!valid_layout(destination.layout)) {
            error = "tile readback memory layout is invalid";
            return false;
        }
        if (destination.output.size() != destination.layout.byte_size()) {
            error = "tile readback output buffer does not match its memory layout";
            return false;
        }
        const auto coordinate = destination.version.coordinate;
        if (!coordinates.emplace(coordinate.x, coordinate.y).second) {
            error = "tile readback request contains a duplicate coordinate";
            return false;
        }
    }
    return true;
}

std::size_t checked_tile_bytes(image::TileExtent extent, std::size_t pixel_bytes) {
    if (extent.height != 0 &&
        extent.width > std::numeric_limits<std::size_t>::max() / extent.height) {
        throw std::overflow_error("tile readback area overflows");
    }
    const std::size_t pixels = static_cast<std::size_t>(extent.width) * extent.height;
    if (pixel_bytes != 0 && pixels > std::numeric_limits<std::size_t>::max() / pixel_bytes) {
        throw std::overflow_error("tile readback byte size overflows");
    }
    return pixels * pixel_bytes;
}

std::vector<std::byte> stage_cpu_tile(const image::TiledImage& image,
                                      image::TileCoordinate coordinate) {
    const image::TileExtent extent = image.tile_extent(coordinate);
    std::vector<std::byte> result(checked_tile_bytes(extent, image.pixel_bytes()));
    const std::uint32_t origin_x = coordinate.x * image.tile_size();
    const std::uint32_t origin_y = coordinate.y * image.tile_size();
    std::size_t offset = 0;
    for (std::uint32_t y = 0; y < extent.height; ++y) {
        for (std::uint32_t x = 0; x < extent.width; ++x) {
            const auto pixel = image.read_pixel(origin_x + x, origin_y + y);
            std::copy(pixel.begin(), pixel.end(), result.begin() + offset);
            offset += pixel.size();
        }
    }
    return result;
}

}  // namespace

TileMemoryLayout tile_memory_layout(const doc::TextureChannels& channels,
                                    std::string_view semantic_id,
                                    image::TileCoordinate coordinate) {
    const image::TiledImage& image = channels.pixels(semantic_id);
    const image::TileExtent extent = image.tile_extent(coordinate);
    const image::PixelFormat format = image.format();
    const std::size_t pixel_stride = format.bytes_per_pixel();
    if (extent.width > std::numeric_limits<std::size_t>::max() / pixel_stride) {
        throw std::overflow_error("tile layout row pitch overflows");
    }
    const std::size_t row_pitch = static_cast<std::size_t>(extent.width) * pixel_stride;
    if (extent.height > std::numeric_limits<std::size_t>::max() / row_pitch) {
        throw std::overflow_error("tile layout byte size overflows");
    }
    return {
        .width = extent.width,
        .height = extent.height,
        .row_pitch_bytes = row_pitch,
        .pixel_stride_bytes = pixel_stride,
        .channel_order = channel_order(format.channel_count),
        .component_type = format.channel_type,
        .component_byte_order = ComponentByteOrder::native,
        .tile_contiguity = TileContiguity::separate_buffers,
    };
}

TileReadback::TileReadback(std::vector<TileReadbackDestination> destinations,
                           TileReadbackStatus status, std::string detail)
    : destinations_(std::move(destinations)), status_(status), detail_(std::move(detail)) {}

TileReadback TileReadback::failed(std::string detail) {
    return {{}, TileReadbackStatus::failed, std::move(detail)};
}

TileReadback TileReadback::begin_cpu(const doc::TextureChannels& channels,
                                     std::string_view semantic_id,
                                     doc::ChannelRevisionCursor cursor,
                                     std::span<const TileReadbackDestination> destinations) {
    std::string error;
    if (!validate_destinations(destinations, TileResidency::cpu, error)) {
        return failed(std::move(error));
    }

    try {
        const image::TiledImage& image = channels.pixels(semantic_id);
        if (image.revision_cursor() != cursor) {
            return failed("CPU tile readback cursor is stale");
        }
        std::vector<std::vector<std::byte>> staged;
        staged.reserve(destinations.size());
        for (const TileReadbackDestination& destination : destinations) {
            const auto coordinate = destination.version.coordinate;
            if (destination.layout != tile_memory_layout(channels, semantic_id, coordinate)) {
                return failed("CPU tile readback memory layout does not match the tile");
            }
            if (destination.version.revision != image.tile_revision(coordinate) ||
                destination.version.generation != image.tile_generation(coordinate)) {
                return failed("CPU tile readback version is stale");
            }
            staged.push_back(stage_cpu_tile(image, coordinate));
            // Destination validation and the exact native layout guarantee this size match.
        }
        if (image.revision_cursor() != cursor) {
            return failed("CPU tile readback changed while staging");
        }

        std::vector<TileReadbackDestination> retained(destinations.begin(), destinations.end());
        for (std::size_t index = 0; index < retained.size(); ++index) {
            std::copy(staged[index].begin(), staged[index].end(), retained[index].output.begin());
        }
        return {std::move(retained), TileReadbackStatus::complete, {}};
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const std::exception& exception) {
        return failed(exception.what());
    }
}

TileReadback TileReadback::begin_host(std::span<const TileReadbackDestination> destinations) {
    std::string error;
    if (!validate_destinations(destinations, TileResidency::host_device, error)) {
        return failed(std::move(error));
    }
    if (destinations.empty()) {
        return {{}, TileReadbackStatus::complete, {}};
    }
    return {{destinations.begin(), destinations.end()}, TileReadbackStatus::pending, {}};
}

bool TileReadback::cancel() noexcept {
    if (status_ != TileReadbackStatus::pending) {
        return false;
    }
    status_ = TileReadbackStatus::cancelled;
    return true;
}

bool TileReadback::complete_host(std::span<const HostTileCompletion> completed_tiles) {
    if (status_ != TileReadbackStatus::pending) {
        return false;
    }
    if (completed_tiles.size() != destinations_.size()) {
        status_ = TileReadbackStatus::failed;
        detail_ = "host completed the wrong number of tile readbacks";
        return false;
    }
    for (std::size_t index = 0; index < destinations_.size(); ++index) {
        if (completed_tiles[index].version != destinations_[index].version ||
            completed_tiles[index].layout != destinations_[index].layout ||
            completed_tiles[index].bytes.size() != destinations_[index].output.size()) {
            status_ = TileReadbackStatus::failed;
            detail_ = "host tile readback completion does not match its request";
            return false;
        }
    }
    for (std::size_t index = 0; index < destinations_.size(); ++index) {
        std::copy(completed_tiles[index].bytes.begin(), completed_tiles[index].bytes.end(),
                  destinations_[index].output.begin());
    }
    status_ = TileReadbackStatus::complete;
    detail_.clear();
    return true;
}

bool TileReadback::fail_host(std::string detail) {
    if (status_ != TileReadbackStatus::pending) {
        return false;
    }
    status_ = TileReadbackStatus::failed;
    detail_ = detail.empty() ? "host tile readback failed" : std::move(detail);
    return true;
}

}  // namespace ctex::xport
