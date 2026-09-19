#ifndef CTEX_EMIT_PREVIEW_EMISSION_HPP
#define CTEX_EMIT_PREVIEW_EMISSION_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/emit/feature_emission.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::emit {

struct PreviewChannelInput {
    std::string semantic_id;
    std::uint8_t component_count{};
    LogicalTexture texture;
    friend bool operator==(const PreviewChannelInput&, const PreviewChannelInput&) = default;
};

struct PreviewEnvironmentInput {
    LogicalTexture radiance;
    LogicalTexture diffuse_irradiance;
    LogicalTexture specular_brdf_lookup;
    friend bool operator==(const PreviewEnvironmentInput&,
                           const PreviewEnvironmentInput&) = default;
};

struct PreviewEmissionRequest {
    std::string stable_identity;
    ShaderTarget target{};
    DeviceFeatureSet features;
    std::vector<PreviewChannelInput> channels;
    LogicalTexture output;
    std::optional<PreviewEnvironmentInput> environment;
    std::size_t analytic_light_count{};
    std::uint32_t vertex_count{};
    friend bool operator==(const PreviewEmissionRequest&, const PreviewEmissionRequest&) = default;
};

enum class PreviewEmissionKind : std::uint8_t { lit, channel_inspection };

struct PreviewEmission {
    PassPlan pass_plan;
    KongShaderProgram shader;
    std::vector<CapabilityWorkaround> workarounds;
    PreviewEmissionKind kind{};
    bool fallback_lighting{};
    std::string inspected_channel;
    friend bool operator==(const PreviewEmission&, const PreviewEmission&) = default;
};

class PreviewEmissionError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

inline constexpr std::size_t maximum_preview_analytic_lights = 4;

[[nodiscard]] PreviewEmission emit_lit_preview(const PreviewEmissionRequest& request);
[[nodiscard]] PreviewEmission emit_channel_inspection(const PreviewEmissionRequest& request,
                                                      std::string_view semantic_id);

}  // namespace ctex::emit

#endif
