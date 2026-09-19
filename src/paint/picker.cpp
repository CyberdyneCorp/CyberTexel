#include <algorithm>
#include <cmath>
#include <ctex/paint/picker.hpp>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

namespace ctex::paint {
namespace {

bool finite(Vec2d value) { return std::isfinite(value.x) && std::isfinite(value.y); }

bool finite(graph::ColourValue value) {
    return std::isfinite(value.r) && std::isfinite(value.g) && std::isfinite(value.b) &&
           std::isfinite(value.a);
}

std::size_t checked_area(const PickerTextureView& view) {
    const Vec2d tile_end{view.tile_origin.x + 1.0, view.tile_origin.y + 1.0};
    if (view.texture_set_id.empty() || !finite(view.tile_origin) || !finite(tile_end) ||
        tile_end.x <= view.tile_origin.x || tile_end.y <= view.tile_origin.y || view.width == 0 ||
        view.height == 0 ||
        static_cast<std::size_t>(view.width) >
            std::numeric_limits<std::size_t>::max() / view.height) {
        throw std::invalid_argument("picker texture view identity or dimensions are invalid");
    }
    return static_cast<std::size_t>(view.width) * view.height;
}

void validate_view(const PickerTextureView& view) {
    const std::size_t count = checked_area(view);
    if (view.enabled_channels.empty()) {
        throw std::invalid_argument("picker texture view requires enabled channels");
    }
    std::set<std::string_view> identifiers;
    for (const PaintToolChannelRaster& channel : view.enabled_channels) {
        if (channel.semantic_id.empty() || channel.component_count == 0 ||
            channel.component_count > 4 || channel.pixels.size() != count ||
            !identifiers.insert(channel.semantic_id).second ||
            !std::all_of(channel.pixels.begin(), channel.pixels.end(),
                         [](graph::ColourValue value) { return finite(value); })) {
            throw std::invalid_argument("picker enabled channels are invalid or duplicated");
        }
    }
    if (view.material_provenance &&
        (view.material_provenance->identities.size() != count ||
         std::any_of(view.material_provenance->identities.begin(),
                     view.material_provenance->identities.end(),
                     [](std::string_view identity) { return identity.empty(); }))) {
        throw std::invalid_argument("picker material provenance is invalid");
    }
}

bool contains(const PickerTextureView& view, std::string_view texture_set_id, Vec2d uv) {
    return view.texture_set_id == texture_set_id && uv.x >= view.tile_origin.x &&
           uv.x < view.tile_origin.x + 1.0 && uv.y >= view.tile_origin.y &&
           uv.y < view.tile_origin.y + 1.0;
}

const PickerTextureView& matching_view(std::span<const PickerTextureView> views,
                                       std::string_view texture_set_id, Vec2d uv) {
    const PickerTextureView* result = nullptr;
    for (const PickerTextureView& view : views) {
        validate_view(view);
        if (!contains(view, texture_set_id, uv)) {
            continue;
        }
        if (result != nullptr) {
            throw std::invalid_argument("picker has overlapping views for the surface hit");
        }
        result = &view;
    }
    if (result == nullptr) {
        throw std::invalid_argument("picker has no texture view for the surface hit");
    }
    return *result;
}

std::size_t texel_index(const PickerTextureView& view, Vec2d uv) {
    const double u = uv.x - view.tile_origin.x;
    const double v = uv.y - view.tile_origin.y;
    const auto x = static_cast<std::uint32_t>(std::floor(u * view.width));
    const auto y =
        std::min(static_cast<std::uint32_t>(std::floor((1.0 - v) * view.height)), view.height - 1);
    return static_cast<std::size_t>(y) * view.width + x;
}

std::vector<PickedChannelValue> channel_values(const PickerTextureView& view, std::size_t texel) {
    std::vector<PickedChannelValue> result;
    result.reserve(view.enabled_channels.size());
    for (const PaintToolChannelRaster& channel : view.enabled_channels) {
        result.push_back({.semantic_id = channel.semantic_id,
                          .component_count = channel.component_count,
                          .value = channel.pixels[texel]});
    }
    return result;
}

}  // namespace

PickerResult pick_enabled_channels(const pick::HitRecord& hit,
                                   std::span<const PickerTextureView> texture_views) {
    const Vec2d uv{hit.uv.x, hit.uv.y};
    if (hit.texture_set_id.empty() || !finite(uv) || texture_views.empty()) {
        throw std::invalid_argument("picker surface hit or texture views are invalid");
    }
    const PickerTextureView& view = matching_view(texture_views, hit.texture_set_id, uv);
    const std::size_t texel = texel_index(view, uv);
    std::optional<std::string> material;
    if (view.material_provenance) {
        material = std::string(view.material_provenance->identities[texel]);
    }
    return {.texture_set_id = std::string(view.texture_set_id),
            .tile_origin = view.tile_origin,
            .uv = uv,
            .texel = texel,
            .channels = channel_values(view, texel),
            .material_identity = std::move(material)};
}

}  // namespace ctex::paint
