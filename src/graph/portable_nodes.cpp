#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <ctex/graph/catalogue.hpp>
#include <ctex/graph/portable_nodes.hpp>
#include <limits>
#include <string>

namespace ctex::graph {
namespace {

struct Rgb {
    float r;
    float g;
    float b;
};

struct Hsv {
    float h;
    float s;
    float v;
};

const NodeProperty& property(const GraphNode& node, std::string_view key) {
    const auto found = std::ranges::find(node.properties, key, &NodeProperty::key);
    if (found == node.properties.end()) {
        throw BuiltinNodeEvaluationError("built-in node '" + node.type_id + "' has no property '" +
                                         std::string(key) + "'");
    }
    return *found;
}

template <typename Value>
const Value& value_as(const SocketValue& value, std::string_view label) {
    const Value* result = std::get_if<Value>(&value);
    if (result == nullptr) {
        throw BuiltinNodeEvaluationError("built-in node value '" + std::string(label) +
                                         "' has the wrong type");
    }
    return *result;
}

float saturate(float value) { return std::clamp(value, 0.0F, 1.0F); }

Rgb rgb(ColourValue value) { return {value.r, value.g, value.b}; }

Rgb mix(Rgb left, Rgb right, float factor) {
    return {left.r + (right.r - left.r) * factor, left.g + (right.g - left.g) * factor,
            left.b + (right.b - left.b) * factor};
}

float blend_normal(float, float blend) { return blend; }
float blend_darken(float base, float blend) { return std::min(base, blend); }
float blend_multiply(float base, float blend) { return base * blend; }
float blend_color_burn(float base, float blend) {
    return blend <= 0.0F ? 0.0F : 1.0F - std::min(1.0F, (1.0F - base) / blend);
}
float blend_lighten(float base, float blend) { return std::max(base, blend); }
float blend_screen(float base, float blend) { return 1.0F - (1.0F - base) * (1.0F - blend); }
float blend_color_dodge(float base, float blend) {
    return blend >= 1.0F ? 1.0F : std::min(1.0F, base / (1.0F - blend));
}
float blend_add(float base, float blend) { return std::min(1.0F, base + blend); }
float blend_overlay(float base, float blend) {
    return base <= 0.5F ? 2.0F * base * blend : 1.0F - 2.0F * (1.0F - base) * (1.0F - blend);
}
float blend_soft_light(float base, float blend) {
    if (blend <= 0.5F) {
        return base - (1.0F - 2.0F * blend) * base * (1.0F - base);
    }
    const float curve =
        base <= 0.25F ? ((16.0F * base - 12.0F) * base + 4.0F) * base : std::sqrt(base);
    return base + (2.0F * blend - 1.0F) * (curve - base);
}
float blend_linear_light(float base, float blend) {
    return std::clamp(base + 2.0F * blend - 1.0F, 0.0F, 1.0F);
}
float blend_difference(float base, float blend) { return std::abs(base - blend); }
float blend_exclusion(float base, float blend) { return base + blend - 2.0F * base * blend; }
float blend_subtract(float base, float blend) { return std::max(0.0F, base - blend); }
float blend_divide(float base, float blend) {
    return blend <= 0.0F ? 1.0F : std::min(1.0F, base / blend);
}

using ChannelBlendFunction = float (*)(float, float);
using NamedChannelBlend = std::pair<std::string_view, ChannelBlendFunction>;

constexpr std::array<NamedChannelBlend, 16> channel_blends{{
    {"normal", blend_normal},
    {"pass_through", blend_normal},
    {"darken", blend_darken},
    {"multiply", blend_multiply},
    {"color_burn", blend_color_burn},
    {"lighten", blend_lighten},
    {"screen", blend_screen},
    {"color_dodge", blend_color_dodge},
    {"add", blend_add},
    {"overlay", blend_overlay},
    {"soft_light", blend_soft_light},
    {"linear_light", blend_linear_light},
    {"difference", blend_difference},
    {"exclusion", blend_exclusion},
    {"subtract", blend_subtract},
    {"divide", blend_divide},
}};

float channel_blend(std::string_view mode, float base, float blend) {
    const auto found = std::ranges::find_if(
        channel_blends,
        [&](const NamedChannelBlend& candidate) { return candidate.first == mode; });
    if (found == channel_blends.end()) {
        throw BuiltinNodeEvaluationError("unknown Blend mode '" + std::string(mode) + "'");
    }
    return found->second(base, blend);
}

Hsv to_hsv(Rgb colour) {
    const float maximum = std::max({colour.r, colour.g, colour.b});
    const float minimum = std::min({colour.r, colour.g, colour.b});
    const float delta = maximum - minimum;
    float hue = 0.0F;
    if (delta > 0.0F) {
        if (maximum == colour.r) {
            hue = std::fmod((colour.g - colour.b) / delta, 6.0F);
        } else if (maximum == colour.g) {
            hue = (colour.b - colour.r) / delta + 2.0F;
        } else {
            hue = (colour.r - colour.g) / delta + 4.0F;
        }
        hue /= 6.0F;
        if (hue < 0.0F) {
            hue += 1.0F;
        }
    }
    return {hue, maximum == 0.0F ? 0.0F : delta / maximum, maximum};
}

Rgb from_hsv(Hsv colour) {
    const float sector = colour.h * 6.0F;
    const int index = static_cast<int>(std::floor(sector)) % 6;
    const float fraction = sector - std::floor(sector);
    const float p = colour.v * (1.0F - colour.s);
    const float q = colour.v * (1.0F - fraction * colour.s);
    const float t = colour.v * (1.0F - (1.0F - fraction) * colour.s);
    switch (index) {
        case 0:
            return {colour.v, t, p};
        case 1:
            return {q, colour.v, p};
        case 2:
            return {p, colour.v, t};
        case 3:
            return {p, q, colour.v};
        case 4:
            return {t, p, colour.v};
        default:
            return {colour.v, p, q};
    }
}

Rgb blend_rgb(std::string_view mode, Rgb base, Rgb blend) {
    if (mode == "hue" || mode == "saturation" || mode == "color" || mode == "value") {
        Hsv base_hsv = to_hsv(base);
        const Hsv blend_hsv = to_hsv(blend);
        if (mode == "hue" || mode == "color") {
            base_hsv.h = blend_hsv.h;
        }
        if (mode == "saturation" || mode == "color") {
            base_hsv.s = blend_hsv.s;
        }
        if (mode == "value") {
            base_hsv.v = blend_hsv.v;
        }
        return from_hsv(base_hsv);
    }
    return {channel_blend(mode, base.r, blend.r), channel_blend(mode, base.g, blend.g),
            channel_blend(mode, base.b, blend.b)};
}

std::uint32_t hash(std::uint32_t value) {
    value ^= value >> 16U;
    value *= 0x7feb352dU;
    value ^= value >> 15U;
    value *= 0x846ca68bU;
    return value ^ (value >> 16U);
}

std::uint32_t coordinate_hash(int x, int y, int z, std::uint32_t seed) {
    std::uint32_t result = seed;
    result = hash(result ^ std::bit_cast<std::uint32_t>(x));
    result = hash(result ^ std::bit_cast<std::uint32_t>(y));
    return hash(result ^ std::bit_cast<std::uint32_t>(z));
}

float hash_unit(int x, int y, int z, std::uint32_t seed) {
    constexpr float reciprocal =
        1.0F / static_cast<float>(std::numeric_limits<std::uint32_t>::max());
    return static_cast<float>(coordinate_hash(x, y, z, seed)) * reciprocal;
}

float fade(float value) { return value * value * (3.0F - 2.0F * value); }

float value_noise(VectorValue coordinate, std::uint32_t seed) {
    const int x = static_cast<int>(std::floor(coordinate.x));
    const int y = static_cast<int>(std::floor(coordinate.y));
    const int z = static_cast<int>(std::floor(coordinate.z));
    const float tx = fade(coordinate.x - static_cast<float>(x));
    const float ty = fade(coordinate.y - static_cast<float>(y));
    const float tz = fade(coordinate.z - static_cast<float>(z));
    std::array<float, 4> xy{};
    for (int dz = 0; dz < 2; ++dz) {
        std::array<float, 2> x_values{};
        for (int dy = 0; dy < 2; ++dy) {
            const float left = hash_unit(x, y + dy, z + dz, seed);
            const float right = hash_unit(x + 1, y + dy, z + dz, seed);
            x_values[dy] = left + (right - left) * tx;
        }
        xy[dz] = x_values[0] + (x_values[1] - x_values[0]) * ty;
    }
    return xy[0] + (xy[1] - xy[0]) * tz;
}

std::uint32_t noise_seed(double seed) {
    const std::uint64_t bits = std::bit_cast<std::uint64_t>(seed);
    return hash(static_cast<std::uint32_t>(bits) ^ static_cast<std::uint32_t>(bits >> 32U));
}

float fractal_noise(VectorValue coordinate, double scale, double detail, double roughness,
                    double lacunarity, double distortion, std::uint32_t seed) {
    const float frequency_scale = static_cast<float>(scale);
    coordinate = {coordinate.x * frequency_scale, coordinate.y * frequency_scale,
                  coordinate.z * frequency_scale};
    if (distortion != 0.0) {
        const float amount = static_cast<float>(distortion);
        coordinate.x += (value_noise(coordinate, seed ^ 0xa511e9b3U) - 0.5F) * amount;
        coordinate.y += (value_noise(coordinate, seed ^ 0x63d83595U) - 0.5F) * amount;
        coordinate.z += (value_noise(coordinate, seed ^ 0x9e3779b9U) - 0.5F) * amount;
    }
    const double bounded_detail = std::clamp(detail, 0.0, 16.0);
    const int full_octaves = static_cast<int>(std::floor(bounded_detail));
    const float fractional_octave = static_cast<float>(bounded_detail - full_octaves);
    const float persistence = static_cast<float>(std::clamp(roughness, 0.0, 1.0));
    const float frequency_step = static_cast<float>(std::max(lacunarity, 1.0));
    float amplitude = 1.0F;
    float total = 0.0F;
    float weight = 0.0F;
    for (int octave = 0; octave <= full_octaves; ++octave) {
        const float octave_weight = octave == full_octaves ? fractional_octave : 1.0F;
        if (octave_weight > 0.0F || full_octaves == 0) {
            const float effective_weight = amplitude * (full_octaves == 0 ? 1.0F : octave_weight);
            total += value_noise(coordinate, seed + static_cast<std::uint32_t>(octave)) *
                     effective_weight;
            weight += effective_weight;
        }
        coordinate = {coordinate.x * frequency_step, coordinate.y * frequency_step,
                      coordinate.z * frequency_step};
        amplitude *= persistence;
    }
    return weight == 0.0F ? 0.0F : saturate(total / weight);
}

std::vector<SocketValue> evaluate_noise(const GraphNode& node,
                                        std::span<const SocketValue> inputs) {
    if (inputs.size() != 6) {
        throw BuiltinNodeEvaluationError("Noise node expects six inputs");
    }
    const VectorValue coordinate = value_as<VectorValue>(inputs[0], "vector");
    const double scale = value_as<double>(inputs[1], "scale");
    const double detail = value_as<double>(inputs[2], "detail");
    const double roughness = value_as<double>(inputs[3], "roughness");
    const double lacunarity = value_as<double>(inputs[4], "lacunarity");
    const double distortion = value_as<double>(inputs[5], "distortion");
    const std::uint32_t seed = noise_seed(value_as<double>(property(node, "seed").value, "seed"));
    const float factor =
        fractal_noise(coordinate, scale, detail, roughness, lacunarity, distortion, seed);
    const float green = fractal_noise(coordinate, scale, detail, roughness, lacunarity, distortion,
                                      seed ^ 0x68bc21ebU);
    const float blue = fractal_noise(coordinate, scale, detail, roughness, lacunarity, distortion,
                                     seed ^ 0x02e5be93U);
    return {static_cast<double>(factor), ColourValue{factor, green, blue, 1.0F}};
}

std::vector<SocketValue> evaluate_blend(const GraphNode& node,
                                        std::span<const SocketValue> inputs) {
    if (inputs.size() != 3) {
        throw BuiltinNodeEvaluationError("Blend node expects three inputs");
    }
    const std::string& mode = value_as<std::string>(property(node, "mode").value, "mode");
    return {blend_colour(mode, value_as<ColourValue>(inputs[1], "a"),
                         value_as<ColourValue>(inputs[2], "b"),
                         value_as<double>(inputs[0], "factor"))};
}

}  // namespace

ColourValue blend_colour(std::string_view mode, ColourValue base, ColourValue blend,
                         double factor) {
    const float amount = saturate(static_cast<float>(factor));
    const Rgb result = mix(rgb(base), blend_rgb(mode, rgb(base), rgb(blend)), amount);
    return {saturate(result.r), saturate(result.g), saturate(result.b),
            saturate(base.a + (blend.a - base.a) * amount)};
}

std::vector<SocketValue> evaluate_builtin_node(const GraphNode& node,
                                               std::span<const SocketValue> inputs) {
    const NodeTypeDeclaration* declaration = find_builtin_node_type(node.type_id);
    if (declaration == nullptr || declaration->version != node.type_version) {
        throw BuiltinNodeEvaluationError("built-in node version is unavailable: " + node.type_id +
                                         "@" + std::to_string(node.type_version));
    }
    if (node.type_id == "ctex.texture.noise") {
        return evaluate_noise(node, inputs);
    }
    if (node.type_id == "ctex.colour.blend") {
        return evaluate_blend(node, inputs);
    }
    throw BuiltinNodeEvaluationError("portable evaluation is unavailable for " + node.type_id);
}

}  // namespace ctex::graph
