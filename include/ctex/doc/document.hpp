#ifndef CTEX_DOC_DOCUMENT_HPP
#define CTEX_DOC_DOCUMENT_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/doc/channels.hpp>
#include <ctex/doc/smart_mask.hpp>
#include <map>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
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
};

class TextureSet {
public:
    explicit TextureSet(
        TextureSetDescriptor descriptor,
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());

    [[nodiscard]] std::string id() const { return {id_.begin(), id_.end()}; }
    [[nodiscard]] TextureSetDescriptor descriptor() const;
    [[nodiscard]] TextureChannels& channels() noexcept { return channels_; }
    [[nodiscard]] const TextureChannels& channels() const noexcept { return channels_; }
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
    std::pmr::memory_resource* memory_resource_;
    std::pmr::string display_name_;
    PartitionSourceKind partition_kind_;
    std::pmr::string partition_key_;
    std::pmr::string uv_set_;
    std::uint32_t width_;
    std::uint32_t height_;
    std::uint8_t default_bit_depth_;
    std::pmr::string id_;
    TextureChannels channels_;
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
    [[nodiscard]] TextureDocumentMemoryReport memory_report() const;

private:
    std::pmr::memory_resource* memory_resource_;
    std::pmr::map<std::pmr::string, TextureSet, std::less<>> texture_sets_;
};

[[nodiscard]] std::string texture_set_stable_id(const TextureSetDescriptor& descriptor);

}  // namespace ctex::doc

#endif
