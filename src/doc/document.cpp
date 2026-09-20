#include <ctex/doc/document.hpp>
#include <ctex/mesh/mesh.hpp>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ctex::doc {

struct TextureSetMemoryState {
    std::size_t mesh_map_pixel_bytes{};
};

namespace {

std::size_t checked_add(std::size_t left, std::size_t right, const char* description) {
    if (right > std::numeric_limits<std::size_t>::max() - left) {
        throw std::overflow_error(description);
    }
    return left + right;
}

std::size_t& category_bytes(TextureSetMemoryState& state, TextureSetMemoryCategory category) {
    switch (category) {
        case TextureSetMemoryCategory::mesh_maps:
            return state.mesh_map_pixel_bytes;
    }
    throw std::invalid_argument("texture-set memory category is invalid");
}

std::pmr::memory_resource* require_memory_resource(std::pmr::memory_resource* memory_resource) {
    if (memory_resource == nullptr) {
        throw std::invalid_argument("texture storage requires a memory resource");
    }
    return memory_resource;
}

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

TextureSetMemoryAccount::TextureSetMemoryAccount(std::shared_ptr<TextureSetMemoryState> state,
                                                 TextureSetMemoryCategory category)
    : state_(std::move(state)), category_(category) {
    if (!state_) {
        throw std::invalid_argument("texture-set memory account requires shared state");
    }
    static_cast<void>(category_bytes(*state_, category_));
}

TextureSetMemoryAccount::~TextureSetMemoryAccount() { release(); }

TextureSetMemoryAccount::TextureSetMemoryAccount(const TextureSetMemoryAccount& other)
    : state_(other.state_), category_(other.category_) {
    set_resident_bytes(other.resident_bytes_);
}

TextureSetMemoryAccount& TextureSetMemoryAccount::operator=(const TextureSetMemoryAccount& other) {
    if (this != &other) {
        TextureSetMemoryAccount next(other);
        *this = std::move(next);
    }
    return *this;
}

TextureSetMemoryAccount::TextureSetMemoryAccount(TextureSetMemoryAccount&& other) noexcept
    : state_(std::move(other.state_)),
      category_(other.category_),
      resident_bytes_(std::exchange(other.resident_bytes_, 0)) {}

TextureSetMemoryAccount& TextureSetMemoryAccount::operator=(
    TextureSetMemoryAccount&& other) noexcept {
    if (this != &other) {
        release();
        state_ = std::move(other.state_);
        category_ = other.category_;
        resident_bytes_ = std::exchange(other.resident_bytes_, 0);
    }
    return *this;
}

void TextureSetMemoryAccount::set_resident_bytes(std::size_t bytes) {
    if (!state_) {
        throw std::logic_error("texture-set memory account has been moved from");
    }
    std::size_t& total = category_bytes(*state_, category_);
    const std::size_t other_accounts = total - resident_bytes_;
    total = checked_add(other_accounts, bytes, "texture-set memory accounting overflow");
    resident_bytes_ = bytes;
}

void TextureSetMemoryAccount::release() noexcept {
    if (state_) {
        switch (category_) {
            case TextureSetMemoryCategory::mesh_maps:
                state_->mesh_map_pixel_bytes -= resident_bytes_;
                break;
        }
        resident_bytes_ = 0;
        state_.reset();
    }
}

std::string texture_set_stable_id(const TextureSetDescriptor& descriptor) {
    validate_texture_set_descriptor(descriptor);
    return mesh::texture_set_stable_id(mesh_partition_kind(descriptor.partition_kind),
                                       descriptor.partition_key, descriptor.uv_set);
}

TextureSet::TextureSet(TextureSetDescriptor descriptor, std::pmr::memory_resource* memory_resource)
    : memory_resource_(require_memory_resource(memory_resource)),
      display_name_(descriptor.display_name, memory_resource_),
      partition_kind_(descriptor.partition_kind),
      partition_key_(descriptor.partition_key, memory_resource_),
      uv_set_(descriptor.uv_set, memory_resource_),
      width_(descriptor.width),
      height_(descriptor.height),
      default_bit_depth_(descriptor.default_bit_depth),
      id_(texture_set_stable_id(descriptor), memory_resource_),
      channels_(width_, height_, default_bit_depth_, metallic_roughness_channels(),
                memory_resource_),
      memory_state_(std::allocate_shared<TextureSetMemoryState>(
          std::pmr::polymorphic_allocator<TextureSetMemoryState>(memory_resource_))),
      preset_applications_(memory_resource_) {}

TextureSetDescriptor TextureSet::descriptor() const {
    return {.display_name = {display_name_.begin(), display_name_.end()},
            .partition_kind = partition_kind_,
            .partition_key = {partition_key_.begin(), partition_key_.end()},
            .uv_set = {uv_set_.begin(), uv_set_.end()},
            .width = width_,
            .height = height_,
            .default_bit_depth = default_bit_depth_};
}

TextureSetMemoryAccount TextureSet::create_memory_account(TextureSetMemoryCategory category) const {
    return TextureSetMemoryAccount(memory_state_, category);
}

LayerChannelParticipation TextureSet::channel_participation(
    std::string_view entry_identifier, std::string_view semantic_id,
    std::span<const LayerMaskSample> mask_samples) const {
    static_cast<void>(channels_.descriptor(semantic_id));
    return layer_stack_.channel_participation(entry_identifier, semantic_id,
                                              channels_.is_enabled(semantic_id), mask_samples);
}

TextureSetMemoryReport TextureSet::memory_report() const {
    const std::size_t channel_bytes = channels_.resident_pixel_bytes();
    const std::size_t map_bytes = memory_state_->mesh_map_pixel_bytes;
    return {.texture_set_id = id(),
            .channel_pixel_bytes = channel_bytes,
            .mesh_map_pixel_bytes = map_bytes,
            .total_resident_bytes =
                checked_add(channel_bytes, map_bytes, "texture-set memory report overflow")};
}

TextureDocument::TextureDocument(std::pmr::memory_resource* memory_resource)
    : memory_resource_(memory_resource), texture_sets_(memory_resource) {
    if (memory_resource == nullptr) {
        throw std::invalid_argument("texture document requires a memory resource");
    }
}

TextureSet& TextureDocument::create_texture_set(TextureSetDescriptor descriptor) {
    TextureSet texture_set(std::move(descriptor), memory_resource_);
    const std::string id = texture_set.id();
    const auto [found, inserted] =
        texture_sets_.emplace(std::pmr::string(id, memory_resource_), std::move(texture_set));
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
        result.emplace_back(id.begin(), id.end());
    }
    return result;
}

TextureDocumentMemoryReport TextureDocument::memory_report() const {
    TextureDocumentMemoryReport result;
    result.texture_sets.reserve(texture_sets_.size());
    for (const auto& [id, texture_set] : texture_sets_) {
        static_cast<void>(id);
        TextureSetMemoryReport report = texture_set.memory_report();
        result.channel_pixel_bytes =
            checked_add(result.channel_pixel_bytes, report.channel_pixel_bytes,
                        "document channel memory report overflow");
        result.mesh_map_pixel_bytes =
            checked_add(result.mesh_map_pixel_bytes, report.mesh_map_pixel_bytes,
                        "document mesh-map memory report overflow");
        result.total_resident_bytes =
            checked_add(result.total_resident_bytes, report.total_resident_bytes,
                        "document total memory report overflow");
        result.texture_sets.push_back(std::move(report));
    }
    return result;
}

}  // namespace ctex::doc
