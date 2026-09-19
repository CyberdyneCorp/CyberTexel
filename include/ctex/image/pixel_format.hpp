#ifndef CTEX_IMAGE_PIXEL_FORMAT_HPP
#define CTEX_IMAGE_PIXEL_FORMAT_HPP

#include <cstddef>
#include <cstdint>

namespace ctex::image {

enum class ChannelType : std::uint8_t {
    uint8_unorm,
    uint16_unorm,
    float32,
};

struct PixelFormat {
    ChannelType channel_type;
    std::uint8_t channel_count;

    friend constexpr bool operator==(PixelFormat, PixelFormat) noexcept = default;

    [[nodiscard]] constexpr std::size_t bytes_per_channel() const noexcept {
        switch (channel_type) {
            case ChannelType::uint8_unorm:
                return 1;
            case ChannelType::uint16_unorm:
                return 2;
            case ChannelType::float32:
                return 4;
        }
        return 0;
    }

    [[nodiscard]] constexpr std::size_t bytes_per_pixel() const noexcept {
        return bytes_per_channel() * channel_count;
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return bytes_per_channel() != 0 && channel_count >= 1 && channel_count <= 4;
    }
};

}  // namespace ctex::image

#endif
