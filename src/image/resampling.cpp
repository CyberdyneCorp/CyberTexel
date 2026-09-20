#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctex/image/resampling.hpp>
#include <limits>
#include <stdexcept>

namespace ctex::image {
namespace {

struct LinearAxis {
    std::uint32_t low{};
    std::uint32_t high{};
    double fraction{};
};

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

std::size_t required_source_bytes(const ImageResampleRequest& request, std::size_t source_row_bytes,
                                  std::size_t source_stride) {
    const std::size_t row_offsets =
        checked_multiply(source_stride, request.source_height - 1, "source image size overflow");
    return checked_add(row_offsets, source_row_bytes, "source image size overflow");
}

double source_position(std::uint32_t destination, std::uint32_t source_extent,
                       std::uint32_t destination_extent) noexcept {
    return ((static_cast<double>(destination) + 0.5) * source_extent / destination_extent) - 0.5;
}

std::uint32_t nearest_coordinate(std::uint32_t destination, std::uint32_t source_extent,
                                 std::uint32_t destination_extent) noexcept {
    const double coordinate =
        (static_cast<double>(destination) + 0.5) * source_extent / destination_extent;
    return std::min(static_cast<std::uint32_t>(coordinate), source_extent - 1);
}

LinearAxis linear_axis(std::uint32_t destination, std::uint32_t source_extent,
                       std::uint32_t destination_extent) noexcept {
    const double coordinate =
        std::clamp(source_position(destination, source_extent, destination_extent), 0.0,
                   static_cast<double>(source_extent - 1));
    const auto low = static_cast<std::uint32_t>(std::floor(coordinate));
    return {
        .low = low,
        .high = std::min(low + 1, source_extent - 1),
        .fraction = coordinate - low,
    };
}

std::span<const std::byte> source_pixel(const ImageResampleRequest& request,
                                        std::size_t source_stride, std::uint32_t x,
                                        std::uint32_t y) {
    const std::size_t offset = static_cast<std::size_t>(y) * source_stride +
                               static_cast<std::size_t>(x) * request.format.bytes_per_pixel();
    return request.pixels.subspan(offset, request.format.bytes_per_pixel());
}

double read_component(std::span<const std::byte> pixel, ChannelType type, std::size_t component) {
    const std::size_t bytes_per_channel = PixelFormat{type, 1}.bytes_per_channel();
    const std::byte* source = pixel.data() + component * bytes_per_channel;
    switch (type) {
        case ChannelType::uint8_unorm:
            return std::to_integer<std::uint8_t>(*source);
        case ChannelType::uint16_unorm: {
            std::uint16_t value = 0;
            std::memcpy(&value, source, sizeof(value));
            return value;
        }
        case ChannelType::float32: {
            float value = 0.0F;
            std::memcpy(&value, source, sizeof(value));
            return value;
        }
    }
    throw std::invalid_argument("unknown channel type");
}

void write_component(std::span<std::byte> pixel, ChannelType type, std::size_t component,
                     double value) {
    const std::size_t bytes_per_channel = PixelFormat{type, 1}.bytes_per_channel();
    std::byte* destination = pixel.data() + component * bytes_per_channel;
    switch (type) {
        case ChannelType::uint8_unorm: {
            const auto output =
                static_cast<std::uint8_t>(std::llround(std::clamp(value, 0.0, 255.0)));
            std::memcpy(destination, &output, sizeof(output));
            return;
        }
        case ChannelType::uint16_unorm: {
            const auto output =
                static_cast<std::uint16_t>(std::llround(std::clamp(value, 0.0, 65535.0)));
            std::memcpy(destination, &output, sizeof(output));
            return;
        }
        case ChannelType::float32: {
            const float output = static_cast<float>(value);
            std::memcpy(destination, &output, sizeof(output));
            return;
        }
    }
    throw std::invalid_argument("unknown channel type");
}

double interpolate(double lower, double upper, double fraction) noexcept {
    return lower + (upper - lower) * fraction;
}

void sample_bilinear(const ImageResampleRequest& request, std::size_t source_stride,
                     LinearAxis horizontal, LinearAxis vertical, std::span<std::byte> destination) {
    const auto top_left = source_pixel(request, source_stride, horizontal.low, vertical.low);
    const auto top_right = source_pixel(request, source_stride, horizontal.high, vertical.low);
    const auto bottom_left = source_pixel(request, source_stride, horizontal.low, vertical.high);
    const auto bottom_right = source_pixel(request, source_stride, horizontal.high, vertical.high);
    for (std::size_t component = 0; component < request.format.channel_count; ++component) {
        const double top = interpolate(
            read_component(top_left, request.format.channel_type, component),
            read_component(top_right, request.format.channel_type, component), horizontal.fraction);
        const double bottom =
            interpolate(read_component(bottom_left, request.format.channel_type, component),
                        read_component(bottom_right, request.format.channel_type, component),
                        horizontal.fraction);
        write_component(destination, request.format.channel_type, component,
                        interpolate(top, bottom, vertical.fraction));
    }
}

void sample_nearest(const ImageResampleRequest& request, std::size_t source_stride,
                    std::uint32_t output_x, std::uint32_t output_y,
                    std::span<std::byte> destination) {
    const std::uint32_t source_x =
        nearest_coordinate(output_x, request.source_width, request.output_width);
    const std::uint32_t source_y =
        nearest_coordinate(output_y, request.source_height, request.output_height);
    const auto source = source_pixel(request, source_stride, source_x, source_y);
    std::memcpy(destination.data(), source.data(), source.size());
}

}  // namespace

ImageResampleResult resample_image(const ImageResampleRequest& request) {
    if (request.source_width == 0 || request.source_height == 0 || request.output_width == 0 ||
        request.output_height == 0 || !request.format.is_valid()) {
        throw std::invalid_argument("resampling requires non-zero dimensions and a valid format");
    }
    if (request.filter != ImageResampleFilter::nearest &&
        request.filter != ImageResampleFilter::bilinear) {
        throw std::invalid_argument("unknown image resampling filter");
    }
    const std::size_t source_row_bytes = checked_multiply(
        request.source_width, request.format.bytes_per_pixel(), "source row size overflow");
    const std::size_t source_stride =
        request.source_row_stride_bytes == 0 ? source_row_bytes : request.source_row_stride_bytes;
    if (source_stride < source_row_bytes) {
        throw std::invalid_argument("source row stride is smaller than one pixel row");
    }
    if (request.pixels.size() < required_source_bytes(request, source_row_bytes, source_stride)) {
        throw std::invalid_argument("source pixel buffer is smaller than its declared layout");
    }
    const std::size_t output_row_bytes = checked_multiply(
        request.output_width, request.format.bytes_per_pixel(), "output row size overflow");
    const std::size_t output_size =
        checked_multiply(output_row_bytes, request.output_height, "output image size overflow");
    if (request.maximum_output_bytes == 0 || output_size > request.maximum_output_bytes) {
        throw std::length_error("resampled image exceeds its output byte limit");
    }

    ImageResampleResult result{.width = request.output_width,
                               .height = request.output_height,
                               .format = request.format,
                               .filter = request.filter,
                               .pixels = {}};
    result.pixels.resize(output_size);
    const std::size_t pixel_bytes = request.format.bytes_per_pixel();
    for (std::uint32_t y = 0; y < request.output_height; ++y) {
        const LinearAxis vertical = linear_axis(y, request.source_height, request.output_height);
        for (std::uint32_t x = 0; x < request.output_width; ++x) {
            const std::size_t output_offset = static_cast<std::size_t>(y) * output_row_bytes +
                                              static_cast<std::size_t>(x) * pixel_bytes;
            const auto destination = std::span(result.pixels).subspan(output_offset, pixel_bytes);
            if (request.filter == ImageResampleFilter::nearest) {
                sample_nearest(request, source_stride, x, y, destination);
            } else {
                sample_bilinear(request, source_stride,
                                linear_axis(x, request.source_width, request.output_width),
                                vertical, destination);
            }
        }
    }
    return result;
}

}  // namespace ctex::image
