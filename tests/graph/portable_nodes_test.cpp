#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/graph/catalogue.hpp>
#include <ctex/graph/portable_nodes.hpp>
#include <iostream>
#include <string_view>

namespace {

using ctex::graph::ColourValue;
using ctex::graph::SocketValue;
using ctex::graph::VectorValue;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ctex::graph::NodeProperty& property(ctex::graph::GraphNode& node, std::string_view key) {
    const auto found = std::ranges::find(node.properties, key, &ctex::graph::NodeProperty::key);
    if (found == node.properties.end()) {
        throw std::out_of_range("test property not found");
    }
    return *found;
}

bool close(float left, float right, float tolerance = 1.0e-6F) {
    return std::abs(left - right) <= tolerance;
}

bool close(ColourValue left, ColourValue right) {
    return close(left.r, right.r) && close(left.g, right.g) && close(left.b, right.b) &&
           close(left.a, right.a);
}

bool seeded_noise_is_repeatable_and_sensitive_to_inputs() {
    auto noise = ctex::graph::make_builtin_node("ctex.texture.noise");
    property(noise, "seed").value = 73.0;
    const std::array<SocketValue, 6> inputs{
        VectorValue{0.125F, -4.5F, 8.25F}, 3.0, 4.25, 0.65, 2.1, 0.2};
    const auto first = ctex::graph::evaluate_builtin_node(noise, inputs);
    const auto second = ctex::graph::evaluate_builtin_node(noise, inputs);

    auto other_seed = noise;
    property(other_seed, "seed").value = 74.0;
    const auto changed_seed = ctex::graph::evaluate_builtin_node(other_seed, inputs);
    auto moved_inputs = inputs;
    moved_inputs[0] = VectorValue{0.126F, -4.5F, 8.25F};
    const auto changed_coordinate = ctex::graph::evaluate_builtin_node(noise, moved_inputs);

    const double factor = std::get<double>(first[0]);
    const ColourValue colour = std::get<ColourValue>(first[1]);
    return expect(first == second, "same noise seed and coordinates changed the output") &&
           expect(first != changed_seed && first != changed_coordinate,
                  "noise ignored its seed or coordinates") &&
           expect(factor >= 0.0 && factor <= 1.0 && colour.r >= 0.0F && colour.r <= 1.0F &&
                      colour.g >= 0.0F && colour.g <= 1.0F && colour.b >= 0.0F &&
                      colour.b <= 1.0F && colour.a == 1.0F,
                  "portable noise escaped its normalized output range");
}

bool every_blend_mode_uses_the_shared_formula() {
    constexpr ColourValue base{0.2F, 0.6F, 0.8F, 0.4F};
    constexpr ColourValue layer{0.9F, 0.3F, 0.1F, 0.8F};
    constexpr double factor = 0.65;
    auto blend = ctex::graph::make_builtin_node("ctex.colour.blend");
    const auto* declaration = ctex::graph::find_builtin_node_type("ctex.colour.blend");
    const auto modes = declaration->properties.front().allowed_values;
    for (const std::string& mode : modes) {
        property(blend, "mode").value = mode;
        const std::array<SocketValue, 3> inputs{factor, base, layer};
        const auto evaluated = ctex::graph::evaluate_builtin_node(blend, inputs);
        const ColourValue shared = ctex::graph::blend_colour(mode, base, layer, factor);
        if (!expect(evaluated.size() == 1 && close(std::get<ColourValue>(evaluated[0]), shared),
                    "Blend node diverged from the shared layer formula for " + mode)) {
            return false;
        }
    }
    return expect(modes.size() == 20, "portable evaluation did not cover all Blend modes");
}

bool representative_blend_values_are_fixed() {
    constexpr ColourValue base{0.25F, 0.5F, 0.75F, 0.2F};
    constexpr ColourValue layer{0.8F, 0.4F, 0.2F, 1.0F};
    return expect(close(ctex::graph::blend_colour("normal", base, layer, 1.0), layer),
                  "normal Blend formula changed") &&
           expect(close(ctex::graph::blend_colour("multiply", base, layer, 1.0),
                        ColourValue{0.2F, 0.2F, 0.15F, 1.0F}),
                  "multiply Blend formula changed") &&
           expect(close(ctex::graph::blend_colour("screen", base, layer, 1.0),
                        ColourValue{0.85F, 0.7F, 0.8F, 1.0F}),
                  "screen Blend formula changed") &&
           expect(close(ctex::graph::blend_colour("difference", base, layer, 1.0),
                        ColourValue{0.55F, 0.1F, 0.55F, 1.0F}),
                  "difference Blend formula changed");
}

bool unsupported_evaluation_is_explicit() {
    const auto checker = ctex::graph::make_builtin_node("ctex.texture.checker");
    try {
        static_cast<void>(ctex::graph::evaluate_builtin_node(checker, {}));
    } catch (const ctex::graph::BuiltinNodeEvaluationError& error) {
        return expect(
            std::string_view(error.what()).find("ctex.texture.checker") != std::string_view::npos,
            "unsupported built-in evaluation did not name its type");
    }
    return expect(false, "unsupported built-in evaluation was silently accepted");
}

}  // namespace

int main() {
    return seeded_noise_is_repeatable_and_sensitive_to_inputs() &&
                   every_blend_mode_uses_the_shared_formula() &&
                   representative_blend_values_are_fixed() && unsupported_evaluation_is_explicit()
               ? 0
               : 1;
}
