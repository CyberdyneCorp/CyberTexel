#include <algorithm>
#include <ctex/doc/document.hpp>
#include <map>
#include <span>
#include <utility>

namespace ctex::doc {
namespace {

[[noreturn]] void invalid_application(std::string message) {
    throw SmartMaterialError(SmartMaterialErrorCode::invalid_preset, std::move(message));
}

void require_new_application(std::span<const AppliedPresetApplication> applications,
                             std::string_view identifier) {
    if (identifier.empty() || std::any_of(applications.begin(), applications.end(),
                                          [&](const AppliedPresetApplication& application) {
                                              return application.identifier == identifier;
                                          })) {
        invalid_application("preset application requires a non-empty unique identity");
    }
}

std::vector<std::string> remap_fragment_entries(SmartMaterialPreset& fragment,
                                                std::string_view application_identifier) {
    std::map<std::string, std::string, std::less<>> identities;
    std::vector<std::string> result;
    result.reserve(fragment.stack.size());
    for (const SmartMaterialEntry& entry : fragment.stack) {
        std::string instantiated = std::string(application_identifier) + "/" + entry.identifier;
        identities.emplace(entry.identifier, instantiated);
        result.push_back(std::move(instantiated));
    }
    for (SmartMaterialEntry& entry : fragment.stack) {
        entry.identifier = identities.at(entry.identifier);
        if (!entry.parent_identifier.empty()) {
            entry.parent_identifier = identities.at(entry.parent_identifier);
        }
    }
    for (ExposedSmartMaterialParameter& parameter : fragment.exposed_parameters) {
        for (SmartMaterialParameterBinding& binding : parameter.bindings) {
            binding.entry_identifier = identities.at(binding.entry_identifier);
        }
    }
    for (std::string& anchor : fragment.anchor_entries) {
        anchor = identities.at(anchor);
    }
    for (SmartMaterialAnchorReference& reference : fragment.anchor_references) {
        reference.anchor_entry_identifier = identities.at(reference.anchor_entry_identifier);
        reference.consumer_entry_identifier = identities.at(reference.consumer_entry_identifier);
    }
    return result;
}

void require_new_entry_identities(std::span<const AppliedPresetApplication> applications,
                                  std::span<const std::string> identities) {
    for (const std::string& identity : identities) {
        for (const AppliedPresetApplication& application : applications) {
            if (std::any_of(application.fragment.stack.begin(), application.fragment.stack.end(),
                            [&](const SmartMaterialEntry& entry) {
                                return entry.identifier == identity;
                            })) {
                invalid_application("preset application entry identity collides with '" + identity +
                                    "'");
            }
        }
    }
}

std::vector<AppliedPresetParameterValue> reset_fragment_parameters(SmartMaterialPreset& fragment) {
    std::vector<AppliedPresetParameterValue> values;
    values.reserve(fragment.exposed_parameters.size());
    for (std::size_t index = 0; index < fragment.exposed_parameters.size(); ++index) {
        const std::string identifier = fragment.exposed_parameters[index].identifier;
        const graph::SocketValue default_value = fragment.exposed_parameters[index].default_value;
        if (fragment.exposed_parameters[index].binding_state ==
            SmartMaterialParameterBindingState::bound) {
            static_cast<void>(
                set_smart_material_parameter_value(fragment, identifier, default_value));
        }
        values.push_back({.parameter_identifier = identifier, .value = default_value});
    }
    return values;
}

AppliedPresetApplication& find_application(std::span<AppliedPresetApplication> applications,
                                           std::string_view identifier) {
    const auto found = std::find_if(applications.begin(), applications.end(),
                                    [&](const AppliedPresetApplication& application) {
                                        return application.identifier == identifier;
                                    });
    if (found == applications.end()) {
        invalid_application("preset application does not exist: " + std::string(identifier));
    }
    return *found;
}

const AppliedPresetApplication& find_entry_application(
    std::span<const AppliedPresetApplication> applications, std::string_view entry_identifier) {
    const auto found = std::find_if(
        applications.begin(), applications.end(), [&](const AppliedPresetApplication& application) {
            return std::any_of(application.fragment.stack.begin(), application.fragment.stack.end(),
                               [&](const SmartMaterialEntry& entry) {
                                   return entry.identifier == entry_identifier;
                               });
        });
    if (found == applications.end()) {
        invalid_application("applied preset entry does not exist: " +
                            std::string(entry_identifier));
    }
    return *found;
}

PresetApplicationReport application_report(const AppliedPresetApplication& application) {
    std::vector<std::string> entries;
    entries.reserve(application.fragment.stack.size());
    for (const SmartMaterialEntry& entry : application.fragment.stack) {
        entries.push_back(entry.identifier);
    }
    return {.application_identifier = application.identifier,
            .origin = application.origin,
            .entry_identifiers = std::move(entries),
            .content = report_smart_material_content(application.fragment)};
}

LayerEntryKind layer_kind(SmartMaterialEntryKind kind) {
    switch (kind) {
        case SmartMaterialEntryKind::layer:
            return LayerEntryKind::paint_layer;
        case SmartMaterialEntryKind::group:
            return LayerEntryKind::group;
        case SmartMaterialEntryKind::mask:
            return LayerEntryKind::mask;
        case SmartMaterialEntryKind::filter:
            return LayerEntryKind::filter;
        case SmartMaterialEntryKind::generator:
            return LayerEntryKind::fill_layer;
    }
    invalid_application("smart-material entry kind cannot be added to the layer stack");
}

LayerEntry layer_entry(const SmartMaterialEntry& entry, std::string_view external_target = {}) {
    const LayerEntryKind kind = layer_kind(entry.kind);
    const bool attachment = kind == LayerEntryKind::mask || kind == LayerEntryKind::filter;
    const std::string target =
        entry.parent_identifier.empty() ? std::string(external_target) : entry.parent_identifier;
    return {.identifier = entry.identifier,
            .display_name = entry.display_name,
            .kind = kind,
            .parent_identifier = attachment ? std::string{} : entry.parent_identifier,
            .target_identifier = attachment ? target : std::string{},
            .enabled = entry.enabled,
            .opacity = entry.opacity,
            .graph = entry.graph,
            .content_revision = 1};
}

void append_fragment(LayerStack& stack, const AppliedPresetApplication& application) {
    std::vector<LayerEntry> entries;
    entries.reserve(application.fragment.stack.size());
    for (const SmartMaterialEntry& entry : application.fragment.stack) {
        entries.push_back(layer_entry(entry, application.target_entry_identifier));
    }
    stack.append(entries);
}

void synchronize_fragment(LayerStack& stack, const AppliedPresetApplication& application) {
    for (const SmartMaterialEntry& entry : application.fragment.stack) {
        LayerEntry replacement = layer_entry(entry, application.target_entry_identifier);
        const LayerEntry& current = stack.entry(entry.identifier);
        replacement.content_revision =
            current.content_revision +
            static_cast<std::uint64_t>(replacement.graph != current.graph);
        stack.replace(entry.identifier, std::move(replacement));
    }
}

}  // namespace

PresetApplicationReport TextureSet::apply_smart_material(const SmartMaterialPreset& preset,
                                                         std::string application_identifier) {
    validate_smart_material(preset);
    require_new_application(preset_applications_, application_identifier);
    AppliedPresetApplication application{
        .identifier = std::move(application_identifier),
        .kind = AppliedPresetKind::smart_material,
        .target_entry_identifier = {},
        .origin = {.preset_identifier = preset.identifier, .schema_version = preset.schema_version},
        .fragment = preset,
        .parameter_values = {},
    };
    application.parameter_values = reset_fragment_parameters(application.fragment);
    const std::vector<std::string> entries =
        remap_fragment_entries(application.fragment, application.identifier);
    require_new_entry_identities(preset_applications_, entries);
    validate_smart_material(application.fragment);
    PresetApplicationReport report = application_report(application);
    LayerStack updated_stack = layer_stack_;
    append_fragment(updated_stack, application);
    preset_applications_.push_back(std::move(application));
    layer_stack_ = std::move(updated_stack);
    return report;
}

PresetApplicationReport TextureSet::apply_smart_mask(const SmartMaskPreset& preset,
                                                     std::string application_identifier,
                                                     std::string_view target_entry_identifier) {
    validate_smart_mask(preset);
    require_new_application(preset_applications_, application_identifier);
    const SmartMaterialEntry& target = applied_entry(target_entry_identifier);
    if (target.kind != SmartMaterialEntryKind::layer &&
        target.kind != SmartMaterialEntryKind::group) {
        invalid_application("smart mask target must be an applied layer or group");
    }
    const SmartMaskTargetKind target_kind = target.kind == SmartMaterialEntryKind::layer
                                                ? SmartMaskTargetKind::layer
                                                : SmartMaskTargetKind::group;
    SmartMaskInstance instance = instantiate_smart_mask(
        preset, application_identifier, std::string(target_entry_identifier), target_kind);
    AppliedPresetApplication application{
        .identifier = std::move(application_identifier),
        .kind = AppliedPresetKind::smart_mask,
        .target_entry_identifier = std::string(target_entry_identifier),
        .origin = {.preset_identifier = instance.origin_preset_identifier,
                   .schema_version = instance.origin_preset_schema_version},
        .fragment = std::move(instance.fragment),
        .parameter_values = {},
    };
    application.parameter_values.reserve(instance.parameter_values.size());
    for (SmartMaskParameterValue& value : instance.parameter_values) {
        application.parameter_values.push_back(
            {.parameter_identifier = std::move(value.parameter_identifier),
             .value = std::move(value.value)});
    }
    const std::vector<std::string> entries =
        remap_fragment_entries(application.fragment, application.identifier);
    require_new_entry_identities(preset_applications_, entries);
    validate_smart_material(application.fragment);
    PresetApplicationReport report = application_report(application);
    LayerStack updated_stack = layer_stack_;
    append_fragment(updated_stack, application);
    preset_applications_.push_back(std::move(application));
    layer_stack_ = std::move(updated_stack);
    return report;
}

SmartMaterialParameterUpdate TextureSet::set_applied_preset_parameter_value(
    std::string_view application_identifier, std::string_view parameter_identifier,
    graph::SocketValue value) {
    AppliedPresetApplication& current =
        find_application(preset_applications_, application_identifier);
    AppliedPresetApplication updated = current;
    SmartMaterialParameterUpdate report =
        set_smart_material_parameter_value(updated.fragment, parameter_identifier, value);
    const auto state =
        std::find_if(updated.parameter_values.begin(), updated.parameter_values.end(),
                     [&](const AppliedPresetParameterValue& parameter) {
                         return parameter.parameter_identifier == parameter_identifier;
                     });
    if (state == updated.parameter_values.end()) {
        invalid_application("applied preset parameter state is incomplete");
    }
    state->value = std::move(value);
    LayerStack updated_stack = layer_stack_;
    synchronize_fragment(updated_stack, updated);
    current = std::move(updated);
    layer_stack_ = std::move(updated_stack);
    return report;
}

PresetApplicationUndoReport TextureSet::undo_last_preset_application() {
    if (preset_applications_.empty()) {
        return {};
    }
    PresetApplicationUndoReport report{
        .removed = true,
        .application_identifier = preset_applications_.back().identifier,
        .entry_identifiers = {},
    };
    for (const SmartMaterialEntry& entry : preset_applications_.back().fragment.stack) {
        report.entry_identifiers.push_back(entry.identifier);
    }
    LayerStack updated_stack = layer_stack_;
    updated_stack.remove(report.entry_identifiers);
    layer_stack_ = std::move(updated_stack);
    preset_applications_.pop_back();
    return report;
}

const SmartMaterialEntry& TextureSet::applied_entry(std::string_view entry_identifier) const {
    const AppliedPresetApplication& application =
        find_entry_application(preset_applications_, entry_identifier);
    return *std::find_if(
        application.fragment.stack.begin(), application.fragment.stack.end(),
        [&](const SmartMaterialEntry& entry) { return entry.identifier == entry_identifier; });
}

void TextureSet::replace_applied_entry(std::string_view entry_identifier,
                                       SmartMaterialEntry replacement) {
    if (replacement.identifier != entry_identifier) {
        invalid_application("an applied preset entry edit cannot change its stable identity");
    }
    const AppliedPresetApplication& found =
        find_entry_application(preset_applications_, entry_identifier);
    AppliedPresetApplication& current = find_application(preset_applications_, found.identifier);
    AppliedPresetApplication updated = current;
    const auto entry = std::find_if(updated.fragment.stack.begin(), updated.fragment.stack.end(),
                                    [&](const SmartMaterialEntry& candidate) {
                                        return candidate.identifier == entry_identifier;
                                    });
    *entry = std::move(replacement);
    validate_smart_material(updated.fragment);
    LayerStack updated_stack = layer_stack_;
    synchronize_fragment(updated_stack, updated);
    current = std::move(updated);
    layer_stack_ = std::move(updated_stack);
}

const AppliedPresetOrigin& TextureSet::applied_entry_origin(
    std::string_view entry_identifier) const {
    return find_entry_application(preset_applications_, entry_identifier).origin;
}

}  // namespace ctex::doc
