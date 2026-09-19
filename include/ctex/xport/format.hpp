#ifndef CTEX_XPORT_FORMAT_HPP
#define CTEX_XPORT_FORMAT_HPP

#include <cstdint>
#include <ctex/image/pixel_format.hpp>
#include <optional>
#include <span>
#include <string>

namespace ctex::xport {

enum class ReadbackConversionPolicy : std::uint8_t { exact_only, allow_conversion };

enum class ReadbackConversion : std::uint8_t {
    none,
    uint8_unorm_to_uint16_unorm,
    uint8_unorm_to_float32,
    uint16_unorm_to_uint8_unorm,
    uint16_unorm_to_float32,
    float32_to_uint8_unorm,
    float32_to_uint16_unorm,
};

struct ReadbackFormatSelection {
    image::PixelFormat source_format;
    image::PixelFormat output_format;
    ReadbackConversion conversion;

    friend constexpr bool operator==(ReadbackFormatSelection,
                                     ReadbackFormatSelection) noexcept = default;

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] constexpr bool converted() const noexcept {
        return conversion != ReadbackConversion::none;
    }
};

enum class FormatNegotiationStatus : std::uint8_t {
    compatible,
    no_common_format,
    invalid_request,
};

struct ReadbackFormatNegotiation {
    FormatNegotiationStatus status;
    std::optional<ReadbackFormatSelection> selection;
    std::string detail;

    [[nodiscard]] constexpr bool compatible() const noexcept {
        return status == FormatNegotiationStatus::compatible && selection.has_value();
    }
};

[[nodiscard]] ReadbackFormatNegotiation negotiate_readback_format(
    image::PixelFormat source_format, std::span<const image::PixelFormat> accepted_formats,
    ReadbackConversionPolicy policy);

}  // namespace ctex::xport

#endif
