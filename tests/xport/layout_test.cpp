#include <algorithm>
#include <array>
#include <cstddef>
#include <ctex/xport/readback.hpp>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

using ctex::doc::TextureChannels;
using ctex::image::ChannelType;
using ctex::image::TileCoordinate;
using ctex::xport::ChannelOrder;
using ctex::xport::ComponentByteOrder;
using ctex::xport::TileContiguity;
using ctex::xport::TileMemoryLayout;
using ctex::xport::TileReadback;
using ctex::xport::TileReadbackDestination;
using ctex::xport::TileReadbackStatus;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

class DirectTextureUploader {
public:
    bool upload(TileMemoryLayout layout, std::span<const std::byte> source) {
        if (source.size() != layout.byte_size()) {
            return false;
        }
        source_address_ = source.data();
        layout_ = layout;
        device_bytes_.assign(source.begin(), source.end());
        return true;
    }

    [[nodiscard]] const std::byte* source_address() const noexcept { return source_address_; }
    [[nodiscard]] TileMemoryLayout layout() const noexcept { return layout_; }
    [[nodiscard]] std::span<const std::byte> device_bytes() const noexcept { return device_bytes_; }

private:
    const std::byte* source_address_{};
    TileMemoryLayout layout_{};
    std::vector<std::byte> device_bytes_;
};

bool visible_edge_tile_uploads_directly() {
    TextureChannels channels(70, 66, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.base_color");
    const auto before = channels.channel_revision_cursor("pbr.base_color");
    const std::array first_pixel{std::byte{11}, std::byte{22}, std::byte{33}};
    const std::array last_pixel{std::byte{44}, std::byte{55}, std::byte{66}};
    channels.pixels("pbr.base_color").write_pixel(64, 64, first_pixel);
    channels.pixels("pbr.base_color").write_pixel(69, 65, last_pixel);
    const auto delta =
        ctex::xport::query_channel_delta_metadata(channels, "pbr.base_color", before);
    const TileCoordinate edge{1, 1};
    const TileMemoryLayout layout =
        ctex::xport::tile_memory_layout(channels, "pbr.base_color", edge);
    std::vector<std::byte> output(layout.byte_size(), std::byte{0x7f});
    const std::array destinations{
        TileReadbackDestination{delta.changed_tiles.front(), layout, output},
    };

    TileReadback readback =
        TileReadback::begin_cpu(channels, "pbr.base_color", delta.current_cursor, destinations);
    DirectTextureUploader uploader;
    const bool uploaded = readback.output_readable() && uploader.upload(layout, output);
    const std::size_t last_offset = layout.row_pitch_bytes + (5 * layout.pixel_stride_bytes);

    return expect(readback.status() == TileReadbackStatus::complete,
                  "edge tile readback did not complete") &&
           expect(layout.width == 6 && layout.height == 2 && layout.row_pitch_bytes == 18 &&
                      layout.pixel_stride_bytes == 3 && layout.channel_order == ChannelOrder::rgb &&
                      layout.component_type == ChannelType::uint8_unorm &&
                      layout.component_byte_order == ComponentByteOrder::native &&
                      layout.tile_contiguity == TileContiguity::separate_buffers,
                  "edge tile layout was not declared exactly") &&
           expect(uploaded && uploader.source_address() == output.data() &&
                      uploader.layout() == layout,
                  "texture upload required an intermediate repack") &&
           expect(std::equal(first_pixel.begin(), first_pixel.end(),
                             uploader.device_bytes().begin()) &&
                      std::equal(last_pixel.begin(), last_pixel.end(),
                                 uploader.device_bytes().begin() + last_offset),
                  "direct texture upload received the wrong pixels");
}

bool component_type_and_channel_order_are_explicit() {
    TextureChannels channels(4, 3, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.height", 16);
    const TileMemoryLayout layout = ctex::xport::tile_memory_layout(channels, "pbr.height", {0, 0});
    return expect(layout.width == 4 && layout.height == 3 && layout.row_pitch_bytes == 8 &&
                      layout.pixel_stride_bytes == 2 && layout.channel_order == ChannelOrder::r &&
                      layout.component_type == ChannelType::uint16_unorm &&
                      layout.byte_size() == 24,
                  "layout did not declare its component representation");
}

bool malformed_layout_is_rejected_before_publication() {
    std::array<std::byte, 6> output{};
    TileMemoryLayout malformed{
        .width = 2,
        .height = 1,
        .row_pitch_bytes = 5,
        .pixel_stride_bytes = 3,
        .channel_order = ChannelOrder::rgb,
        .component_type = ChannelType::uint8_unorm,
        .component_byte_order = ComponentByteOrder::native,
        .tile_contiguity = TileContiguity::separate_buffers,
    };
    const ctex::xport::TileVersion version{
        .coordinate = {0, 0},
        .revision = 1,
        .generation = 1,
        .residency = ctex::xport::TileResidency::host_device,
    };
    const std::array destinations{TileReadbackDestination{version, malformed, output}};
    TileReadback readback = TileReadback::begin_host(destinations);
    return expect(readback.status() == TileReadbackStatus::failed && !readback.output_readable(),
                  "host readback accepted a malformed memory layout");
}

bool host_completion_must_preserve_the_declared_layout() {
    std::array<std::byte, 6> output{std::byte{0x7f}, std::byte{0x7f}, std::byte{0x7f},
                                    std::byte{0x7f}, std::byte{0x7f}, std::byte{0x7f}};
    const TileMemoryLayout requested{
        .width = 2,
        .height = 1,
        .row_pitch_bytes = 6,
        .pixel_stride_bytes = 3,
        .channel_order = ChannelOrder::rgb,
        .component_type = ChannelType::uint8_unorm,
        .component_byte_order = ComponentByteOrder::native,
        .tile_contiguity = TileContiguity::separate_buffers,
    };
    const TileMemoryLayout wrong{
        .width = 1,
        .height = 2,
        .row_pitch_bytes = 3,
        .pixel_stride_bytes = 3,
        .channel_order = ChannelOrder::rgb,
        .component_type = ChannelType::uint8_unorm,
        .component_byte_order = ComponentByteOrder::native,
        .tile_contiguity = TileContiguity::separate_buffers,
    };
    const ctex::xport::TileVersion version{
        .coordinate = {0, 0},
        .revision = 1,
        .generation = 1,
        .residency = ctex::xport::TileResidency::host_device,
    };
    const std::array destinations{TileReadbackDestination{version, requested, output}};
    TileReadback readback = TileReadback::begin_host(destinations);
    const std::array payload{std::byte{1}, std::byte{2}, std::byte{3},
                             std::byte{4}, std::byte{5}, std::byte{6}};
    const std::array completion{ctex::xport::HostTileCompletion{version, wrong, payload}};

    return expect(!readback.complete_host(completion) &&
                      readback.status() == TileReadbackStatus::failed &&
                      std::all_of(output.begin(), output.end(),
                                  [](std::byte value) { return value == std::byte{0x7f}; }),
                  "host completion changed the declared layout or published bytes");
}

}  // namespace

int main() {
    return visible_edge_tile_uploads_directly() &&
                   component_type_and_channel_order_are_explicit() &&
                   malformed_layout_is_rejected_before_publication() &&
                   host_completion_must_preserve_the_declared_layout()
               ? 0
               : 1;
}
