#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/io/document_mesh_state.hpp>
#include <ctex/io/texture_document.hpp>
#include <limits>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace ctex::io {
namespace {

constexpr std::array<std::byte, 8> state_magic{std::byte{'C'}, std::byte{'T'}, std::byte{'E'},
                                               std::byte{'X'}, std::byte{'M'}, std::byte{'A'},
                                               std::byte{'P'}, std::byte{'S'}};
constexpr std::uint32_t maximum_mesh_map_kind = 12;

class Writer {
public:
    void u8(std::uint8_t value) { bytes_.push_back(static_cast<std::byte>(value)); }

    void u32(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            bytes_.push_back(static_cast<std::byte>(value >> shift));
        }
    }

    void u64(std::uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8) {
            bytes_.push_back(static_cast<std::byte>(value >> shift));
        }
    }

    void string(std::string_view value) {
        if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw DocumentMeshStateIoError("document mesh-state string exceeds format limit");
        }
        u32(static_cast<std::uint32_t>(value.size()));
        bytes_.insert(bytes_.end(), reinterpret_cast<const std::byte*>(value.data()),
                      reinterpret_cast<const std::byte*>(value.data() + value.size()));
    }

    void bytes(std::span<const std::byte> value) {
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    [[nodiscard]] std::vector<std::byte> finish() && { return std::move(bytes_); }

private:
    std::vector<std::byte> bytes_;
};

class Reader {
public:
    Reader(std::span<const std::byte> bytes, DocumentMeshStateReadLimits limits)
        : bytes_(bytes), limits_(limits) {
        if (bytes.size() > limits.maximum_payload_bytes) {
            throw DocumentMeshStateIoError("document mesh-state payload exceeds configured limit");
        }
    }

    [[nodiscard]] std::span<const std::byte> take(std::size_t count, std::string_view field) {
        if (count > bytes_.size() - offset_) {
            throw DocumentMeshStateIoError(std::string(field) + " exceeds remaining payload");
        }
        const auto result = bytes_.subspan(offset_, count);
        offset_ += count;
        return result;
    }

    [[nodiscard]] std::uint8_t u8(std::string_view field) {
        return std::to_integer<std::uint8_t>(take(1, field).front());
    }

    [[nodiscard]] std::uint32_t u32(std::string_view field) {
        const auto value = take(4, field);
        std::uint32_t result{};
        for (unsigned index = 0; index < 4; ++index) {
            result |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value[index]))
                      << (index * 8U);
        }
        return result;
    }

    [[nodiscard]] std::uint64_t u64(std::string_view field) {
        const auto value = take(8, field);
        std::uint64_t result{};
        for (unsigned index = 0; index < 8; ++index) {
            result |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(value[index]))
                      << (index * 8U);
        }
        return result;
    }

    [[nodiscard]] std::string string(std::string_view field) {
        const std::uint32_t size = u32(field);
        if (size > limits_.maximum_string_bytes) {
            throw DocumentMeshStateIoError(std::string(field) + " exceeds configured limit");
        }
        const auto value = take(size, field);
        return {reinterpret_cast<const char*>(value.data()), value.size()};
    }

    [[nodiscard]] bool empty() const noexcept { return offset_ == bytes_.size(); }
    [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - offset_; }

private:
    std::span<const std::byte> bytes_;
    DocumentMeshStateReadLimits limits_;
    std::size_t offset_{};
};

std::string encoded_segment(std::string_view value) {
    constexpr char hexadecimal[] = "0123456789abcdef";
    std::string result;
    result.reserve(value.size() * 2);
    for (const unsigned char byte : value) {
        result.push_back(hexadecimal[byte >> 4U]);
        result.push_back(hexadecimal[byte & 0x0fU]);
    }
    return result;
}

std::string map_image_id(std::string_view asset_id, const StoredDocumentMeshMap& map) {
    return std::string(asset_id) + "/pixels/" + encoded_segment(map.texture_set_id) + '/' +
           std::to_string(map.kind);
}

void validate_tangent_frame(const mesh::TangentFrameDescriptor& frame, std::string_view uv_set) {
    if (frame.algorithm > mesh::TangentBasisAlgorithm::mikktspace ||
        frame.normal_orientation > mesh::NormalOrientation::inverted_vertex_normals ||
        frame.coordinate_handedness > mesh::CoordinateSystemHandedness::left_handed ||
        frame.uv_v_axis > mesh::UvVAxis::downward ||
        frame.handedness_encoding != mesh::TangentHandednessEncoding::tangent_w_sign ||
        frame.algorithm_version == 0 || frame.uv_set != uv_set) {
        throw DocumentMeshStateIoError("document mesh-state tangent frame is invalid");
    }
}

void validate_state(const ProjectContainer& project, const DocumentMeshState& state) {
    if (state.document_asset_id.empty() || state.mesh_resource_id.empty() ||
        state.mesh_revision == 0) {
        throw DocumentMeshStateIoError(
            "document mesh state requires document, mesh resource and non-zero revision");
    }
    const auto document =
        std::ranges::find(project.assets, state.document_asset_id, &StandaloneAsset::identifier);
    if (document == project.assets.end() || document->kind != texture_document_asset_kind) {
        throw DocumentMeshStateIoError("document mesh state names no texture document");
    }
    const auto resource =
        std::ranges::find(project.resources, state.mesh_resource_id, &ProjectResource::identifier);
    if (resource == project.resources.end() || resource->kind != "mesh") {
        throw DocumentMeshStateIoError("document mesh state names no mesh resource");
    }
    const doc::TextureDocument texture_document =
        unpack_texture_document(project, state.document_asset_id);
    std::set<std::pair<std::string_view, std::uint32_t>> identities;
    for (const StoredDocumentMeshMap& map : state.maps) {
        if (map.kind > maximum_mesh_map_kind || map.texture_set_id.empty() || map.uv_set.empty() ||
            map.produced_mesh_revision == 0 ||
            !identities.emplace(map.texture_set_id, map.kind).second) {
            throw DocumentMeshStateIoError("document mesh-state map metadata is invalid");
        }
        if (!texture_document.contains_texture_set(map.texture_set_id) ||
            texture_document.texture_set(map.texture_set_id).descriptor().uv_set != map.uv_set) {
            throw DocumentMeshStateIoError(
                "document mesh-state map does not match its texture set");
        }
        if (map.normal_convention && *map.normal_convention > 1) {
            throw DocumentMeshStateIoError("document mesh-state normal convention is invalid");
        }
        if (map.tangent_frame) validate_tangent_frame(*map.tangent_frame, map.uv_set);
    }
}

std::size_t restored_tile_bytes(const StoredTiledImage& image) {
    const std::size_t pixel_bytes = image.format.bytes_per_pixel();
    if (image.tile_size > std::numeric_limits<std::size_t>::max() / image.tile_size ||
        static_cast<std::size_t>(image.tile_size) * image.tile_size >
            std::numeric_limits<std::size_t>::max() / pixel_bytes) {
        throw DocumentMeshStateIoError("document mesh-state tile allocation overflows");
    }
    return static_cast<std::size_t>(image.tile_size) * image.tile_size * pixel_bytes;
}

void charge_map_pixels(std::size_t bytes, std::size_t limit, std::size_t& total) {
    if (total > limit || bytes > limit - total) {
        throw DocumentMeshStateIoError("document mesh-state pixels exceed configured limit");
    }
    total += bytes;
}

mesh::TangentFrameDescriptor read_tangent_frame(Reader& reader, std::string_view uv_set);

StoredDocumentMeshMap read_map(
    Reader& reader, const StandaloneAsset& asset,
    const std::map<std::string_view, const StoredTiledImage*, std::less<>>& images,
    std::set<std::string>& used_images, const DocumentMeshStateReadLimits& limits,
    std::size_t& total_map_pixel_bytes) {
    const std::uint32_t kind = reader.u32("mesh-map kind");
    std::string texture_set = reader.string("mesh-map texture set");
    std::string uv_set = reader.string("mesh-map UV set");
    const mesh::MeshRevision produced = reader.u64("mesh-map produced revision");
    std::optional<std::uint32_t> convention;
    const std::uint8_t has_convention = reader.u8("normal convention presence");
    if (has_convention > 1) {
        throw DocumentMeshStateIoError("normal convention presence is invalid");
    }
    if (has_convention != 0) convention = reader.u32("normal convention");
    std::optional<mesh::TangentFrameDescriptor> tangent_frame;
    const std::uint8_t has_tangent_frame = reader.u8("tangent-frame presence");
    if (has_tangent_frame > 1) {
        throw DocumentMeshStateIoError("tangent-frame presence is invalid");
    }
    if (has_tangent_frame != 0) tangent_frame = read_tangent_frame(reader, uv_set);
    const std::string image_id = reader.string("mesh-map image identity");
    if (std::ranges::find(asset.tiled_image_dependencies, image_id) ==
            asset.tiled_image_dependencies.end() ||
        !used_images.insert(image_id).second) {
        throw DocumentMeshStateIoError("document mesh-state image dependency is invalid");
    }
    const auto image = images.find(image_id);
    if (image == images.end()) {
        throw DocumentMeshStateIoError("document mesh-state image is missing");
    }
    const std::size_t tile_bytes = restored_tile_bytes(*image->second);
    for ([[maybe_unused]] const StoredTile& tile : image->second->occupied_tiles) {
        charge_map_pixels(tile_bytes, limits.maximum_total_map_pixel_bytes, total_map_pixel_bytes);
    }
    return {.kind = kind,
            .texture_set_id = std::move(texture_set),
            .uv_set = std::move(uv_set),
            .produced_mesh_revision = produced,
            .normal_convention = convention,
            .tangent_frame = std::move(tangent_frame),
            .pixels = restore_tiled_image(*image->second)};
}

void write_tangent_frame(Writer& writer, const mesh::TangentFrameDescriptor& frame) {
    writer.u8(static_cast<std::uint8_t>(frame.algorithm));
    writer.u32(frame.algorithm_version);
    writer.u8(static_cast<std::uint8_t>(frame.normal_orientation));
    writer.u8(static_cast<std::uint8_t>(frame.coordinate_handedness));
    writer.u8(static_cast<std::uint8_t>(frame.uv_v_axis));
    writer.u8(static_cast<std::uint8_t>(frame.handedness_encoding));
    writer.string(frame.uv_set);
}

mesh::TangentFrameDescriptor read_tangent_frame(Reader& reader, std::string_view uv_set) {
    const auto algorithm = static_cast<mesh::TangentBasisAlgorithm>(reader.u8("tangent algorithm"));
    const std::uint32_t algorithm_version = reader.u32("tangent algorithm version");
    const auto normal_orientation =
        static_cast<mesh::NormalOrientation>(reader.u8("normal orientation"));
    const auto coordinate_handedness =
        static_cast<mesh::CoordinateSystemHandedness>(reader.u8("coordinate handedness"));
    const auto uv_v_axis = static_cast<mesh::UvVAxis>(reader.u8("UV V axis"));
    const auto handedness_encoding =
        static_cast<mesh::TangentHandednessEncoding>(reader.u8("tangent handedness encoding"));
    const std::string frame_uv_set = reader.string("tangent UV set");
    mesh::TangentFrameDescriptor frame{.algorithm = algorithm,
                                       .algorithm_version = algorithm_version,
                                       .normal_orientation = normal_orientation,
                                       .coordinate_handedness = coordinate_handedness,
                                       .uv_v_axis = uv_v_axis,
                                       .handedness_encoding = handedness_encoding,
                                       .uv_set = std::pmr::string(frame_uv_set)};
    validate_tangent_frame(frame, uv_set);
    return frame;
}

bool image_is_used_elsewhere(const ProjectContainer& project, std::string_view asset_id,
                             std::string_view image_id) {
    return std::ranges::any_of(project.assets, [&](const StandaloneAsset& asset) {
        return asset.identifier != asset_id &&
               std::ranges::find(asset.tiled_image_dependencies, image_id) !=
                   asset.tiled_image_dependencies.end();
    });
}

}  // namespace

std::string document_mesh_state_asset_id(std::string_view document_asset_id) {
    if (document_asset_id.empty()) {
        throw DocumentMeshStateIoError("document mesh state requires a document identity");
    }
    return std::string(document_asset_id) + "/mesh-state";
}

void upsert_document_mesh_state(ProjectContainer& project, const DocumentMeshState& state) {
    validate_state(project, state);
    const std::string asset_id = document_mesh_state_asset_id(state.document_asset_id);
    Writer writer;
    writer.bytes(state_magic);
    writer.u32(current_document_mesh_state_schema);
    writer.string(state.document_asset_id);
    writer.string(state.mesh_resource_id);
    writer.u64(state.mesh_revision);
    if (state.maps.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw DocumentMeshStateIoError("document mesh-state map count exceeds format limit");
    }
    writer.u32(static_cast<std::uint32_t>(state.maps.size()));
    std::vector<StoredTiledImage> images;
    std::vector<std::string> dependencies;
    images.reserve(state.maps.size());
    dependencies.reserve(state.maps.size());
    std::vector<const StoredDocumentMeshMap*> ordered_maps;
    ordered_maps.reserve(state.maps.size());
    for (const StoredDocumentMeshMap& map : state.maps) ordered_maps.push_back(&map);
    std::ranges::sort(ordered_maps, [](const auto* left, const auto* right) {
        return std::tie(left->texture_set_id, left->kind) <
               std::tie(right->texture_set_id, right->kind);
    });
    for (const StoredDocumentMeshMap* map_pointer : ordered_maps) {
        const StoredDocumentMeshMap& map = *map_pointer;
        const std::string image_id = map_image_id(asset_id, map);
        writer.u32(map.kind);
        writer.string(map.texture_set_id);
        writer.string(map.uv_set);
        writer.u64(map.produced_mesh_revision);
        writer.u8(map.normal_convention.has_value() ? 1 : 0);
        if (map.normal_convention) writer.u32(*map.normal_convention);
        writer.u8(map.tangent_frame.has_value() ? 1 : 0);
        if (map.tangent_frame) write_tangent_frame(writer, *map.tangent_frame);
        writer.string(image_id);
        images.push_back(snapshot_tiled_image(image_id, map.pixels));
        dependencies.push_back(image_id);
    }
    StandaloneAsset replacement{.identifier = asset_id,
                                .kind = std::string(document_mesh_state_asset_kind),
                                .format_version = current_document_mesh_state_schema,
                                .resource_dependencies = {state.mesh_resource_id},
                                .tiled_image_dependencies = dependencies,
                                .payload = std::move(writer).finish()};

    ProjectContainer candidate = project;
    const auto old_asset =
        std::ranges::find(candidate.assets, asset_id, &StandaloneAsset::identifier);
    if (old_asset != candidate.assets.end()) {
        if (old_asset->kind != document_mesh_state_asset_kind) {
            throw DocumentMeshStateIoError(
                "document mesh-state identity conflicts with another asset kind");
        }
        const auto old_dependencies = old_asset->tiled_image_dependencies;
        *old_asset = replacement;
        std::erase_if(candidate.tiled_images, [&](const StoredTiledImage& image) {
            return std::ranges::find(old_dependencies, image.resource_id) !=
                       old_dependencies.end() &&
                   !image_is_used_elsewhere(candidate, asset_id, image.resource_id);
        });
    } else {
        candidate.assets.push_back(std::move(replacement));
    }
    for (StoredTiledImage& image : images) {
        if (std::ranges::find(candidate.tiled_images, image.resource_id,
                              &StoredTiledImage::resource_id) != candidate.tiled_images.end()) {
            throw DocumentMeshStateIoError("document mesh-state image identity conflicts");
        }
        candidate.tiled_images.push_back(std::move(image));
    }
    std::ranges::sort(candidate.assets, {}, &StandaloneAsset::identifier);
    std::ranges::sort(candidate.tiled_images, {}, &StoredTiledImage::resource_id);
    static_cast<void>(write_project_container(candidate));
    project = std::move(candidate);
}

std::optional<DocumentMeshState> read_document_mesh_state(const ProjectContainer& project,
                                                          std::string_view document_asset_id,
                                                          DocumentMeshStateReadLimits limits) {
    const std::string asset_id = document_mesh_state_asset_id(document_asset_id);
    const auto asset = std::ranges::find(project.assets, asset_id, &StandaloneAsset::identifier);
    if (asset == project.assets.end()) return std::nullopt;
    if (asset->kind != document_mesh_state_asset_kind ||
        asset->format_version != current_document_mesh_state_schema ||
        asset->resource_dependencies.size() != 1) {
        throw DocumentMeshStateIoError("asset is not a supported document mesh state");
    }
    Reader reader(asset->payload, limits);
    if (!std::ranges::equal(reader.take(state_magic.size(), "document mesh-state magic"),
                            state_magic) ||
        reader.u32("document mesh-state schema") != current_document_mesh_state_schema) {
        throw DocumentMeshStateIoError("document mesh-state header is invalid");
    }
    DocumentMeshState state{.document_asset_id = reader.string("document identity"),
                            .mesh_resource_id = reader.string("mesh resource identity"),
                            .mesh_revision = reader.u64("mesh revision"),
                            .maps = {}};
    if (state.document_asset_id != document_asset_id ||
        state.mesh_resource_id != asset->resource_dependencies.front()) {
        throw DocumentMeshStateIoError("document mesh-state dependencies do not match payload");
    }
    const std::uint32_t count = reader.u32("mesh-map count");
    constexpr std::size_t minimum_map_record_bytes = 26;
    if (count > limits.maximum_maps || count > reader.remaining() / minimum_map_record_bytes) {
        throw DocumentMeshStateIoError("document mesh-state map count exceeds configured limit");
    }
    std::map<std::string_view, const StoredTiledImage*, std::less<>> images;
    for (const StoredTiledImage& image : project.tiled_images) {
        images.emplace(image.resource_id, &image);
    }
    std::set<std::string> used_images;
    std::size_t total_map_pixel_bytes{};
    state.maps.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        state.maps.push_back(
            read_map(reader, *asset, images, used_images, limits, total_map_pixel_bytes));
    }
    if (!reader.empty() || used_images.size() != asset->tiled_image_dependencies.size()) {
        throw DocumentMeshStateIoError(
            "document mesh-state dependencies do not exactly match payload");
    }
    validate_state(project, state);
    return state;
}

}  // namespace ctex::io
