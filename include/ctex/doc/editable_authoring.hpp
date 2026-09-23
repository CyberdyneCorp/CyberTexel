#ifndef CTEX_DOC_EDITABLE_AUTHORING_HPP
#define CTEX_DOC_EDITABLE_AUTHORING_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::doc {

enum class EditableEntryKind : std::uint8_t { decal = 0, text = 1, surface_path = 2 };

struct EditablePlacementFrame {
    std::array<double, 3> position{};
    std::array<double, 3> normal{0.0, 0.0, 1.0};
    double rotation_radians{};
    double uniform_scale{1.0};
    std::array<double, 2> axis_scale{1.0, 1.0};
    friend bool operator==(const EditablePlacementFrame&, const EditablePlacementFrame&) = default;
};

struct EditableMaterialParameter {
    std::string identifier;
    std::uint8_t component_count{};
    std::array<double, 4> value{};
    friend bool operator==(const EditableMaterialParameter&,
                           const EditableMaterialParameter&) = default;
};

struct EditableTileDependency {
    std::string semantic_id;
    std::uint32_t tile_x{};
    std::uint32_t tile_y{};
    friend bool operator==(const EditableTileDependency&, const EditableTileDependency&) = default;
};

struct EditableSurfacePoint {
    std::array<double, 3> position{};
    std::array<double, 3> normal{0.0, 0.0, 1.0};
    std::uint32_t triangle{};
    std::array<double, 3> barycentric{1.0, 0.0, 0.0};
    double width{1.0};
    friend bool operator==(const EditableSurfacePoint&, const EditableSurfacePoint&) = default;
};

struct EditableAuthoringEntry {
    std::string identifier;
    EditableEntryKind kind{EditableEntryKind::decal};
    std::uint64_t revision{1};
    EditablePlacementFrame placement;
    std::string material_identity;
    std::vector<EditableMaterialParameter> material_parameters;
    std::string text;
    std::string font_identity;
    std::uint64_t mesh_revision{};
    std::vector<EditableSurfacePoint> surface_points;
    std::vector<EditableTileDependency> dependent_tiles;
    friend bool operator==(const EditableAuthoringEntry&, const EditableAuthoringEntry&) = default;
};

enum class EditableAuthoringErrorCode : std::uint8_t {
    invalid_entry,
    duplicate_entry,
    missing_entry,
    stale_entry,
    no_undo,
    no_redo,
};

class EditableAuthoringError final : public std::runtime_error {
public:
    EditableAuthoringError(EditableAuthoringErrorCode code, std::string message);
    [[nodiscard]] EditableAuthoringErrorCode code() const noexcept { return code_; }

private:
    EditableAuthoringErrorCode code_;
};

struct EditableMutationReport {
    std::string identifier;
    std::uint64_t document_revision{};
    std::uint64_t entry_revision{};
    bool entry_present{};
    std::vector<EditableTileDependency> invalidated_tiles;
};

struct EditableRasterizationPlan {
    EditableAuthoringEntry entry;
    std::uint64_t document_revision{};
    std::vector<EditableTileDependency> invalidated_tiles;
};

class EditableAuthoringStore {
public:
    EditableAuthoringStore() = default;
    explicit EditableAuthoringStore(std::vector<EditableAuthoringEntry> entries);

    [[nodiscard]] EditableMutationReport add(EditableAuthoringEntry entry);
    [[nodiscard]] EditableMutationReport edit(EditableAuthoringEntry replacement,
                                              std::uint64_t expected_revision);
    [[nodiscard]] EditableMutationReport remove(std::string_view identifier,
                                                std::uint64_t expected_revision);
    [[nodiscard]] EditableMutationReport undo();
    [[nodiscard]] EditableMutationReport redo();

    [[nodiscard]] const EditableAuthoringEntry& entry(std::string_view identifier) const;
    [[nodiscard]] std::span<const EditableAuthoringEntry> entries() const noexcept {
        return entries_;
    }
    [[nodiscard]] EditableRasterizationPlan rasterization_plan(std::string_view identifier) const;
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }
    [[nodiscard]] std::size_t undo_step_count() const noexcept { return undo_.size(); }
    [[nodiscard]] std::size_t redo_step_count() const noexcept { return redo_.size(); }

private:
    struct Command {
        std::string identifier;
        std::optional<EditableAuthoringEntry> before;
        std::optional<EditableAuthoringEntry> after;
    };

    [[nodiscard]] EditableMutationReport apply_history(std::vector<Command>& source,
                                                       std::vector<Command>& destination,
                                                       EditableAuthoringErrorCode empty_code);

    std::vector<EditableAuthoringEntry> entries_;
    std::uint64_t revision_{};
    std::vector<Command> undo_;
    std::vector<Command> redo_;
};

void validate_editable_authoring_entry(const EditableAuthoringEntry& entry);

}  // namespace ctex::doc

#endif
