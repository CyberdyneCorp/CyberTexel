#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/doc/layer_stack.hpp>
#include <ctex/graph/portable_nodes.hpp>
#include <limits>
#include <map>
#include <set>
#include <utility>

namespace ctex::doc {
namespace {

[[noreturn]] void refuse(LayerStackRule rule, std::string message,
                         std::vector<std::string> related_identifiers = {}) {
    throw LayerStackError(rule, std::move(message), std::move(related_identifiers));
}

bool can_own_children(LayerEntryKind kind) { return kind == LayerEntryKind::group; }

bool can_receive_attachment(LayerEntryKind kind) {
    return kind == LayerEntryKind::paint_layer || kind == LayerEntryKind::fill_layer ||
           kind == LayerEntryKind::group || kind == LayerEntryKind::instance ||
           kind == LayerEntryKind::editable_decal || kind == LayerEntryKind::editable_text ||
           kind == LayerEntryKind::surface_path;
}

bool is_attachment(LayerEntryKind kind) {
    return kind == LayerEntryKind::mask || kind == LayerEntryKind::filter;
}

bool is_channel_layer(LayerEntryKind kind) {
    return kind == LayerEntryKind::paint_layer || kind == LayerEntryKind::fill_layer ||
           kind == LayerEntryKind::instance || kind == LayerEntryKind::editable_decal ||
           kind == LayerEntryKind::editable_text || kind == LayerEntryKind::surface_path;
}

std::size_t index_of(std::span<const LayerEntry> entries, std::string_view identifier) {
    const auto found = std::find_if(entries.begin(), entries.end(), [&](const LayerEntry& entry) {
        return entry.identifier == identifier;
    });
    if (found == entries.end()) {
        refuse(LayerStackRule::identity,
               "layer-stack identity is not present: " + std::string(identifier));
    }
    return static_cast<std::size_t>(found - entries.begin());
}

using EntryIndices = std::map<std::string, std::size_t, std::less<>>;

std::size_t resolved_content_index(std::span<const LayerEntry> entries, const EntryIndices& indices,
                                   std::size_t index) {
    std::set<std::size_t> visited;
    while (entries[index].kind == LayerEntryKind::instance) {
        if (!visited.insert(index).second) {
            refuse(LayerStackRule::instance_cycle,
                   "layer-stack instance reference would form a cycle");
        }
        const auto source = indices.find(entries[index].source_identifier);
        if (source == indices.end()) {
            refuse(LayerStackRule::instance_source, "layer-stack instance source is not present: " +
                                                        entries[index].source_identifier);
        }
        index = source->second;
    }
    return index;
}

bool is_enclosing_group(std::span<const LayerEntry> entries, const EntryIndices& indices,
                        std::size_t entry_index, std::size_t candidate_group) {
    while (!entries[entry_index].parent_identifier.empty()) {
        entry_index = indices.at(entries[entry_index].parent_identifier);
        if (entry_index == candidate_group) {
            return true;
        }
    }
    return false;
}

void validate_entry_content(const LayerEntry& entry) {
    if (!std::isfinite(entry.opacity) || entry.opacity < 0.0 || entry.opacity > 1.0) {
        refuse(LayerStackRule::entry_content,
               "layer-stack opacity must be finite and within [0, 1]");
    }
    if (!graph::is_blend_mode(entry.blend_mode)) {
        refuse(LayerStackRule::blend_mode,
               "layer-stack blend mode is unknown: " + entry.blend_mode);
    }
    if (entry.blend_mode == "pass_through" && entry.kind != LayerEntryKind::group) {
        refuse(LayerStackRule::blend_mode,
               "layer-stack Pass Through blend mode is valid only on a group");
    }
    std::set<std::string, std::less<>> channel_ids;
    for (const LayerEntry::ChannelModulation& channel : entry.channels) {
        if (channel.semantic_id.empty() || !channel_ids.insert(channel.semantic_id).second ||
            !std::isfinite(channel.opacity) || channel.opacity < 0.0 || channel.opacity > 1.0) {
            refuse(LayerStackRule::entry_content,
                   "layer-stack channel modulation requires a unique semantic identity and "
                   "finite opacity within [0, 1]");
        }
    }
}

std::vector<std::string> dependent_instances(std::span<const LayerEntry> entries,
                                             const EntryIndices& indices,
                                             const std::set<std::string, std::less<>>& removed) {
    std::vector<std::string> result;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (entries[index].kind != LayerEntryKind::instance ||
            removed.contains(entries[index].identifier)) {
            continue;
        }
        std::size_t source_index = index;
        while (entries[source_index].kind == LayerEntryKind::instance) {
            const std::string& source_id = entries[source_index].source_identifier;
            if (removed.contains(source_id)) {
                result.push_back(entries[index].identifier);
                break;
            }
            source_index = indices.at(source_id);
        }
    }
    return result;
}

std::string named_instances(std::span<const std::string> identifiers) {
    std::string result;
    for (const std::string& identifier : identifiers) {
        if (!result.empty()) {
            result += ", ";
        }
        result += identifier;
    }
    return result;
}

const LayerEntry::ChannelModulation* find_channel(const LayerEntry& entry,
                                                  std::string_view semantic_id) {
    const auto found = std::find_if(entry.channels.begin(), entry.channels.end(),
                                    [&](const LayerEntry::ChannelModulation& channel) {
                                        return channel.semantic_id == semantic_id;
                                    });
    return found == entry.channels.end() ? nullptr : &*found;
}

std::vector<std::size_t> target_chain(std::span<const LayerEntry> entries,
                                      const EntryIndices& indices, std::size_t index) {
    std::vector<std::size_t> result;
    while (true) {
        result.push_back(index);
        if (entries[index].parent_identifier.empty()) {
            break;
        }
        index = indices.at(entries[index].parent_identifier);
    }
    return result;
}

std::vector<std::string> active_masks(std::span<const LayerEntry> entries,
                                      std::span<const std::size_t> target_indices) {
    std::set<std::string_view, std::less<>> targets;
    for (const std::size_t index : target_indices) {
        targets.insert(entries[index].identifier);
    }
    std::vector<std::string> result;
    for (const LayerEntry& entry : entries) {
        if (entry.kind == LayerEntryKind::mask && entry.enabled &&
            targets.contains(entry.target_identifier)) {
            result.push_back(entry.identifier);
        }
    }
    return result;
}

double apply_mask_samples(std::span<const std::string> required,
                          std::span<const LayerMaskSample> samples, double opacity) {
    std::map<std::string_view, double, std::less<>> supplied;
    for (const LayerMaskSample& sample : samples) {
        if (sample.mask_identifier.empty() || !std::isfinite(sample.value) || sample.value < 0.0 ||
            sample.value > 1.0 || !supplied.emplace(sample.mask_identifier, sample.value).second) {
            refuse(LayerStackRule::mask_sample,
                   "layer-stack mask samples require unique identities and finite values within "
                   "[0, 1]");
        }
    }
    if (supplied.size() != required.size()) {
        refuse(LayerStackRule::mask_sample,
               "layer-stack mask samples must cover every active applicable mask exactly once");
    }
    for (const std::string& identifier : required) {
        const auto found = supplied.find(identifier);
        if (found == supplied.end()) {
            refuse(LayerStackRule::mask_sample,
                   "layer-stack mask sample is missing for active mask: " + identifier,
                   {identifier});
        }
        opacity *= found->second;
    }
    return opacity;
}

}  // namespace

LayerStackError::LayerStackError(LayerStackRule rule, std::string message,
                                 std::vector<std::string> related_identifiers)
    : std::invalid_argument(std::move(message)),
      rule_(rule),
      related_identifiers_(std::move(related_identifiers)) {}

bool LayerStack::contains(std::string_view identifier) const noexcept {
    return std::any_of(entries_.begin(), entries_.end(),
                       [&](const LayerEntry& entry) { return entry.identifier == identifier; });
}

const LayerEntry& LayerStack::entry(std::string_view identifier) const {
    return entries_[index_of(entries_, identifier)];
}

const LayerEntry& LayerStack::resolved_content(std::string_view identifier) const {
    EntryIndices indices;
    for (std::size_t index = 0; index < entries_.size(); ++index) {
        indices.emplace(entries_[index].identifier, index);
    }
    return entries_[resolved_content_index(entries_, indices, index_of(entries_, identifier))];
}

const LayerEntry& LayerStack::paint_target(std::string_view identifier) const {
    const LayerEntry& target = entry(identifier);
    if (target.kind == LayerEntryKind::instance) {
        refuse(LayerStackRule::direct_instance_paint,
               "cannot paint instance '" + target.identifier + "'; it references source '" +
                   target.source_identifier + "'",
               {target.source_identifier});
    }
    if (target.kind != LayerEntryKind::paint_layer) {
        refuse(LayerStackRule::entry_content,
               "paint target must be a paint-layer entry: " + target.identifier);
    }
    return target;
}

graph::ColourValue LayerStack::evaluate_blend(std::string_view identifier, graph::ColourValue base,
                                              graph::ColourValue layer, double factor) const {
    if (!std::isfinite(factor)) {
        refuse(LayerStackRule::blend_mode, "layer-stack blend factor must be finite");
    }
    return graph::blend_colour(entry(identifier).blend_mode, base, layer, factor);
}

std::vector<std::string> LayerStack::applicable_masks(std::string_view identifier) const {
    EntryIndices indices;
    for (std::size_t index = 0; index < entries_.size(); ++index) {
        indices.emplace(entries_[index].identifier, index);
    }
    const std::vector<std::size_t> targets =
        target_chain(entries_, indices, index_of(entries_, identifier));
    return active_masks(entries_, targets);
}

LayerChannelParticipation LayerStack::channel_participation(
    std::string_view identifier, std::string_view semantic_id, bool texture_channel_enabled,
    std::span<const LayerMaskSample> mask_samples) const {
    const std::size_t layer_index = index_of(entries_, identifier);
    const LayerEntry& layer = entries_[layer_index];
    if (!is_channel_layer(layer.kind)) {
        refuse(LayerStackRule::channel_participation,
               "layer-stack channel participation requires a content layer: " + layer.identifier);
    }
    if (semantic_id.empty()) {
        refuse(LayerStackRule::channel_participation,
               "layer-stack channel participation requires a semantic identity");
    }
    const LayerEntry::ChannelModulation* channel = find_channel(layer, semantic_id);
    if (!texture_channel_enabled || !layer.enabled || channel == nullptr || !channel->enabled) {
        return {};
    }

    EntryIndices indices;
    for (std::size_t index = 0; index < entries_.size(); ++index) {
        indices.emplace(entries_[index].identifier, index);
    }
    const std::vector<std::size_t> targets = target_chain(entries_, indices, layer_index);
    double opacity = layer.opacity * channel->opacity;
    for (const std::size_t index : targets) {
        if (index == layer_index) {
            continue;
        }
        const LayerEntry& group = entries_[index];
        if (!group.enabled) {
            return {};
        }
        opacity *= group.opacity;
    }
    std::vector<std::string> masks = active_masks(entries_, targets);
    opacity = apply_mask_samples(masks, mask_samples, opacity);
    return {
        .participates = true, .effective_opacity = opacity, .mask_identifiers = std::move(masks)};
}

void LayerStack::append(LayerEntry entry) {
    const std::array additions{std::move(entry)};
    append(additions);
}

void LayerStack::append(std::span<const LayerEntry> entries) {
    std::vector<LayerEntry> candidate = entries_;
    candidate.insert(candidate.end(), entries.begin(), entries.end());
    validate(candidate);
    entries_ = std::move(candidate);
}

void LayerStack::replace(std::string_view identifier, LayerEntry replacement) {
    if (replacement.identifier != identifier) {
        refuse(LayerStackRule::identity, "layer-stack replacement cannot change stable identity");
    }
    std::vector<LayerEntry> candidate = entries_;
    candidate[index_of(candidate, identifier)] = std::move(replacement);
    validate(candidate);
    entries_ = std::move(candidate);
}

void LayerStack::set_layout(std::string_view identifier, std::string parent_identifier,
                            std::string target_identifier) {
    LayerEntry replacement = entry(identifier);
    replacement.parent_identifier = std::move(parent_identifier);
    replacement.target_identifier = std::move(target_identifier);
    replace(identifier, std::move(replacement));
}

void LayerStack::set_fill_graph(std::string_view identifier, graph::GraphDocument graph) {
    LayerEntry replacement = entry(identifier);
    if (replacement.kind != LayerEntryKind::fill_layer) {
        refuse(LayerStackRule::entry_content, "fill-graph replacement requires a fill-layer entry");
    }
    if (replacement.content_revision == std::numeric_limits<std::uint64_t>::max()) {
        refuse(LayerStackRule::entry_content, "fill content revision is exhausted");
    }
    replacement.graph = std::move(graph);
    ++replacement.content_revision;
    replace(identifier, std::move(replacement));
}

void LayerStack::record_paint(std::string_view identifier) {
    static_cast<void>(paint_target(identifier));
    LayerEntry replacement = entry(identifier);
    if (replacement.content_revision == std::numeric_limits<std::uint64_t>::max()) {
        refuse(LayerStackRule::entry_content, "paint content revision is exhausted");
    }
    ++replacement.content_revision;
    replace(identifier, std::move(replacement));
}

void LayerStack::set_opacity(std::string_view identifier, double opacity) {
    LayerEntry replacement = entry(identifier);
    replacement.opacity = opacity;
    replace(identifier, std::move(replacement));
}

void LayerStack::set_blend_mode(std::string_view identifier, std::string blend_mode) {
    LayerEntry replacement = entry(identifier);
    replacement.blend_mode = std::move(blend_mode);
    replace(identifier, std::move(replacement));
}

void LayerStack::set_channel_modulation(std::string_view identifier,
                                        LayerEntry::ChannelModulation channel) {
    LayerEntry replacement = entry(identifier);
    const auto found = std::find_if(replacement.channels.begin(), replacement.channels.end(),
                                    [&](const LayerEntry::ChannelModulation& value) {
                                        return value.semantic_id == channel.semantic_id;
                                    });
    if (found == replacement.channels.end()) {
        replacement.channels.push_back(std::move(channel));
    } else {
        *found = std::move(channel);
    }
    replace(identifier, std::move(replacement));
}

void LayerStack::remove(std::span<const std::string> identifiers,
                        ReferencedSourceDeletionPolicy policy) {
    const std::set<std::string, std::less<>> removed(identifiers.begin(), identifiers.end());
    if (removed.size() != identifiers.size()) {
        refuse(LayerStackRule::identity, "layer-stack removal identities must be unique");
    }
    EntryIndices indices;
    for (std::size_t index = 0; index < entries_.size(); ++index) {
        indices.emplace(entries_[index].identifier, index);
    }
    for (const std::string& identifier : removed) {
        if (!indices.contains(identifier)) {
            refuse(LayerStackRule::identity,
                   "layer-stack removal identity is not present: " + identifier);
        }
    }
    const std::vector<std::string> dependents = dependent_instances(entries_, indices, removed);
    if (!dependents.empty() && policy == ReferencedSourceDeletionPolicy::refuse) {
        refuse(LayerStackRule::live_instances,
               "cannot delete a source with live instances: " + named_instances(dependents),
               dependents);
    }
    std::vector<LayerEntry> candidate = entries_;
    if (policy == ReferencedSourceDeletionPolicy::make_instances_independent) {
        for (const std::string& identifier : dependents) {
            LayerEntry& instance = candidate[index_of(candidate, identifier)];
            const LayerEntry& content =
                entries_[resolved_content_index(entries_, indices, indices.at(identifier))];
            instance.kind = content.kind;
            instance.source_identifier.clear();
            instance.graph = content.graph;
            instance.content_revision = content.content_revision;
        }
    }
    std::vector<LayerEntry> retained;
    retained.reserve(candidate.size());
    for (LayerEntry& entry : candidate) {
        if (!removed.contains(entry.identifier)) {
            retained.push_back(std::move(entry));
        }
    }
    validate(retained);
    entries_ = std::move(retained);
}

void LayerStack::validate(std::span<const LayerEntry> entries) {
    EntryIndices indices;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const LayerEntry& entry = entries[index];
        if (entry.identifier.empty() || entry.display_name.empty() ||
            !indices.emplace(entry.identifier, index).second) {
            refuse(LayerStackRule::identity,
                   "layer-stack identity and display name must be non-empty and identity unique");
        }
        validate_entry_content(entry);
    }
    std::vector<std::size_t> group_depths(entries.size());
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const LayerEntry& entry = entries[index];
        if (!entry.parent_identifier.empty()) {
            const auto parent = indices.find(entry.parent_identifier);
            if (parent == indices.end() || parent->second >= index) {
                refuse(LayerStackRule::evaluation_order,
                       "layer-stack parent must precede its child in evaluation order");
            }
            if (!can_own_children(entries[parent->second].kind)) {
                refuse(LayerStackRule::group_parent, "layer-stack parent must be a group");
            }
            group_depths[index] = group_depths[parent->second];
        }
        if (entry.kind == LayerEntryKind::group) {
            ++group_depths[index];
        }
        if (group_depths[index] > maximum_group_depth) {
            refuse(LayerStackRule::maximum_group_depth,
                   "layer-stack maximum group depth of 32 was exceeded");
        }
        if (is_attachment(entry.kind)) {
            if (entry.target_identifier.empty() || !entry.parent_identifier.empty()) {
                refuse(LayerStackRule::single_attachment,
                       "layer-stack mask or filter must attach to exactly one target");
            }
            const auto target = indices.find(entry.target_identifier);
            if (target == indices.end() || target->second >= index) {
                refuse(LayerStackRule::evaluation_order,
                       "layer-stack attachment target must precede it in evaluation order");
            }
            if (!can_receive_attachment(entries[target->second].kind)) {
                refuse(LayerStackRule::attachment_target,
                       "layer-stack mask or filter target must be a layer or group");
            }
        } else if (!entry.target_identifier.empty()) {
            refuse(LayerStackRule::single_attachment,
                   "only a layer-stack mask or filter may have an attachment target");
        }
        if (entry.kind == LayerEntryKind::instance) {
            if (entry.source_identifier.empty() || entry.graph.has_value()) {
                refuse(LayerStackRule::instance_source,
                       "layer-stack instance must reference content without copying a graph");
            }
            const std::size_t source_index = resolved_content_index(entries, indices, index);
            if (source_index == index || indices.at(entry.source_identifier) >= index) {
                refuse(LayerStackRule::instance_source,
                       "layer-stack instance source must precede it in evaluation order");
            }
            if (entries[source_index].kind == LayerEntryKind::group &&
                is_enclosing_group(entries, indices, index, source_index)) {
                refuse(LayerStackRule::instance_cycle,
                       "layer-stack instance cannot source an enclosing group's content");
            }
            if (!can_receive_attachment(entries[source_index].kind)) {
                refuse(LayerStackRule::instance_source,
                       "layer-stack instance source must resolve to a layer or group");
            }
        } else if (!entry.source_identifier.empty()) {
            refuse(LayerStackRule::instance_source,
                   "only a layer-stack instance may have a source reference");
        }
    }
}

}  // namespace ctex::doc
