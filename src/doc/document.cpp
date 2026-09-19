#include <ctex/doc/document.hpp>
#include <ctex/mesh/mesh.hpp>
#include <stdexcept>
#include <utility>

namespace ctex::doc {
namespace {

mesh::PartitionKind mesh_partition_kind(PartitionSourceKind kind) {
    switch (kind) {
        case PartitionSourceKind::material:
            return mesh::PartitionKind::material;
        case PartitionSourceKind::object:
            return mesh::PartitionKind::object;
        case PartitionSourceKind::submesh:
            return mesh::PartitionKind::submesh;
        case PartitionSourceKind::explicit_faces:
            return mesh::PartitionKind::explicit_faces;
    }
    throw std::invalid_argument("unsupported partition source kind");
}

void validate_texture_set_descriptor(const TextureSetDescriptor& descriptor) {
    if (descriptor.display_name.empty()) {
        throw std::invalid_argument("texture-set display name must not be empty");
    }
    if (descriptor.partition_key.empty()) {
        throw std::invalid_argument("texture-set partition key must not be empty");
    }
    if (descriptor.uv_set.empty()) {
        throw std::invalid_argument("texture-set UV set name must not be empty");
    }
    if (descriptor.width == 0 || descriptor.height == 0) {
        throw std::invalid_argument("texture-set resolution must be non-zero");
    }
    if (descriptor.default_bit_depth != 8 && descriptor.default_bit_depth != 16 &&
        descriptor.default_bit_depth != 32) {
        throw std::invalid_argument("texture-set bit depth must be 8, 16 or 32");
    }
}

PartitionSourceKind document_partition_kind(mesh::PartitionKind kind) {
    switch (kind) {
        case mesh::PartitionKind::material:
            return PartitionSourceKind::material;
        case mesh::PartitionKind::object:
            return PartitionSourceKind::object;
        case mesh::PartitionKind::submesh:
            return PartitionSourceKind::submesh;
        case mesh::PartitionKind::explicit_faces:
            return PartitionSourceKind::explicit_faces;
    }
    throw std::invalid_argument("unsupported mesh partition kind");
}

}  // namespace

std::string texture_set_stable_id(const TextureSetDescriptor& descriptor) {
    validate_texture_set_descriptor(descriptor);
    return mesh::texture_set_stable_id(mesh_partition_kind(descriptor.partition_kind),
                                       descriptor.partition_key, descriptor.uv_set);
}

TextureSet::TextureSet(TextureSetDescriptor descriptor)
    : descriptor_(std::move(descriptor)),
      id_(texture_set_stable_id(descriptor_)),
      channels_(descriptor_.width, descriptor_.height, descriptor_.default_bit_depth,
                metallic_roughness_channels()) {}

TextureSet& TextureDocument::create_texture_set(TextureSetDescriptor descriptor) {
    TextureSet texture_set(std::move(descriptor));
    const std::string id = texture_set.id();
    const auto [found, inserted] = texture_sets_.emplace(id, std::move(texture_set));
    if (!inserted) {
        throw std::invalid_argument("texture-set identity is already present: " + id);
    }
    return found->second;
}

std::vector<std::string> TextureDocument::create_texture_sets_from_mesh(
    const mesh::MeshView& mesh, std::string_view uv_set, std::uint32_t width, std::uint32_t height,
    std::uint8_t default_bit_depth) {
    static_cast<void>(mesh.uv_set(uv_set));
    std::vector<std::string> result;
    result.reserve(mesh.descriptor().partitions.size());
    for (const mesh::MeshPartition& partition : mesh.descriptor().partitions) {
        TextureSet& texture_set = create_texture_set({
            .display_name = std::string(partition.display_name),
            .partition_kind = document_partition_kind(partition.kind),
            .partition_key = std::string(partition.stable_key),
            .uv_set = std::string(uv_set),
            .width = width,
            .height = height,
            .default_bit_depth = default_bit_depth,
        });
        result.push_back(texture_set.id());
    }
    return result;
}

bool TextureDocument::contains_texture_set(std::string_view stable_id) const noexcept {
    return texture_sets_.find(stable_id) != texture_sets_.end();
}

TextureSet& TextureDocument::texture_set(std::string_view stable_id) {
    const auto found = texture_sets_.find(stable_id);
    if (found == texture_sets_.end()) {
        throw std::out_of_range("texture-set identity is not present: " + std::string(stable_id));
    }
    return found->second;
}

const TextureSet& TextureDocument::texture_set(std::string_view stable_id) const {
    const auto found = texture_sets_.find(stable_id);
    if (found == texture_sets_.end()) {
        throw std::out_of_range("texture-set identity is not present: " + std::string(stable_id));
    }
    return found->second;
}

std::vector<std::string> TextureDocument::texture_set_ids() const {
    std::vector<std::string> result;
    result.reserve(texture_sets_.size());
    for (const auto& [id, unused] : texture_sets_) {
        static_cast<void>(unused);
        result.push_back(id);
    }
    return result;
}

}  // namespace ctex::doc
