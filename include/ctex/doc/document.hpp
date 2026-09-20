#ifndef CTEX_DOC_DOCUMENT_HPP
#define CTEX_DOC_DOCUMENT_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/doc/channels.hpp>
#include <ctex/doc/layer_compositor.hpp>
#include <ctex/doc/layer_operations.hpp>
#include <ctex/doc/layer_stack.hpp>
#include <ctex/doc/smart_mask.hpp>
#include <ctex/doc/tile_history.hpp>
#include <ctex/doc/transaction.hpp>
#include <map>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ctex::mesh {
class MeshView;
}

namespace ctex::doc {

struct TextureSetMemoryState;

enum class TextureSetMemoryCategory : std::uint8_t { mesh_maps };

struct TextureSetMemoryReport {
    std::string texture_set_id;
    std::size_t channel_pixel_bytes{};
    std::size_t mesh_map_pixel_bytes{};
    std::size_t total_resident_bytes{};
    friend bool operator==(const TextureSetMemoryReport&, const TextureSetMemoryReport&) = default;
};

struct TextureDocumentMemoryReport {
    std::vector<TextureSetMemoryReport> texture_sets;
    std::size_t channel_pixel_bytes{};
    std::size_t mesh_map_pixel_bytes{};
    std::size_t total_resident_bytes{};
    friend bool operator==(const TextureDocumentMemoryReport&,
                           const TextureDocumentMemoryReport&) = default;
};

enum class AppliedPresetKind : std::uint8_t { smart_material, smart_mask };

struct AppliedPresetOrigin {
    std::string preset_identifier;
    std::uint32_t schema_version{};
    friend bool operator==(const AppliedPresetOrigin&, const AppliedPresetOrigin&) = default;
};

struct AppliedPresetParameterValue {
    std::string parameter_identifier;
    graph::SocketValue value;
    friend bool operator==(const AppliedPresetParameterValue&,
                           const AppliedPresetParameterValue&) = default;
};

struct AppliedPresetApplication {
    std::string identifier;
    AppliedPresetKind kind{AppliedPresetKind::smart_material};
    std::string target_entry_identifier;
    AppliedPresetOrigin origin;
    SmartMaterialPreset fragment;
    std::vector<AppliedPresetParameterValue> parameter_values;
    friend bool operator==(const AppliedPresetApplication&,
                           const AppliedPresetApplication&) = default;
};

struct PresetApplicationReport {
    std::string application_identifier;
    AppliedPresetOrigin origin;
    std::vector<std::string> entry_identifiers;
    SmartMaterialContentReport content;
    friend bool operator==(const PresetApplicationReport&,
                           const PresetApplicationReport&) = default;
};

struct PresetApplicationUndoReport {
    bool removed{};
    std::string application_identifier;
    std::vector<std::string> entry_identifiers;
    friend bool operator==(const PresetApplicationUndoReport&,
                           const PresetApplicationUndoReport&) = default;
};

class TextureSetMemoryAccount {
public:
    ~TextureSetMemoryAccount();
    TextureSetMemoryAccount(const TextureSetMemoryAccount& other);
    TextureSetMemoryAccount& operator=(const TextureSetMemoryAccount& other);
    TextureSetMemoryAccount(TextureSetMemoryAccount&& other) noexcept;
    TextureSetMemoryAccount& operator=(TextureSetMemoryAccount&& other) noexcept;

    void set_resident_bytes(std::size_t bytes);
    [[nodiscard]] std::size_t resident_bytes() const noexcept { return resident_bytes_; }

private:
    friend class TextureSet;
    TextureSetMemoryAccount(std::shared_ptr<TextureSetMemoryState> state,
                            TextureSetMemoryCategory category);
    void release() noexcept;

    std::shared_ptr<TextureSetMemoryState> state_;
    TextureSetMemoryCategory category_{};
    std::size_t resident_bytes_{};
};

enum class PartitionSourceKind { material, object, submesh, explicit_faces };

struct TextureSetDescriptor {
    std::string display_name;
    PartitionSourceKind partition_kind;
    std::string partition_key;
    std::string uv_set;
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t default_bit_depth;
    bool udim_tiling{false};
};

struct UdimCoordinate {
    std::uint32_t u{};
    std::uint32_t v{};
    friend bool operator==(UdimCoordinate, UdimCoordinate) = default;
};

struct UdimPixelWrite {
    double u{};
    double v{};
    std::span<const std::byte> pixel;
};

struct UdimWriteResult {
    std::vector<std::uint32_t> changed_tiles;
    std::vector<std::uint32_t> allocated_tiles;
    std::size_t changed_pixel_count{};
};

struct AtlasRegion {
    std::string texture_set_identifier;
    std::uint32_t x{};
    std::uint32_t y{};
    std::uint32_t width{};
    std::uint32_t height{};
    friend bool operator==(const AtlasRegion&, const AtlasRegion&) = default;
};

struct AtlasDescriptor {
    std::string identifier;
    std::string display_name;
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<AtlasRegion> regions;
    friend bool operator==(const AtlasDescriptor&, const AtlasDescriptor&) = default;
};

[[nodiscard]] std::uint32_t udim_number(UdimCoordinate coordinate);
[[nodiscard]] UdimCoordinate udim_coordinate(std::uint32_t number);

class TextureSet {
public:
    explicit TextureSet(
        TextureSetDescriptor descriptor,
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());

    [[nodiscard]] std::string id() const { return {id_.begin(), id_.end()}; }
    [[nodiscard]] TextureSetDescriptor descriptor() const;
    [[nodiscard]] bool uses_udim_tiling() const noexcept { return udim_tiling_; }
    [[nodiscard]] TextureChannels& channels() noexcept { return channels_; }
    [[nodiscard]] const TextureChannels& channels() const noexcept { return channels_; }
    [[nodiscard]] UdimWriteResult write_udim_pixels(std::string_view semantic_id,
                                                    std::span<const UdimPixelWrite> writes);
    [[nodiscard]] std::vector<std::uint32_t> ensure_udim_tiles(
        std::span<const std::uint32_t> tile_numbers);
    [[nodiscard]] std::vector<std::byte> read_udim_pixel(std::string_view semantic_id,
                                                         std::uint32_t tile_number, std::uint32_t x,
                                                         std::uint32_t y) const;
    [[nodiscard]] std::vector<std::uint32_t> occupied_udim_tiles() const;
    [[nodiscard]] const TextureChannels& udim_channels(std::uint32_t tile_number) const;
    [[nodiscard]] bool can_clear_channels() const noexcept;
    void clear_channels();
    [[nodiscard]] LayerStack& layer_stack() noexcept { return layer_stack_; }
    [[nodiscard]] const LayerStack& layer_stack() const noexcept { return layer_stack_; }
    [[nodiscard]] LayerChannelParticipation channel_participation(
        std::string_view entry_identifier, std::string_view semantic_id,
        std::span<const LayerMaskSample> mask_samples = {}) const;
    [[nodiscard]] LayerCompositeResult composite_cpu(const LayerCompositeRequest& request) const;
    [[nodiscard]] LayerOperationResult apply_layer_operation(LayerOperationRequest request);
    void configure_tile_history_budget(std::size_t budget_bytes) {
        tile_history_.configure_budget(budget_bytes);
    }
    [[nodiscard]] TileHistoryBudgetReport tile_history_budget_report(
        std::size_t proposed_step_bytes = 0) const noexcept {
        return tile_history_.budget_report(proposed_step_bytes);
    }
    [[nodiscard]] TileHistoryCapture begin_tile_history_step(
        std::string step_identifier, std::span<const TileHistoryTarget> targets) const {
        return tile_history_.begin_step(channels_, std::move(step_identifier), targets);
    }
    [[nodiscard]] TileHistoryCommitResult commit_tile_history_step(TileHistoryCapture capture) {
        return tile_history_.commit_step(channels_, std::move(capture));
    }
    [[nodiscard]] TextureSetTransaction begin_transaction(
        std::string step_identifier, std::span<const TileHistoryTarget> targets = {});
    [[nodiscard]] TileHistoryRestoreResult undo_tiles() {
        return tile_history_.undo(channels_, layer_stack_);
    }
    [[nodiscard]] TileHistoryRestoreResult redo_tiles() {
        return tile_history_.redo(channels_, layer_stack_);
    }
    [[nodiscard]] TextureSetMemoryAccount create_memory_account(
        TextureSetMemoryCategory category) const;
    [[nodiscard]] TextureSetMemoryReport memory_report() const;
    [[nodiscard]] PresetApplicationReport apply_smart_material(const SmartMaterialPreset& preset,
                                                               std::string application_identifier);
    [[nodiscard]] PresetApplicationReport apply_smart_mask(
        const SmartMaskPreset& preset, std::string application_identifier,
        std::string_view target_entry_identifier);
    [[nodiscard]] SmartMaterialParameterUpdate set_applied_preset_parameter_value(
        std::string_view application_identifier, std::string_view parameter_identifier,
        graph::SocketValue value);
    [[nodiscard]] PresetApplicationUndoReport undo_last_preset_application();
    [[nodiscard]] std::size_t preset_application_count() const noexcept {
        return preset_applications_.size();
    }
    [[nodiscard]] std::size_t preset_undo_step_count() const noexcept {
        return preset_applications_.size();
    }
    [[nodiscard]] std::span<const AppliedPresetApplication> preset_applications() const noexcept {
        return preset_applications_;
    }
    [[nodiscard]] const SmartMaterialEntry& applied_entry(std::string_view entry_identifier) const;
    void replace_applied_entry(std::string_view entry_identifier, SmartMaterialEntry replacement);
    [[nodiscard]] const AppliedPresetOrigin& applied_entry_origin(
        std::string_view entry_identifier) const;

private:
    friend class TextureSetTransaction;
    std::pmr::memory_resource* memory_resource_;
    std::pmr::string display_name_;
    PartitionSourceKind partition_kind_;
    std::pmr::string partition_key_;
    std::pmr::string uv_set_;
    std::uint32_t width_;
    std::uint32_t height_;
    std::uint8_t default_bit_depth_;
    bool udim_tiling_;
    std::pmr::string id_;
    TextureChannels channels_;
    std::pmr::map<std::uint32_t, TextureChannels> udim_tiles_;
    LayerStack layer_stack_;
    TileHistory tile_history_;
    std::shared_ptr<TextureSetMemoryState> memory_state_;
    std::pmr::vector<AppliedPresetApplication> preset_applications_;
};

class TextureDocument {
public:
    explicit TextureDocument(
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());
    TextureSet& create_texture_set(TextureSetDescriptor descriptor);
    std::vector<std::string> create_texture_sets_from_mesh(const mesh::MeshView& mesh,
                                                           std::string_view uv_set,
                                                           std::uint32_t width,
                                                           std::uint32_t height,
                                                           std::uint8_t default_bit_depth);
    [[nodiscard]] bool contains_texture_set(std::string_view stable_id) const noexcept;
    [[nodiscard]] TextureSet& texture_set(std::string_view stable_id);
    [[nodiscard]] const TextureSet& texture_set(std::string_view stable_id) const;
    [[nodiscard]] std::vector<std::string> texture_set_ids() const;
    [[nodiscard]] std::size_t texture_set_count() const noexcept { return texture_sets_.size(); }
    [[nodiscard]] const AtlasDescriptor& create_atlas(AtlasDescriptor descriptor);
    [[nodiscard]] bool contains_atlas(std::string_view identifier) const noexcept;
    [[nodiscard]] const AtlasDescriptor& atlas(std::string_view identifier) const;
    [[nodiscard]] std::vector<std::string> atlas_ids() const;
    [[nodiscard]] std::size_t atlas_count() const noexcept { return atlases_.size(); }
    [[nodiscard]] TextureDocumentMemoryReport memory_report() const;

private:
    std::pmr::memory_resource* memory_resource_;
    std::pmr::map<std::pmr::string, TextureSet, std::less<>> texture_sets_;
    std::pmr::map<std::pmr::string, AtlasDescriptor, std::less<>> atlases_;
};

[[nodiscard]] std::string texture_set_stable_id(const TextureSetDescriptor& descriptor);

}  // namespace ctex::doc

#endif
