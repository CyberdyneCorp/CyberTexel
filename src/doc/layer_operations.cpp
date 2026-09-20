#include <algorithm>
#include <cmath>
#include <ctex/doc/document.hpp>
#include <limits>
#include <map>
#include <set>
#include <type_traits>
#include <utility>

namespace ctex::doc {
namespace {

[[noreturn]] void fail(LayerOperationErrorCode code, std::string message) {
    throw LayerOperationError(code, std::move(message));
}

bool is_attachment(LayerEntryKind kind) {
    return kind == LayerEntryKind::mask || kind == LayerEntryKind::filter;
}

bool owns_raster(LayerEntryKind kind) {
    return kind == LayerEntryKind::paint_layer || kind == LayerEntryKind::fill_layer ||
           kind == LayerEntryKind::filter || kind == LayerEntryKind::editable_decal ||
           kind == LayerEntryKind::editable_text || kind == LayerEntryKind::surface_path;
}

bool can_destructively_edit(LayerEntryKind kind) {
    return owns_raster(kind) && kind != LayerEntryKind::filter;
}

bool can_merge(LayerEntryKind kind) {
    return kind != LayerEntryKind::mask && kind != LayerEntryKind::filter;
}

std::size_t index_of(std::span<const LayerEntry> entries, std::string_view identifier) {
    const auto found = std::ranges::find(entries, identifier, &LayerEntry::identifier);
    if (found == entries.end()) {
        fail(LayerOperationErrorCode::invalid_operation,
             "layer operation identity is not present: " + std::string(identifier));
    }
    return static_cast<std::size_t>(std::distance(entries.begin(), found));
}

std::set<std::string, std::less<>> ownership_closure(std::span<const LayerEntry> entries,
                                                     std::string_view identifier) {
    static_cast<void>(index_of(entries, identifier));
    std::set<std::string, std::less<>> result{std::string(identifier)};
    bool changed = true;
    while (changed) {
        changed = false;
        for (const LayerEntry& entry : entries) {
            const bool owned_child =
                !entry.parent_identifier.empty() && result.contains(entry.parent_identifier);
            const bool owned_attachment =
                is_attachment(entry.kind) && result.contains(entry.target_identifier);
            if ((owned_child || owned_attachment) && result.insert(entry.identifier).second) {
                changed = true;
            }
        }
    }
    return result;
}

std::vector<std::string> ordered_identifiers(std::span<const LayerEntry> entries,
                                             const std::set<std::string, std::less<>>& selected) {
    std::vector<std::string> result;
    for (const LayerEntry& entry : entries) {
        if (selected.contains(entry.identifier)) {
            result.push_back(entry.identifier);
        }
    }
    return result;
}

void erase_snapshot(LayerCompositeRequest& snapshot,
                    const std::set<std::string, std::less<>>& identifiers) {
    std::erase_if(snapshot.content, [&](const LayerCompositeRaster& raster) {
        return identifiers.contains(raster.entry_identifier);
    });
    std::erase_if(snapshot.masks, [&](const LayerCompositeMaskRaster& raster) {
        return identifiers.contains(raster.mask_identifier);
    });
}

void append_replacements(LayerCompositeRequest& snapshot,
                         std::vector<LayerCompositeRaster> replacements) {
    snapshot.content.insert(snapshot.content.end(), std::make_move_iterator(replacements.begin()),
                            std::make_move_iterator(replacements.end()));
}

std::size_t checked_add(std::size_t left, std::size_t right) {
    if (right > std::numeric_limits<std::size_t>::max() - left) {
        fail(LayerOperationErrorCode::allocation_limit,
             "layer operation raster byte count exceeds addressable storage");
    }
    return left + right;
}

std::size_t snapshot_bytes(const LayerCompositeRequest& snapshot) {
    std::size_t result = 0;
    for (const LayerCompositeRaster& raster : snapshot.content) {
        result = checked_add(result, raster.pixels.size() * sizeof(graph::ColourValue));
        result = checked_add(result, raster.coverage.size() * sizeof(float));
    }
    for (const LayerCompositeMaskRaster& raster : snapshot.masks) {
        result = checked_add(result, raster.values.size() * sizeof(double));
    }
    return result;
}

void enforce_budget(const LayerCompositeRequest& snapshot, std::size_t limit) {
    if (snapshot_bytes(snapshot) > limit) {
        fail(LayerOperationErrorCode::allocation_limit,
             "layer operation resolved content exceeds the declared output byte limit");
    }
}

std::vector<LayerEntry::ChannelModulation> replacement_channels(
    std::span<const LayerCompositeRaster> replacements, std::string_view identifier) {
    std::set<std::string, std::less<>> seen;
    std::vector<LayerEntry::ChannelModulation> result;
    for (const LayerCompositeRaster& raster : replacements) {
        if (raster.entry_identifier != identifier || !seen.insert(raster.semantic_id).second) {
            fail(LayerOperationErrorCode::invalid_content,
                 "layer operation replacement rasters require the output identity and unique "
                 "channels");
        }
        result.push_back({.semantic_id = raster.semantic_id, .enabled = true, .opacity = 1.0});
    }
    if (result.empty()) {
        fail(LayerOperationErrorCode::invalid_content,
             "layer operation replacement content must not be empty");
    }
    return result;
}

void make_baked_paint(LayerEntry& entry, std::span<const LayerCompositeRaster> replacements) {
    entry.kind = LayerEntryKind::paint_layer;
    entry.source_identifier.clear();
    entry.graph.reset();
    entry.enabled = true;
    entry.opacity = 1.0;
    entry.blend_mode = "normal";
    entry.channels = replacement_channels(replacements, entry.identifier);
    if (entry.content_revision == std::numeric_limits<std::uint64_t>::max()) {
        fail(LayerOperationErrorCode::invalid_operation,
             "layer operation content revision is exhausted");
    }
    ++entry.content_revision;
}

void advance_revision(LayerEntry& entry) {
    if (entry.content_revision == std::numeric_limits<std::uint64_t>::max()) {
        fail(LayerOperationErrorCode::invalid_operation,
             "layer operation content revision is exhausted");
    }
    ++entry.content_revision;
}

void make_authored_paint(LayerEntry& entry) {
    entry.kind = LayerEntryKind::paint_layer;
    entry.source_identifier.clear();
    entry.graph.reset();
    advance_revision(entry);
}

bool close(float left, float right, float tolerance) { return std::abs(left - right) <= tolerance; }

bool same_appearance(const LayerCompositeResult& left, const LayerCompositeResult& right,
                     float tolerance) {
    if (left.width != right.width || left.height != right.height ||
        left.channels.size() != right.channels.size()) {
        return false;
    }
    for (std::size_t channel = 0; channel < left.channels.size(); ++channel) {
        const LayerCompositeChannel& a = left.channels[channel];
        const LayerCompositeChannel& b = right.channels[channel];
        if (a.semantic_id != b.semantic_id || a.component_count != b.component_count ||
            a.pixels.size() != b.pixels.size()) {
            return false;
        }
        for (std::size_t pixel = 0; pixel < a.pixels.size(); ++pixel) {
            if (!close(a.pixels[pixel].r, b.pixels[pixel].r, tolerance) ||
                !close(a.pixels[pixel].g, b.pixels[pixel].g, tolerance) ||
                !close(a.pixels[pixel].b, b.pixels[pixel].b, tolerance) ||
                !close(a.pixels[pixel].a, b.pixels[pixel].a, tolerance)) {
                return false;
            }
        }
    }
    return true;
}

struct Candidate {
    std::vector<LayerEntry> entries;
    LayerCompositeRequest snapshot;
    std::vector<std::string> affected;
    bool preserves_appearance{};
};

void create_layer(Candidate& candidate, CreateLayerOperation operation) {
    const std::string identifier = operation.entry.identifier;
    candidate.entries.push_back(std::move(operation.entry));
    candidate.snapshot.content.insert(candidate.snapshot.content.end(),
                                      std::make_move_iterator(operation.content.begin()),
                                      std::make_move_iterator(operation.content.end()));
    candidate.snapshot.masks.insert(candidate.snapshot.masks.end(),
                                    std::make_move_iterator(operation.masks.begin()),
                                    std::make_move_iterator(operation.masks.end()));
    candidate.affected.push_back(identifier);
}

void duplicate_layer(Candidate& candidate, const DuplicateLayerOperation& operation) {
    if (operation.duplicate_identifier.empty()) {
        fail(LayerOperationErrorCode::invalid_operation,
             "layer duplicate requires a non-empty new identity");
    }
    const std::set<std::string, std::less<>> selected =
        ownership_closure(candidate.entries, operation.identifier);
    std::map<std::string, std::string, std::less<>> identities;
    identities.emplace(operation.identifier, operation.duplicate_identifier);
    for (const std::string& identifier : selected) {
        if (identifier != operation.identifier) {
            identities.emplace(identifier, operation.duplicate_identifier + "/" + identifier);
        }
    }
    std::vector<LayerEntry> copies;
    for (const LayerEntry& entry : candidate.entries) {
        if (!selected.contains(entry.identifier)) {
            continue;
        }
        LayerEntry copy = entry;
        copy.identifier = identities.at(entry.identifier);
        copy.display_name += " Copy";
        if (identities.contains(copy.parent_identifier)) {
            copy.parent_identifier = identities.at(copy.parent_identifier);
        }
        if (identities.contains(copy.target_identifier)) {
            copy.target_identifier = identities.at(copy.target_identifier);
        }
        if (identities.contains(copy.source_identifier)) {
            copy.source_identifier = identities.at(copy.source_identifier);
        }
        candidate.affected.push_back(copy.identifier);
        copies.push_back(std::move(copy));
    }
    candidate.entries.insert(candidate.entries.end(), copies.begin(), copies.end());
    const std::vector<LayerCompositeRaster> content = candidate.snapshot.content;
    for (LayerCompositeRaster raster : content) {
        const auto found = identities.find(raster.entry_identifier);
        if (found != identities.end()) {
            raster.entry_identifier = found->second;
            candidate.snapshot.content.push_back(std::move(raster));
        }
    }
    const std::vector<LayerCompositeMaskRaster> masks = candidate.snapshot.masks;
    for (LayerCompositeMaskRaster raster : masks) {
        const auto found = identities.find(raster.mask_identifier);
        if (found != identities.end()) {
            raster.mask_identifier = found->second;
            candidate.snapshot.masks.push_back(std::move(raster));
        }
    }
}

void preserve_instance_content(Candidate& candidate,
                               const std::set<std::string, std::less<>>& removed) {
    LayerStack original;
    original.assign(candidate.entries);
    const std::vector<LayerCompositeRaster> content = candidate.snapshot.content;
    for (const LayerEntry& entry : candidate.entries) {
        if (entry.kind != LayerEntryKind::instance || removed.contains(entry.identifier)) {
            continue;
        }
        const LayerEntry& source = original.resolved_content(entry.identifier);
        if (!removed.contains(source.identifier)) {
            continue;
        }
        if (source.kind == LayerEntryKind::group) {
            fail(LayerOperationErrorCode::invalid_content,
                 "making a group instance independent requires baked resolved content");
        }
        bool copied = false;
        for (LayerCompositeRaster raster : content) {
            if (raster.entry_identifier == source.identifier) {
                raster.entry_identifier = entry.identifier;
                candidate.snapshot.content.push_back(std::move(raster));
                copied = true;
            }
        }
        if (!copied && owns_raster(source.kind)) {
            fail(LayerOperationErrorCode::invalid_content,
                 "making an instance independent requires its resolved source rasters");
        }
    }
}

void delete_layer(Candidate& candidate, const DeleteLayerOperation& operation) {
    const std::set<std::string, std::less<>> removed =
        ownership_closure(candidate.entries, operation.identifier);
    if (operation.instance_policy == ReferencedSourceDeletionPolicy::make_instances_independent) {
        preserve_instance_content(candidate, removed);
    }
    LayerStack stack;
    stack.assign(candidate.entries);
    const std::vector<std::string> identifiers = ordered_identifiers(candidate.entries, removed);
    stack.remove(identifiers, operation.instance_policy);
    candidate.entries.assign(stack.entries().begin(), stack.entries().end());
    erase_snapshot(candidate.snapshot, removed);
    candidate.affected = identifiers;
}

void move_layer(Candidate& candidate, std::string_view identifier,
                std::string_view parent_identifier, std::string_view before_identifier,
                bool change_parent) {
    const std::size_t root_index = index_of(candidate.entries, identifier);
    if (is_attachment(candidate.entries[root_index].kind)) {
        fail(LayerOperationErrorCode::invalid_operation,
             "layer reorder and reparent require a layer or group identity");
    }
    const std::string target_parent = change_parent
                                          ? std::string(parent_identifier)
                                          : candidate.entries[root_index].parent_identifier;
    if (!before_identifier.empty()) {
        const LayerEntry& before =
            candidate.entries[index_of(candidate.entries, before_identifier)];
        if (is_attachment(before.kind) || before.parent_identifier != target_parent) {
            fail(LayerOperationErrorCode::invalid_operation,
                 "layer placement anchor must be a sibling in the destination scope");
        }
    }
    const std::set<std::string, std::less<>> selected =
        ownership_closure(candidate.entries, identifier);
    if (selected.contains(before_identifier)) {
        fail(LayerOperationErrorCode::invalid_operation,
             "layer placement anchor cannot be inside the moved ownership subtree");
    }
    std::vector<LayerEntry> moved;
    std::vector<LayerEntry> retained;
    for (LayerEntry& entry : candidate.entries) {
        (selected.contains(entry.identifier) ? moved : retained).push_back(std::move(entry));
    }
    if (change_parent) {
        moved.front().parent_identifier = target_parent;
    }
    const auto insertion =
        before_identifier.empty()
            ? retained.end()
            : std::ranges::find(retained, before_identifier, &LayerEntry::identifier);
    retained.insert(insertion, std::make_move_iterator(moved.begin()),
                    std::make_move_iterator(moved.end()));
    candidate.entries = std::move(retained);
    candidate.affected = ordered_identifiers(candidate.entries, selected);
}

void clear_or_invert(Candidate& candidate, std::string_view identifier,
                     std::string_view semantic_id, bool invert, const TextureSet& texture_set) {
    const std::size_t entry_index = index_of(candidate.entries, identifier);
    LayerEntry& entry = candidate.entries[entry_index];
    if (!can_destructively_edit(entry.kind)) {
        fail(LayerOperationErrorCode::invalid_operation,
             "clear and invert require authored or resolved raster content");
    }
    bool changed = false;
    for (LayerCompositeRaster& raster : candidate.snapshot.content) {
        if (raster.entry_identifier != identifier ||
            (!semantic_id.empty() && raster.semantic_id != semantic_id)) {
            continue;
        }
        const ChannelDescriptor& descriptor = texture_set.channels().descriptor(raster.semantic_id);
        if (invert) {
            for (graph::ColourValue& pixel : raster.pixels) {
                float* components[] = {&pixel.r, &pixel.g, &pixel.b, &pixel.a};
                for (std::size_t component = 0; component < descriptor.component_count;
                     ++component) {
                    *components[component] = 1.0F - *components[component];
                }
            }
        } else {
            std::ranges::fill(raster.pixels, graph::ColourValue{});
            raster.coverage.assign(raster.pixels.size(), 0.0F);
        }
        changed = true;
    }
    if (!changed) {
        fail(LayerOperationErrorCode::invalid_content,
             "clear or invert did not find the requested resolved channel content");
    }
    make_authored_paint(entry);
    candidate.affected.push_back(std::string(identifier));
}

std::vector<std::size_t> sibling_indices(std::span<const LayerEntry> entries,
                                         std::string_view parent_identifier) {
    std::vector<std::size_t> result;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (!is_attachment(entries[index].kind) &&
            entries[index].parent_identifier == parent_identifier) {
            result.push_back(index);
        }
    }
    return result;
}

void merge_down(Candidate& candidate, MergeDownLayerOperation operation) {
    const std::size_t selected_index = index_of(candidate.entries, operation.identifier);
    const LayerEntry& selected = candidate.entries[selected_index];
    if (!can_merge(selected.kind)) {
        fail(LayerOperationErrorCode::invalid_operation,
             "merge down requires a raster content layer");
    }
    const std::vector<std::size_t> siblings =
        sibling_indices(candidate.entries, selected.parent_identifier);
    const auto position = std::ranges::find(siblings, selected_index);
    if (position == siblings.end() || position == siblings.begin()) {
        fail(LayerOperationErrorCode::invalid_operation,
             "merge down requires a lower sibling in the same scope");
    }
    const std::size_t lower_index = *(position - 1);
    if (!can_merge(candidate.entries[lower_index].kind)) {
        fail(LayerOperationErrorCode::invalid_operation,
             "merge down lower sibling must be raster content");
    }
    const std::string lower_identifier = candidate.entries[lower_index].identifier;
    const auto selected_closure = ownership_closure(candidate.entries, operation.identifier);
    const auto lower_closure = ownership_closure(candidate.entries, lower_identifier);
    std::set<std::string, std::less<>> removed = selected_closure;
    removed.insert(lower_closure.begin(), lower_closure.end());
    removed.erase(lower_identifier);
    std::vector<LayerEntry> result;
    for (LayerEntry entry : candidate.entries) {
        if (entry.identifier == lower_identifier) {
            make_baked_paint(entry, operation.replacement_content);
            result.push_back(std::move(entry));
        } else if (!removed.contains(entry.identifier)) {
            result.push_back(std::move(entry));
        }
    }
    std::set<std::string, std::less<>> replaced = removed;
    replaced.insert(lower_identifier);
    erase_snapshot(candidate.snapshot, replaced);
    append_replacements(candidate.snapshot, std::move(operation.replacement_content));
    candidate.entries = std::move(result);
    candidate.affected = ordered_identifiers(candidate.entries, {lower_identifier});
    candidate.affected.push_back(operation.identifier);
    candidate.preserves_appearance = true;
}

void merge_group(Candidate& candidate, MergeGroupLayerOperation operation) {
    const std::size_t group_index = index_of(candidate.entries, operation.identifier);
    if (candidate.entries[group_index].kind != LayerEntryKind::group) {
        fail(LayerOperationErrorCode::invalid_operation, "merge group requires a group identity");
    }
    const auto closure = ownership_closure(candidate.entries, operation.identifier);
    std::vector<LayerEntry> result;
    for (LayerEntry entry : candidate.entries) {
        if (entry.identifier == operation.identifier) {
            make_baked_paint(entry, operation.replacement_content);
            result.push_back(std::move(entry));
        } else if (!closure.contains(entry.identifier)) {
            result.push_back(std::move(entry));
        }
    }
    erase_snapshot(candidate.snapshot, closure);
    append_replacements(candidate.snapshot, std::move(operation.replacement_content));
    candidate.entries = std::move(result);
    candidate.affected = ordered_identifiers(candidate.entries, {operation.identifier});
    candidate.preserves_appearance = true;
}

void flatten_layers(Candidate& candidate, FlattenLayersOperation operation) {
    if (operation.output_entry.kind != LayerEntryKind::paint_layer ||
        !operation.output_entry.parent_identifier.empty() ||
        !operation.output_entry.target_identifier.empty() ||
        !operation.output_entry.source_identifier.empty()) {
        fail(LayerOperationErrorCode::invalid_operation,
             "flatten output must be a root paint layer");
    }
    make_baked_paint(operation.output_entry, operation.replacement_content);
    candidate.affected.clear();
    for (const LayerEntry& entry : candidate.entries) {
        candidate.affected.push_back(entry.identifier);
    }
    candidate.affected.push_back(operation.output_entry.identifier);
    candidate.entries = {std::move(operation.output_entry)};
    candidate.snapshot.content.clear();
    candidate.snapshot.masks.clear();
    append_replacements(candidate.snapshot, std::move(operation.replacement_content));
    candidate.preserves_appearance = true;
}

void convert_layer(Candidate& candidate, ConvertLayerOperation operation) {
    LayerEntry& entry = candidate.entries[index_of(candidate.entries, operation.identifier)];
    const bool paint_to_fill = entry.kind == LayerEntryKind::paint_layer &&
                               operation.target_kind == LayerEntryKind::fill_layer;
    const bool fill_to_paint = entry.kind == LayerEntryKind::fill_layer &&
                               operation.target_kind == LayerEntryKind::paint_layer;
    if ((!paint_to_fill && !fill_to_paint) || (paint_to_fill && !operation.fill_graph) ||
        (fill_to_paint && operation.fill_graph)) {
        fail(LayerOperationErrorCode::invalid_operation,
             "convert supports paint/fill transitions and requires a graph only for fill output");
    }
    entry.kind = operation.target_kind;
    entry.graph = std::move(operation.fill_graph);
    advance_revision(entry);
    candidate.affected.push_back(entry.identifier);
    candidate.preserves_appearance = true;
}

void apply_mask(Candidate& candidate, ApplyMaskLayerOperation operation) {
    const std::size_t mask_index = index_of(candidate.entries, operation.mask_identifier);
    const LayerEntry mask = candidate.entries[mask_index];
    if (mask.kind != LayerEntryKind::mask) {
        fail(LayerOperationErrorCode::invalid_operation,
             "apply mask requires a mask attachment identity");
    }
    const std::size_t target_index = index_of(candidate.entries, mask.target_identifier);
    const auto closure = ownership_closure(candidate.entries, mask.target_identifier);
    LayerEntry target = candidate.entries[target_index];
    if (target.kind == LayerEntryKind::group) {
        std::vector<LayerEntry> result;
        for (const LayerEntry& entry : candidate.entries) {
            if (entry.identifier == target.identifier) {
                make_baked_paint(target, operation.replacement_content);
                result.push_back(target);
            } else if (!closure.contains(entry.identifier)) {
                result.push_back(entry);
            }
        }
        candidate.entries = std::move(result);
        erase_snapshot(candidate.snapshot, closure);
    } else {
        if (!can_merge(target.kind)) {
            fail(LayerOperationErrorCode::invalid_operation,
                 "apply mask target must own raster content or be a group");
        }
        make_baked_paint(target, operation.replacement_content);
        candidate.entries[target_index] = std::move(target);
        candidate.entries.erase(candidate.entries.begin() +
                                static_cast<std::ptrdiff_t>(mask_index));
        erase_snapshot(candidate.snapshot,
                       std::set<std::string, std::less<>>{mask.identifier, mask.target_identifier});
    }
    append_replacements(candidate.snapshot, std::move(operation.replacement_content));
    candidate.affected = {mask.target_identifier, mask.identifier};
    candidate.preserves_appearance = true;
}

void apply_operation(Candidate& candidate, LayerOperation operation,
                     const TextureSet& texture_set) {
    std::visit(
        [&](auto value) {
            using Value = decltype(value);
            if constexpr (std::is_same_v<Value, CreateLayerOperation>) {
                create_layer(candidate, std::move(value));
            } else if constexpr (std::is_same_v<Value, DuplicateLayerOperation>) {
                duplicate_layer(candidate, value);
            } else if constexpr (std::is_same_v<Value, DeleteLayerOperation>) {
                delete_layer(candidate, value);
            } else if constexpr (std::is_same_v<Value, ReorderLayerOperation>) {
                const std::string parent =
                    candidate.entries[index_of(candidate.entries, value.identifier)]
                        .parent_identifier;
                move_layer(candidate, value.identifier, parent, value.before_identifier, false);
            } else if constexpr (std::is_same_v<Value, ReparentLayerOperation>) {
                move_layer(candidate, value.identifier, value.parent_identifier,
                           value.before_identifier, true);
            } else if constexpr (std::is_same_v<Value, ClearLayerOperation>) {
                clear_or_invert(candidate, value.identifier, value.semantic_id, false, texture_set);
            } else if constexpr (std::is_same_v<Value, InvertLayerOperation>) {
                clear_or_invert(candidate, value.identifier, value.semantic_id, true, texture_set);
            } else if constexpr (std::is_same_v<Value, MergeDownLayerOperation>) {
                merge_down(candidate, std::move(value));
            } else if constexpr (std::is_same_v<Value, MergeGroupLayerOperation>) {
                merge_group(candidate, std::move(value));
            } else if constexpr (std::is_same_v<Value, FlattenLayersOperation>) {
                flatten_layers(candidate, std::move(value));
            } else if constexpr (std::is_same_v<Value, ConvertLayerOperation>) {
                convert_layer(candidate, std::move(value));
            } else if constexpr (std::is_same_v<Value, ApplyMaskLayerOperation>) {
                apply_mask(candidate, std::move(value));
            }
        },
        std::move(operation));
}

LayerCompositeResult composite_candidate(const TextureSet& texture_set,
                                         const std::vector<LayerEntry>& entries,
                                         const LayerCompositeRequest& snapshot) {
    TextureSet evaluation = texture_set;
    evaluation.layer_stack().assign(entries);
    return evaluation.composite_cpu(snapshot);
}

}  // namespace

LayerOperationError::LayerOperationError(LayerOperationErrorCode code, std::string message)
    : std::invalid_argument(std::move(message)), code_(code) {}

LayerOperationResult apply_layer_operation(TextureSet& texture_set, LayerOperationRequest request) {
    try {
        if (!std::isfinite(request.appearance_tolerance) || request.appearance_tolerance < 0.0F) {
            fail(LayerOperationErrorCode::invalid_operation,
                 "layer operation appearance tolerance must be finite and non-negative");
        }
        enforce_budget(request.resolved_content, request.maximum_output_bytes);
        const LayerCompositeResult before = texture_set.composite_cpu(request.resolved_content);
        Candidate candidate{
            .entries = std::vector<LayerEntry>(texture_set.layer_stack().entries().begin(),
                                               texture_set.layer_stack().entries().end()),
            .snapshot = std::move(request.resolved_content),
            .affected = {},
            .preserves_appearance = false,
        };
        apply_operation(candidate, std::move(request.operation), texture_set);
        enforce_budget(candidate.snapshot, request.maximum_output_bytes);
        const LayerCompositeResult after =
            composite_candidate(texture_set, candidate.entries, candidate.snapshot);
        if (candidate.preserves_appearance &&
            !same_appearance(before, after, request.appearance_tolerance)) {
            fail(LayerOperationErrorCode::appearance_mismatch,
                 "layer operation candidate does not preserve the composited appearance");
        }
        LayerStack committed;
        committed.assign(std::move(candidate.entries));
        texture_set.layer_stack() = std::move(committed);
        return {.resolved_content = std::move(candidate.snapshot),
                .affected_identifiers = std::move(candidate.affected)};
    } catch (const LayerOperationError&) {
        throw;
    } catch (const LayerCompositeError& error) {
        throw LayerOperationError(LayerOperationErrorCode::invalid_content, error.what());
    } catch (const LayerStackError& error) {
        throw LayerOperationError(LayerOperationErrorCode::invalid_operation, error.what());
    }
}

}  // namespace ctex::doc
