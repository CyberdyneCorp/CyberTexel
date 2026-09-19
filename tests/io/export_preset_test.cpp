#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/io/export_preset.hpp>
#include <iostream>
#include <set>
#include <string_view>

namespace {

using namespace ctex::io;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, ExportPresetErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const ExportPresetError& error) {
        return expect(error.code() == code, "export preset returned the wrong error code");
    } catch (...) {
    }
    return expect(false, message);
}

bool close(double left, double right) { return std::abs(left - right) < 1e-12; }

bool complete_token_vocabulary_round_trips() {
    constexpr std::array names{
        "base_color.r",
        "base_color.g",
        "base_color.b",
        "opacity",
        "roughness",
        "smoothness",
        "metallic",
        "normal.x",
        "normal.y",
        "normal.z",
        "normal.directx_y",
        "height",
        "occlusion",
        "emission",
        "emission.r",
        "emission.g",
        "emission.b",
        "subsurface",
        "diffuse.r",
        "diffuse.g",
        "diffuse.b",
        "specular.r",
        "specular.g",
        "specular.b",
        "0.0",
        "1.0",
        "mesh_map:curvature",
        "mesh_map:position:2",
        "channel:pbr.coat_weight:0",
    };
    const bool round_trip = std::all_of(names.begin(), names.end(), [](std::string_view name) {
        return export_channel_token_name(parse_export_channel_token(name)) == name;
    });
    return expect(round_trip, "export token vocabulary did not round-trip canonically") &&
           expect_error([] { static_cast<void>(parse_export_channel_token("unknown")); },
                        ExportPresetErrorCode::invalid_token,
                        "unknown export token was accepted") &&
           expect_error(
               [] { static_cast<void>(parse_export_channel_token("channel:pbr.coat_weight:4")); },
               ExportPresetErrorCode::invalid_token,
               "out-of-range registered channel component was accepted");
}

bool derived_and_dynamic_tokens_evaluate_exactly() {
    ExportChannelSample sample{
        .base_color = {0.8, 0.4, 0.2},
        .opacity = 0.7,
        .roughness = 0.25,
        .metallic = 0.75,
        .normal = {0.1, 0.3, 0.9},
        .height = 0.6,
        .occlusion = 0.8,
        .emission = {1.0, 0.5, 0.25},
        .subsurface = 0.2,
        .mesh_maps = {{"position", {.component_count = 3, .components = {0.2, 0.4, 0.6, 0.0}}}},
        .registered_channels = {{"pbr.coat_weight",
                                 {.component_count = 1, .components = {0.35, 0.0, 0.0, 0.0}}}},
    };
    const auto value = [&](std::string_view name) {
        return evaluate_export_channel_token(parse_export_channel_token(name), sample);
    };
    const double expected_emission = 0.2126 + 0.7152 * 0.5 + 0.0722 * 0.25;
    return expect(close(value("smoothness"), 0.75), "smoothness was not one minus roughness") &&
           expect(close(value("normal.directx_y"), 0.7),
                  "DirectX normal green was not one minus OpenGL green") &&
           expect(close(value("diffuse.r"), 0.8 * 0.25),
                  "diffuse token did not scale base colour by one minus metallic") &&
           expect(close(value("specular.g"), 0.04 * 0.25 + 0.4 * 0.75),
                  "specular token did not use the documented dielectric formula") &&
           expect(close(value("emission"), expected_emission),
                  "scalar emission did not use linear Rec. 709 luminance") &&
           expect(close(value("mesh_map:position:2"), 0.6),
                  "named mesh-map component was not resolved") &&
           expect(close(value("channel:pbr.coat_weight:0"), 0.35),
                  "registered channel component was not resolved") &&
           expect_error([&] { static_cast<void>(value("mesh_map:missing")); },
                        ExportPresetErrorCode::missing_source,
                        "missing named mesh map did not produce an explicit diagnostic");
}

bool custom_presets_are_only_data() {
    const ExportPreset custom{
        .identifier = "studio-engine",
        .display_name = "Studio engine packing",
        .textures = {{.suffix = "_Custom",
                      .rgba = {parse_export_channel_token("channel:pbr.coat_weight:0"),
                               parse_export_channel_token("mesh_map:curvature"),
                               parse_export_channel_token("smoothness"),
                               parse_export_channel_token("1.0")},
                      .color_space = ctex::image::ColorSpace::linear_rec709,
                      .bit_depth = ExportBitDepth::bits_16}},
    };
    validate_export_preset(custom);
    return expect(custom.textures.front().rgba[0].identifier == "pbr.coat_weight" &&
                      custom.textures.front().rgba[1].identifier == "curvature",
                  "custom engine convention required a built-in code path");
}

bool required_built_in_presets_are_shipped() {
    const std::span presets = built_in_export_presets();
    const std::set<std::string_view> identifiers{
        "pbr-individual",
        "occlusion-roughness-metallic",
        "metallic-occlusion-smoothness",
        "metallic-emission-roughness",
        "base-color",
        "specular-glossiness",
    };
    const bool all_present = std::all_of(identifiers.begin(), identifiers.end(), [&](auto id) {
        return std::any_of(presets.begin(), presets.end(),
                           [id](const ExportPreset& preset) { return preset.identifier == id; });
    });
    const ExportPreset& orm = built_in_export_preset("occlusion-roughness-metallic");
    const ExportPreset& specular = built_in_export_preset("specular-glossiness");
    return expect(presets.size() >= identifiers.size() && all_present,
                  "required built-in export preset is missing") &&
           expect(default_export_preset().identifier == "pbr-individual",
                  "individual PBR preset is not the default") &&
           expect(orm.textures.size() == 1 &&
                      export_channel_token_name(orm.textures.front().rgba[0]) == "occlusion" &&
                      export_channel_token_name(orm.textures.front().rgba[1]) == "roughness" &&
                      export_channel_token_name(orm.textures.front().rgba[2]) == "metallic",
                  "packed ORM preset does not use the required RGB ordering") &&
           expect(specular.textures.size() == 2,
                  "specular-glossiness preset does not contain diffuse and specular outputs");
}

bool invalid_preset_data_is_refused() {
    ExportPreset duplicate = default_export_preset();
    duplicate.textures.push_back(duplicate.textures.front());
    duplicate.textures.back().rgba[0] = {
        .kind = static_cast<ExportChannelTokenKind>(255), .identifier = {}, .component = 0};
    const bool invalid_token_refused =
        expect_error([&] { validate_export_preset(duplicate); },
                     ExportPresetErrorCode::invalid_preset, "duplicate export suffix was accepted");
    ExportPreset invalid_token_preset = default_export_preset();
    invalid_token_preset.textures.front().rgba[0] = {
        .kind = static_cast<ExportChannelTokenKind>(255), .identifier = {}, .component = 0};
    ExportPreset invalid_color = default_export_preset();
    invalid_color.textures.front().color_space = static_cast<ctex::image::ColorSpace>(255);
    ExportPreset invalid_depth = default_export_preset();
    invalid_depth.textures.front().bit_depth = static_cast<ExportBitDepth>(24);
    return invalid_token_refused &&
           expect_error([&] { validate_export_preset(invalid_token_preset); },
                        ExportPresetErrorCode::invalid_token,
                        "invalid export channel token was accepted") &&
           expect_error([&] { validate_export_preset(invalid_color); },
                        ExportPresetErrorCode::invalid_preset,
                        "invalid export colour space was accepted") &&
           expect_error([&] { validate_export_preset(invalid_depth); },
                        ExportPresetErrorCode::invalid_preset,
                        "invalid export bit depth was accepted") &&
           expect_error([] { static_cast<void>(built_in_export_preset("missing")); },
                        ExportPresetErrorCode::invalid_preset,
                        "missing built-in preset did not produce a diagnostic");
}

}  // namespace

int main() {
    return complete_token_vocabulary_round_trips() &&
                   derived_and_dynamic_tokens_evaluate_exactly() &&
                   custom_presets_are_only_data() && required_built_in_presets_are_shipped() &&
                   invalid_preset_data_is_refused()
               ? 0
               : 1;
}
