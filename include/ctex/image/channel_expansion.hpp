#ifndef CTEX_IMAGE_CHANNEL_EXPANSION_HPP
#define CTEX_IMAGE_CHANNEL_EXPANSION_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/pixel_format.hpp>
#include <span>
#include <vector>

namespace ctex::image {

enum class ChannelExpansionRule : std::uint8_t {
    identity,
    grayscale_to_rgb,
    grayscale_to_rgba,
    grayscale_alpha_to_rgba,
    rgb_to_rgba,
};

struct ChannelExpansionRequest {
    std::span<const std::byte> pixels;
    std::uint32_t width{};
    std::uint32_t height{};
    PixelFormat source_format{};
    std::size_t row_stride_bytes{};
    std::uint8_t target_channel_count{};
};

struct ChannelExpansionResult {
    PixelFormat format{};
    ChannelExpansionRule rule{};
    std::vector<std::byte> pixels;
};

[[nodiscard]] ChannelExpansionResult expand_channels(const ChannelExpansionRequest& request);

}  // namespace ctex::image

#endif
