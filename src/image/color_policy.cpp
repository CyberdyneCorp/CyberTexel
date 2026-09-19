#include <algorithm>
#include <cmath>
#include <ctex/image/color_policy.hpp>
#include <numeric>
#include <stdexcept>

namespace ctex::image {
namespace {

double quantize_unorm(double value, std::uint32_t maximum) noexcept {
    const double clamped = std::clamp(value, 0.0, 1.0);
    return std::round(clamped * maximum) / maximum;
}

}  // namespace

std::string_view channel_name(ChannelSemantic channel) {
    switch (channel) {
        case ChannelSemantic::base_color:
            return "base color";
        case ChannelSemantic::opacity:
            return "opacity";
        case ChannelSemantic::roughness:
            return "roughness";
        case ChannelSemantic::metallic:
            return "metallic";
        case ChannelSemantic::normal:
            return "normal";
        case ChannelSemantic::height:
            return "height";
        case ChannelSemantic::occlusion:
            return "occlusion";
        case ChannelSemantic::emission:
            return "emission";
        case ChannelSemantic::subsurface:
            return "subsurface";
    }
    throw std::invalid_argument("unsupported channel semantic");
}

bool is_color_valued(ChannelSemantic channel) {
    switch (channel) {
        case ChannelSemantic::base_color:
        case ChannelSemantic::emission:
            return true;
        case ChannelSemantic::opacity:
        case ChannelSemantic::roughness:
        case ChannelSemantic::metallic:
        case ChannelSemantic::normal:
        case ChannelSemantic::height:
        case ChannelSemantic::occlusion:
        case ChannelSemantic::subsurface:
            return false;
    }
    throw std::invalid_argument("unsupported channel semantic");
}

ResolvedInputSpace resolve_input_space(InputColorSpace declaration, ChannelSemantic channel) {
    switch (declaration) {
        case InputColorSpace::automatic:
            return {
                is_color_valued(channel) ? ColorSpace::srgb_rec709 : ColorSpace::linear_rec709,
                true,
            };
        case InputColorSpace::linear_rec709:
            return {ColorSpace::linear_rec709, false};
        case InputColorSpace::srgb_rec709:
            return {ColorSpace::srgb_rec709, false};
    }
    throw std::invalid_argument("unsupported input color space declaration");
}

RgbColor input_to_working_space(RgbColor value, InputColorSpace declaration,
                                ChannelSemantic channel) {
    const ResolvedInputSpace resolved = resolve_input_space(declaration, channel);
    if (!is_color_valued(channel)) {
        return value;
    }
    return convert_color(value, resolved.color_space, working_color_space());
}

std::uint8_t recommended_bit_depth(ChannelSemantic channel) {
    switch (channel) {
        case ChannelSemantic::normal:
        case ChannelSemantic::height:
            return 16;
        default:
            static_cast<void>(channel_name(channel));
            return 8;
    }
}

std::optional<BitDepthWarning> bit_depth_warning(ChannelSemantic channel,
                                                 std::uint8_t selected_bits) {
    if (selected_bits != 8 && selected_bits != 16 && selected_bits != 32) {
        throw std::invalid_argument("selected bit depth must be 8, 16 or 32");
    }
    const std::uint8_t recommended = recommended_bit_depth(channel);
    if (selected_bits >= recommended) {
        return std::nullopt;
    }
    return BitDepthWarning{channel, selected_bits, recommended};
}

double accumulate_height(std::span<const double> contributions, std::uint8_t storage_bits) {
    const double accumulated = std::accumulate(contributions.begin(), contributions.end(), 0.0);
    switch (storage_bits) {
        case 8:
            return quantize_unorm(accumulated, 255);
        case 16:
            return quantize_unorm(accumulated, 65535);
        case 32:
            return accumulated;
        default:
            throw std::invalid_argument("storage bit depth must be 8, 16 or 32");
    }
}

std::uint8_t quantize_unorm8(double value, std::uint32_t x, std::uint32_t y, bool dither) noexcept {
    constexpr std::uint8_t matrix[4][4] = {
        {0, 8, 2, 10},
        {12, 4, 14, 6},
        {3, 11, 1, 9},
        {15, 7, 13, 5},
    };
    double adjusted = value;
    if (dither) {
        const double threshold = (static_cast<double>(matrix[y % 4][x % 4]) + 0.5) / 16.0;
        adjusted += (threshold - 0.5) / 255.0;
    }
    return static_cast<std::uint8_t>(std::round(std::clamp(adjusted, 0.0, 1.0) * 255.0));
}

}  // namespace ctex::image
