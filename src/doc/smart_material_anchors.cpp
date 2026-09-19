#include <algorithm>
#include <ctex/doc/smart_material.hpp>
#include <map>
#include <set>
#include <sstream>
#include <utility>

namespace ctex::doc {
namespace {

using EntryIndex = std::map<std::string_view, std::size_t, std::less<>>;
using Adjacency = std::vector<std::vector<std::size_t>>;

[[noreturn]] void anchor_error(SmartMaterialErrorCode code, std::string message) {
    throw SmartMaterialError(code, std::move(message));
}

EntryIndex index_entries(const SmartMaterialPreset& preset) {
    EntryIndex result;
    for (std::size_t index = 0; index < preset.stack.size(); ++index) {
        result.emplace(preset.stack[index].identifier, index);
    }
    return result;
}

const graph::NodeSocket& referenced_input(const SmartMaterialPreset& preset,
                                          const SmartMaterialAnchorReference& reference,
                                          std::size_t consumer_index) {
    const SmartMaterialEntry& consumer = preset.stack[consumer_index];
    if (!consumer.graph.has_value() || reference.consumer_node_id == 0 ||
        reference.consumer_input_identifier.empty()) {
        anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                     "anchor reference consumer requires a graph and input target");
    }
    const auto node = std::find_if(consumer.graph->nodes().begin(), consumer.graph->nodes().end(),
                                   [&](const graph::GraphNode& candidate) {
                                       return candidate.id == reference.consumer_node_id;
                                   });
    if (node == consumer.graph->nodes().end()) {
        anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                     "anchor reference consumer node does not exist");
    }
    const auto input = std::find_if(
        node->inputs.begin(), node->inputs.end(), [&](const graph::NodeSocket& candidate) {
            return candidate.identifier == reference.consumer_input_identifier;
        });
    if (input == node->inputs.end() || input->type != graph::SocketType::image) {
        anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                     "anchor reference target must be an existing image input");
    }
    return *input;
}

void validate_target_is_active(const SmartMaterialPreset& preset,
                               const SmartMaterialAnchorReference& reference,
                               std::size_t consumer_index) {
    static_cast<void>(referenced_input(preset, reference, consumer_index));
    const auto same_target = [&](graph::NodeId node, std::string_view input) {
        return node == reference.consumer_node_id && input == reference.consumer_input_identifier;
    };
    const graph::GraphDocument& graph = *preset.stack[consumer_index].graph;
    if (std::any_of(graph.links().begin(), graph.links().end(), [&](const graph::GraphLink& link) {
            return same_target(link.target_node, link.target_socket);
        })) {
        anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                     "anchor reference cannot target a linked graph input");
    }
    for (const ExposedSmartMaterialParameter& parameter : preset.exposed_parameters) {
        if (std::any_of(parameter.bindings.begin(), parameter.bindings.end(),
                        [&](const SmartMaterialParameterBinding& binding) {
                            return binding.entry_identifier ==
                                       reference.consumer_entry_identifier &&
                                   binding.target_kind == SmartMaterialBindingTargetKind::input &&
                                   same_target(binding.node_id, binding.target_identifier);
                        })) {
            anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                         "anchor reference cannot target a parameter-bound graph input");
        }
    }
}

Adjacency build_adjacency(const SmartMaterialPreset& preset, const EntryIndex& entries) {
    Adjacency result(preset.stack.size());
    for (const SmartMaterialAnchorReference& reference : preset.anchor_references) {
        result.at(entries.at(reference.anchor_entry_identifier))
            .push_back(entries.at(reference.consumer_entry_identifier));
    }
    for (auto& dependents : result) {
        std::sort(dependents.begin(), dependents.end());
    }
    return result;
}

bool find_cycle_from(std::size_t entry, const Adjacency& adjacency,
                     std::vector<std::uint8_t>& state, std::vector<std::size_t>& path,
                     std::vector<std::size_t>& cycle) {
    state[entry] = 1;
    path.push_back(entry);
    for (const std::size_t dependent : adjacency[entry]) {
        if (state[dependent] == 0 && find_cycle_from(dependent, adjacency, state, path, cycle)) {
            return true;
        }
        if (state[dependent] == 1) {
            const auto begin = std::find(path.begin(), path.end(), dependent);
            cycle.assign(begin, path.end());
            cycle.push_back(dependent);
            return true;
        }
    }
    path.pop_back();
    state[entry] = 2;
    return false;
}

std::vector<std::size_t> find_cycle(const Adjacency& adjacency) {
    std::vector<std::uint8_t> state(adjacency.size());
    std::vector<std::size_t> path;
    std::vector<std::size_t> cycle;
    for (std::size_t entry = 0; entry < adjacency.size(); ++entry) {
        if (state[entry] == 0 && find_cycle_from(entry, adjacency, state, path, cycle)) {
            return cycle;
        }
    }
    return {};
}

std::string cycle_message(const SmartMaterialPreset& preset, std::span<const std::size_t> cycle) {
    std::ostringstream message;
    message << "anchor reference would create cycle ";
    for (std::size_t index = 0; index < cycle.size(); ++index) {
        if (index != 0) {
            message << " -> ";
        }
        message << preset.stack[cycle[index]].identifier;
    }
    return message.str();
}

void mark_dependents(std::size_t entry, const Adjacency& adjacency, std::vector<bool>& affected) {
    for (const std::size_t dependent : adjacency[entry]) {
        if (affected[dependent]) {
            continue;
        }
        affected[dependent] = true;
        mark_dependents(dependent, adjacency, affected);
    }
}

}  // namespace

void validate_smart_material_anchors(const SmartMaterialPreset& preset) {
    const EntryIndex entries = index_entries(preset);
    std::set<std::string_view, std::less<>> anchors;
    for (const std::string& identifier : preset.anchor_entries) {
        const auto entry = entries.find(identifier);
        if (entry == entries.end() ||
            (preset.stack[entry->second].kind != SmartMaterialEntryKind::layer &&
             preset.stack[entry->second].kind != SmartMaterialEntryKind::mask) ||
            !anchors.insert(identifier).second) {
            anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                         "anchor must uniquely name an existing layer or mask");
        }
    }

    std::set<std::tuple<std::string_view, graph::NodeId, std::string_view>> targets;
    for (const SmartMaterialAnchorReference& reference : preset.anchor_references) {
        const auto anchor = entries.find(reference.anchor_entry_identifier);
        const auto consumer = entries.find(reference.consumer_entry_identifier);
        if (anchor == entries.end() || consumer == entries.end() ||
            !anchors.contains(reference.anchor_entry_identifier)) {
            anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                         "anchor reference must name existing marked entries");
        }
        if (preset.stack[consumer->second].kind != SmartMaterialEntryKind::layer) {
            anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                         "anchor reference consumer must be a layer");
        }
        validate_target_is_active(preset, reference, consumer->second);
        if (!targets
                 .emplace(reference.consumer_entry_identifier, reference.consumer_node_id,
                          reference.consumer_input_identifier)
                 .second) {
            anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                         "anchor graph input is referenced more than once");
        }
    }

    const Adjacency adjacency = build_adjacency(preset, entries);
    const std::vector<std::size_t> cycle = find_cycle(adjacency);
    if (!cycle.empty()) {
        anchor_error(SmartMaterialErrorCode::anchor_cycle, cycle_message(preset, cycle));
    }
    for (const SmartMaterialAnchorReference& reference : preset.anchor_references) {
        if (entries.at(reference.anchor_entry_identifier) >=
            entries.at(reference.consumer_entry_identifier)) {
            anchor_error(SmartMaterialErrorCode::anchor_ordering_violation,
                         "anchor ordering rule requires consumer '" +
                             reference.consumer_entry_identifier + "' above anchor '" +
                             reference.anchor_entry_identifier + "'");
        }
    }
}

void set_smart_material_anchor(SmartMaterialPreset& preset, std::string_view entry_identifier,
                               bool marked) {
    SmartMaterialPreset updated = preset;
    const auto existing =
        std::find(updated.anchor_entries.begin(), updated.anchor_entries.end(), entry_identifier);
    if (marked && existing == updated.anchor_entries.end()) {
        updated.anchor_entries.emplace_back(entry_identifier);
    } else if (!marked && existing != updated.anchor_entries.end()) {
        updated.anchor_entries.erase(existing);
    }
    validate_smart_material(updated);
    preset = std::move(updated);
}

void add_smart_material_anchor_reference(SmartMaterialPreset& preset,
                                         SmartMaterialAnchorReference reference) {
    SmartMaterialPreset updated = preset;
    updated.anchor_references.push_back(std::move(reference));
    validate_smart_material(updated);
    preset = std::move(updated);
}

SmartMaterialAnchorEvaluationPlan plan_smart_material_anchor_evaluation(
    const SmartMaterialPreset& preset, std::span<const std::string_view> changed_anchor_entries) {
    validate_smart_material(preset);
    const EntryIndex entries = index_entries(preset);
    const Adjacency adjacency = build_adjacency(preset, entries);
    std::vector<bool> affected(preset.stack.size());
    std::set<std::string_view, std::less<>> anchors(preset.anchor_entries.begin(),
                                                    preset.anchor_entries.end());
    for (const std::string_view identifier : changed_anchor_entries) {
        const auto entry = entries.find(identifier);
        if (entry == entries.end() || !anchors.contains(identifier)) {
            anchor_error(SmartMaterialErrorCode::invalid_anchor_reference,
                         "changed anchor must name a marked anchor entry");
        }
        mark_dependents(entry->second, adjacency, affected);
    }
    SmartMaterialAnchorEvaluationPlan result;
    for (std::size_t index = 0; index < affected.size(); ++index) {
        if (affected[index]) {
            result.entry_identifiers.push_back(preset.stack[index].identifier);
        }
    }
    return result;
}

}  // namespace ctex::doc
