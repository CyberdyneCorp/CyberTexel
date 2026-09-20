#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctex/doc/channels.hpp>
#include <exception>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using ctex::doc::BlendingPolicy;
using ctex::doc::ChannelClassification;
using ctex::doc::ChannelDescriptor;
using ctex::doc::ScalarRepresentation;
using ctex::doc::TextureChannels;
using ctex::image::ChannelType;
using ctex::image::PixelFormat;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

std::uint16_t uint16_from(std::span<const std::byte> bytes) {
    std::uint16_t value = 0;
    std::memcpy(&value, bytes.data(), sizeof(value));
    return value;
}

bool built_in_preset_is_complete() {
    const auto descriptors = ctex::doc::metallic_roughness_channels();
    std::vector<std::string_view> ids;
    ids.reserve(descriptors.size());
    for (const ChannelDescriptor& descriptor : descriptors) {
        ids.push_back(descriptor.semantic_id);
    }
    const std::array required{
        "pbr.base_color", "pbr.opacity",   "pbr.roughness", "pbr.metallic",   "pbr.normal",
        "pbr.height",     "pbr.occlusion", "pbr.emission",  "pbr.subsurface",
    };
    return expect(descriptors.size() == required.size(),
                  "built-in preset does not have nine channels") &&
           expect(std::all_of(required.begin(), required.end(),
                              [&](std::string_view required_id) {
                                  return std::find(ids.begin(), ids.end(), required_id) !=
                                         ids.end();
                              }),
                  "built-in preset is missing a semantic identifier");
}

bool disabled_channels_allocate_no_storage() {
    const auto descriptors = ctex::doc::metallic_roughness_channels();
    TextureChannels channels(1024, 1024, 8, descriptors);
    channels.enable("pbr.base_color");
    channels.enable("pbr.opacity");
    return expect(channels.enabled_channel_count() == 2, "wrong enabled channel count") &&
           expect(!channels.is_enabled("pbr.roughness"), "disabled roughness has storage") &&
           expect(!channels.is_enabled("pbr.normal"), "disabled normal has storage") &&
           expect(channels.resident_pixel_bytes() == 0,
                  "constant enabled channels allocated tiles");
}

bool enabling_uses_default_without_disturbing_existing_channels() {
    const auto descriptors = ctex::doc::metallic_roughness_channels();
    TextureChannels channels(8, 8, 8, descriptors);
    channels.enable("pbr.base_color");
    const std::array painted{std::byte{10}, std::byte{20}, std::byte{30}};
    channels.pixels("pbr.base_color").write_pixel(2, 3, painted);
    channels.enable("pbr.height", 16);
    const auto base = channels.pixels("pbr.base_color").read_pixel(2, 3);
    const auto height = channels.pixels("pbr.height").read_pixel(2, 3);
    return expect(channels.pixels("pbr.base_color").format() ==
                      PixelFormat{ChannelType::uint8_unorm, 3},
                  "base color did not keep 8-bit precision") &&
           expect(
               channels.pixels("pbr.height").format() == PixelFormat{ChannelType::uint16_unorm, 1},
               "height override did not use 16-bit precision") &&
           expect(std::equal(base.begin(), base.end(), painted.begin()),
                  "enabling height changed base color") &&
           expect(uint16_from(height) == 0, "height did not use its documented default");
}

bool custom_channel_is_preserved_and_independent() {
    TextureChannels channels(4, 4, 8);
    channels.register_descriptor({
        .semantic_id = "openpbr.coat_weight",
        .component_count = 1,
        .scalar_representation = ScalarRepresentation::unsigned_normalized,
        .preferred_bit_depth = 8,
        .default_value = {0.25},
        .classification = ChannelClassification::data,
        .blending_policy = BlendingPolicy::scalar,
        .export_mapping = "coatWeight",
        .evaluable = false,
    });
    channels.enable("openpbr.coat_weight");
    const auto& descriptor = channels.descriptor("openpbr.coat_weight");
    const auto pixel = channels.pixels("openpbr.coat_weight").read_pixel(0, 0);
    return expect(descriptor.semantic_id == "openpbr.coat_weight", "custom semantic ID changed") &&
           expect(!descriptor.evaluable, "unknown semantic was silently treated as evaluable") &&
           expect(pixel[0] == std::byte{64}, "custom channel default was not initialized") &&
           expect(channels.semantic_ids() == std::vector<std::string>{"openpbr.coat_weight"},
                  "custom descriptor was not enumerable");
}

bool copied_channels_own_independent_image_state() {
    TextureChannels original(64, 64, 8, ctex::doc::metallic_roughness_channels());
    original.enable("pbr.base_color");
    const std::array before{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array after{std::byte{4}, std::byte{5}, std::byte{6}};
    original.pixels("pbr.base_color").write_pixel(0, 0, before);
    const void* shared_identity =
        original.pixels("pbr.base_color").snapshot_tile_storage({0, 0}).identity();
    TextureChannels copy = original;
    const bool initially_shared =
        copy.pixels("pbr.base_color").snapshot_tile_storage({0, 0}).identity() == shared_identity;
    copy.pixels("pbr.base_color").write_pixel(0, 0, after);
    return expect(initially_shared, "channel copy eagerly duplicated immutable tile storage") &&
           expect(original.pixels("pbr.base_color").snapshot_tile_storage({0, 0}).identity() ==
                          shared_identity &&
                      std::equal(before.begin(), before.end(),
                                 original.pixels("pbr.base_color").read_pixel(0, 0).begin()),
                  "writing a channel copy changed the original image state") &&
           expect(copy.pixels("pbr.base_color").snapshot_tile_storage({0, 0}).identity() !=
                          shared_identity &&
                      std::equal(after.begin(), after.end(),
                                 copy.pixels("pbr.base_color").read_pixel(0, 0).begin()),
                  "channel copy did not detach its tile through copy-on-write");
}

}  // namespace

int main() {
    return built_in_preset_is_complete() && disabled_channels_allocate_no_storage() &&
                   enabling_uses_default_without_disturbing_existing_channels() &&
                   custom_channel_is_preserved_and_independent() &&
                   copied_channels_own_independent_image_state()
               ? 0
               : 1;
}
