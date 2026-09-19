#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <ctex/doc/channels.hpp>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ctex::doc {
namespace {

bool valid_bit_depth(std::uint8_t bit_depth) noexcept {
    return bit_depth == 8 || bit_depth == 16 || bit_depth == 32;
}

image::ChannelType channel_type(ScalarRepresentation representation, std::uint8_t bit_depth) {
    if (representation == ScalarRepresentation::floating_point) {
        if (bit_depth != 32) {
            throw std::invalid_argument("floating-point channels require 32-bit storage");
        }
        return image::ChannelType::float32;
    }
    switch (bit_depth) {
        case 8:
            return image::ChannelType::uint8_unorm;
        case 16:
            return image::ChannelType::uint16_unorm;
        case 32:
            return image::ChannelType::float32;
        default:
            throw std::invalid_argument("normalized channels require 8-, 16- or 32-bit storage");
    }
}

void validate_descriptor(const ChannelDescriptor& descriptor) {
    if (descriptor.semantic_id.empty()) {
        throw std::invalid_argument("channel semantic identifier must not be empty");
    }
    if (descriptor.component_count == 0 || descriptor.component_count > 4) {
        throw std::invalid_argument("channel component count must be between one and four");
    }
    if (!valid_bit_depth(descriptor.preferred_bit_depth)) {
        throw std::invalid_argument("channel bit depth must be 8, 16 or 32");
    }
    static_cast<void>(
        channel_type(descriptor.scalar_representation, descriptor.preferred_bit_depth));
    if (descriptor.default_value.size() != descriptor.component_count) {
        throw std::invalid_argument("channel default value must match its component count");
    }
    if (descriptor.export_mapping.empty()) {
        throw std::invalid_argument("channel export mapping must not be empty");
    }
}

std::vector<std::byte> encode_clear_pixel(const ChannelDescriptor& descriptor,
                                          std::uint8_t bit_depth) {
    const image::ChannelType type = channel_type(descriptor.scalar_representation, bit_depth);
    const image::PixelFormat format{type, descriptor.component_count};
    std::vector<std::byte> result(format.bytes_per_pixel());
    for (std::size_t component = 0; component < descriptor.default_value.size(); ++component) {
        const double value = descriptor.default_value[component];
        const std::size_t offset = component * format.bytes_per_channel();
        if (type == image::ChannelType::uint8_unorm) {
            const auto encoded = static_cast<std::uint8_t>(
                std::round(std::clamp(value, 0.0, 1.0) * std::numeric_limits<std::uint8_t>::max()));
            std::memcpy(result.data() + offset, &encoded, sizeof(encoded));
        } else if (type == image::ChannelType::uint16_unorm) {
            const auto encoded = static_cast<std::uint16_t>(std::round(
                std::clamp(value, 0.0, 1.0) * std::numeric_limits<std::uint16_t>::max()));
            std::memcpy(result.data() + offset, &encoded, sizeof(encoded));
        } else {
            const float encoded = static_cast<float>(value);
            std::memcpy(result.data() + offset, &encoded, sizeof(encoded));
        }
    }
    return result;
}

ChannelDescriptor descriptor(std::string semantic_id, std::uint8_t components,
                             std::uint8_t preferred_bits, std::vector<double> defaults,
                             ChannelClassification classification, BlendingPolicy blending,
                             std::string export_mapping) {
    return {
        std::move(semantic_id),
        components,
        ScalarRepresentation::unsigned_normalized,
        preferred_bits,
        std::move(defaults),
        classification,
        blending,
        std::move(export_mapping),
        true,
    };
}

}  // namespace

std::vector<ChannelDescriptor> metallic_roughness_channels() {
    using Classification = ChannelClassification;
    using Blending = BlendingPolicy;
    return {
        descriptor("pbr.base_color", 3, 8, {0.5, 0.5, 0.5}, Classification::color, Blending::color,
                   "baseColor"),
        descriptor("pbr.opacity", 1, 8, {1.0}, Classification::data, Blending::scalar, "opacity"),
        descriptor("pbr.roughness", 1, 8, {0.5}, Classification::data, Blending::scalar,
                   "roughness"),
        descriptor("pbr.metallic", 1, 8, {0.0}, Classification::data, Blending::scalar, "metallic"),
        descriptor("pbr.normal", 3, 16, {0.5, 0.5, 1.0}, Classification::data,
                   Blending::normal_vector, "normal"),
        descriptor("pbr.height", 1, 16, {0.0}, Classification::data, Blending::additive, "height"),
        descriptor("pbr.occlusion", 1, 8, {1.0}, Classification::data, Blending::scalar,
                   "occlusion"),
        descriptor("pbr.emission", 3, 8, {0.0, 0.0, 0.0}, Classification::color, Blending::color,
                   "emissive"),
        descriptor("pbr.subsurface", 1, 8, {0.0}, Classification::data, Blending::scalar,
                   "subsurface"),
    };
}

TextureChannels::TextureChannels(std::uint32_t width, std::uint32_t height,
                                 std::uint8_t default_bit_depth,
                                 std::span<const ChannelDescriptor> descriptors)
    : width_(width), height_(height), default_bit_depth_(default_bit_depth) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("channel storage dimensions must be non-zero");
    }
    if (!valid_bit_depth(default_bit_depth)) {
        throw std::invalid_argument("texture-set bit depth must be 8, 16 or 32");
    }
    for (const ChannelDescriptor& value : descriptors) {
        register_descriptor(value);
    }
}

void TextureChannels::register_descriptor(ChannelDescriptor value) {
    validate_descriptor(value);
    const std::string id = value.semantic_id;
    const auto [unused, inserted] = channels_.emplace(id, ChannelEntry{std::move(value), nullptr});
    static_cast<void>(unused);
    if (!inserted) {
        throw std::invalid_argument("channel semantic identifier is already registered: " + id);
    }
}

const ChannelDescriptor& TextureChannels::descriptor(std::string_view semantic_id) const {
    return entry(semantic_id).descriptor;
}

std::vector<std::string> TextureChannels::semantic_ids() const {
    std::vector<std::string> result;
    result.reserve(channels_.size());
    for (const auto& [semantic_id, unused] : channels_) {
        static_cast<void>(unused);
        result.push_back(semantic_id);
    }
    return result;
}

void TextureChannels::enable(std::string_view semantic_id,
                             std::optional<std::uint8_t> bit_depth_override) {
    ChannelEntry& channel = entry(semantic_id);
    if (channel.pixels) {
        return;
    }
    const std::uint8_t bit_depth = bit_depth_override.value_or(default_bit_depth_);
    const image::ChannelType type =
        channel_type(channel.descriptor.scalar_representation, bit_depth);
    const std::vector<std::byte> clear_pixel = encode_clear_pixel(channel.descriptor, bit_depth);
    channel.pixels = std::make_unique<image::TiledImage>(
        width_, height_, image::PixelFormat{type, channel.descriptor.component_count},
        image::default_tile_size, clear_pixel);
}

void TextureChannels::disable(std::string_view semantic_id) noexcept {
    const auto found = channels_.find(semantic_id);
    if (found != channels_.end()) {
        found->second.pixels.reset();
    }
}

bool TextureChannels::is_enabled(std::string_view semantic_id) const noexcept {
    const auto found = channels_.find(semantic_id);
    return found != channels_.end() && static_cast<bool>(found->second.pixels);
}

image::TiledImage& TextureChannels::pixels(std::string_view semantic_id) {
    ChannelEntry& channel = entry(semantic_id);
    if (!channel.pixels) {
        throw std::logic_error("channel is disabled: " + std::string(semantic_id));
    }
    return *channel.pixels;
}

const image::TiledImage& TextureChannels::pixels(std::string_view semantic_id) const {
    const ChannelEntry& channel = entry(semantic_id);
    if (!channel.pixels) {
        throw std::logic_error("channel is disabled: " + std::string(semantic_id));
    }
    return *channel.pixels;
}

ChannelRevision TextureChannels::channel_revision(std::string_view semantic_id) const {
    return pixels(semantic_id).revision();
}

ChannelRevisionCursor TextureChannels::channel_revision_cursor(std::string_view semantic_id) const {
    return pixels(semantic_id).revision_cursor();
}

TileRevision TextureChannels::tile_revision(std::string_view semantic_id,
                                            image::TileCoordinate tile) const {
    return pixels(semantic_id).tile_revision(tile);
}

ChannelRevisionCursor TextureChannels::reset_revision_history(std::string_view semantic_id) {
    return pixels(semantic_id).reset_revision_history();
}

std::size_t TextureChannels::enabled_channel_count() const noexcept {
    return static_cast<std::size_t>(
        std::count_if(channels_.begin(), channels_.end(),
                      [](const auto& item) { return item.second.pixels != nullptr; }));
}

std::size_t TextureChannels::resident_pixel_bytes() const noexcept {
    std::size_t result = 0;
    for (const auto& [unused, channel] : channels_) {
        static_cast<void>(unused);
        if (channel.pixels) {
            result += channel.pixels->resident_pixel_bytes();
        }
    }
    return result;
}

TextureChannels::ChannelEntry& TextureChannels::entry(std::string_view semantic_id) {
    const auto found = channels_.find(semantic_id);
    if (found == channels_.end()) {
        throw std::out_of_range("channel semantic identifier is not registered: " +
                                std::string(semantic_id));
    }
    return found->second;
}

const TextureChannels::ChannelEntry& TextureChannels::entry(std::string_view semantic_id) const {
    const auto found = channels_.find(semantic_id);
    if (found == channels_.end()) {
        throw std::out_of_range("channel semantic identifier is not registered: " +
                                std::string(semantic_id));
    }
    return found->second;
}

}  // namespace ctex::doc
