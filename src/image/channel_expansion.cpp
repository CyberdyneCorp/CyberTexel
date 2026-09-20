#include <array>
#include <bit>
#include <cstring>
#include <ctex/image/channel_expansion.hpp>
#include <limits>
#include <stdexcept>

namespace ctex::image {
namespace {

std::size_t checked_multiply(std::size_t left, std::size_t right, const char* description) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw std::overflow_error(description);
    }
    return left * right;
}

std::size_t checked_add(std::size_t left, std::size_t right, const char* description) {
    if (left > std::numeric_limits<std::size_t>::max() - right) {
        throw std::overflow_error(description);
    }
    return left + right;
}

ChannelExpansionRule expansion_rule(std::uint8_t source, std::uint8_t target) {
    if (source == target) {
        return ChannelExpansionRule::identity;
    }
    if (source == 1 && target == 3) {
        return ChannelExpansionRule::grayscale_to_rgb;
    }
    if (source == 1 && target == 4) {
        return ChannelExpansionRule::grayscale_to_rgba;
    }
    if (source == 2 && target == 4) {
        return ChannelExpansionRule::grayscale_alpha_to_rgba;
    }
    if (source == 3 && target == 4) {
        return ChannelExpansionRule::rgb_to_rgba;
    }
    throw std::invalid_argument(
        "unsupported channel expansion; supported mappings are 1-to-3, "
        "1-to-4, 2-to-4 and 3-to-4");
}

std::array<std::byte, 4> opaque_component(ChannelType type) {
    switch (type) {
        case ChannelType::uint8_unorm:
            return {std::byte{0xff}, std::byte{0}, std::byte{0}, std::byte{0}};
        case ChannelType::uint16_unorm:
            return {std::byte{0xff}, std::byte{0xff}, std::byte{0}, std::byte{0}};
        case ChannelType::float32:
            return std::bit_cast<std::array<std::byte, 4>>(1.0F);
    }
    throw std::invalid_argument("unknown channel type");
}

std::array<std::uint8_t, 4> source_components(ChannelExpansionRule rule) noexcept {
    switch (rule) {
        case ChannelExpansionRule::identity:
            return {0, 1, 2, 3};
        case ChannelExpansionRule::grayscale_to_rgb:
        case ChannelExpansionRule::grayscale_to_rgba:
        case ChannelExpansionRule::grayscale_alpha_to_rgba:
            return {0, 0, 0, 1};
        case ChannelExpansionRule::rgb_to_rgba:
            return {0, 1, 2, 3};
    }
    return {};
}

bool component_is_opaque(ChannelExpansionRule rule, std::size_t component) noexcept {
    return component == 3 && (rule == ChannelExpansionRule::grayscale_to_rgba ||
                              rule == ChannelExpansionRule::rgb_to_rgba);
}

void expand_pixel(std::span<const std::byte> source, std::span<std::byte> destination,
                  std::size_t bytes_per_channel, std::uint8_t target_channels,
                  ChannelExpansionRule rule, std::span<const std::byte> opaque) {
    const auto components = source_components(rule);
    for (std::size_t component = 0; component < target_channels; ++component) {
        const std::span<const std::byte> value =
            component_is_opaque(rule, component)
                ? opaque
                : source.subspan(components[component] * bytes_per_channel, bytes_per_channel);
        std::memcpy(destination.data() + component * bytes_per_channel, value.data(),
                    bytes_per_channel);
    }
}

}  // namespace

ChannelExpansionResult expand_channels(const ChannelExpansionRequest& request) {
    if (request.width == 0 || request.height == 0 || !request.source_format.is_valid()) {
        throw std::invalid_argument(
            "channel expansion requires non-zero dimensions and a valid "
            "source pixel format");
    }
    if (request.target_channel_count == 0 || request.target_channel_count > 4) {
        throw std::invalid_argument("target channel count must be from one through four");
    }
    const ChannelExpansionRule rule =
        expansion_rule(request.source_format.channel_count, request.target_channel_count);
    const std::size_t source_row_bytes = checked_multiply(
        request.width, request.source_format.bytes_per_pixel(), "source row size overflow");
    const std::size_t source_stride =
        request.row_stride_bytes == 0 ? source_row_bytes : request.row_stride_bytes;
    if (source_stride < source_row_bytes) {
        throw std::invalid_argument("source row stride is smaller than one pixel row");
    }
    const std::size_t row_offsets =
        checked_multiply(source_stride, request.height - 1, "source image size overflow");
    const std::size_t required_source_bytes =
        checked_add(row_offsets, source_row_bytes, "source image size overflow");
    if (request.pixels.size() < required_source_bytes) {
        throw std::invalid_argument("source pixel buffer is smaller than its declared layout");
    }

    const PixelFormat target_format{request.source_format.channel_type,
                                    request.target_channel_count};
    const std::size_t target_row_bytes = checked_multiply(
        request.width, target_format.bytes_per_pixel(), "target row size overflow");
    const std::size_t target_size =
        checked_multiply(target_row_bytes, request.height, "target image size overflow");
    ChannelExpansionResult result{.format = target_format, .rule = rule, .pixels = {}};
    result.pixels.resize(target_size);

    const std::size_t source_pixel_bytes = request.source_format.bytes_per_pixel();
    const std::size_t target_pixel_bytes = target_format.bytes_per_pixel();
    const auto opaque = opaque_component(request.source_format.channel_type);
    const auto opaque_value = std::span(opaque).first(request.source_format.bytes_per_channel());
    for (std::uint32_t y = 0; y < request.height; ++y) {
        for (std::uint32_t x = 0; x < request.width; ++x) {
            const std::size_t source_offset = y * source_stride + x * source_pixel_bytes;
            const std::size_t target_offset = y * target_row_bytes + x * target_pixel_bytes;
            expand_pixel(request.pixels.subspan(source_offset, source_pixel_bytes),
                         std::span(result.pixels).subspan(target_offset, target_pixel_bytes),
                         request.source_format.bytes_per_channel(), request.target_channel_count,
                         rule, opaque_value);
        }
    }
    return result;
}

}  // namespace ctex::image
