#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctex/maps/mesh_maps.hpp>
#include <limits>
#include <set>
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

void validate_tangent_frame(const mesh::TangentFrameDescriptor& frame, std::string_view uv_set) {
    static_cast<void>(mesh::tangent_basis_algorithm_name(frame.algorithm));
    if (frame.algorithm_version == 0 || std::string_view(frame.uv_set) != uv_set) {
        throw std::invalid_argument("tangent frame requires a version and the texture-set UV set");
    }
    if (frame.normal_orientation != mesh::NormalOrientation::vertex_normals &&
        frame.normal_orientation != mesh::NormalOrientation::inverted_vertex_normals) {
        throw std::invalid_argument("tangent frame normal orientation is invalid");
    }
    if (frame.coordinate_handedness != mesh::CoordinateSystemHandedness::right_handed &&
        frame.coordinate_handedness != mesh::CoordinateSystemHandedness::left_handed) {
        throw std::invalid_argument("tangent frame coordinate handedness is invalid");
    }
    if (frame.uv_v_axis != mesh::UvVAxis::upward && frame.uv_v_axis != mesh::UvVAxis::downward) {
        throw std::invalid_argument("tangent frame UV convention is invalid");
    }
    if (frame.handedness_encoding != mesh::TangentHandednessEncoding::tangent_w_sign) {
        throw std::invalid_argument("tangent frame handedness encoding is invalid");
    }
}

void validate_tangent_binding(
    const MeshMapDescriptor& descriptor,
    const std::optional<mesh::TangentFrameDescriptor>& target_tangent_frame,
    std::string_view uv_set) {
    if (descriptor.kind != MeshMapKind::tangent_space_normal) {
        if (descriptor.tangent_frame) {
            throw std::invalid_argument("only tangent-space normal maps declare a tangent frame");
        }
        return;
    }
    if (!descriptor.tangent_frame) {
        throw std::invalid_argument("tangent-space normal map requires a tangent-frame descriptor");
    }
    validate_tangent_frame(*descriptor.tangent_frame, uv_set);
    if (!target_tangent_frame) {
        throw std::invalid_argument(
            "tangent-space normal map cannot be validated without the mesh tangent frame");
    }
    if (!mesh::tangent_frames_compatible(*descriptor.tangent_frame, *target_tangent_frame)) {
        throw std::invalid_argument(
            "tangent-space normal map basis is incompatible with the mesh tangent frame");
    }
}

void validate_descriptor(const MeshMapDescriptor& descriptor, std::string_view texture_set_id,
                         std::string_view uv_set,
                         const std::optional<mesh::TangentFrameDescriptor>& tangent_frame) {
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
    if (descriptor.mesh_revision == 0) {
        throw std::invalid_argument("mesh map '" + std::string(name) +
                                    "' has no source mesh revision");
    }
    if (mesh_map_uses_normal_convention(descriptor.kind) && !descriptor.normal_convention) {
        throw std::invalid_argument("mesh map '" + std::string(name) +
                                    "' requires a normal-map convention");
    }
    if (!mesh_map_uses_normal_convention(descriptor.kind) && descriptor.normal_convention) {
        throw std::invalid_argument("non-normal mesh map '" + std::string(name) +
                                    "' declares a normal-map convention");
    }
    if (descriptor.normal_convention) {
        static_cast<void>(normal_map_convention_name(*descriptor.normal_convention));
    }
    validate_tangent_binding(descriptor, tangent_frame, uv_set);
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

MeshMapSample convert_normal_convention(const MeshMapDescriptor& descriptor, MeshMapSample sample) {
    if (descriptor.normal_convention == NormalMapConvention::direct_x) {
        sample.values[1] = 1.0 - sample.values[1];
    }
    return sample;
}

std::string map_names(std::span<const MeshMapKind> maps) {
    std::string names;
    for (const MeshMapKind kind : maps) {
        if (!names.empty()) {
            names += ", ";
        }
        names += mesh_map_name(kind);
    }
    return names;
}

std::optional<MeshMapStaleness> staleness(const MeshMapDescriptor& descriptor,
                                          mesh::MeshRevision current_revision) {
    if (descriptor.mesh_revision == current_revision) {
        return std::nullopt;
    }
    return MeshMapStaleness{.kind = descriptor.kind,
                            .produced_mesh_revision = descriptor.mesh_revision,
                            .current_mesh_revision = current_revision};
}

std::string requirement_message(std::string_view consumer, std::string_view texture_set_id,
                                std::span<const MeshMapKind> missing,
                                std::span<const MeshMapStaleness> stale) {
    std::string message = "mesh-map consumer '" + std::string(consumer) + "' for texture set '" +
                          std::string(texture_set_id) + "'";
    if (!missing.empty()) {
        message += " requires missing map";
        message += missing.size() == 1 ? " '" : "s '";
        message += map_names(missing) + "'";
    }
    if (!stale.empty()) {
        message += missing.empty() ? " has stale map" : "; stale map";
        message += stale.size() == 1 ? " " : "s ";
        for (std::size_t index = 0; index < stale.size(); ++index) {
            if (index != 0) {
                message += "; ";
            }
            message += "'" + std::string(mesh_map_name(stale[index].kind)) +
                       "' produced from mesh revision " +
                       std::to_string(stale[index].produced_mesh_revision) + ", current revision " +
                       std::to_string(stale[index].current_mesh_revision);
        }
    }
    return message;
}

std::size_t resident_pixel_bytes(const std::map<MeshMapKind, MeshMapDescriptor>& maps) {
    std::size_t total = 0;
    for (const auto& [kind, descriptor] : maps) {
        static_cast<void>(kind);
        const std::size_t bytes = descriptor.pixels->resident_pixel_bytes();
        if (bytes > std::numeric_limits<std::size_t>::max() - total) {
            throw std::overflow_error("mesh-map resident pixel byte count overflows");
        }
        total += bytes;
    }
    return total;
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

std::string_view normal_map_convention_name(NormalMapConvention convention) {
    switch (convention) {
        case NormalMapConvention::open_gl:
            return "OpenGL";
        case NormalMapConvention::direct_x:
            return "DirectX";
    }
    throw std::invalid_argument("normal-map convention is invalid");
}

bool mesh_map_uses_normal_convention(MeshMapKind kind) {
    switch (kind) {
        case MeshMapKind::tangent_space_normal:
        case MeshMapKind::object_space_normal:
        case MeshMapKind::bent_normal:
            return true;
        case MeshMapKind::world_space_direction:
        case MeshMapKind::ambient_occlusion:
        case MeshMapKind::curvature:
        case MeshMapKind::thickness:
        case MeshMapKind::position:
        case MeshMapKind::height:
        case MeshMapKind::material_id:
        case MeshMapKind::object_id:
        case MeshMapKind::uv_density:
        case MeshMapKind::vertex_colour:
            return false;
    }
    throw std::invalid_argument("mesh map kind is invalid");
}

MissingMeshMapsError::MissingMeshMapsError(MeshMapRequirementReport report)
    : std::out_of_range(report.message), report_(std::move(report)) {}

MeshMapSet::MeshMapSet(const doc::TextureSet& texture_set, mesh::MeshRevision mesh_revision,
                       std::optional<mesh::TangentFrameDescriptor> tangent_frame)
    : texture_set_id_(texture_set.id()),
      uv_set_(texture_set.descriptor().uv_set),
      texture_set_width_(texture_set.descriptor().width),
      texture_set_height_(texture_set.descriptor().height),
      mesh_revision_(mesh_revision),
      tangent_frame_(std::move(tangent_frame)),
      memory_account_(texture_set.create_memory_account(doc::TextureSetMemoryCategory::mesh_maps)) {
    if (mesh_revision_ == 0) {
        throw std::invalid_argument("mesh map set requires a source mesh revision");
    }
    if (tangent_frame_) {
        validate_tangent_frame(*tangent_frame_, uv_set_);
    }
}

MeshMapSet::MeshMapSet(const doc::TextureSet& texture_set, const mesh::MeshBinding& mesh)
    : MeshMapSet(texture_set, mesh.revision(),
                 std::string_view(mesh.view().tangent_frames().descriptor.uv_set) ==
                         texture_set.descriptor().uv_set
                     ? std::optional(mesh.view().tangent_frames().descriptor)
                     : std::nullopt) {}

MeshMapBindResult MeshMapSet::bind(MeshMapDescriptor descriptor) {
    validate_descriptor(descriptor, texture_set_id_, uv_set_, tangent_frame_);
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
    const auto stale = staleness(descriptor, mesh_revision_);
    auto next_maps = maps_;
    next_maps.insert_or_assign(kind, std::move(descriptor));
    memory_account_.set_resident_bytes(resident_pixel_bytes(next_maps));
    maps_.swap(next_maps);
    return {.replaced_existing = replaced,
            .resolution_mismatch = std::move(mismatch),
            .staleness = stale};
}

bool MeshMapSet::contains(MeshMapKind kind) const noexcept { return maps_.contains(kind); }

const MeshMapDescriptor& MeshMapSet::map(MeshMapKind kind) const {
    const auto found = maps_.find(kind);
    if (found == maps_.end()) {
        const std::array required{kind};
        throw MissingMeshMapsError(check_required_maps("direct mesh-map read", required));
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

MeshMapRequirementReport MeshMapSet::check_required_maps(
    std::string_view consumer, std::span<const MeshMapKind> required) const {
    if (consumer.empty()) {
        throw std::invalid_argument("mesh-map consumer identity must not be empty");
    }
    std::set<MeshMapKind> missing;
    std::vector<MeshMapStaleness> stale;
    for (const MeshMapKind kind : required) {
        static_cast<void>(mesh_map_name(kind));
        const auto found = maps_.find(kind);
        if (found == maps_.end()) {
            missing.insert(kind);
            continue;
        }
        const auto map_staleness = staleness(found->second, mesh_revision_);
        if (map_staleness &&
            std::ranges::find(stale, kind, &MeshMapStaleness::kind) == stale.end()) {
            stale.push_back(*map_staleness);
        }
    }
    std::ranges::sort(stale, {}, &MeshMapStaleness::kind);
    MeshMapRequirementReport result{.consumer = std::string(consumer),
                                    .texture_set_id = texture_set_id_,
                                    .missing_maps = {missing.begin(), missing.end()},
                                    .stale_maps = std::move(stale),
                                    .message = {}};
    if (!result.satisfied() || !result.stale_maps.empty()) {
        result.message = requirement_message(result.consumer, result.texture_set_id,
                                             result.missing_maps, result.stale_maps);
    }
    return result;
}

MeshMapRequirementReport MeshMapSet::require_maps(std::string_view consumer,
                                                  std::span<const MeshMapKind> required) const {
    MeshMapRequirementReport report = check_required_maps(consumer, required);
    if (!report.satisfied()) {
        throw MissingMeshMapsError(std::move(report));
    }
    return report;
}

std::vector<MeshMapStaleness> MeshMapSet::synchronize_mesh_revision(mesh::MeshRevision revision) {
    if (revision == 0) {
        throw std::invalid_argument("current mesh revision must not be zero");
    }
    mesh_revision_ = revision;
    return stale_maps();
}

std::vector<MeshMapStaleness> MeshMapSet::synchronize_mesh_revision(const mesh::MeshBinding& mesh) {
    const mesh::TangentFrameDescriptor frame = mesh.view().tangent_frames().descriptor;
    tangent_frame_ =
        std::string_view(frame.uv_set) == uv_set_ ? std::optional(std::move(frame)) : std::nullopt;
    return synchronize_mesh_revision(mesh.revision());
}

std::vector<MeshMapStaleness> MeshMapSet::stale_maps() const {
    std::vector<MeshMapStaleness> result;
    for (const auto& [kind, descriptor] : maps_) {
        static_cast<void>(kind);
        if (const auto stale = staleness(descriptor, mesh_revision_)) {
            result.push_back(*stale);
        }
    }
    return result;
}

MeshMapReadResult MeshMapSet::sample(MeshMapKind kind, double u, double v) const {
    if (!std::isfinite(u) || !std::isfinite(v) || u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0) {
        throw std::invalid_argument("mesh map sample coordinates must be normalized and finite");
    }
    const BoundSampler sampler = bind_sampler(kind);
    return {.sample = sampler.at(u, v), .staleness = sampler.staleness()};
}

MeshMapSample MeshMapSet::BoundSampler::at(double u, double v) const {
    if (!std::isfinite(u) || !std::isfinite(v) || u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0) {
        throw std::invalid_argument("mesh map sample coordinates must be normalized and finite");
    }
    const MeshMapDescriptor& descriptor = *descriptor_;
    const double x = u * static_cast<double>(descriptor.pixels->width() - 1);
    const double y = (1.0 - v) * static_cast<double>(descriptor.pixels->height() - 1);
    const MeshMapSample filtered = is_identifier_map(descriptor.kind)
                                       ? nearest_sample(descriptor, x, y)
                                       : bilinear_sample(descriptor, x, y);
    return convert_normal_convention(descriptor, filtered);
}

MeshMapSet::BoundSampler MeshMapSet::bind_sampler(MeshMapKind kind) const {
    const MeshMapDescriptor& descriptor = map(kind);
    validate_tangent_binding(descriptor, tangent_frame_, uv_set_);
    return {descriptor, staleness(descriptor, mesh_revision_)};
}

MeshMapMemoryReport MeshMapSet::memory_report() const {
    MeshMapMemoryReport result{.texture_set_id = texture_set_id_,
                               .maps = {},
                               .resident_pixel_bytes = memory_account_.resident_bytes()};
    result.maps.reserve(maps_.size());
    for (const auto& [kind, descriptor] : maps_) {
        result.maps.push_back({.kind = kind,
                               .width = descriptor.pixels->width(),
                               .height = descriptor.pixels->height(),
                               .resident_pixel_bytes = descriptor.pixels->resident_pixel_bytes()});
    }
    return result;
}

MeshMapReleaseResult MeshMapSet::release_map(MeshMapKind kind) {
    static_cast<void>(mesh_map_name(kind));
    const auto found = maps_.find(kind);
    if (found == maps_.end()) {
        return {};
    }
    const std::size_t before = memory_account_.resident_bytes();
    maps_.erase(found);
    const std::size_t after = resident_pixel_bytes(maps_);
    memory_account_.set_resident_bytes(after);
    return {.released_maps = {kind}, .resident_pixel_bytes_released = before - after};
}

MeshMapReleaseResult MeshMapSet::release_all_maps() {
    MeshMapReleaseResult result{.released_maps = bound_maps(),
                                .resident_pixel_bytes_released = memory_account_.resident_bytes()};
    maps_.clear();
    memory_account_.set_resident_bytes(0);
    return result;
}

MeshMapBindingSnapshot MeshMapSet::snapshot_bindings() const {
    MeshMapBindingSnapshot snapshot{
        .texture_set_id = texture_set_id_, .uv_set = uv_set_, .maps = {}};
    snapshot.maps.reserve(maps_.size());
    for (const auto& [kind, descriptor] : maps_) {
        static_cast<void>(kind);
        snapshot.maps.push_back(descriptor);
    }
    return snapshot;
}

void MeshMapSet::restore_bindings(MeshMapBindingSnapshot snapshot) {
    if (snapshot.texture_set_id != texture_set_id_ || snapshot.uv_set != uv_set_) {
        throw std::invalid_argument(
            "mesh-map snapshot belongs to a different texture set or UV set");
    }
    std::map<MeshMapKind, MeshMapDescriptor> restored;
    for (MeshMapDescriptor& descriptor : snapshot.maps) {
        validate_descriptor(descriptor, texture_set_id_, uv_set_, tangent_frame_);
        const MeshMapKind kind = descriptor.kind;
        if (!restored.emplace(kind, std::move(descriptor)).second) {
            throw std::invalid_argument("mesh-map snapshot repeats map '" +
                                        std::string(mesh_map_name(kind)) + "'");
        }
    }
    memory_account_.set_resident_bytes(resident_pixel_bytes(restored));
    maps_.swap(restored);
}

}  // namespace ctex::maps
