#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/doc/layer_stack.hpp>
#include <map>
#include <set>
#include <utility>

namespace ctex::doc {
namespace {

[[noreturn]] void refuse(LayerStackRule rule, std::string message) {
    throw LayerStackError(rule, std::move(message));
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

}  // namespace

LayerStackError::LayerStackError(LayerStackRule rule, std::string message)
    : std::invalid_argument(std::move(message)), rule_(rule) {}

bool LayerStack::contains(std::string_view identifier) const noexcept {
    return std::any_of(entries_.begin(), entries_.end(),
                       [&](const LayerEntry& entry) { return entry.identifier == identifier; });
}

const LayerEntry& LayerStack::entry(std::string_view identifier) const {
    return entries_[index_of(entries_, identifier)];
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
    replacement.graph = std::move(graph);
    ++replacement.content_revision;
    replace(identifier, std::move(replacement));
}

void LayerStack::remove(std::span<const std::string> identifiers) {
    const std::set<std::string, std::less<>> removed(identifiers.begin(), identifiers.end());
    if (removed.size() != identifiers.size()) {
        refuse(LayerStackRule::identity, "layer-stack removal identities must be unique");
    }
    std::vector<LayerEntry> candidate;
    candidate.reserve(entries_.size());
    for (const LayerEntry& entry : entries_) {
        if (!removed.contains(entry.identifier)) {
            candidate.push_back(entry);
        }
    }
    if (candidate.size() + removed.size() != entries_.size()) {
        refuse(LayerStackRule::identity, "layer-stack removal identity is not present");
    }
    validate(candidate);
    entries_ = std::move(candidate);
}

void LayerStack::validate(std::span<const LayerEntry> entries) {
    std::map<std::string, std::size_t, std::less<>> indices;
    std::vector<std::size_t> group_depths(entries.size());
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const LayerEntry& entry = entries[index];
        if (entry.identifier.empty() || entry.display_name.empty() ||
            !indices.emplace(entry.identifier, index).second) {
            refuse(LayerStackRule::identity,
                   "layer-stack identity and display name must be non-empty and identity unique");
        }
        if (!std::isfinite(entry.opacity) || entry.opacity < 0.0 || entry.opacity > 1.0) {
            refuse(LayerStackRule::entry_content,
                   "layer-stack opacity must be finite and within [0, 1]");
        }
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
    }
}

}  // namespace ctex::doc
