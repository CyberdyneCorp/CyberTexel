#ifndef CTEX_PAINT_PICKER_HPP
#define CTEX_PAINT_PICKER_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/brush.hpp>
#include <ctex/pick/hit.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

struct MaterialProvenanceView {
    std::span<const std::string_view> identities;
};

struct PickerTextureView {
    std::string_view texture_set_id;
    Vec2d tile_origin;
    std::uint32_t width{};
    std::uint32_t height{};
    std::span<const PaintToolChannelRaster> enabled_channels;
    std::optional<MaterialProvenanceView> material_provenance;
};

struct PickedChannelValue {
    std::string semantic_id;
    std::uint8_t component_count{};
    graph::ColourValue value;
    friend bool operator==(const PickedChannelValue&, const PickedChannelValue&) = default;
};

struct PickerResult {
    std::string texture_set_id;
    Vec2d tile_origin;
    Vec2d uv;
    std::size_t texel{};
    std::vector<PickedChannelValue> channels;
    std::optional<std::string> material_identity;
};

[[nodiscard]] PickerResult pick_enabled_channels(const pick::HitRecord& hit,
                                                 std::span<const PickerTextureView> texture_views);

}  // namespace ctex::paint

#endif
