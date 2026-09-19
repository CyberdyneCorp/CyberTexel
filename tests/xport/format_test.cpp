#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctex/xport/readback.hpp>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

using ctex::doc::TextureChannels;
using ctex::image::ChannelType;
using ctex::image::PixelFormat;
using ctex::xport::FormatNegotiationStatus;
using ctex::xport::ReadbackConversion;
using ctex::xport::ReadbackConversionPolicy;
using ctex::xport::ReadbackFormatSelection;
using ctex::xport::TileReadback;
using ctex::xport::TileReadbackDestination;
using ctex::xport::TileReadbackStatus;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

std::array<std::byte, 2> uint16_bytes(std::uint16_t value) {
    std::array<std::byte, 2> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(value));
    return bytes;
}

std::vector<std::byte> half_sample(ChannelType type) {
    if (type == ChannelType::uint8_unorm) {
        return {std::byte{128}};
    }
    if (type == ChannelType::uint16_unorm) {
        const auto bytes = uint16_bytes(32768);
        return {bytes.begin(), bytes.end()};
    }
    const float value = 0.5F;
    std::vector<std::byte> bytes(sizeof(value));
    std::memcpy(bytes.data(), &value, sizeof(value));
    return bytes;
}

double normalized_sample(std::span<const std::byte> bytes, ChannelType type) {
    if (type == ChannelType::uint8_unorm) {
        return static_cast<double>(std::to_integer<std::uint8_t>(bytes.front())) / 255.0;
    }
    if (type == ChannelType::uint16_unorm) {
        std::uint16_t value{};
        std::memcpy(&value, bytes.data(), sizeof(value));
        return static_cast<double>(value) / 65535.0;
    }
    float value{};
    std::memcpy(&value, bytes.data(), sizeof(value));
    return value;
}

std::uint8_t bit_depth(ChannelType type) {
    if (type == ChannelType::uint8_unorm) {
        return 8;
    }
    if (type == ChannelType::uint16_unorm) {
        return 16;
    }
    return 32;
}

bool host_controls_conversion_or_refusal() {
    TextureChannels channels(2, 1, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.height", 16);
    const PixelFormat source = channels.pixels("pbr.height").format();
    const std::array accepted{PixelFormat{ChannelType::uint8_unorm, 1}};

    const auto refused = ctex::xport::negotiate_readback_format(
        source, accepted, ReadbackConversionPolicy::exact_only);
    const auto converted = ctex::xport::negotiate_readback_format(
        source, accepted, ReadbackConversionPolicy::allow_conversion);

    if (!expect(refused.status == FormatNegotiationStatus::no_common_format &&
                    !refused.selection.has_value() && !refused.detail.empty(),
                "exact-only host did not receive an explicit format mismatch") ||
        !expect(converted.compatible() && converted.selection->converted() &&
                    converted.selection->conversion ==
                        ReadbackConversion::uint16_unorm_to_uint8_unorm &&
                    converted.selection->output_format == accepted.front(),
                "conversion-enabled host did not receive the selected conversion")) {
        return false;
    }

    const auto before = channels.channel_revision_cursor("pbr.height");
    channels.pixels("pbr.height").write_pixel(0, 0, uint16_bytes(32768));
    channels.pixels("pbr.height").write_pixel(1, 0, uint16_bytes(65535));
    const auto delta = ctex::xport::query_channel_delta(channels, "pbr.height", before);
    const ReadbackFormatSelection selection = *converted.selection;
    const auto layout = ctex::xport::tile_memory_layout(channels, "pbr.height", {0, 0}, selection);
    std::vector<std::byte> output(layout.byte_size(), std::byte{0x7f});
    const std::array destinations{
        TileReadbackDestination{delta.changed_tiles.front(), layout, output},
    };
    TileReadback readback = TileReadback::begin_cpu(channels, "pbr.height", delta.current_cursor,
                                                    selection, destinations);

    return expect(readback.status() == TileReadbackStatus::complete &&
                      readback.format_selection() == converted.selection &&
                      layout.component_type == ChannelType::uint8_unorm &&
                      layout.pixel_stride_bytes == 1 && layout.row_pitch_bytes == 2,
                  "converted readback did not publish its negotiated layout") &&
           expect(output[0] == std::byte{128} && output[1] == std::byte{255},
                  "16-bit readback was not converted to the negotiated 8-bit format");
}

bool host_readback_reports_and_enforces_its_selection() {
    const PixelFormat source{ChannelType::uint16_unorm, 1};
    const std::array accepted{PixelFormat{ChannelType::uint8_unorm, 1}};
    const auto negotiated = ctex::xport::negotiate_readback_format(
        source, accepted, ReadbackConversionPolicy::allow_conversion);
    const ctex::xport::TileVersion version{
        .coordinate = {0, 0},
        .revision = 1,
        .generation = 1,
        .residency = ctex::xport::TileResidency::host_device,
    };
    const ctex::xport::TileMemoryLayout layout{
        .width = 2,
        .height = 1,
        .row_pitch_bytes = 2,
        .pixel_stride_bytes = 1,
        .channel_order = ctex::xport::ChannelOrder::r,
        .component_type = ChannelType::uint8_unorm,
        .component_byte_order = ctex::xport::ComponentByteOrder::native,
        .tile_contiguity = ctex::xport::TileContiguity::separate_buffers,
    };
    std::array<std::byte, 2> output{std::byte{0x7f}, std::byte{0x7f}};
    const std::array destinations{TileReadbackDestination{version, layout, output}};
    TileReadback readback = TileReadback::begin_host(*negotiated.selection, destinations);
    auto wrong_layout = layout;
    wrong_layout.width = 1;
    wrong_layout.row_pitch_bytes = 2;
    wrong_layout.pixel_stride_bytes = 2;
    wrong_layout.channel_order = ctex::xport::ChannelOrder::rg;
    std::array<std::byte, 2> wrong_output{std::byte{0x6d}, std::byte{0x6d}};
    const std::array wrong_destinations{
        TileReadbackDestination{version, wrong_layout, wrong_output},
    };
    TileReadback wrong = TileReadback::begin_host(*negotiated.selection, wrong_destinations);
    const std::array payload{std::byte{128}, std::byte{255}};
    const std::array completions{ctex::xport::HostTileCompletion{version, layout, payload}};

    return expect(readback.status() == TileReadbackStatus::pending &&
                      readback.format_selection() == negotiated.selection,
                  "host readback did not report its negotiated format") &&
           expect(wrong.status() == TileReadbackStatus::failed &&
                      wrong_output.front() == std::byte{0x6d},
                  "host readback accepted a layout outside its negotiated format") &&
           expect(readback.complete_host(completions) &&
                      readback.status() == TileReadbackStatus::complete && output == payload,
                  "host readback did not deliver its negotiated format");
}

bool host_order_selects_the_preferred_compatible_format() {
    const PixelFormat source{ChannelType::uint8_unorm, 3};
    const std::array accepted{
        PixelFormat{ChannelType::float32, 3},
        PixelFormat{ChannelType::uint16_unorm, 3},
        source,
    };
    const auto result = ctex::xport::negotiate_readback_format(
        source, accepted, ReadbackConversionPolicy::allow_conversion);
    return expect(result.compatible() && result.selection->output_format == accepted.front() &&
                      result.selection->conversion == ReadbackConversion::uint8_unorm_to_float32,
                  "negotiation ignored the host's accepted-format order");
}

bool every_declared_component_conversion_is_negotiable() {
    const std::array types{
        ChannelType::uint8_unorm,
        ChannelType::uint16_unorm,
        ChannelType::float32,
    };
    for (const ChannelType source_type : types) {
        for (const ChannelType output_type : types) {
            const PixelFormat source{source_type, 4};
            const std::array accepted{PixelFormat{output_type, 4}};
            const auto result = ctex::xport::negotiate_readback_format(
                source, accepted, ReadbackConversionPolicy::allow_conversion);
            if (!result.compatible() || !result.selection->is_valid() ||
                result.selection->converted() != (source_type != output_type)) {
                return expect(false, "a declared component conversion could not be negotiated");
            }
        }
    }
    return true;
}

bool every_negotiated_component_conversion_is_executable() {
    const std::array types{
        ChannelType::uint8_unorm,
        ChannelType::uint16_unorm,
        ChannelType::float32,
    };
    for (const ChannelType source_type : types) {
        for (const ChannelType output_type : types) {
            TextureChannels channels(1, 1, 8);
            channels.register_descriptor({
                .semantic_id = "test.value",
                .component_count = 1,
                .scalar_representation = source_type == ChannelType::float32
                                             ? ctex::doc::ScalarRepresentation::floating_point
                                             : ctex::doc::ScalarRepresentation::unsigned_normalized,
                .preferred_bit_depth = bit_depth(source_type),
                .default_value = {0.0},
                .classification = ctex::doc::ChannelClassification::data,
                .blending_policy = ctex::doc::BlendingPolicy::scalar,
                .export_mapping = "value",
            });
            channels.enable("test.value", bit_depth(source_type));
            const auto before = channels.channel_revision_cursor("test.value");
            channels.pixels("test.value").write_pixel(0, 0, half_sample(source_type));
            const auto delta = ctex::xport::query_channel_delta(channels, "test.value", before);
            const std::array accepted{PixelFormat{output_type, 1}};
            const auto negotiated = ctex::xport::negotiate_readback_format(
                channels.pixels("test.value").format(), accepted,
                ReadbackConversionPolicy::allow_conversion);
            if (!negotiated.compatible()) {
                return expect(false, "executable conversion was not negotiated");
            }
            const auto layout = ctex::xport::tile_memory_layout(channels, "test.value", {0, 0},
                                                                *negotiated.selection);
            std::vector<std::byte> output(layout.byte_size());
            const std::array destinations{
                TileReadbackDestination{delta.changed_tiles.front(), layout, output},
            };
            TileReadback readback = TileReadback::begin_cpu(
                channels, "test.value", delta.current_cursor, *negotiated.selection, destinations);
            if (readback.status() != TileReadbackStatus::complete ||
                std::abs(normalized_sample(output, output_type) - 0.5) > 0.0021) {
                return expect(false, "negotiated component conversion produced the wrong value");
            }
        }
    }
    return true;
}

bool invalid_and_incompatible_declarations_are_reported() {
    const PixelFormat source{ChannelType::uint16_unorm, 1};
    const std::array invalid{PixelFormat{ChannelType::uint8_unorm, 0}};
    const std::array wrong_channels{PixelFormat{ChannelType::uint8_unorm, 3}};
    const auto invalid_result = ctex::xport::negotiate_readback_format(
        source, invalid, ReadbackConversionPolicy::allow_conversion);
    const auto mismatch_result = ctex::xport::negotiate_readback_format(
        source, wrong_channels, ReadbackConversionPolicy::allow_conversion);
    return expect(invalid_result.status == FormatNegotiationStatus::invalid_request &&
                      !invalid_result.selection.has_value(),
                  "invalid host format was treated as a compatibility mismatch") &&
           expect(mismatch_result.status == FormatNegotiationStatus::no_common_format &&
                      !mismatch_result.selection.has_value(),
                  "channel-count mismatch was silently converted");
}

bool forged_conversion_is_rejected_without_publication() {
    TextureChannels channels(1, 1, 8, ctex::doc::metallic_roughness_channels());
    channels.enable("pbr.height", 16);
    const auto before = channels.channel_revision_cursor("pbr.height");
    channels.pixels("pbr.height").write_pixel(0, 0, uint16_bytes(65535));
    const auto delta = ctex::xport::query_channel_delta(channels, "pbr.height", before);
    const ReadbackFormatSelection forged{
        .source_format = PixelFormat{ChannelType::uint16_unorm, 1},
        .output_format = PixelFormat{ChannelType::uint8_unorm, 1},
        .conversion = ReadbackConversion::none,
    };
    std::array<std::byte, 1> output{std::byte{0x7f}};
    const auto native_layout = ctex::xport::tile_memory_layout(channels, "pbr.height", {0, 0});
    auto output_layout = native_layout;
    output_layout.row_pitch_bytes = 1;
    output_layout.pixel_stride_bytes = 1;
    output_layout.component_type = ChannelType::uint8_unorm;
    const std::array destinations{
        TileReadbackDestination{delta.changed_tiles.front(), output_layout, output},
    };
    TileReadback readback =
        TileReadback::begin_cpu(channels, "pbr.height", delta.current_cursor, forged, destinations);
    return expect(readback.status() == TileReadbackStatus::failed && !readback.output_readable() &&
                      output.front() == std::byte{0x7f},
                  "forged format conversion published output bytes");
}

}  // namespace

int main() {
    return host_controls_conversion_or_refusal() &&
                   host_readback_reports_and_enforces_its_selection() &&
                   host_order_selects_the_preferred_compatible_format() &&
                   every_declared_component_conversion_is_negotiable() &&
                   every_negotiated_component_conversion_is_executable() &&
                   invalid_and_incompatible_declarations_are_reported() &&
                   forged_conversion_is_rejected_without_publication()
               ? 0
               : 1;
}
