#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctex/maps/mesh_maps.hpp>
#include <stdexcept>
#include <utility>

namespace ctex::maps {
namespace {

struct ChannelCountRange {
    std::uint8_t minimum;
    std::uint8_t maximum;
};

ChannelCountRange channel_count_range(MeshMapKind kind) {
    switch (kind) {
        case MeshMapKind::tangent_space_normal:
        case MeshMapKind::object_space_normal:
        case MeshMapKind::world_space_direction:
        case MeshMapKind::position:
        case MeshMapKind::bent_normal:
            return {3, 3};
        case MeshMapKind::vertex_colour:
            return {3, 4};
        case MeshMapKind::ambient_occlusion:
        case MeshMapKind::curvature:
        case MeshMapKind::thickness:
        case MeshMapKind::height:
        case MeshMapKind::material_id:
        case MeshMapKind::object_id:
        case MeshMapKind::uv_density:
            return {1, 1};
    }
    throw std::invalid_argument("mesh map kind is invalid");
}

bool is_identifier_map(MeshMapKind kind) {
    return kind == MeshMapKind::material_id || kind == MeshMapKind::object_id;
}

void validate_descriptor(const MeshMapDescriptor& descriptor, std::string_view texture_set_id,
                         std::string_view uv_set) {
    const std::string_view name = mesh_map_name(descriptor.kind);
    if (descriptor.texture_set_id != texture_set_id) {
        throw std::invalid_argument("mesh map '" + std::string(name) + "' names texture set '" +
                                    descriptor.texture_set_id + "', expected '" +
                                    std::string(texture_set_id) + "'");
    }
    if (descriptor.uv_set != uv_set) {
        throw std::invalid_argument("mesh map '" + std::string(name) + "' names UV set '" +
                                    descriptor.uv_set + "', expected '" + std::string(uv_set) +
                                    "'");
    }
    if (!descriptor.pixels) {
        throw std::invalid_argument("mesh map '" + std::string(name) + "' has no pixels");
    }
    const ChannelCountRange channels = channel_count_range(descriptor.kind);
    const std::uint8_t actual = descriptor.pixels->format().channel_count;
    if (actual < channels.minimum || actual > channels.maximum) {
        throw std::invalid_argument("mesh map '" + std::string(name) +
                                    "' has an incompatible channel count");
    }
}

double decode_component(std::span<const std::byte> pixel, image::PixelFormat format,
                        std::size_t component, std::string_view map_name) {
    const std::size_t offset = component * format.bytes_per_channel();
    switch (format.channel_type) {
        case image::ChannelType::uint8_unorm:
            return std::to_integer<std::uint8_t>(pixel[offset]) / 255.0;
        case image::ChannelType::uint16_unorm: {
            std::uint16_t value{};
            std::memcpy(&value, pixel.data() + offset, sizeof(value));
            return value / 65535.0;
        }
        case image::ChannelType::float32: {
            float value{};
            std::memcpy(&value, pixel.data() + offset, sizeof(value));
            if (!std::isfinite(value)) {
                throw std::runtime_error("mesh map '" + std::string(map_name) +
                                         "' contains a non-finite component");
            }
            return value;
        }
    }
    throw std::invalid_argument("mesh map pixel format is invalid");
}

MeshMapSample texel(const MeshMapDescriptor& descriptor, std::uint32_t x, std::uint32_t y) {
    const image::PixelFormat format = descriptor.pixels->format();
    const std::span<const std::byte> pixel = descriptor.pixels->read_pixel(x, y);
    MeshMapSample result{.component_count = format.channel_count};
    for (std::size_t component = 0; component < format.channel_count; ++component) {
        result.values[component] =
            decode_component(pixel, format, component, mesh_map_name(descriptor.kind));
    }
    return result;
}

MeshMapSample nearest_sample(const MeshMapDescriptor& descriptor, double x, double y) {
    const auto texel_x = static_cast<std::uint32_t>(std::floor(x + 0.5));
    const auto texel_y = static_cast<std::uint32_t>(std::floor(y + 0.5));
    return texel(descriptor, texel_x, texel_y);
}

MeshMapSample bilinear_sample(const MeshMapDescriptor& descriptor, double x, double y) {
    const std::uint32_t x0 = static_cast<std::uint32_t>(std::floor(x));
    const std::uint32_t y0 = static_cast<std::uint32_t>(std::floor(y));
    const std::uint32_t x1 = std::min(x0 + 1, descriptor.pixels->width() - 1);
    const std::uint32_t y1 = std::min(y0 + 1, descriptor.pixels->height() - 1);
    const double x_fraction = x - x0;
    const double y_fraction = y - y0;
    const MeshMapSample top_left = texel(descriptor, x0, y0);
    const MeshMapSample top_right = texel(descriptor, x1, y0);
    const MeshMapSample bottom_left = texel(descriptor, x0, y1);
    const MeshMapSample bottom_right = texel(descriptor, x1, y1);
    MeshMapSample result{.component_count = top_left.component_count};
    for (std::size_t component = 0; component < result.component_count; ++component) {
        const double top =
            std::lerp(top_left.values[component], top_right.values[component], x_fraction);
        const double bottom =
            std::lerp(bottom_left.values[component], bottom_right.values[component], x_fraction);
        result.values[component] = std::lerp(top, bottom, y_fraction);
    }
    return result;
}

}  // namespace

std::string_view mesh_map_name(MeshMapKind kind) {
    switch (kind) {
        case MeshMapKind::tangent_space_normal:
            return "tangent-space-normal";
        case MeshMapKind::object_space_normal:
            return "object-space-normal";
        case MeshMapKind::world_space_direction:
            return "world-space-direction";
        case MeshMapKind::ambient_occlusion:
            return "ambient-occlusion";
        case MeshMapKind::curvature:
            return "curvature";
        case MeshMapKind::thickness:
            return "thickness";
        case MeshMapKind::position:
            return "position";
        case MeshMapKind::height:
            return "height";
        case MeshMapKind::bent_normal:
            return "bent-normal";
        case MeshMapKind::material_id:
            return "material-id";
        case MeshMapKind::object_id:
            return "object-id";
        case MeshMapKind::uv_density:
            return "uv-density";
        case MeshMapKind::vertex_colour:
            return "vertex-colour";
    }
    throw std::invalid_argument("mesh map kind is invalid");
}

MeshMapSet::MeshMapSet(const doc::TextureSet& texture_set)
    : texture_set_id_(texture_set.id()),
      uv_set_(texture_set.descriptor().uv_set),
      texture_set_width_(texture_set.descriptor().width),
      texture_set_height_(texture_set.descriptor().height) {}

MeshMapBindResult MeshMapSet::bind(MeshMapDescriptor descriptor) {
    validate_descriptor(descriptor, texture_set_id_, uv_set_);
    const bool mismatched = descriptor.pixels->width() != texture_set_width_ ||
                            descriptor.pixels->height() != texture_set_height_;
    std::optional<MapResolutionMismatch> mismatch;
    if (mismatched) {
        mismatch = MapResolutionMismatch{.kind = descriptor.kind,
                                         .texture_set_id = texture_set_id_,
                                         .map_width = descriptor.pixels->width(),
                                         .map_height = descriptor.pixels->height(),
                                         .texture_set_width = texture_set_width_,
                                         .texture_set_height = texture_set_height_};
    }
    const MeshMapKind kind = descriptor.kind;
    const bool replaced = maps_.contains(kind);
    maps_.insert_or_assign(kind, std::move(descriptor));
    return {.replaced_existing = replaced, .resolution_mismatch = std::move(mismatch)};
}

bool MeshMapSet::contains(MeshMapKind kind) const noexcept { return maps_.contains(kind); }

const MeshMapDescriptor& MeshMapSet::map(MeshMapKind kind) const {
    const auto found = maps_.find(kind);
    if (found == maps_.end()) {
        throw std::out_of_range("mesh map '" + std::string(mesh_map_name(kind)) +
                                "' is not bound to texture set '" + texture_set_id_ + "'");
    }
    return found->second;
}

std::vector<MeshMapKind> MeshMapSet::bound_maps() const {
    std::vector<MeshMapKind> result;
    result.reserve(maps_.size());
    for (const auto& [kind, descriptor] : maps_) {
        static_cast<void>(descriptor);
        result.push_back(kind);
    }
    return result;
}

MeshMapSample MeshMapSet::sample(MeshMapKind kind, double u, double v) const {
    if (!std::isfinite(u) || !std::isfinite(v) || u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0) {
        throw std::invalid_argument("mesh map sample coordinates must be normalized and finite");
    }
    const MeshMapDescriptor& descriptor = map(kind);
    const double x = u * static_cast<double>(descriptor.pixels->width() - 1);
    const double y = (1.0 - v) * static_cast<double>(descriptor.pixels->height() - 1);
    return is_identifier_map(kind) ? nearest_sample(descriptor, x, y)
                                   : bilinear_sample(descriptor, x, y);
}

}  // namespace ctex::maps
