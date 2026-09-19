#ifndef CTEX_IO_EXPORT_PRESET_HPP
#define CTEX_IO_EXPORT_PRESET_HPP

#include <array>
#include <cstdint>
#include <ctex/image/color.hpp>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::io {

inline constexpr double dielectric_specular_reflectance = 0.04;

enum class ExportChannelTokenKind : std::uint8_t {
    base_color_red,
    base_color_green,
    base_color_blue,
    opacity,
    roughness,
    smoothness,
    metallic,
    normal_x,
    normal_y,
    normal_z,
    normal_directx_y,
    height,
    occlusion,
    emission,
    emission_red,
    emission_green,
    emission_blue,
    subsurface,
    diffuse_red,
    diffuse_green,
    diffuse_blue,
    specular_red,
    specular_green,
    specular_blue,
    constant_zero,
    constant_one,
    mesh_map,
    registered_channel,
};

struct ExportChannelToken {
    ExportChannelTokenKind kind{};
    std::string identifier;
    std::uint8_t component{};
    friend bool operator==(const ExportChannelToken&, const ExportChannelToken&) = default;
};

enum class ExportPresetErrorCode : std::uint8_t {
    invalid_token,
    invalid_preset,
    missing_source,
};

class ExportPresetError : public std::runtime_error {
public:
    ExportPresetError(ExportPresetErrorCode code, std::string message);
    [[nodiscard]] ExportPresetErrorCode code() const noexcept { return code_; }

private:
    ExportPresetErrorCode code_;
};

[[nodiscard]] ExportChannelToken parse_export_channel_token(std::string_view text);
[[nodiscard]] std::string export_channel_token_name(const ExportChannelToken& token);

struct ExportSampleValue {
    std::uint8_t component_count{};
    std::array<double, 4> components{};
    friend bool operator==(const ExportSampleValue&, const ExportSampleValue&) = default;
};

struct ExportChannelSample {
    std::array<double, 3> base_color{};
    double opacity{1.0};
    double roughness{};
    double metallic{};
    std::array<double, 3> normal{0.5, 0.5, 1.0};
    double height{};
    double occlusion{1.0};
    std::array<double, 3> emission{};
    double subsurface{};
    std::map<std::string, ExportSampleValue, std::less<>> mesh_maps;
    std::map<std::string, ExportSampleValue, std::less<>> registered_channels;
};

[[nodiscard]] double evaluate_export_channel_token(const ExportChannelToken& token,
                                                   const ExportChannelSample& sample);

enum class ExportBitDepth : std::uint8_t { bits_8 = 8, bits_16 = 16, bits_32 = 32 };

struct ExportTexturePreset {
    std::string suffix;
    std::array<ExportChannelToken, 4> rgba;
    image::ColorSpace color_space{image::ColorSpace::linear_rec709};
    ExportBitDepth bit_depth{ExportBitDepth::bits_8};
    friend bool operator==(const ExportTexturePreset&, const ExportTexturePreset&) = default;
};

struct ExportPreset {
    std::string identifier;
    std::string display_name;
    std::vector<ExportTexturePreset> textures;
    friend bool operator==(const ExportPreset&, const ExportPreset&) = default;
};

void validate_export_preset(const ExportPreset& preset);
[[nodiscard]] std::span<const ExportPreset> built_in_export_presets();
[[nodiscard]] const ExportPreset& default_export_preset();
[[nodiscard]] const ExportPreset& built_in_export_preset(std::string_view identifier);

}  // namespace ctex::io

#endif
