#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <ctex/io/export_preset.hpp>
#include <set>
#include <utility>

namespace ctex::io {
namespace {

using Kind = ExportChannelTokenKind;

constexpr std::array<std::pair<std::string_view, Kind>, 26> named_tokens{{
    {"base_color.r", Kind::base_color_red},
    {"base_color.g", Kind::base_color_green},
    {"base_color.b", Kind::base_color_blue},
    {"opacity", Kind::opacity},
    {"roughness", Kind::roughness},
    {"smoothness", Kind::smoothness},
    {"metallic", Kind::metallic},
    {"normal.x", Kind::normal_x},
    {"normal.y", Kind::normal_y},
    {"normal.z", Kind::normal_z},
    {"normal.directx_y", Kind::normal_directx_y},
    {"height", Kind::height},
    {"occlusion", Kind::occlusion},
    {"emission", Kind::emission},
    {"emission.r", Kind::emission_red},
    {"emission.g", Kind::emission_green},
    {"emission.b", Kind::emission_blue},
    {"subsurface", Kind::subsurface},
    {"diffuse.r", Kind::diffuse_red},
    {"diffuse.g", Kind::diffuse_green},
    {"diffuse.b", Kind::diffuse_blue},
    {"specular.r", Kind::specular_red},
    {"specular.g", Kind::specular_green},
    {"specular.b", Kind::specular_blue},
    {"0.0", Kind::constant_zero},
    {"1.0", Kind::constant_one},
}};

[[noreturn]] void invalid_token(std::string_view message) {
    throw ExportPresetError(ExportPresetErrorCode::invalid_token, std::string(message));
}

std::uint8_t parse_component(std::string_view text) {
    unsigned value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value > 3) {
        invalid_token("export channel component must be an integer from 0 through 3");
    }
    return static_cast<std::uint8_t>(value);
}

ExportChannelToken parse_dynamic_token(std::string_view text, std::string_view prefix, Kind kind,
                                       bool component_required) {
    std::string_view body = text.substr(prefix.size());
    std::uint8_t component = 0;
    const std::size_t separator = body.rfind(':');
    if (separator != std::string_view::npos) {
        component = parse_component(body.substr(separator + 1));
        body = body.substr(0, separator);
    } else if (component_required) {
        invalid_token("registered channel token requires a component index");
    }
    if (body.empty()) {
        invalid_token("export channel token identifier must not be empty");
    }
    return {.kind = kind, .identifier = std::string(body), .component = component};
}

const ExportSampleValue& find_sample_value(
    const std::map<std::string, ExportSampleValue, std::less<>>& values,
    const ExportChannelToken& token, std::string_view source_name) {
    const auto found = values.find(token.identifier);
    if (found == values.end()) {
        throw ExportPresetError(
            ExportPresetErrorCode::missing_source,
            std::string(source_name) + " is not available: " + token.identifier);
    }
    if (found->second.component_count == 0 || found->second.component_count > 4 ||
        token.component >= found->second.component_count) {
        throw ExportPresetError(ExportPresetErrorCode::missing_source,
                                std::string(source_name) + " does not have component " +
                                    std::to_string(token.component) + ": " + token.identifier);
    }
    return found->second;
}

double component(const std::array<double, 3>& values, Kind kind, Kind first) {
    return values[static_cast<std::size_t>(kind) - static_cast<std::size_t>(first)];
}

ExportChannelToken token(Kind kind) { return {.kind = kind, .identifier = {}, .component = 0}; }

ExportTexturePreset texture(std::string suffix, std::array<Kind, 4> kinds,
                            image::ColorSpace color_space, ExportBitDepth bit_depth) {
    return {.suffix = std::move(suffix),
            .rgba = {token(kinds[0]), token(kinds[1]), token(kinds[2]), token(kinds[3])},
            .color_space = color_space,
            .bit_depth = bit_depth};
}

ExportTexturePreset scalar_texture(std::string suffix, Kind kind, ExportBitDepth bit_depth) {
    return texture(std::move(suffix), {kind, kind, kind, Kind::constant_one},
                   image::ColorSpace::linear_rec709, bit_depth);
}

ExportPreset individual_pbr_preset() {
    return {
        .identifier = "pbr-individual",
        .display_name = "Individual PBR maps",
        .textures =
            {
                texture("_BaseColor",
                        {Kind::base_color_red, Kind::base_color_green, Kind::base_color_blue,
                         Kind::opacity},
                        image::ColorSpace::srgb_rec709, ExportBitDepth::bits_8),
                scalar_texture("_Opacity", Kind::opacity, ExportBitDepth::bits_8),
                scalar_texture("_Roughness", Kind::roughness, ExportBitDepth::bits_8),
                scalar_texture("_Metallic", Kind::metallic, ExportBitDepth::bits_8),
                texture("_Normal",
                        {Kind::normal_x, Kind::normal_y, Kind::normal_z, Kind::constant_one},
                        image::ColorSpace::linear_rec709, ExportBitDepth::bits_16),
                scalar_texture("_Height", Kind::height, ExportBitDepth::bits_16),
                scalar_texture("_Occlusion", Kind::occlusion, ExportBitDepth::bits_8),
                texture("_Emission",
                        {Kind::emission_red, Kind::emission_green, Kind::emission_blue,
                         Kind::constant_one},
                        image::ColorSpace::srgb_rec709, ExportBitDepth::bits_8),
                scalar_texture("_Subsurface", Kind::subsurface, ExportBitDepth::bits_8),
            },
    };
}

std::vector<ExportPreset> make_built_in_presets() {
    std::vector<ExportPreset> presets{
        individual_pbr_preset(),
        {.identifier = "occlusion-roughness-metallic",
         .display_name = "Packed occlusion, roughness, metallic",
         .textures = {texture(
             "_ORM", {Kind::occlusion, Kind::roughness, Kind::metallic, Kind::constant_one},
             image::ColorSpace::linear_rec709, ExportBitDepth::bits_8)}},
        {.identifier = "metallic-occlusion-smoothness",
         .display_name = "Packed metallic, occlusion, smoothness",
         .textures = {texture(
             "_MOS", {Kind::metallic, Kind::occlusion, Kind::smoothness, Kind::constant_one},
             image::ColorSpace::linear_rec709, ExportBitDepth::bits_8)}},
        {.identifier = "metallic-emission-roughness",
         .display_name = "Packed metallic, emission, roughness",
         .textures = {texture("_MER",
                              {Kind::metallic, Kind::emission, Kind::roughness, Kind::constant_one},
                              image::ColorSpace::linear_rec709, ExportBitDepth::bits_8)}},
        {.identifier = "base-color",
         .display_name = "Base colour only",
         .textures = {texture(
             "_BaseColor",
             {Kind::base_color_red, Kind::base_color_green, Kind::base_color_blue, Kind::opacity},
             image::ColorSpace::srgb_rec709, ExportBitDepth::bits_8)}},
        {.identifier = "specular-glossiness",
         .display_name = "Specular-glossiness",
         .textures =
             {
                 texture(
                     "_Diffuse",
                     {Kind::diffuse_red, Kind::diffuse_green, Kind::diffuse_blue, Kind::opacity},
                     image::ColorSpace::srgb_rec709, ExportBitDepth::bits_8),
                 texture("_SpecularGlossiness",
                         {Kind::specular_red, Kind::specular_green, Kind::specular_blue,
                          Kind::smoothness},
                         image::ColorSpace::srgb_rec709, ExportBitDepth::bits_8),
             }},
    };
    std::set<std::string_view> identifiers;
    for (const ExportPreset& preset : presets) {
        validate_export_preset(preset);
        if (!identifiers.insert(preset.identifier).second) {
            throw ExportPresetError(ExportPresetErrorCode::invalid_preset,
                                    "built-in export preset identifiers must be unique");
        }
    }
    return presets;
}

}  // namespace

ExportPresetError::ExportPresetError(ExportPresetErrorCode code, std::string message)
    : std::runtime_error(std::move(message)), code_(code) {}

ExportChannelToken parse_export_channel_token(std::string_view text) {
    const auto found = std::find_if(named_tokens.begin(), named_tokens.end(),
                                    [text](const auto& entry) { return entry.first == text; });
    if (found != named_tokens.end()) {
        return token(found->second);
    }
    constexpr std::string_view mesh_prefix = "mesh_map:";
    constexpr std::string_view channel_prefix = "channel:";
    if (text.starts_with(mesh_prefix)) {
        return parse_dynamic_token(text, mesh_prefix, Kind::mesh_map, false);
    }
    if (text.starts_with(channel_prefix)) {
        return parse_dynamic_token(text, channel_prefix, Kind::registered_channel, true);
    }
    invalid_token("unknown export channel token: " + std::string(text));
}

std::string export_channel_token_name(const ExportChannelToken& value) {
    const auto found =
        std::find_if(named_tokens.begin(), named_tokens.end(),
                     [value](const auto& entry) { return entry.second == value.kind; });
    if (found != named_tokens.end() && value.identifier.empty() && value.component == 0) {
        return std::string(found->first);
    }
    if (value.kind == Kind::mesh_map && !value.identifier.empty() && value.component <= 3) {
        std::string name = "mesh_map:" + value.identifier;
        if (value.component != 0) {
            name += ":" + std::to_string(value.component);
        }
        return name;
    }
    if (value.kind == Kind::registered_channel && !value.identifier.empty() &&
        value.component <= 3) {
        return "channel:" + value.identifier + ":" + std::to_string(value.component);
    }
    invalid_token("export channel token is not canonical");
}

double evaluate_export_channel_token(const ExportChannelToken& value,
                                     const ExportChannelSample& sample) {
    switch (value.kind) {
        case Kind::base_color_red:
        case Kind::base_color_green:
        case Kind::base_color_blue:
            return component(sample.base_color, value.kind, Kind::base_color_red);
        case Kind::opacity:
            return sample.opacity;
        case Kind::roughness:
            return sample.roughness;
        case Kind::smoothness:
            return 1.0 - sample.roughness;
        case Kind::metallic:
            return sample.metallic;
        case Kind::normal_x:
        case Kind::normal_y:
        case Kind::normal_z:
            return component(sample.normal, value.kind, Kind::normal_x);
        case Kind::normal_directx_y:
            return 1.0 - sample.normal[1];
        case Kind::height:
            return sample.height;
        case Kind::occlusion:
            return sample.occlusion;
        case Kind::emission:
            return 0.2126 * sample.emission[0] + 0.7152 * sample.emission[1] +
                   0.0722 * sample.emission[2];
        case Kind::emission_red:
        case Kind::emission_green:
        case Kind::emission_blue:
            return component(sample.emission, value.kind, Kind::emission_red);
        case Kind::subsurface:
            return sample.subsurface;
        case Kind::diffuse_red:
        case Kind::diffuse_green:
        case Kind::diffuse_blue:
            return component(sample.base_color, value.kind, Kind::diffuse_red) *
                   (1.0 - sample.metallic);
        case Kind::specular_red:
        case Kind::specular_green:
        case Kind::specular_blue:
            return dielectric_specular_reflectance * (1.0 - sample.metallic) +
                   component(sample.base_color, value.kind, Kind::specular_red) * sample.metallic;
        case Kind::constant_zero:
            return 0.0;
        case Kind::constant_one:
            return 1.0;
        case Kind::mesh_map:
            return find_sample_value(sample.mesh_maps, value, "mesh map")
                .components[value.component];
        case Kind::registered_channel:
            return find_sample_value(sample.registered_channels, value, "registered channel")
                .components[value.component];
    }
    invalid_token("export channel token kind is invalid");
}

void validate_export_preset(const ExportPreset& preset) {
    if (preset.identifier.empty() || preset.display_name.empty() || preset.textures.empty()) {
        throw ExportPresetError(ExportPresetErrorCode::invalid_preset,
                                "export preset identity, display name, and textures are required");
    }
    std::set<std::string_view> suffixes;
    for (const ExportTexturePreset& texture : preset.textures) {
        if (texture.suffix.empty() || !suffixes.insert(texture.suffix).second) {
            throw ExportPresetError(ExportPresetErrorCode::invalid_preset,
                                    "export texture suffixes must be non-empty and unique");
        }
        if (texture.color_space != image::ColorSpace::linear_rec709 &&
            texture.color_space != image::ColorSpace::srgb_rec709) {
            throw ExportPresetError(ExportPresetErrorCode::invalid_preset,
                                    "export texture colour space is invalid");
        }
        if (texture.bit_depth != ExportBitDepth::bits_8 &&
            texture.bit_depth != ExportBitDepth::bits_16 &&
            texture.bit_depth != ExportBitDepth::bits_32) {
            throw ExportPresetError(ExportPresetErrorCode::invalid_preset,
                                    "export texture bit depth must be 8, 16, or 32");
        }
        for (const ExportChannelToken& channel : texture.rgba) {
            static_cast<void>(export_channel_token_name(channel));
        }
    }
}

std::span<const ExportPreset> built_in_export_presets() {
    static const std::vector<ExportPreset> presets = make_built_in_presets();
    return presets;
}

const ExportPreset& default_export_preset() { return built_in_export_presets().front(); }

const ExportPreset& built_in_export_preset(std::string_view identifier) {
    const std::span presets = built_in_export_presets();
    const auto found =
        std::find_if(presets.begin(), presets.end(),
                     [identifier](const auto& preset) { return preset.identifier == identifier; });
    if (found == presets.end()) {
        throw ExportPresetError(ExportPresetErrorCode::invalid_preset,
                                "unknown built-in export preset: " + std::string(identifier));
    }
    return *found;
}

}  // namespace ctex::io
