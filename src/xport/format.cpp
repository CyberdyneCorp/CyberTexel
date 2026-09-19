#include <ctex/xport/format.hpp>
#include <utility>

namespace ctex::xport {
namespace {

std::optional<ReadbackConversion> conversion_between(image::ChannelType source,
                                                     image::ChannelType output) {
    using enum image::ChannelType;
    if (source == output) {
        return ReadbackConversion::none;
    }
    if (source == uint8_unorm && output == uint16_unorm) {
        return ReadbackConversion::uint8_unorm_to_uint16_unorm;
    }
    if (source == uint8_unorm && output == float32) {
        return ReadbackConversion::uint8_unorm_to_float32;
    }
    if (source == uint16_unorm && output == uint8_unorm) {
        return ReadbackConversion::uint16_unorm_to_uint8_unorm;
    }
    if (source == uint16_unorm && output == float32) {
        return ReadbackConversion::uint16_unorm_to_float32;
    }
    if (source == float32 && output == uint8_unorm) {
        return ReadbackConversion::float32_to_uint8_unorm;
    }
    if (source == float32 && output == uint16_unorm) {
        return ReadbackConversion::float32_to_uint16_unorm;
    }
    return std::nullopt;
}

ReadbackFormatNegotiation invalid(std::string detail) {
    return {FormatNegotiationStatus::invalid_request, std::nullopt, std::move(detail)};
}

}  // namespace

bool ReadbackFormatSelection::is_valid() const noexcept {
    if (!source_format.is_valid() || !output_format.is_valid() ||
        source_format.channel_count != output_format.channel_count) {
        return false;
    }
    const auto expected =
        conversion_between(source_format.channel_type, output_format.channel_type);
    return expected.has_value() && *expected == conversion;
}

ReadbackFormatNegotiation negotiate_readback_format(
    image::PixelFormat source_format, std::span<const image::PixelFormat> accepted_formats,
    ReadbackConversionPolicy policy) {
    if (!source_format.is_valid()) {
        return invalid("readback source format is invalid");
    }
    if (policy != ReadbackConversionPolicy::exact_only &&
        policy != ReadbackConversionPolicy::allow_conversion) {
        return invalid("readback conversion policy is invalid");
    }
    for (const image::PixelFormat accepted : accepted_formats) {
        if (!accepted.is_valid()) {
            return invalid("host declared an invalid readback format");
        }
    }

    for (const image::PixelFormat accepted : accepted_formats) {
        if (policy == ReadbackConversionPolicy::exact_only && accepted != source_format) {
            continue;
        }
        if (accepted.channel_count != source_format.channel_count) {
            continue;
        }
        const auto conversion =
            conversion_between(source_format.channel_type, accepted.channel_type);
        if (conversion.has_value()) {
            return {
                FormatNegotiationStatus::compatible,
                ReadbackFormatSelection{source_format, accepted, *conversion},
                {},
            };
        }
    }
    return {
        FormatNegotiationStatus::no_common_format,
        std::nullopt,
        "host and channel have no common readback format",
    };
}

}  // namespace ctex::xport
