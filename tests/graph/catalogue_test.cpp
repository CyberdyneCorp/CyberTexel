#include <algorithm>
#include <array>
#include <ctex/graph/catalogue.hpp>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using ctex::graph::NodeCategory;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <std::size_t Size>
bool category_is_exact(NodeCategory category, const std::array<std::string_view, Size>& expected) {
    std::set<std::string_view> actual;
    for (const auto& declaration : ctex::graph::builtin_node_types()) {
        if (declaration.category == category) {
            actual.insert(declaration.display_name);
        }
    }
    return expect(actual == std::set<std::string_view>(expected.begin(), expected.end()),
                  "built-in node family differs from the specified catalogue");
}

bool all_four_families_are_complete() {
    constexpr std::array input_names{
        std::string_view{"Anchor Point"},       std::string_view{"Constant Colour"},
        std::string_view{"Constant Value"},     std::string_view{"Geometry"},
        std::string_view{"Layer Reference"},    std::string_view{"Mesh Map"},
        std::string_view{"Object Info"},        std::string_view{"Picker"},
        std::string_view{"Texture Coordinate"}, std::string_view{"UV Set"},
    };
    constexpr std::array texture_names{
        std::string_view{"Brick"},    std::string_view{"Checker"},    std::string_view{"Gabor"},
        std::string_view{"Gradient"}, std::string_view{"Image"},      std::string_view{"Magic"},
        std::string_view{"Noise"},    std::string_view{"Tile Sheet"}, std::string_view{"Voronoi"},
        std::string_view{"Wave"},
    };
    constexpr std::array colour_names{
        std::string_view{"Blend"},
        std::string_view{"Blur"},
        std::string_view{"Brightness/Contrast"},
        std::string_view{"Colour Mask"},
        std::string_view{"Colour Ramp"},
        std::string_view{"Combine Colour"},
        std::string_view{"Curves"},
        std::string_view{"Gamma"},
        std::string_view{"Grayscale Conversion"},
        std::string_view{"Hue/Saturation/Value"},
        std::string_view{"Invert"},
        std::string_view{"Levels"},
        std::string_view{"Mix"},
        std::string_view{"Quantize"},
        std::string_view{"Replace Colour"},
        std::string_view{"Separate Colour"},
        std::string_view{"Sharpen"},
        std::string_view{"Warp"},
    };
    constexpr std::array vector_names{
        std::string_view{"Bump"},          std::string_view{"Clamp"},
        std::string_view{"Combine XYZ"},   std::string_view{"Float Curve"},
        std::string_view{"Map Range"},     std::string_view{"Mapping"},
        std::string_view{"Math"},          std::string_view{"Mix Normal Map"},
        std::string_view{"Normal Map"},    std::string_view{"Separate XYZ"},
        std::string_view{"Vector Curves"}, std::string_view{"Vector Math"},
        std::string_view{"Vector Rotate"}, std::string_view{"Vector Transform"},
    };
    return expect(ctex::graph::builtin_node_types().size() == 52,
                  "built-in catalogue does not contain exactly 52 node types") &&
           category_is_exact(NodeCategory::input, input_names) &&
           category_is_exact(NodeCategory::texture, texture_names) &&
           category_is_exact(NodeCategory::colour_filter, colour_names) &&
           category_is_exact(NodeCategory::vector_math, vector_names);
}

const ctex::graph::NodePropertyDeclaration* property(const ctex::graph::NodeTypeDeclaration& type,
                                                     std::string_view identifier) {
    const auto found =
        std::find_if(type.properties.begin(), type.properties.end(),
                     [&](const auto& candidate) { return candidate.identifier == identifier; });
    return found == type.properties.end() ? nullptr : &*found;
}

bool declarations_are_stable_and_instantiable() {
    const auto catalogue = ctex::graph::builtin_node_types();
    ctex::graph::GraphNode output{
        .role = ctex::graph::NodeRole::output,
        .type_id = "test.output",
        .type_version = 1,
        .display_name = "Output",
        .position = {},
        .inputs = {},
        .outputs = {},
        .properties = {},
    };
    ctex::graph::GraphDocument graph(std::move(output));
    std::string_view previous_id;
    for (const auto& declaration : catalogue) {
        if (!expect(!declaration.type_id.empty() && declaration.version == 1 &&
                        declaration.type_id > previous_id && !declaration.outputs.empty() &&
                        ctex::graph::find_builtin_node_type(declaration.type_id) == &declaration,
                    "catalogue identity, ordering, version, or lookup is invalid")) {
            return false;
        }
        const auto node = ctex::graph::make_builtin_node(declaration.type_id, {2.0F, 3.0F});
        if (!expect(node.type_id == declaration.type_id &&
                        node.type_version == declaration.version &&
                        node.display_name == declaration.display_name &&
                        node.inputs == declaration.inputs && node.outputs == declaration.outputs &&
                        node.position == ctex::graph::NodePosition{2.0F, 3.0F} &&
                        node.properties.size() == declaration.properties.size(),
                    "built-in node factory diverges from its declaration")) {
            return false;
        }
        for (std::size_t index = 0; index < node.properties.size(); ++index) {
            if (!expect(
                    node.properties[index].key == declaration.properties[index].identifier &&
                        node.properties[index].value == declaration.properties[index].default_value,
                    "built-in node factory changed a declared property default")) {
                return false;
            }
        }
        static_cast<void>(graph.add_node(node));
        previous_id = declaration.type_id;
    }
    if (!expect(ctex::graph::deserialize_graph(ctex::graph::serialize_graph(graph)) == graph,
                "graph containing every built-in node did not round-trip")) {
        return false;
    }
    try {
        static_cast<void>(ctex::graph::make_builtin_node("ctex.missing"));
    } catch (const std::out_of_range&) {
        return true;
    }
    return expect(false, "unknown built-in node type was instantiated");
}

bool property_choices_are_valid() {
    for (const auto& declaration : ctex::graph::builtin_node_types()) {
        std::set<std::string_view> property_ids;
        for (const auto& declared_property : declaration.properties) {
            if (!expect(property_ids.insert(declared_property.identifier).second,
                        "node declaration repeats a property identifier")) {
                return false;
            }
            if (declared_property.allowed_values.empty()) {
                continue;
            }
            const auto* selected = std::get_if<std::string>(&declared_property.default_value);
            const bool default_allowed =
                selected != nullptr &&
                std::find(declared_property.allowed_values.begin(),
                          declared_property.allowed_values.end(),
                          *selected) != declared_property.allowed_values.end();
            const std::set<std::string_view> unique(declared_property.allowed_values.begin(),
                                                    declared_property.allowed_values.end());
            if (!expect(default_allowed && unique.size() == declared_property.allowed_values.size(),
                        "node choice has an invalid default or duplicate value")) {
                return false;
            }
        }
    }
    return true;
}

bool blend_and_normal_modes_are_complete() {
    constexpr std::array blend_modes{
        std::string_view{"normal"},       std::string_view{"darken"},
        std::string_view{"multiply"},     std::string_view{"color_burn"},
        std::string_view{"lighten"},      std::string_view{"screen"},
        std::string_view{"color_dodge"},  std::string_view{"add"},
        std::string_view{"overlay"},      std::string_view{"soft_light"},
        std::string_view{"linear_light"}, std::string_view{"difference"},
        std::string_view{"exclusion"},    std::string_view{"subtract"},
        std::string_view{"divide"},       std::string_view{"hue"},
        std::string_view{"saturation"},   std::string_view{"color"},
        std::string_view{"value"},        std::string_view{"pass_through"},
    };
    constexpr std::array normal_modes{std::string_view{"partial_derivative"},
                                      std::string_view{"whiteout"}, std::string_view{"reoriented"}};
    const auto* blend = ctex::graph::find_builtin_node_type("ctex.colour.blend");
    const auto* mix_normal = ctex::graph::find_builtin_node_type("ctex.vector.mix-normal-map");
    const auto* blend_property = blend == nullptr ? nullptr : property(*blend, "mode");
    const auto* normal_property = mix_normal == nullptr ? nullptr : property(*mix_normal, "mode");
    return expect(blend_property != nullptr &&
                      blend_property->allowed_values ==
                          std::vector<std::string>(blend_modes.begin(), blend_modes.end()),
                  "Blend node does not expose the complete blend-mode set") &&
           expect(normal_property != nullptr &&
                      normal_property->allowed_values ==
                          std::vector<std::string>(normal_modes.begin(), normal_modes.end()),
                  "Mix Normal Map node does not expose all required modes");
}

bool operations_have_unique_formulas(std::span<const ctex::graph::OperationDefinition> operations,
                                     std::string_view node_type) {
    std::set<std::string_view> identifiers;
    for (const auto& operation : operations) {
        if (!expect(!operation.identifier.empty() && !operation.display_name.empty() &&
                        !operation.formula.empty() &&
                        identifiers.insert(operation.identifier).second,
                    "math operation is unnamed, undocumented, or duplicated")) {
            return false;
        }
    }
    const auto* node = ctex::graph::find_builtin_node_type(node_type);
    const auto* operation_property = node == nullptr ? nullptr : property(*node, "operation");
    if (!expect(operation_property != nullptr &&
                    operation_property->allowed_values.size() == operations.size(),
                "math node choices diverge from operation definitions")) {
        return false;
    }
    return std::equal(
        operations.begin(), operations.end(), operation_property->allowed_values.begin(),
        [](const auto& operation, const auto& choice) { return operation.identifier == choice; });
}

}  // namespace

int main() {
    return all_four_families_are_complete() && declarations_are_stable_and_instantiable() &&
                   property_choices_are_valid() && blend_and_normal_modes_are_complete() &&
                   operations_have_unique_formulas(ctex::graph::math_operations(),
                                                   "ctex.math.scalar") &&
                   operations_have_unique_formulas(ctex::graph::vector_math_operations(),
                                                   "ctex.math.vector")
               ? 0
               : 1;
}
