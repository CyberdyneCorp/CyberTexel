#ifndef CTEX_DOC_CHANNELS_HPP
#define CTEX_DOC_CHANNELS_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/tiled_image.hpp>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::doc {

using ChannelRevision = image::Revision;
using TileRevision = image::Revision;

enum class ScalarRepresentation { unsigned_normalized, floating_point };
enum class ChannelClassification { color, data };
enum class BlendingPolicy { color, scalar, normal_vector, additive };

struct ChannelDescriptor {
    std::string semantic_id;
    std::uint8_t component_count;
    ScalarRepresentation scalar_representation;
    std::uint8_t preferred_bit_depth;
    std::vector<double> default_value;
    ChannelClassification classification;
    BlendingPolicy blending_policy;
    std::string export_mapping;
    bool evaluable = true;
};

[[nodiscard]] std::vector<ChannelDescriptor> metallic_roughness_channels();

class TextureChannels {
public:
    TextureChannels(std::uint32_t width, std::uint32_t height, std::uint8_t default_bit_depth,
                    std::span<const ChannelDescriptor> descriptors = {});

    void register_descriptor(ChannelDescriptor descriptor);
    [[nodiscard]] const ChannelDescriptor& descriptor(std::string_view semantic_id) const;
    [[nodiscard]] std::vector<std::string> semantic_ids() const;

    void enable(std::string_view semantic_id,
                std::optional<std::uint8_t> bit_depth_override = std::nullopt);
    void disable(std::string_view semantic_id) noexcept;
    [[nodiscard]] bool is_enabled(std::string_view semantic_id) const noexcept;
    [[nodiscard]] image::TiledImage& pixels(std::string_view semantic_id);
    [[nodiscard]] const image::TiledImage& pixels(std::string_view semantic_id) const;
    [[nodiscard]] ChannelRevision channel_revision(std::string_view semantic_id) const;
    [[nodiscard]] TileRevision tile_revision(std::string_view semantic_id,
                                             image::TileCoordinate tile) const;

    [[nodiscard]] std::size_t enabled_channel_count() const noexcept;
    [[nodiscard]] std::size_t resident_pixel_bytes() const noexcept;
    [[nodiscard]] std::uint8_t default_bit_depth() const noexcept { return default_bit_depth_; }

private:
    struct ChannelEntry {
        ChannelDescriptor descriptor;
        std::unique_ptr<image::TiledImage> pixels;
    };

    [[nodiscard]] ChannelEntry& entry(std::string_view semantic_id);
    [[nodiscard]] const ChannelEntry& entry(std::string_view semantic_id) const;

    std::uint32_t width_;
    std::uint32_t height_;
    std::uint8_t default_bit_depth_;
    std::map<std::string, ChannelEntry, std::less<>> channels_;
};

}  // namespace ctex::doc

#endif
