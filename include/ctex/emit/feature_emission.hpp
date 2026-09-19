#ifndef CTEX_EMIT_FEATURE_EMISSION_HPP
#define CTEX_EMIT_FEATURE_EMISSION_HPP

#include <cstdint>
#include <ctex/emit/kong_context.hpp>
#include <ctex/emit/pass_plan.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace ctex::emit {

struct DeviceFeatureSet {
    std::uint32_t binding_budget{};
    std::uint32_t maximum_texture_dimension{};
    std::vector<TextureFormat> supported_texture_formats;
    bool floating_point_filtering{};
    bool compute_available{};
    friend bool operator==(const DeviceFeatureSet&, const DeviceFeatureSet&) = default;
};

struct LayerStackInput {
    std::string identifier;
    LogicalTexture texture;
    friend bool operator==(const LayerStackInput&, const LayerStackInput&) = default;
};

struct LayerStackEmissionRequest {
    std::string stable_identity;
    ShaderTarget target{};
    DeviceFeatureSet features;
    std::vector<LayerStackInput> layers;
    LogicalTexture output;
    FilterMode requested_filter{FilterMode::linear};
    friend bool operator==(const LayerStackEmissionRequest&,
                           const LayerStackEmissionRequest&) = default;
};

struct CapabilityWorkaround {
    std::string code;
    std::string message;
    friend bool operator==(const CapabilityWorkaround&, const CapabilityWorkaround&) = default;
};

struct EmittedLayerStackPass {
    std::string pass_identifier;
    KongShaderProgram shader;
    friend bool operator==(const EmittedLayerStackPass&, const EmittedLayerStackPass&) = default;
};

struct LayerStackEmission {
    PassPlan pass_plan;
    std::vector<EmittedLayerStackPass> shaders;
    std::vector<CapabilityWorkaround> workarounds;
    bool compute_used{};
    friend bool operator==(const LayerStackEmission&, const LayerStackEmission&) = default;
};

class FeatureEmissionError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

// Layers are ordered bottom-to-top and contain premultiplied linear RGBA. The
// generated passes preserve that order when an intermediate split is required.
[[nodiscard]] LayerStackEmission emit_layer_stack(const LayerStackEmissionRequest& request);

}  // namespace ctex::emit

#endif
