#include <algorithm>
#include <cmath>
#include <ctex/doc/document.hpp>
#include <ctex/mesh/mesh.hpp>
#include <limits>
#include <set>
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

std::size_t checked_multiply(std::size_t left, std::size_t right, const char* description) {
    if (left != 0 && right > std::numeric_limits<std::size_t>::max() / left) {
        throw std::overflow_error(description);
    }
    return left * right;
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

struct ResolvedUdimWrite {
    std::uint32_t tile_number{};
    std::uint32_t x{};
    std::uint32_t y{};
    std::span<const std::byte> pixel;
};

ResolvedUdimWrite resolve_udim_write(const TextureSetDescriptor& descriptor,
                                     const UdimPixelWrite& write) {
    if (!std::isfinite(write.u) || !std::isfinite(write.v) || write.u < 0.0 || write.v < 0.0) {
        throw std::invalid_argument("UDIM UV coordinates must be finite and non-negative");
    }
    const double tile_u = std::floor(write.u);
    const double tile_v = std::floor(write.v);
    if (tile_u > 9.0 || tile_v > std::numeric_limits<std::uint32_t>::max()) {
        throw std::out_of_range("UDIM UV coordinate is outside the supported tile range");
    }
    const UdimCoordinate coordinate{.u = static_cast<std::uint32_t>(tile_u),
                                    .v = static_cast<std::uint32_t>(tile_v)};
    const double local_u = write.u - tile_u;
    const double local_v = write.v - tile_v;
    const auto pixel_coordinate = [](double local, std::uint32_t extent) {
        return std::min(static_cast<std::uint32_t>(local * extent), extent - 1);
    };
    return {.tile_number = udim_number(coordinate),
            .x = pixel_coordinate(local_u, descriptor.width),
            .y = pixel_coordinate(local_v, descriptor.height),
            .pixel = write.pixel};
}

bool atlas_regions_overlap(const AtlasRegion& left, const AtlasRegion& right) {
    const std::uint64_t left_right = static_cast<std::uint64_t>(left.x) + left.width;
    const std::uint64_t right_right = static_cast<std::uint64_t>(right.x) + right.width;
    const std::uint64_t left_bottom = static_cast<std::uint64_t>(left.y) + left.height;
    const std::uint64_t right_bottom = static_cast<std::uint64_t>(right.y) + right.height;
    return left.x < right_right && right.x < left_right && left.y < right_bottom &&
           right.y < left_bottom;
}

void validate_atlas_region(const AtlasDescriptor& atlas, const AtlasRegion& region) {
    if (region.texture_set_identifier.empty() || region.width == 0 || region.height == 0) {
        throw std::invalid_argument(
            "atlas regions require a texture-set identity and non-zero dimensions");
    }
    const std::uint64_t right = static_cast<std::uint64_t>(region.x) + region.width;
    const std::uint64_t bottom = static_cast<std::uint64_t>(region.y) + region.height;
    if (right > atlas.width || bottom > atlas.height) {
        throw std::out_of_range("atlas region is outside atlas bounds: " +
                                region.texture_set_identifier);
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

std::uint32_t udim_number(UdimCoordinate coordinate) {
    if (coordinate.u > 9) {
        throw std::out_of_range("UDIM U coordinate must be between zero and nine");
    }
    constexpr std::uint32_t base = 1001;
    if (coordinate.v > (std::numeric_limits<std::uint32_t>::max() - base - coordinate.u) / 10) {
        throw std::overflow_error("UDIM number exceeds the supported integer range");
    }
    return base + coordinate.u + 10 * coordinate.v;
}

UdimCoordinate udim_coordinate(std::uint32_t number) {
    if (number < 1001) {
        throw std::out_of_range("UDIM number must be at least 1001");
    }
    const std::uint32_t offset = number - 1001;
    return {.u = offset % 10, .v = offset / 10};
}

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
      udim_tiling_(descriptor.udim_tiling),
      id_(texture_set_stable_id(descriptor), memory_resource_),
      channels_(width_, height_, default_bit_depth_, metallic_roughness_channels(),
                memory_resource_),
      udim_tiles_(memory_resource_),
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
            .default_bit_depth = default_bit_depth_,
            .udim_tiling = udim_tiling_};
}

UdimWriteResult TextureSet::write_udim_pixels(std::string_view semantic_id,
                                              std::span<const UdimPixelWrite> writes) {
    if (!udim_tiling_) {
        throw std::logic_error("texture set does not use UDIM tiling");
    }
    const image::TiledImage& prototype = channels_.pixels(semantic_id);
    const TextureSetDescriptor set_descriptor = descriptor();
    std::vector<ResolvedUdimWrite> resolved;
    resolved.reserve(writes.size());
    for (const UdimPixelWrite& write : writes) {
        if (write.pixel.size() != prototype.pixel_bytes()) {
            throw std::invalid_argument("UDIM pixel size does not match the channel format");
        }
        resolved.push_back(resolve_udim_write(set_descriptor, write));
    }

    std::set<std::uint32_t> changed_tiles;
    std::set<std::uint32_t> allocated_tiles;
    std::size_t changed_pixel_count = 0;
    for (const ResolvedUdimWrite& write : resolved) {
        auto found = udim_tiles_.find(write.tile_number);
        if (found == udim_tiles_.end()) {
            TextureChannels candidate = channels_.clone_configuration();
            image::TiledImage& image = candidate.pixels(semantic_id);
            const image::Revision before = image.revision();
            image.write_pixel(write.x, write.y, write.pixel);
            if (image.revision() == before) {
                continue;
            }
            found = udim_tiles_.emplace(write.tile_number, std::move(candidate)).first;
            allocated_tiles.insert(write.tile_number);
        } else {
            found->second.synchronize_configuration(channels_);
            image::TiledImage& image = found->second.pixels(semantic_id);
            const image::Revision before = image.revision();
            image.write_pixel(write.x, write.y, write.pixel);
            if (image.revision() == before) {
                continue;
            }
        }
        changed_tiles.insert(write.tile_number);
        ++changed_pixel_count;
    }
    return {.changed_tiles = {changed_tiles.begin(), changed_tiles.end()},
            .allocated_tiles = {allocated_tiles.begin(), allocated_tiles.end()},
            .changed_pixel_count = changed_pixel_count};
}

std::vector<std::uint32_t> TextureSet::ensure_udim_tiles(
    std::span<const std::uint32_t> tile_numbers) {
    if (!udim_tiling_) {
        throw std::logic_error("texture set does not use UDIM tiling");
    }
    std::set<std::uint32_t> unique;
    for (const std::uint32_t number : tile_numbers) {
        static_cast<void>(udim_coordinate(number));
        if (!unique.insert(number).second) {
            throw std::invalid_argument("UDIM tile declaration contains a duplicate number");
        }
    }
    std::vector<std::uint32_t> allocated;
    allocated.reserve(unique.size());
    for (const std::uint32_t number : unique) {
        const auto [unused, inserted] =
            udim_tiles_.try_emplace(number, channels_.clone_configuration());
        static_cast<void>(unused);
        if (inserted) {
            allocated.push_back(number);
        }
    }
    return allocated;
}

std::vector<std::byte> TextureSet::read_udim_pixel(std::string_view semantic_id,
                                                   std::uint32_t tile_number, std::uint32_t x,
                                                   std::uint32_t y) const {
    if (!udim_tiling_) {
        throw std::logic_error("texture set does not use UDIM tiling");
    }
    static_cast<void>(udim_coordinate(tile_number));
    const image::TiledImage& prototype = channels_.pixels(semantic_id);
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("UDIM pixel coordinate is outside the tile");
    }
    const auto found = udim_tiles_.find(tile_number);
    const std::span<const std::byte> pixel =
        found != udim_tiles_.end() && found->second.is_enabled(semantic_id)
            ? found->second.pixels(semantic_id).read_pixel(x, y)
            : prototype.clear_pixel();
    return {pixel.begin(), pixel.end()};
}

std::vector<std::uint32_t> TextureSet::occupied_udim_tiles() const {
    std::vector<std::uint32_t> result;
    result.reserve(udim_tiles_.size());
    for (const auto& [number, channels] : udim_tiles_) {
        static_cast<void>(channels);
        result.push_back(number);
    }
    return result;
}

TextureChannels& TextureSet::udim_channels(std::uint32_t tile_number) {
    if (!udim_tiling_) {
        throw std::logic_error("texture set does not use UDIM tiling");
    }
    static_cast<void>(udim_coordinate(tile_number));
    const auto found = udim_tiles_.find(tile_number);
    if (found == udim_tiles_.end()) {
        throw std::out_of_range("UDIM tile has no allocated storage: " +
                                std::to_string(tile_number));
    }
    return found->second;
}

const TextureChannels& TextureSet::udim_channels(std::uint32_t tile_number) const {
    return const_cast<TextureSet*>(this)->udim_channels(tile_number);
}

bool TextureSet::can_clear_channels() const noexcept {
    return channels_.can_clear_enabled() && std::ranges::all_of(udim_tiles_, [](const auto& item) {
               return item.second.can_clear_enabled();
           });
}

void TextureSet::clear_channels() {
    if (!can_clear_channels()) {
        throw std::overflow_error("texture-set channel revision epoch space is exhausted");
    }
    channels_.clear_enabled();
    for (auto& [unused, channels] : udim_tiles_) {
        static_cast<void>(unused);
        channels.clear_enabled();
    }
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

LayerCompositeResult TextureSet::composite_cpu(const LayerCompositeRequest& request) const {
    return composite_texture_set_cpu(*this, request);
}

LayerOperationResult TextureSet::apply_layer_operation(LayerOperationRequest request) {
    return ctex::doc::apply_layer_operation(*this, std::move(request));
}

TextureSetMemoryReport TextureSet::memory_report() const {
    std::size_t channel_bytes = channels_.resident_pixel_bytes();
    for (const auto& [unused, channels] : udim_tiles_) {
        static_cast<void>(unused);
        channel_bytes = checked_add(channel_bytes, channels.resident_pixel_bytes(),
                                    "UDIM channel memory report overflow");
    }
    const std::size_t map_bytes = memory_state_->mesh_map_pixel_bytes;
    const std::size_t history_bytes = tile_history_.budget_report().retained_bytes;
    const std::size_t pixel_and_map_bytes =
        checked_add(channel_bytes, map_bytes, "texture-set save estimate overflow");
    constexpr std::size_t container_metadata_estimate = 512;
    return {.texture_set_id = id(),
            .channel_pixel_bytes = channel_bytes,
            .history_retained_bytes = history_bytes,
            .mesh_map_pixel_bytes = map_bytes,
            .total_resident_bytes = checked_add(pixel_and_map_bytes, history_bytes,
                                                "texture-set memory report overflow"),
            .estimated_save_bytes = checked_add(pixel_and_map_bytes, container_metadata_estimate,
                                                "texture-set save estimate overflow")};
}

TextureDocument::TextureDocument(std::pmr::memory_resource* memory_resource)
    : memory_resource_(memory_resource), texture_sets_(memory_resource), atlases_(memory_resource) {
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

const AtlasDescriptor& TextureDocument::create_atlas(AtlasDescriptor descriptor) {
    if (descriptor.identifier.empty() || descriptor.display_name.empty() || descriptor.width == 0 ||
        descriptor.height == 0 || descriptor.regions.empty()) {
        throw std::invalid_argument(
            "atlas requires identity, display name, dimensions, and at least one region");
    }
    if (atlases_.contains(std::string_view(descriptor.identifier))) {
        throw std::invalid_argument("atlas identity is already present: " + descriptor.identifier);
    }
    std::set<std::string_view, std::less<>> members;
    for (std::size_t index = 0; index < descriptor.regions.size(); ++index) {
        const AtlasRegion& region = descriptor.regions[index];
        validate_atlas_region(descriptor, region);
        if (!contains_texture_set(region.texture_set_identifier)) {
            throw std::invalid_argument("atlas region names a missing texture set: " +
                                        region.texture_set_identifier);
        }
        if (!members.insert(region.texture_set_identifier).second) {
            throw std::invalid_argument("atlas repeats a texture set: " +
                                        region.texture_set_identifier);
        }
        for (std::size_t previous = 0; previous < index; ++previous) {
            if (atlas_regions_overlap(region, descriptor.regions[previous])) {
                throw std::invalid_argument(
                    "atlas regions overlap: " + region.texture_set_identifier + " and " +
                    descriptor.regions[previous].texture_set_identifier);
            }
        }
        for (const auto& [unused, existing] : atlases_) {
            static_cast<void>(unused);
            if (std::ranges::any_of(existing.regions, [&](const AtlasRegion& existing_region) {
                    return existing_region.texture_set_identifier == region.texture_set_identifier;
                })) {
                throw std::invalid_argument("texture set already belongs to an atlas: " +
                                            region.texture_set_identifier);
            }
        }
    }
    const std::string identifier = descriptor.identifier;
    return atlases_.emplace(std::pmr::string(identifier, memory_resource_), std::move(descriptor))
        .first->second;
}

bool TextureDocument::contains_atlas(std::string_view identifier) const noexcept {
    return atlases_.contains(identifier);
}

const AtlasDescriptor& TextureDocument::atlas(std::string_view identifier) const {
    const auto found = atlases_.find(identifier);
    if (found == atlases_.end()) {
        throw std::out_of_range("atlas identity is not present: " + std::string(identifier));
    }
    return found->second;
}

std::vector<std::string> TextureDocument::atlas_ids() const {
    std::vector<std::string> result;
    result.reserve(atlases_.size());
    for (const auto& [identifier, unused] : atlases_) {
        static_cast<void>(unused);
        result.emplace_back(identifier.begin(), identifier.end());
    }
    return result;
}

TextureDocumentMemoryReport TextureDocument::memory_report() const {
    TextureDocumentMemoryReport result;
    constexpr std::size_t container_header_estimate = 64;
    constexpr std::size_t atlas_metadata_estimate = 128;
    result.estimated_save_bytes = container_header_estimate;
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
        result.history_retained_bytes =
            checked_add(result.history_retained_bytes, report.history_retained_bytes,
                        "document history memory report overflow");
        result.total_resident_bytes =
            checked_add(result.total_resident_bytes, report.total_resident_bytes,
                        "document total memory report overflow");
        result.estimated_save_bytes =
            checked_add(result.estimated_save_bytes, report.estimated_save_bytes,
                        "document save estimate overflow");
        result.texture_sets.push_back(std::move(report));
    }
    for (const auto& [unused, atlas] : atlases_) {
        static_cast<void>(unused);
        const std::size_t region_bytes =
            checked_add(atlas_metadata_estimate,
                        checked_multiply(atlas.regions.size(), sizeof(AtlasRegion),
                                         "document atlas save estimate overflow"),
                        "document atlas save estimate overflow");
        result.estimated_save_bytes = checked_add(result.estimated_save_bytes, region_bytes,
                                                  "document save estimate overflow");
    }
    return result;
}

}  // namespace ctex::doc
