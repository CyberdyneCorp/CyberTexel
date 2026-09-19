#ifndef CTEX_IMAGE_COLOR_POLICY_HPP
#define CTEX_IMAGE_COLOR_POLICY_HPP

#include <cstdint>
#include <ctex/image/color.hpp>
#include <optional>
#include <span>
#include <string_view>

namespace ctex::image {

enum class ChannelSemantic {
    base_color,
    opacity,
    roughness,
    metallic,
    normal,
    height,
    occlusion,
    emission,
    subsurface,
};

enum class InputColorSpace {
    automatic,
    linear_rec709,
    srgb_rec709,
};

struct ResolvedInputSpace {
    ColorSpace color_space;
    bool inferred;
};

struct BitDepthWarning {
    ChannelSemantic channel;
    std::uint8_t selected_bits;
    std::uint8_t recommended_bits;
};

[[nodiscard]] std::string_view channel_name(ChannelSemantic channel);
[[nodiscard]] bool is_color_valued(ChannelSemantic channel);
[[nodiscard]] ResolvedInputSpace resolve_input_space(InputColorSpace declaration,
                                                     ChannelSemantic channel);
[[nodiscard]] RgbColor input_to_working_space(RgbColor value, InputColorSpace declaration,
                                              ChannelSemantic channel);
[[nodiscard]] std::uint8_t recommended_bit_depth(ChannelSemantic channel);
[[nodiscard]] std::optional<BitDepthWarning> bit_depth_warning(ChannelSemantic channel,
                                                               std::uint8_t selected_bits);
[[nodiscard]] double accumulate_height(std::span<const double> contributions,
                                       std::uint8_t storage_bits);
[[nodiscard]] std::uint8_t quantize_unorm8(double value, std::uint32_t x, std::uint32_t y,
                                           bool dither = true) noexcept;

}  // namespace ctex::image

#endif
