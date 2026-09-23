#include <algorithm>
#include <array>
#include <ctex/graph/catalogue.hpp>
#include <ctex/graph/portable_nodes.hpp>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace ctex::graph {
namespace {

using enum NodeCategory;
using enum SocketType;

constexpr std::array math_operation_definitions{
    OperationDefinition{"add", "Add", "a + b"},
    OperationDefinition{"subtract", "Subtract", "a - b"},
    OperationDefinition{"multiply", "Multiply", "a * b"},
    OperationDefinition{"divide", "Divide", "b == 0 ? 0 : a / b"},
    OperationDefinition{"multiply_add", "Multiply Add", "a * b + c"},
    OperationDefinition{"power", "Power", "pow(a, b) over the real domain; otherwise 0"},
    OperationDefinition{"logarithm", "Logarithm", "a > 0 and b > 0 and b != 1 ? log(a)/log(b) : 0"},
    OperationDefinition{"square_root", "Square Root", "sqrt(max(a, 0))"},
    OperationDefinition{"inverse_square_root", "Inverse Square Root", "a > 0 ? 1/sqrt(a) : 0"},
    OperationDefinition{"absolute", "Absolute", "abs(a)"},
    OperationDefinition{"exponent", "Exponent", "exp(a)"},
    OperationDefinition{"minimum", "Minimum", "min(a, b)"},
    OperationDefinition{"maximum", "Maximum", "max(a, b)"},
    OperationDefinition{"less_than", "Less Than", "a < b ? 1 : 0"},
    OperationDefinition{"greater_than", "Greater Than", "a > b ? 1 : 0"},
    OperationDefinition{"sign", "Sign", "a < 0 ? -1 : (a > 0 ? 1 : 0)"},
    OperationDefinition{"compare", "Compare", "abs(a - b) <= max(c, 0) ? 1 : 0"},
    OperationDefinition{"smooth_minimum", "Smooth Minimum",
                        "h=max(c-abs(a-b),0)/c; min(a,b)-h*h*c/4; c<=0 uses min"},
    OperationDefinition{"smooth_maximum", "Smooth Maximum", "-smooth_minimum(-a, -b, c)"},
    OperationDefinition{"round", "Round", "floor(a + 0.5)"},
    OperationDefinition{"floor", "Floor", "floor(a)"},
    OperationDefinition{"ceil", "Ceil", "ceil(a)"},
    OperationDefinition{"truncate", "Truncate", "trunc(a)"},
    OperationDefinition{"fraction", "Fraction", "a - floor(a)"},
    OperationDefinition{"modulo", "Modulo", "b == 0 ? 0 : a - b*floor(a/b)"},
    OperationDefinition{"wrap", "Wrap", "b==c ? b : a-(c-b)*floor((a-b)/(c-b))"},
    OperationDefinition{"snap", "Snap", "b == 0 ? 0 : floor(a/b)*b"},
    OperationDefinition{"ping_pong", "Ping-Pong", "b<=0 ? 0 : b-abs(modulo(a,2*b)-b)"},
    OperationDefinition{"sine", "Sine", "sin(a)"},
    OperationDefinition{"cosine", "Cosine", "cos(a)"},
    OperationDefinition{"tangent", "Tangent", "tan(a)"},
    OperationDefinition{"arcsine", "Arcsine", "asin(clamp(a,-1,1))"},
    OperationDefinition{"arccosine", "Arccosine", "acos(clamp(a,-1,1))"},
    OperationDefinition{"arctangent", "Arctangent", "atan(a)"},
    OperationDefinition{"arctan2", "Arctan2", "atan2(a, b)"},
    OperationDefinition{"sinh", "Hyperbolic Sine", "sinh(a)"},
    OperationDefinition{"cosh", "Hyperbolic Cosine", "cosh(a)"},
    OperationDefinition{"tanh", "Hyperbolic Tangent", "tanh(a)"},
    OperationDefinition{"radians", "To Radians", "a * pi / 180"},
    OperationDefinition{"degrees", "To Degrees", "a * 180 / pi"},
};

constexpr std::array vector_math_operation_definitions{
    OperationDefinition{"add", "Add", "a + b component-wise"},
    OperationDefinition{"subtract", "Subtract", "a - b component-wise"},
    OperationDefinition{"multiply", "Multiply", "a * b component-wise"},
    OperationDefinition{"divide", "Divide", "a / b component-wise; a zero divisor yields 0"},
    OperationDefinition{"multiply_add", "Multiply Add", "a * b + c component-wise"},
    OperationDefinition{"cross_product", "Cross Product", "cross(a, b)"},
    OperationDefinition{"project", "Project", "dot(a,b)/dot(b,b) * b; zero b yields zero"},
    OperationDefinition{"reflect", "Reflect", "a - 2*dot(a,n)*n with n=normalize(b)"},
    OperationDefinition{"refract", "Refract",
                        "refract(a, normalize(b), scale); total internal reflection yields zero"},
    OperationDefinition{"faceforward", "Faceforward", "dot(c,b)<0 ? a : -a"},
    OperationDefinition{"dot_product", "Dot Product", "dot(a, b), returned by Value"},
    OperationDefinition{"distance", "Distance", "length(a - b), returned by Value"},
    OperationDefinition{"length", "Length", "length(a), returned by Value"},
    OperationDefinition{"scale", "Scale", "a * scale"},
    OperationDefinition{"normalize", "Normalize", "length(a)==0 ? zero : a/length(a)"},
    OperationDefinition{"absolute", "Absolute", "abs(a) component-wise"},
    OperationDefinition{"minimum", "Minimum", "min(a, b) component-wise"},
    OperationDefinition{"maximum", "Maximum", "max(a, b) component-wise"},
    OperationDefinition{"floor", "Floor", "floor(a) component-wise"},
    OperationDefinition{"ceil", "Ceil", "ceil(a) component-wise"},
    OperationDefinition{"fraction", "Fraction", "a - floor(a) component-wise"},
    OperationDefinition{"modulo", "Modulo",
                        "a - b*floor(a/b) component-wise; zero divisor yields 0"},
    OperationDefinition{"wrap", "Wrap",
                        "wrap each a component between corresponding b and c components"},
    OperationDefinition{"snap", "Snap", "floor(a/b)*b component-wise; zero step yields 0"},
    OperationDefinition{"sine", "Sine", "sin(a) component-wise"},
    OperationDefinition{"cosine", "Cosine", "cos(a) component-wise"},
    OperationDefinition{"tangent", "Tangent", "tan(a) component-wise"},
};

NodeSocket socket(std::string identifier, std::string display_name, SocketType type,
                  SocketValue value = {}) {
    return {.identifier = std::move(identifier),
            .display_name = std::move(display_name),
            .type = type,
            .value = std::move(value)};
}

NodeSocket input_socket(std::string identifier, std::string display_name, SocketType type,
                        SocketValue value = {}) {
    return socket(std::move(identifier), std::move(display_name), type, std::move(value));
}

NodeSocket output(std::string identifier, std::string display_name, SocketType type) {
    return socket(std::move(identifier), std::move(display_name), type);
}

NodePropertyDeclaration property(std::string identifier, std::string display_name,
                                 SocketValue value) {
    return {.identifier = std::move(identifier),
            .display_name = std::move(display_name),
            .default_value = std::move(value),
            .allowed_values = {}};
}

NodePropertyDeclaration choice(std::string identifier, std::string display_name,
                               std::string selected,
                               std::initializer_list<std::string_view> choices) {
    NodePropertyDeclaration result{.identifier = std::move(identifier),
                                   .display_name = std::move(display_name),
                                   .default_value = std::move(selected),
                                   .allowed_values = {}};
    result.allowed_values.reserve(choices.size());
    for (std::string_view value : choices) {
        result.allowed_values.emplace_back(value);
    }
    return result;
}

NodePropertyDeclaration operation_choice(std::string identifier, std::string display_name,
                                         std::span<const OperationDefinition> operations) {
    NodePropertyDeclaration result{.identifier = std::move(identifier),
                                   .display_name = std::move(display_name),
                                   .default_value = std::string(operations.front().identifier),
                                   .allowed_values = {}};
    result.allowed_values.reserve(operations.size());
    for (const OperationDefinition& operation : operations) {
        result.allowed_values.emplace_back(operation.identifier);
    }
    return result;
}

NodePropertyDeclaration blend_mode_choice() {
    NodePropertyDeclaration result{.identifier = "mode",
                                   .display_name = "Mode",
                                   .default_value = std::string("normal"),
                                   .allowed_values = {}};
    for (const BlendModeDefinition& mode : blend_mode_definitions()) {
        result.allowed_values.emplace_back(mode.identifier);
    }
    return result;
}

NodeTypeDeclaration type(NodeCategory category, std::string type_id, std::string display_name,
                         std::vector<NodeSocket> inputs, std::vector<NodeSocket> outputs,
                         std::vector<NodePropertyDeclaration> properties = {}) {
    return {.type_id = std::move(type_id),
            .version = 1,
            .display_name = std::move(display_name),
            .category = category,
            .inputs = std::move(inputs),
            .outputs = std::move(outputs),
            .properties = std::move(properties)};
}

std::vector<NodeTypeDeclaration> input_nodes() {
    return {
        type(input, "ctex.input.constant-value", "Constant Value", {},
             {output("value", "Value", scalar)}, {property("value", "Value", 0.0)}),
        type(input, "ctex.input.constant-colour", "Constant Colour", {},
             {output("colour", "Colour", colour)},
             {property("colour", "Colour", ColourValue{0.8F, 0.8F, 0.8F, 1.0F})}),
        type(input, "ctex.input.texture-coordinate", "Texture Coordinate", {},
             {output("generated", "Generated", vector), output("normal", "Normal", vector),
              output("uv", "UV", vector), output("object", "Object", vector),
              output("camera", "Camera", vector), output("window", "Window", vector),
              output("reflection", "Reflection", vector)}),
        type(input, "ctex.input.uv-set", "UV Set", {}, {output("uv", "UV", vector)},
             {property("uv_set", "UV Set", std::string("UVMap"))}),
        type(input, "ctex.input.geometry", "Geometry", {},
             {output("position", "Position", vector), output("normal", "Normal", vector),
              output("geometric_normal", "Geometric Normal", vector),
              output("tangent", "Tangent", vector), output("incoming", "Incoming", vector),
              output("backfacing", "Backfacing", boolean)}),
        type(input, "ctex.input.object-info", "Object Info", {},
             {output("location", "Location", vector), output("scale", "Scale", vector),
              output("random", "Random", scalar), output("object_id", "Object ID", string)}),
        type(input, "ctex.input.mesh-map", "Mesh Map", {},
             {output("value", "Value", scalar), output("colour", "Colour", colour)},
             {property("map", "Map", std::string("ambient_occlusion"))}),
        type(input, "ctex.input.layer-reference", "Layer Reference", {},
             {output("value", "Value", scalar), output("colour", "Colour", colour)},
             {property("layer", "Layer", std::string{})}),
        type(input, "ctex.input.anchor-point", "Anchor Point", {},
             {output("value", "Value", scalar), output("colour", "Colour", colour)},
             {property("anchor", "Anchor", std::string{})}),
        type(input, "ctex.input.picker", "Picker", {},
             {output("value", "Value", scalar), output("colour", "Colour", colour),
              output("vector", "Vector", vector)},
             {property("channel", "Channel", std::string("base_color"))}),
    };
}

std::vector<NodeTypeDeclaration> texture_nodes() {
    return {
        type(texture, "ctex.texture.image", "Image",
             {input_socket("vector", "Vector", vector, VectorValue{})},
             {output("colour", "Colour", colour), output("alpha", "Alpha", scalar)},
             {property("image", "Image", ImageValue{}),
              choice("interpolation", "Interpolation", "linear", {"nearest", "linear", "cubic"}),
              choice("extension", "Extension", "repeat", {"repeat", "extend", "clip", "mirror"}),
              property("colour_space", "Colour Space", std::string("automatic"))}),
        type(texture, "ctex.texture.noise", "Noise",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("scale", "Scale", scalar, 5.0),
              input_socket("detail", "Detail", scalar, 2.0),
              input_socket("roughness", "Roughness", scalar, 0.5),
              input_socket("lacunarity", "Lacunarity", scalar, 2.0),
              input_socket("distortion", "Distortion", scalar, 0.0)},
             {output("factor", "Factor", scalar), output("colour", "Colour", colour)},
             {choice("dimensions", "Dimensions", "3d", {"1d", "2d", "3d", "4d"}),
              property("normalize", "Normalize", false), property("seed", "Seed", 0.0)}),
        type(texture, "ctex.texture.voronoi", "Voronoi",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("scale", "Scale", scalar, 5.0),
              input_socket("randomness", "Randomness", scalar, 1.0)},
             {output("distance", "Distance", scalar), output("colour", "Colour", colour),
              output("position", "Position", vector)},
             {choice("feature", "Feature", "f1", {"f1", "f2", "smooth_f1", "distance_to_edge"}),
              choice("distance", "Distance", "euclidean",
                     {"euclidean", "manhattan", "chebyshev", "minkowski"}),
              property("seed", "Seed", 0.0)}),
        type(texture, "ctex.texture.gradient", "Gradient",
             {input_socket("vector", "Vector", vector, VectorValue{})},
             {output("factor", "Factor", scalar)},
             {choice("mode", "Mode", "linear",
                     {"linear", "quadratic", "easing", "diagonal", "radial", "quadratic_sphere",
                      "spherical"})}),
        type(texture, "ctex.texture.checker", "Checker",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("colour_a", "Colour A", colour, ColourValue{0.8F, 0.8F, 0.8F, 1.0F}),
              input_socket("colour_b", "Colour B", colour, ColourValue{0.2F, 0.2F, 0.2F, 1.0F}),
              input_socket("scale", "Scale", scalar, 5.0)},
             {output("colour", "Colour", colour), output("factor", "Factor", scalar)}),
        type(texture, "ctex.texture.brick", "Brick",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("brick_colour", "Brick Colour", colour,
                           ColourValue{0.6F, 0.2F, 0.1F, 1.0F}),
              input_socket("brick_colour_2", "Brick Colour 2", colour,
                           ColourValue{0.4F, 0.1F, 0.05F, 1.0F}),
              input_socket("mortar_colour", "Mortar Colour", colour,
                           ColourValue{0.05F, 0.05F, 0.05F, 1.0F}),
              input_socket("scale", "Scale", scalar, 5.0),
              input_socket("mortar_size", "Mortar Size", scalar, 0.02),
              input_socket("bias", "Bias", scalar, 0.0),
              input_socket("brick_width", "Brick Width", scalar, 0.5),
              input_socket("row_height", "Row Height", scalar, 0.25)},
             {output("colour", "Colour", colour), output("factor", "Factor", scalar)},
             {property("offset", "Offset", 0.5),
              property("offset_frequency", "Offset Frequency", 2.0),
              property("squash", "Squash", 1.0),
              property("squash_frequency", "Squash Frequency", 2.0)}),
        type(texture, "ctex.texture.wave", "Wave",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("scale", "Scale", scalar, 5.0),
              input_socket("distortion", "Distortion", scalar, 0.0),
              input_socket("detail", "Detail", scalar, 2.0),
              input_socket("detail_scale", "Detail Scale", scalar, 1.0),
              input_socket("detail_roughness", "Detail Roughness", scalar, 0.5),
              input_socket("phase", "Phase", scalar, 0.0)},
             {output("colour", "Colour", colour), output("factor", "Factor", scalar)},
             {choice("wave_type", "Wave Type", "bands", {"bands", "rings"}),
              choice("direction", "Direction", "x", {"x", "y", "z", "diagonal"}),
              choice("profile", "Profile", "sine", {"sine", "saw", "triangle"})}),
        type(texture, "ctex.texture.magic", "Magic",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("scale", "Scale", scalar, 5.0),
              input_socket("distortion", "Distortion", scalar, 1.0)},
             {output("colour", "Colour", colour), output("factor", "Factor", scalar)},
             {property("depth", "Depth", 2.0)}),
        type(texture, "ctex.texture.gabor", "Gabor",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("scale", "Scale", scalar, 5.0),
              input_socket("frequency", "Frequency", scalar, 1.0),
              input_socket("anisotropy", "Anisotropy", scalar, 0.0),
              input_socket("orientation", "Orientation", scalar, 0.0)},
             {output("value", "Value", scalar), output("phase", "Phase", scalar),
              output("intensity", "Intensity", scalar)},
             {choice("dimensions", "Dimensions", "2d", {"2d", "3d"}),
              property("seed", "Seed", 0.0)}),
        type(texture, "ctex.texture.tile-sheet", "Tile Sheet",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("tile", "Tile", scalar, 0.0)},
             {output("colour", "Colour", colour), output("alpha", "Alpha", scalar)},
             {property("image", "Image", ImageValue{}), property("columns", "Columns", 1.0),
              property("rows", "Rows", 1.0), property("padding", "Padding", 0.0)}),
    };
}

std::vector<NodeTypeDeclaration> colour_filter_nodes() {
    return {
        type(colour_filter, "ctex.colour.mix", "Mix",
             {input_socket("factor", "Factor", scalar, 0.5),
              input_socket("a", "A", colour, ColourValue{0.0F, 0.0F, 0.0F, 1.0F}),
              input_socket("b", "B", colour, ColourValue{1.0F, 1.0F, 1.0F, 1.0F})},
             {output("colour", "Colour", colour)}, {property("clamp", "Clamp", false)}),
        type(colour_filter, "ctex.colour.blend", "Blend",
             {input_socket("background", "Background", colour, ColourValue{0.0F, 0.0F, 0.0F, 1.0F}),
              input_socket("foreground", "Foreground", colour, ColourValue{1.0F, 1.0F, 1.0F, 1.0F}),
              input_socket("opacity", "Opacity", scalar, 1.0)},
             {output("colour", "Colour", colour)}, {blend_mode_choice()}),
        type(colour_filter, "ctex.colour.levels", "Levels",
             {input_socket("colour", "Colour", colour), input_socket("black", "Black", scalar, 0.0),
              input_socket("white", "White", scalar, 1.0),
              input_socket("gamma", "Gamma", scalar, 1.0)},
             {output("colour", "Colour", colour)}),
        type(colour_filter, "ctex.colour.curves", "Curves",
             {input_socket("factor", "Factor", scalar, 1.0),
              input_socket("colour", "Colour", colour)},
             {output("colour", "Colour", colour)},
             {property("curve", "Curve", std::string("0:0;1:1"))}),
        type(colour_filter, "ctex.colour.ramp", "Colour Ramp",
             {input_socket("factor", "Factor", scalar, 0.5)}, {output("colour", "Colour", colour)},
             {property("stops", "Stops", std::string("0:000000ff;1:ffffffff")),
              choice("interpolation", "Interpolation", "linear",
                     {"constant", "linear", "ease", "cardinal", "b_spline"})}),
        type(colour_filter, "ctex.colour.hsv", "Hue/Saturation/Value",
             {input_socket("factor", "Factor", scalar, 1.0),
              input_socket("colour", "Colour", colour), input_socket("hue", "Hue", scalar, 0.5),
              input_socket("saturation", "Saturation", scalar, 1.0),
              input_socket("value", "Value", scalar, 1.0)},
             {output("colour", "Colour", colour)}),
        type(colour_filter, "ctex.colour.brightness-contrast", "Brightness/Contrast",
             {input_socket("colour", "Colour", colour),
              input_socket("brightness", "Brightness", scalar, 0.0),
              input_socket("contrast", "Contrast", scalar, 0.0)},
             {output("colour", "Colour", colour)}),
        type(
            colour_filter, "ctex.colour.gamma", "Gamma",
            {input_socket("colour", "Colour", colour), input_socket("gamma", "Gamma", scalar, 1.0)},
            {output("colour", "Colour", colour)}),
        type(colour_filter, "ctex.colour.invert", "Invert",
             {input_socket("factor", "Factor", scalar, 1.0),
              input_socket("colour", "Colour", colour)},
             {output("colour", "Colour", colour)}),
        type(
            colour_filter, "ctex.colour.quantize", "Quantize",
            {input_socket("colour", "Colour", colour), input_socket("steps", "Steps", scalar, 8.0)},
            {output("colour", "Colour", colour)}),
        type(colour_filter, "ctex.colour.replace", "Replace Colour",
             {input_socket("colour", "Colour", colour), input_socket("from", "From", colour),
              input_socket("to", "To", colour), input_socket("tolerance", "Tolerance", scalar, 0.1),
              input_socket("softness", "Softness", scalar, 0.0)},
             {output("colour", "Colour", colour), output("mask", "Mask", scalar)}),
        type(colour_filter, "ctex.colour.mask", "Colour Mask",
             {input_socket("colour", "Colour", colour), input_socket("key", "Key", colour),
              input_socket("tolerance", "Tolerance", scalar, 0.1),
              input_socket("softness", "Softness", scalar, 0.0)},
             {output("mask", "Mask", scalar)}),
        type(colour_filter, "ctex.colour.separate", "Separate Colour",
             {input_socket("colour", "Colour", colour)},
             {output("channel_1", "Channel 1", scalar), output("channel_2", "Channel 2", scalar),
              output("channel_3", "Channel 3", scalar), output("alpha", "Alpha", scalar)},
             {choice("mode", "Mode", "rgb", {"rgb", "hsv", "hsl"})}),
        type(colour_filter, "ctex.colour.combine", "Combine Colour",
             {input_socket("channel_1", "Channel 1", scalar, 0.0),
              input_socket("channel_2", "Channel 2", scalar, 0.0),
              input_socket("channel_3", "Channel 3", scalar, 0.0),
              input_socket("alpha", "Alpha", scalar, 1.0)},
             {output("colour", "Colour", colour)},
             {choice("mode", "Mode", "rgb", {"rgb", "hsv", "hsl"})}),
        type(colour_filter, "ctex.filter.blur", "Blur",
             {input_socket("image", "Image", image), input_socket("radius", "Radius", scalar, 1.0)},
             {output("image", "Image", image)},
             {choice("kernel", "Kernel", "gaussian", {"box", "gaussian"})}),
        type(colour_filter, "ctex.filter.sharpen", "Sharpen",
             {input_socket("image", "Image", image), input_socket("amount", "Amount", scalar, 1.0),
              input_socket("radius", "Radius", scalar, 1.0)},
             {output("image", "Image", image)}),
        type(colour_filter, "ctex.filter.warp", "Warp",
             {input_socket("image", "Image", image), input_socket("vector", "Vector", vector),
              input_socket("strength", "Strength", scalar, 1.0)},
             {output("image", "Image", image)}),
        type(colour_filter, "ctex.colour.grayscale", "Grayscale Conversion",
             {input_socket("colour", "Colour", colour)}, {output("value", "Value", scalar)},
             {choice("method", "Method", "luminance", {"luminance", "average", "lightness"})}),
    };
}

std::vector<NodeTypeDeclaration> vector_math_nodes() {
    return {
        type(vector_math, "ctex.math.scalar", "Math",
             {input_socket("a", "A", scalar, 0.0), input_socket("b", "B", scalar, 0.0),
              input_socket("c", "C", scalar, 0.0)},
             {output("value", "Value", scalar)},
             {operation_choice("operation", "Operation", math_operations())}),
        type(vector_math, "ctex.math.vector", "Vector Math",
             {input_socket("a", "A", vector, VectorValue{}),
              input_socket("b", "B", vector, VectorValue{}),
              input_socket("c", "C", vector, VectorValue{}),
              input_socket("scale", "Scale", scalar, 1.0)},
             {output("vector", "Vector", vector), output("value", "Value", scalar)},
             {operation_choice("operation", "Operation", vector_math_operations())}),
        type(vector_math, "ctex.math.map-range", "Map Range",
             {input_socket("value", "Value", scalar, 0.0),
              input_socket("from_min", "From Min", scalar, 0.0),
              input_socket("from_max", "From Max", scalar, 1.0),
              input_socket("to_min", "To Min", scalar, 0.0),
              input_socket("to_max", "To Max", scalar, 1.0),
              input_socket("steps", "Steps", scalar, 4.0)},
             {output("value", "Value", scalar)},
             {choice("interpolation", "Interpolation", "linear",
                     {"linear", "stepped", "smoothstep", "smootherstep"}),
              property("clamp", "Clamp", false)}),
        type(vector_math, "ctex.math.clamp", "Clamp",
             {input_socket("value", "Value", scalar, 0.0),
              input_socket("minimum", "Minimum", scalar, 0.0),
              input_socket("maximum", "Maximum", scalar, 1.0)},
             {output("value", "Value", scalar)},
             {choice("mode", "Mode", "min_max", {"min_max", "range"})}),
        type(vector_math, "ctex.vector.mapping", "Mapping",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("location", "Location", vector, VectorValue{}),
              input_socket("rotation", "Rotation", vector, VectorValue{}),
              input_socket("scale", "Scale", vector, VectorValue{1.0F, 1.0F, 1.0F})},
             {output("vector", "Vector", vector)},
             {choice("vector_type", "Vector Type", "point",
                     {"point", "texture", "vector", "normal"})}),
        type(vector_math, "ctex.vector.normal-map", "Normal Map",
             {input_socket("strength", "Strength", scalar, 1.0),
              input_socket("colour", "Colour", colour, ColourValue{0.5F, 0.5F, 1.0F, 1.0F})},
             {output("normal", "Normal", vector)},
             {choice("space", "Space", "tangent", {"tangent", "object", "world"}),
              property("uv_set", "UV Set", std::string("UVMap"))}),
        type(vector_math, "ctex.vector.mix-normal-map", "Mix Normal Map",
             {input_socket("factor", "Factor", scalar, 0.5),
              input_socket("normal_a", "Normal A", vector, VectorValue{0.0F, 0.0F, 1.0F}),
              input_socket("normal_b", "Normal B", vector, VectorValue{0.0F, 0.0F, 1.0F})},
             {output("normal", "Normal", vector)},
             {choice("mode", "Mode", "reoriented",
                     {"partial_derivative", "whiteout", "reoriented"})}),
        type(vector_math, "ctex.vector.bump", "Bump",
             {input_socket("strength", "Strength", scalar, 1.0),
              input_socket("distance", "Distance", scalar, 1.0),
              input_socket("height", "Height", scalar, 0.5),
              input_socket("normal", "Normal", vector, VectorValue{0.0F, 0.0F, 1.0F})},
             {output("normal", "Normal", vector)}, {property("invert", "Invert", false)}),
        type(vector_math, "ctex.vector.separate-xyz", "Separate XYZ",
             {input_socket("vector", "Vector", vector, VectorValue{})},
             {output("x", "X", scalar), output("y", "Y", scalar), output("z", "Z", scalar)}),
        type(vector_math, "ctex.vector.combine-xyz", "Combine XYZ",
             {input_socket("x", "X", scalar, 0.0), input_socket("y", "Y", scalar, 0.0),
              input_socket("z", "Z", scalar, 0.0)},
             {output("vector", "Vector", vector)}),
        type(vector_math, "ctex.vector.rotate", "Vector Rotate",
             {input_socket("vector", "Vector", vector, VectorValue{}),
              input_socket("center", "Center", vector, VectorValue{}),
              input_socket("axis", "Axis", vector, VectorValue{0.0F, 0.0F, 1.0F}),
              input_socket("angle", "Angle", scalar, 0.0),
              input_socket("rotation", "Rotation", vector, VectorValue{})},
             {output("vector", "Vector", vector)},
             {choice("rotation_type", "Rotation Type", "axis_angle",
                     {"axis_angle", "x", "y", "z", "euler_xyz"}),
              property("invert", "Invert", false)}),
        type(vector_math, "ctex.vector.transform", "Vector Transform",
             {input_socket("vector", "Vector", vector, VectorValue{})},
             {output("vector", "Vector", vector)},
             {choice("vector_type", "Vector Type", "vector", {"point", "vector", "normal"}),
              choice("from_space", "From Space", "object", {"object", "world", "camera"}),
              choice("to_space", "To Space", "world", {"object", "world", "camera"})}),
        type(vector_math, "ctex.math.float-curve", "Float Curve",
             {input_socket("factor", "Factor", scalar, 1.0),
              input_socket("value", "Value", scalar, 0.0)},
             {output("value", "Value", scalar)},
             {property("curve", "Curve", std::string("0:0;1:1"))}),
        type(vector_math, "ctex.vector.curves", "Vector Curves",
             {input_socket("factor", "Factor", scalar, 1.0),
              input_socket("vector", "Vector", vector, VectorValue{})},
             {output("vector", "Vector", vector)},
             {property("x_curve", "X Curve", std::string("0:0;1:1")),
              property("y_curve", "Y Curve", std::string("0:0;1:1")),
              property("z_curve", "Z Curve", std::string("0:0;1:1"))}),
    };
}

void append_nodes(std::vector<NodeTypeDeclaration>& destination,
                  std::vector<NodeTypeDeclaration> source) {
    destination.insert(destination.end(), std::make_move_iterator(source.begin()),
                       std::make_move_iterator(source.end()));
}

std::vector<NodeTypeDeclaration> make_catalogue() {
    std::vector<NodeTypeDeclaration> result;
    result.reserve(52);
    append_nodes(result, input_nodes());
    append_nodes(result, texture_nodes());
    append_nodes(result, colour_filter_nodes());
    append_nodes(result, vector_math_nodes());
    std::sort(result.begin(), result.end(),
              [](const auto& left, const auto& right) { return left.type_id < right.type_id; });
    return result;
}

const std::vector<NodeTypeDeclaration>& catalogue_storage() {
    static const std::vector<NodeTypeDeclaration> catalogue = make_catalogue();
    return catalogue;
}

}  // namespace

std::span<const OperationDefinition> math_operations() noexcept {
    return math_operation_definitions;
}

std::span<const OperationDefinition> vector_math_operations() noexcept {
    return vector_math_operation_definitions;
}

std::span<const NodeTypeDeclaration> builtin_node_types() { return catalogue_storage(); }

const NodeTypeDeclaration* find_builtin_node_type(std::string_view type_id) {
    const auto& catalogue = catalogue_storage();
    const auto found =
        std::lower_bound(catalogue.begin(), catalogue.end(), type_id,
                         [](const NodeTypeDeclaration& declaration, std::string_view sought) {
                             return declaration.type_id < sought;
                         });
    return found == catalogue.end() || found->type_id != type_id ? nullptr : &*found;
}

GraphNode make_builtin_node(std::string_view type_id, NodePosition position) {
    const NodeTypeDeclaration* declaration = find_builtin_node_type(type_id);
    if (declaration == nullptr) {
        throw std::out_of_range("built-in graph node type does not exist: " + std::string(type_id));
    }
    std::vector<NodeProperty> properties;
    properties.reserve(declaration->properties.size());
    for (const NodePropertyDeclaration& property_declaration : declaration->properties) {
        properties.push_back({property_declaration.identifier, property_declaration.default_value});
    }
    return {.role = NodeRole::regular,
            .type_id = declaration->type_id,
            .type_version = declaration->version,
            .display_name = declaration->display_name,
            .position = position,
            .inputs = declaration->inputs,
            .outputs = declaration->outputs,
            .properties = std::move(properties)};
}

}  // namespace ctex::graph
