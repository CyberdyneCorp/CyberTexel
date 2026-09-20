#include <array>
#include <cmath>
#include <ctex/doc/document.hpp>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

using namespace ctex::doc;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_rule(Callable&& callable, LayerStackRule rule, std::string_view message) {
    try {
        callable();
    } catch (const LayerStackError& error) {
        return expect(error.rule() == rule, message);
    } catch (...) {
    }
    return expect(false, message);
}

LayerEntry entry(std::string identifier, LayerEntryKind kind = LayerEntryKind::paint_layer) {
    return {.identifier = identifier,
            .display_name = identifier,
            .kind = kind,
            .parent_identifier = {},
            .target_identifier = {},
            .source_identifier = {},
            .enabled = true,
            .opacity = 1.0,
            .blend_mode = "normal",
            .channels = {},
            .graph = std::nullopt,
            .content_revision = 1};
}

TextureSet texture_set() {
    return TextureSet({.display_name = "Material",
                       .partition_kind = PartitionSourceKind::material,
                       .partition_key = "material",
                       .uv_set = "uv0",
                       .width = 4,
                       .height = 4,
                       .default_bit_depth = 8});
}

bool roughness_only_layer_respects_texture_set_enablement() {
    TextureSet set = texture_set();
    set.channels().enable("pbr.base_color");
    set.channels().enable("pbr.metallic");
    set.channels().enable("pbr.roughness");
    LayerEntry roughness = entry("roughness");
    roughness.opacity = 0.8;
    roughness.channels.push_back(
        {.semantic_id = "pbr.base_color", .enabled = false, .opacity = 0.75});
    roughness.channels.push_back({.semantic_id = "pbr.roughness", .enabled = true, .opacity = 0.5});
    set.layer_stack().append(std::move(roughness));

    const LayerChannelParticipation active =
        set.channel_participation("roughness", "pbr.roughness");
    const LayerChannelParticipation absent = set.channel_participation("roughness", "pbr.metallic");
    const LayerChannelParticipation explicitly_disabled =
        set.channel_participation("roughness", "pbr.base_color");
    set.channels().disable("pbr.roughness");
    const LayerChannelParticipation disabled =
        set.channel_participation("roughness", "pbr.roughness");
    return expect(active.participates && std::abs(active.effective_opacity - 0.4) < 1.0e-12,
                  "enabled roughness did not use its independent opacity") &&
           expect(!absent.participates && absent.effective_opacity == 0.0,
                  "a missing layer channel participated") &&
           expect(!explicitly_disabled.participates && explicitly_disabled.effective_opacity == 0.0,
                  "an explicitly disabled layer channel participated") &&
           expect(!disabled.participates && disabled.effective_opacity == 0.0,
                  "a disabled texture-set channel participated");
}

bool nested_groups_multiply_effective_opacity() {
    LayerStack stack;
    LayerEntry group = entry("group", LayerEntryKind::group);
    group.opacity = 0.5;
    stack.append(std::move(group));
    LayerEntry layer = entry("layer");
    layer.parent_identifier = "group";
    layer.opacity = 0.5;
    layer.channels.push_back({.semantic_id = "pbr.roughness", .enabled = true, .opacity = 1.0});
    stack.append(std::move(layer));

    const LayerChannelParticipation result =
        stack.channel_participation("layer", "pbr.roughness", true);
    stack.set_opacity("group", 0.25);
    const LayerChannelParticipation updated =
        stack.channel_participation("layer", "pbr.roughness", true);
    return expect(result.participates && std::abs(result.effective_opacity - 0.25) < 1.0e-12,
                  "layer and group opacity did not multiply to 0.25") &&
           expect(updated.participates && std::abs(updated.effective_opacity - 0.125) < 1.0e-12,
                  "group opacity edit was not reflected dynamically");
}

LayerStack masked_stack() {
    LayerStack stack;
    LayerEntry outer = entry("outer", LayerEntryKind::group);
    outer.opacity = 0.8;
    stack.append(std::move(outer));
    LayerEntry inner = entry("inner", LayerEntryKind::group);
    inner.parent_identifier = "outer";
    inner.opacity = 0.5;
    stack.append(std::move(inner));
    LayerEntry layer = entry("layer");
    layer.parent_identifier = "inner";
    layer.opacity = 0.5;
    layer.channels.push_back({.semantic_id = "pbr.height", .enabled = true, .opacity = 0.5});
    stack.append(std::move(layer));
    LayerEntry group_mask = entry("group-mask", LayerEntryKind::mask);
    group_mask.target_identifier = "outer";
    stack.append(std::move(group_mask));
    LayerEntry layer_mask = entry("layer-mask", LayerEntryKind::mask);
    layer_mask.target_identifier = "layer";
    stack.append(std::move(layer_mask));
    LayerEntry disabled_mask = entry("disabled-mask", LayerEntryKind::mask);
    disabled_mask.target_identifier = "inner";
    disabled_mask.enabled = false;
    stack.append(std::move(disabled_mask));
    return stack;
}

bool direct_and_group_masks_form_one_chain() {
    const LayerStack stack = masked_stack();
    const std::array samples{LayerMaskSample{"layer-mask", 0.5},
                             LayerMaskSample{"group-mask", 0.25}};
    const LayerChannelParticipation result =
        stack.channel_participation("layer", "pbr.height", true, samples);
    return expect(stack.applicable_masks("layer") ==
                      std::vector<std::string>{"group-mask", "layer-mask"},
                  "mask chain did not include direct and enclosing-group masks in stack order") &&
           expect(result.participates && std::abs(result.effective_opacity - 0.0125) < 1.0e-12,
                  "entry, channel, group, and per-texel mask opacity did not multiply") &&
           expect(result.mask_identifiers == std::vector<std::string>{"group-mask", "layer-mask"},
                  "participation report did not name every applied mask");
}

bool incomplete_or_invalid_mask_samples_are_refused() {
    const LayerStack stack = masked_stack();
    const std::array missing{LayerMaskSample{"group-mask", 0.5}};
    const std::array duplicate{LayerMaskSample{"group-mask", 0.5},
                               LayerMaskSample{"group-mask", 0.25}};
    const std::array unknown{LayerMaskSample{"group-mask", 0.5}, LayerMaskSample{"unknown", 0.25}};
    const std::array invalid{
        LayerMaskSample{"group-mask", 0.5},
        LayerMaskSample{"layer-mask", std::numeric_limits<double>::quiet_NaN()}};
    const std::array out_of_range{LayerMaskSample{"group-mask", 0.5},
                                  LayerMaskSample{"layer-mask", 1.01}};
    return expect_rule(
               [&] {
                   static_cast<void>(
                       stack.channel_participation("layer", "pbr.height", true, missing));
               },
               LayerStackRule::mask_sample, "missing mask sample was accepted") &&
           expect_rule(
               [&] {
                   static_cast<void>(
                       stack.channel_participation("layer", "pbr.height", true, duplicate));
               },
               LayerStackRule::mask_sample, "duplicate mask sample was accepted") &&
           expect_rule(
               [&] {
                   static_cast<void>(
                       stack.channel_participation("layer", "pbr.height", true, unknown));
               },
               LayerStackRule::mask_sample, "unknown mask sample was accepted") &&
           expect_rule(
               [&] {
                   static_cast<void>(
                       stack.channel_participation("layer", "pbr.height", true, invalid));
               },
               LayerStackRule::mask_sample, "non-finite mask sample was accepted") &&
           expect_rule(
               [&] {
                   static_cast<void>(
                       stack.channel_participation("layer", "pbr.height", true, out_of_range));
               },
               LayerStackRule::mask_sample, "out-of-range mask sample was accepted");
}

bool disabled_entries_gate_the_chain() {
    LayerStack stack = masked_stack();
    LayerEntry disabled_layer = stack.entry("layer");
    disabled_layer.enabled = false;
    stack.replace("layer", std::move(disabled_layer));
    const LayerChannelParticipation layer_result =
        stack.channel_participation("layer", "pbr.height", true);
    LayerEntry enabled_layer = stack.entry("layer");
    enabled_layer.enabled = true;
    stack.replace("layer", std::move(enabled_layer));
    LayerEntry disabled_group = stack.entry("inner");
    disabled_group.enabled = false;
    stack.replace("inner", std::move(disabled_group));
    const LayerChannelParticipation group_result =
        stack.channel_participation("layer", "pbr.height", true);
    return expect(!layer_result.participates && layer_result.effective_opacity == 0.0 &&
                      layer_result.mask_identifiers.empty(),
                  "a disabled layer participated") &&
           expect(!group_result.participates && group_result.effective_opacity == 0.0 &&
                      group_result.mask_identifiers.empty(),
                  "a layer inside a disabled group participated");
}

}  // namespace

int main() {
    return roughness_only_layer_respects_texture_set_enablement() &&
                   nested_groups_multiply_effective_opacity() &&
                   direct_and_group_masks_form_one_chain() &&
                   incomplete_or_invalid_mask_samples_are_refused() &&
                   disabled_entries_gate_the_chain()
               ? 0
               : 1;
}
