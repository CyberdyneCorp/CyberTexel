#include <algorithm>
#include <cmath>
#include <cstring>
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

bool layouts_match_format(std::span<const TileReadbackDestination> destinations,
                          image::PixelFormat format) {
    return std::all_of(destinations.begin(), destinations.end(), [&](const auto& destination) {
        return destination.layout.channel_order == channel_order(format.channel_count) &&
               destination.layout.component_type == format.channel_type;
    });
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

TileMemoryLayout make_tile_memory_layout(image::TileExtent extent,
                                         image::PixelFormat output_format) {
    const std::size_t pixel_stride = output_format.bytes_per_pixel();
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
        .channel_order = channel_order(output_format.channel_count),
        .component_type = output_format.channel_type,
        .component_byte_order = ComponentByteOrder::native,
        .tile_contiguity = TileContiguity::separate_buffers,
    };
}

TileMemoryLayout make_tile_memory_layout(const image::TiledImage& image,
                                         image::TileCoordinate coordinate,
                                         image::PixelFormat output_format) {
    return make_tile_memory_layout(image.tile_extent(coordinate), output_format);
}

double decode_component(std::span<const std::byte> bytes, image::ChannelType type) {
    switch (type) {
        case image::ChannelType::uint8_unorm:
            return static_cast<double>(std::to_integer<std::uint8_t>(bytes.front())) / 255.0;
        case image::ChannelType::uint16_unorm: {
            std::uint16_t value{};
            std::memcpy(&value, bytes.data(), sizeof(value));
            return static_cast<double>(value) / 65535.0;
        }
        case image::ChannelType::float32: {
            float value{};
            std::memcpy(&value, bytes.data(), sizeof(value));
            return value;
        }
    }
    throw std::invalid_argument("unsupported readback source component type");
}

void encode_component(double value, image::ChannelType type, std::span<std::byte> output) {
    const double numeric_value = std::isnan(value) ? 0.0 : value;
    switch (type) {
        case image::ChannelType::uint8_unorm: {
            const auto encoded =
                static_cast<std::uint8_t>(std::lround(std::clamp(numeric_value, 0.0, 1.0) * 255.0));
            std::memcpy(output.data(), &encoded, sizeof(encoded));
            return;
        }
        case image::ChannelType::uint16_unorm: {
            const auto encoded = static_cast<std::uint16_t>(
                std::lround(std::clamp(numeric_value, 0.0, 1.0) * 65535.0));
            std::memcpy(output.data(), &encoded, sizeof(encoded));
            return;
        }
        case image::ChannelType::float32: {
            const float encoded = static_cast<float>(numeric_value);
            std::memcpy(output.data(), &encoded, sizeof(encoded));
            return;
        }
    }
    throw std::invalid_argument("unsupported readback output component type");
}

void append_pixel(std::span<const std::byte> pixel, const ReadbackFormatSelection& format,
                  std::vector<std::byte>& result, std::size_t& offset) {
    if (!format.converted()) {
        std::copy(pixel.begin(), pixel.end(), result.begin() + offset);
        offset += pixel.size();
        return;
    }
    for (std::uint8_t channel = 0; channel < format.source_format.channel_count; ++channel) {
        const std::size_t source_offset = channel * format.source_format.bytes_per_channel();
        const std::size_t output_bytes = format.output_format.bytes_per_channel();
        const double value =
            decode_component(pixel.subspan(source_offset, format.source_format.bytes_per_channel()),
                             format.source_format.channel_type);
        encode_component(value, format.output_format.channel_type,
                         std::span(result).subspan(offset, output_bytes));
        offset += output_bytes;
    }
}

std::vector<std::byte> stage_cpu_tile(const image::TiledImage& image,
                                      image::TileCoordinate coordinate,
                                      const ReadbackFormatSelection& format) {
    const image::TileExtent extent = image.tile_extent(coordinate);
    std::vector<std::byte> result(
        checked_tile_bytes(extent, format.output_format.bytes_per_pixel()));
    const std::uint32_t origin_x = coordinate.x * image.tile_size();
    const std::uint32_t origin_y = coordinate.y * image.tile_size();
    std::size_t offset = 0;
    for (std::uint32_t y = 0; y < extent.height; ++y) {
        for (std::uint32_t x = 0; x < extent.width; ++x) {
            const auto pixel = image.read_pixel(origin_x + x, origin_y + y);
            append_pixel(pixel, format, result, offset);
        }
    }
    return result;
}

std::vector<std::byte> stage_pinned_tile(std::span<const std::byte> storage,
                                         image::TileExtent extent, std::uint32_t tile_size,
                                         const ReadbackFormatSelection& format) {
    const std::size_t source_pixel_bytes = format.source_format.bytes_per_pixel();
    const std::size_t physical_row_bytes = checked_tile_bytes({tile_size, 1}, source_pixel_bytes);
    const std::size_t required_storage =
        checked_tile_bytes({tile_size, tile_size}, source_pixel_bytes);
    if (storage.size() != required_storage) {
        throw std::logic_error("pinned tile storage has the wrong size");
    }
    std::vector<std::byte> result(
        checked_tile_bytes(extent, format.output_format.bytes_per_pixel()));
    std::size_t output_offset = 0;
    for (std::uint32_t y = 0; y < extent.height; ++y) {
        for (std::uint32_t x = 0; x < extent.width; ++x) {
            const std::size_t source_offset =
                (static_cast<std::size_t>(y) * physical_row_bytes) + (x * source_pixel_bytes);
            append_pixel(storage.subspan(source_offset, source_pixel_bytes), format, result,
                         output_offset);
        }
    }
    return result;
}

}  // namespace

TileMemoryLayout tile_memory_layout(const doc::TextureChannels& channels,
                                    std::string_view semantic_id,
                                    image::TileCoordinate coordinate) {
    const image::TiledImage& image = channels.pixels(semantic_id);
    const image::PixelFormat format = image.format();
    return make_tile_memory_layout(image, coordinate, format);
}

TileMemoryLayout tile_memory_layout(const doc::TextureChannels& channels,
                                    std::string_view semantic_id, image::TileCoordinate coordinate,
                                    const ReadbackFormatSelection& format) {
    const image::TiledImage& image = channels.pixels(semantic_id);
    if (!format.is_valid() || format.source_format != image.format()) {
        throw std::invalid_argument("readback format selection does not match the channel");
    }
    return make_tile_memory_layout(image, coordinate, format.output_format);
}

TileMemoryLayout tile_memory_layout(const PreviewResource& preview,
                                    image::TileCoordinate coordinate) {
    return make_tile_memory_layout(preview.pixels(), coordinate, preview.format());
}

TileMemoryLayout tile_memory_layout(const PreviewResource& preview,
                                    image::TileCoordinate coordinate,
                                    const ReadbackFormatSelection& format) {
    if (!format.is_valid() || format.source_format != preview.format()) {
        throw std::invalid_argument("readback format selection does not match the preview");
    }
    return make_tile_memory_layout(preview.pixels(), coordinate, format.output_format);
}

TileReadback::TileReadback(std::vector<TileReadbackDestination> destinations,
                           TileReadbackStatus status, std::string detail,
                           std::optional<ReadbackFormatSelection> format_selection)
    : destinations_(std::move(destinations)),
      status_(status),
      detail_(std::move(detail)),
      format_selection_(std::move(format_selection)) {}

TileReadback TileReadback::failed(std::string detail,
                                  std::optional<ReadbackFormatSelection> format_selection) {
    return {{}, TileReadbackStatus::failed, std::move(detail), std::move(format_selection)};
}

TileReadback TileReadback::begin_cpu(const doc::TextureChannels& channels,
                                     std::string_view semantic_id,
                                     doc::ChannelRevisionCursor cursor,
                                     std::span<const TileReadbackDestination> destinations) {
    try {
        const image::PixelFormat source_format = channels.pixels(semantic_id).format();
        const ReadbackFormatSelection native_format{
            source_format,
            source_format,
            ReadbackConversion::none,
        };
        return begin_cpu(channels, semantic_id, cursor, native_format, destinations);
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const std::exception& exception) {
        return failed(exception.what());
    }
}

TileReadback TileReadback::begin_cpu(const doc::TextureChannels& channels,
                                     std::string_view semantic_id,
                                     doc::ChannelRevisionCursor cursor,
                                     const ReadbackFormatSelection& format,
                                     std::span<const TileReadbackDestination> destinations) {
    std::string error;
    if (!validate_destinations(destinations, TileResidency::cpu, error)) {
        return failed(std::move(error));
    }

    try {
        const image::TiledImage& image = channels.pixels(semantic_id);
        if (!format.is_valid() || format.source_format != image.format()) {
            return failed("CPU tile readback format selection does not match the channel");
        }
        if (image.revision_cursor() != cursor) {
            return failed("CPU tile readback cursor is stale");
        }
        std::vector<std::vector<std::byte>> staged;
        staged.reserve(destinations.size());
        for (const TileReadbackDestination& destination : destinations) {
            const auto coordinate = destination.version.coordinate;
            if (destination.layout !=
                tile_memory_layout(channels, semantic_id, coordinate, format)) {
                return failed("CPU tile readback memory layout does not match the tile");
            }
            if (destination.version.revision != image.tile_revision(coordinate) ||
                destination.version.generation != image.tile_generation(coordinate)) {
                return failed("CPU tile readback version is stale");
            }
            staged.push_back(stage_cpu_tile(image, coordinate, format));
            // Destination validation and the selected layout guarantee this size match.
        }
        if (image.revision_cursor() != cursor) {
            return failed("CPU tile readback changed while staging");
        }

        std::vector<TileReadbackDestination> retained(destinations.begin(), destinations.end());
        for (std::size_t index = 0; index < retained.size(); ++index) {
            std::copy(staged[index].begin(), staged[index].end(), retained[index].output.begin());
        }
        return {std::move(retained), TileReadbackStatus::complete, {}, format};
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const std::exception& exception) {
        return failed(exception.what());
    }
}

TileReadback TileReadback::begin_cpu(const SnapshotToken& snapshot,
                                     std::span<const TileReadbackDestination> destinations) {
    const image::PixelFormat source_format = snapshot.source_format();
    return begin_cpu(snapshot,
                     ReadbackFormatSelection{
                         source_format,
                         source_format,
                         ReadbackConversion::none,
                     },
                     destinations);
}

TileReadback TileReadback::begin_cpu(const SnapshotToken& snapshot,
                                     const ReadbackFormatSelection& format,
                                     std::span<const TileReadbackDestination> destinations) {
    std::string error;
    if (!validate_destinations(destinations, TileResidency::cpu, error)) {
        return failed(std::move(error));
    }
    if (!snapshot.active()) {
        return failed("CPU tile readback snapshot is released");
    }
    if (!format.is_valid() || format.source_format != snapshot.source_format()) {
        return failed("CPU tile readback format selection does not match the snapshot");
    }

    try {
        std::vector<std::vector<std::byte>> staged;
        staged.reserve(destinations.size());
        for (const TileReadbackDestination& destination : destinations) {
            const auto pinned = snapshot.access(destination.version);
            if (destination.layout !=
                make_tile_memory_layout(pinned.extent, format.output_format)) {
                return failed("CPU tile readback memory layout does not match the snapshot tile",
                              format);
            }
            staged.push_back(
                stage_pinned_tile(pinned.bytes, pinned.extent, pinned.tile_size, format));
        }

        std::vector<TileReadbackDestination> retained(destinations.begin(), destinations.end());
        for (std::size_t index = 0; index < retained.size(); ++index) {
            std::copy(staged[index].begin(), staged[index].end(), retained[index].output.begin());
        }
        return {std::move(retained), TileReadbackStatus::complete, {}, format};
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const std::exception& exception) {
        return failed(exception.what(), format);
    }
}

TileReadback TileReadback::begin_host(std::span<const TileReadbackDestination> destinations) {
    if (destinations.empty()) {
        return {{}, TileReadbackStatus::complete, {}};
    }
    const TileMemoryLayout& layout = destinations.front().layout;
    const image::PixelFormat native_format{
        layout.component_type,
        channel_count(layout.channel_order),
    };
    return begin_host(
        ReadbackFormatSelection{native_format, native_format, ReadbackConversion::none},
        destinations);
}

TileReadback TileReadback::begin_host(const ReadbackFormatSelection& format,
                                      std::span<const TileReadbackDestination> destinations) {
    std::string error;
    if (!validate_destinations(destinations, TileResidency::host_device, error)) {
        return failed(std::move(error));
    }
    if (!format.is_valid()) {
        return failed("host tile readback format selection is invalid");
    }
    if (!layouts_match_format(destinations, format.output_format)) {
        return failed("host tile readback layout does not match its selected format", format);
    }
    if (destinations.empty()) {
        return {{}, TileReadbackStatus::complete, {}, format};
    }
    return {{destinations.begin(), destinations.end()}, TileReadbackStatus::pending, {}, format};
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
