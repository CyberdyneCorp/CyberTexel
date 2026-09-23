#include <algorithm>
#include <cmath>
#include <ctex/doc/editable_authoring.hpp>
#include <limits>
#include <set>
#include <utility>

namespace ctex::doc {
namespace {

[[noreturn]] void fail(EditableAuthoringErrorCode code, std::string message) {
    throw EditableAuthoringError(code, std::move(message));
}

bool finite(std::span<const double> values) {
    return std::ranges::all_of(values, [](double value) { return std::isfinite(value); });
}

double length_squared(const std::array<double, 3>& value) {
    return value[0] * value[0] + value[1] * value[1] + value[2] * value[2];
}

std::vector<EditableTileDependency> invalidated_tiles(
    const std::optional<EditableAuthoringEntry>& before,
    const std::optional<EditableAuthoringEntry>& after) {
    std::vector<EditableTileDependency> result;
    if (before.has_value()) {
        result = before->dependent_tiles;
    }
    if (after.has_value()) {
        result.insert(result.end(), after->dependent_tiles.begin(), after->dependent_tiles.end());
    }
    std::ranges::sort(result, {}, [](const EditableTileDependency& value) {
        return std::tuple(value.semantic_id, value.tile_y, value.tile_x);
    });
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

auto find_entry(std::vector<EditableAuthoringEntry>& entries, std::string_view identifier) {
    return std::ranges::find(entries, identifier, &EditableAuthoringEntry::identifier);
}

auto find_entry(const std::vector<EditableAuthoringEntry>& entries, std::string_view identifier) {
    return std::ranges::find(entries, identifier, &EditableAuthoringEntry::identifier);
}

void replace_state(std::vector<EditableAuthoringEntry>& entries, std::string_view identifier,
                   const std::optional<EditableAuthoringEntry>& value) {
    auto found = find_entry(entries, identifier);
    if (value.has_value()) {
        if (found == entries.end()) {
            entries.push_back(*value);
        } else {
            *found = *value;
        }
    } else if (found != entries.end()) {
        entries.erase(found);
    }
    std::ranges::sort(entries, {}, &EditableAuthoringEntry::identifier);
}

}  // namespace

EditableAuthoringError::EditableAuthoringError(EditableAuthoringErrorCode code, std::string message)
    : std::runtime_error(std::move(message)), code_(code) {}

void validate_editable_authoring_entry(const EditableAuthoringEntry& entry) {
    if (entry.identifier.empty() || entry.material_identity.empty() || entry.revision == 0) {
        fail(EditableAuthoringErrorCode::invalid_entry,
             "editable entry requires identities and a non-zero revision");
    }
    if (entry.kind != EditableEntryKind::decal && entry.kind != EditableEntryKind::text &&
        entry.kind != EditableEntryKind::surface_path) {
        fail(EditableAuthoringErrorCode::invalid_entry, "editable entry kind is invalid");
    }
    if (!finite(entry.placement.position) || !finite(entry.placement.normal) ||
        !std::isfinite(entry.placement.rotation_radians) ||
        !std::isfinite(entry.placement.uniform_scale) || !finite(entry.placement.axis_scale) ||
        length_squared(entry.placement.normal) <= 1.0e-18 || entry.placement.uniform_scale <= 0.0 ||
        entry.placement.axis_scale[0] <= 0.0 || entry.placement.axis_scale[1] <= 0.0) {
        fail(EditableAuthoringErrorCode::invalid_entry,
             "editable placement frame is not finite and non-degenerate");
    }
    std::set<std::string_view> parameter_ids;
    for (const EditableMaterialParameter& parameter : entry.material_parameters) {
        if (parameter.identifier.empty() || parameter.component_count == 0 ||
            parameter.component_count > parameter.value.size() ||
            !finite(std::span(parameter.value).first(parameter.component_count)) ||
            !parameter_ids.insert(parameter.identifier).second) {
            fail(EditableAuthoringErrorCode::invalid_entry,
                 "editable material parameters are invalid or duplicated");
        }
    }
    std::set<std::tuple<std::string_view, std::uint32_t, std::uint32_t>> tile_ids;
    for (const EditableTileDependency& tile : entry.dependent_tiles) {
        if (tile.semantic_id.empty() ||
            !tile_ids.emplace(tile.semantic_id, tile.tile_x, tile.tile_y).second) {
            fail(EditableAuthoringErrorCode::invalid_entry,
                 "editable tile dependencies are invalid or duplicated");
        }
    }
    if (entry.dependent_tiles.empty()) {
        fail(EditableAuthoringErrorCode::invalid_entry,
             "editable entry requires at least one dependent tile");
    }
    if (entry.kind == EditableEntryKind::text &&
        (entry.text.empty() || entry.font_identity.empty())) {
        fail(EditableAuthoringErrorCode::invalid_entry,
             "editable text requires text and a supplied font identity");
    }
    if (entry.kind != EditableEntryKind::text &&
        (!entry.text.empty() || !entry.font_identity.empty())) {
        fail(EditableAuthoringErrorCode::invalid_entry,
             "only editable text may carry text and font identity");
    }
    if (entry.kind == EditableEntryKind::surface_path) {
        if (entry.mesh_revision == 0 || entry.surface_points.size() < 2) {
            fail(EditableAuthoringErrorCode::invalid_entry,
                 "editable surface path requires a mesh revision and two control points");
        }
        for (const EditableSurfacePoint& point : entry.surface_points) {
            const double barycentric_sum =
                point.barycentric[0] + point.barycentric[1] + point.barycentric[2];
            if (!finite(point.position) || !finite(point.normal) || !finite(point.barycentric) ||
                length_squared(point.normal) <= 1.0e-18 || !std::isfinite(point.width) ||
                point.width <= 0.0 || std::abs(barycentric_sum - 1.0) > 1.0e-6 ||
                std::ranges::any_of(point.barycentric,
                                    [](double value) { return value < 0.0 || value > 1.0; })) {
                fail(EditableAuthoringErrorCode::invalid_entry,
                     "editable surface path control point is invalid");
            }
        }
    } else if (entry.mesh_revision != 0 || !entry.surface_points.empty()) {
        fail(EditableAuthoringErrorCode::invalid_entry,
             "only editable surface paths may carry surface control points");
    }
}

EditableAuthoringStore::EditableAuthoringStore(std::vector<EditableAuthoringEntry> entries)
    : entries_(std::move(entries)) {
    std::set<std::string_view> identities;
    for (const EditableAuthoringEntry& value : entries_) {
        validate_editable_authoring_entry(value);
        if (!identities.insert(value.identifier).second) {
            fail(EditableAuthoringErrorCode::duplicate_entry,
                 "editable entry identity is duplicated: " + value.identifier);
        }
    }
    std::ranges::sort(entries_, {}, &EditableAuthoringEntry::identifier);
}

EditableMutationReport EditableAuthoringStore::add(EditableAuthoringEntry value) {
    validate_editable_authoring_entry(value);
    if (find_entry(entries_, value.identifier) != entries_.end()) {
        fail(EditableAuthoringErrorCode::duplicate_entry,
             "editable entry already exists: " + value.identifier);
    }
    if (revision_ == std::numeric_limits<std::uint64_t>::max()) {
        fail(EditableAuthoringErrorCode::stale_entry, "editable document revision is exhausted");
    }
    Command command{.identifier = value.identifier, .before = std::nullopt, .after = value};
    replace_state(entries_, command.identifier, command.after);
    redo_.clear();
    undo_.push_back(command);
    ++revision_;
    return {.identifier = command.identifier,
            .document_revision = revision_,
            .entry_revision = value.revision,
            .entry_present = true,
            .invalidated_tiles = invalidated_tiles(command.before, command.after)};
}

EditableMutationReport EditableAuthoringStore::edit(EditableAuthoringEntry replacement,
                                                    std::uint64_t expected_revision) {
    auto found = find_entry(entries_, replacement.identifier);
    if (found == entries_.end()) {
        fail(EditableAuthoringErrorCode::missing_entry,
             "editable entry is missing: " + replacement.identifier);
    }
    if (found->revision != expected_revision || expected_revision == 0 ||
        found->revision == std::numeric_limits<std::uint64_t>::max()) {
        fail(EditableAuthoringErrorCode::stale_entry,
             "editable entry revision does not match the edit precondition");
    }
    if (replacement.kind != found->kind) {
        fail(EditableAuthoringErrorCode::invalid_entry,
             "editable entry kind cannot change during an edit");
    }
    replacement.revision = found->revision + 1;
    validate_editable_authoring_entry(replacement);
    if (revision_ == std::numeric_limits<std::uint64_t>::max()) {
        fail(EditableAuthoringErrorCode::stale_entry, "editable document revision is exhausted");
    }
    Command command{.identifier = replacement.identifier, .before = *found, .after = replacement};
    const auto invalidated = invalidated_tiles(command.before, command.after);
    *found = replacement;
    redo_.clear();
    undo_.push_back(command);
    ++revision_;
    return {.identifier = replacement.identifier,
            .document_revision = revision_,
            .entry_revision = replacement.revision,
            .entry_present = true,
            .invalidated_tiles = invalidated};
}

EditableMutationReport EditableAuthoringStore::remove(std::string_view identifier,
                                                      std::uint64_t expected_revision) {
    auto found = find_entry(entries_, identifier);
    if (found == entries_.end()) {
        fail(EditableAuthoringErrorCode::missing_entry,
             "editable entry is missing: " + std::string(identifier));
    }
    if (found->revision != expected_revision || expected_revision == 0) {
        fail(EditableAuthoringErrorCode::stale_entry,
             "editable entry revision does not match the remove precondition");
    }
    if (revision_ == std::numeric_limits<std::uint64_t>::max()) {
        fail(EditableAuthoringErrorCode::stale_entry, "editable document revision is exhausted");
    }
    Command command{.identifier = found->identifier, .before = *found, .after = std::nullopt};
    const auto invalidated = invalidated_tiles(command.before, command.after);
    entries_.erase(found);
    redo_.clear();
    undo_.push_back(command);
    ++revision_;
    return {.identifier = command.identifier,
            .document_revision = revision_,
            .entry_revision = 0,
            .entry_present = false,
            .invalidated_tiles = invalidated};
}

EditableMutationReport EditableAuthoringStore::apply_history(
    std::vector<Command>& source, std::vector<Command>& destination,
    EditableAuthoringErrorCode empty_code) {
    if (source.empty()) {
        fail(empty_code, empty_code == EditableAuthoringErrorCode::no_undo
                             ? "editable authoring has no undo step"
                             : "editable authoring has no redo step");
    }
    if (revision_ == std::numeric_limits<std::uint64_t>::max()) {
        fail(EditableAuthoringErrorCode::stale_entry, "editable document revision is exhausted");
    }
    Command command = std::move(source.back());
    source.pop_back();
    const bool undoing = empty_code == EditableAuthoringErrorCode::no_undo;
    const std::optional<EditableAuthoringEntry>& target = undoing ? command.before : command.after;
    replace_state(entries_, command.identifier, target);
    const auto invalidated = invalidated_tiles(command.before, command.after);
    destination.push_back(std::move(command));
    ++revision_;
    return {.identifier = destination.back().identifier,
            .document_revision = revision_,
            .entry_revision = target.has_value() ? target->revision : 0,
            .entry_present = target.has_value(),
            .invalidated_tiles = invalidated};
}

EditableMutationReport EditableAuthoringStore::undo() {
    return apply_history(undo_, redo_, EditableAuthoringErrorCode::no_undo);
}

EditableMutationReport EditableAuthoringStore::redo() {
    return apply_history(redo_, undo_, EditableAuthoringErrorCode::no_redo);
}

const EditableAuthoringEntry& EditableAuthoringStore::entry(std::string_view identifier) const {
    const auto found = find_entry(entries_, identifier);
    if (found == entries_.end()) {
        fail(EditableAuthoringErrorCode::missing_entry,
             "editable entry is missing: " + std::string(identifier));
    }
    return *found;
}

EditableRasterizationPlan EditableAuthoringStore::rasterization_plan(
    std::string_view identifier) const {
    const EditableAuthoringEntry& value = entry(identifier);
    return {
        .entry = value, .document_revision = revision_, .invalidated_tiles = value.dependent_tiles};
}

}  // namespace ctex::doc
