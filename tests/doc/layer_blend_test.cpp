#include <array>
#include <cmath>
#include <ctex/doc/layer_stack.hpp>
#include <ctex/graph/portable_nodes.hpp>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace {

using ctex::doc::LayerEntry;
using ctex::doc::LayerEntryKind;
using ctex::doc::LayerStack;
using ctex::doc::LayerStackError;
using ctex::doc::LayerStackRule;
using ctex::graph::BlendModeDefinition;
using ctex::graph::ColourValue;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool close(float left, float right) { return std::abs(left - right) <= 1.0e-6F; }

bool close(ColourValue left, ColourValue right) {
    return close(left.r, right.r) && close(left.g, right.g) && close(left.b, right.b) &&
           close(left.a, right.a);
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

struct ExpectedBlend {
    std::string_view mode;
    ColourValue result;
};

constexpr std::array expected_blends{
    ExpectedBlend{"normal", {0.8F, 0.4F, 0.2F, 1.0F}},
    ExpectedBlend{"darken", {0.25F, 0.4F, 0.2F, 1.0F}},
    ExpectedBlend{"multiply", {0.2F, 0.2F, 0.15F, 1.0F}},
    ExpectedBlend{"color_burn", {0.0625F, 0.0F, 0.0F, 1.0F}},
    ExpectedBlend{"lighten", {0.8F, 0.5F, 0.75F, 1.0F}},
    ExpectedBlend{"screen", {0.85F, 0.7F, 0.8F, 1.0F}},
    ExpectedBlend{"color_dodge", {1.0F, 0.83333333F, 0.9375F, 1.0F}},
    ExpectedBlend{"add", {1.0F, 0.9F, 0.95F, 1.0F}},
    ExpectedBlend{"overlay", {0.4F, 0.4F, 0.6F, 1.0F}},
    ExpectedBlend{"soft_light", {0.4F, 0.45F, 0.6375F, 1.0F}},
    ExpectedBlend{"linear_light", {0.85F, 0.3F, 0.15F, 1.0F}},
    ExpectedBlend{"difference", {0.55F, 0.1F, 0.55F, 1.0F}},
    ExpectedBlend{"exclusion", {0.65F, 0.5F, 0.65F, 1.0F}},
    ExpectedBlend{"subtract", {0.0F, 0.1F, 0.55F, 1.0F}},
    ExpectedBlend{"divide", {0.3125F, 1.0F, 1.0F, 1.0F}},
    ExpectedBlend{"hue", {0.75F, 0.41666667F, 0.25F, 1.0F}},
    ExpectedBlend{"saturation", {0.1875F, 0.46875F, 0.75F, 1.0F}},
    ExpectedBlend{"color", {0.75F, 0.375F, 0.1875F, 1.0F}},
    ExpectedBlend{"value", {0.26666667F, 0.53333333F, 0.8F, 1.0F}},
    ExpectedBlend{"pass_through", {0.8F, 0.4F, 0.2F, 1.0F}},
};

bool catalogue_has_named_formulas() {
    const std::span<const BlendModeDefinition> definitions = ctex::graph::blend_mode_definitions();
    std::set<std::string_view> identifiers;
    bool complete = definitions.size() == expected_blends.size();
    for (std::size_t index = 0; index < definitions.size(); ++index) {
        complete = complete && !definitions[index].display_name.empty() &&
                   !definitions[index].formula.empty() &&
                   identifiers.insert(definitions[index].identifier).second &&
                   definitions[index].identifier == expected_blends[index].mode;
    }
    return expect(complete, "blend-mode catalogue is incomplete, duplicated, or undocumented");
}

bool every_document_mode_has_a_fixed_formula() {
    constexpr ColourValue base{0.25F, 0.5F, 0.75F, 0.2F};
    constexpr ColourValue layer{0.8F, 0.4F, 0.2F, 1.0F};
    for (const ExpectedBlend& expected : expected_blends) {
        LayerStack stack;
        LayerEntry blended =
            entry("blended", expected.mode == "pass_through" ? LayerEntryKind::group
                                                             : LayerEntryKind::paint_layer);
        blended.blend_mode = expected.mode;
        stack.append(std::move(blended));
        if (!expect(close(stack.evaluate_blend("blended", base, layer, 1.0), expected.result),
                    "document blend formula changed for " + std::string(expected.mode))) {
            return false;
        }
    }
    return true;
}

bool invalid_modes_are_transactionally_refused() {
    LayerStack stack;
    stack.append(entry("paint"));
    const LayerStack before = stack;
    bool pass_through_refused = false;
    try {
        stack.set_blend_mode("paint", "pass_through");
    } catch (const LayerStackError& error) {
        pass_through_refused = error.rule() == LayerStackRule::blend_mode;
    }
    bool unknown_refused = false;
    try {
        stack.set_blend_mode("paint", "unknown");
    } catch (const LayerStackError& error) {
        unknown_refused = error.rule() == LayerStackRule::blend_mode;
    }
    bool non_finite_refused = false;
    try {
        static_cast<void>(
            stack.evaluate_blend("paint", {}, {}, std::numeric_limits<double>::quiet_NaN()));
    } catch (const LayerStackError& error) {
        non_finite_refused = error.rule() == LayerStackRule::blend_mode;
    }
    return expect(pass_through_refused && unknown_refused && non_finite_refused && stack == before,
                  "invalid blend mode changed the layer or returned the wrong rule");
}

}  // namespace

int main() {
    return catalogue_has_named_formulas() && every_document_mode_has_a_fixed_formula() &&
                   invalid_modes_are_transactionally_refused()
               ? 0
               : 1;
}
