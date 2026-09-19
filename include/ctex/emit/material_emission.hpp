#ifndef CTEX_EMIT_MATERIAL_EMISSION_HPP
#define CTEX_EMIT_MATERIAL_EMISSION_HPP

#include <ctex/emit/feature_emission.hpp>
#include <ctex/emit/graph_emission.hpp>
#include <string>
#include <vector>

namespace ctex::emit {

struct MaterialResourceInput {
    std::string identifier;
    LogicalTexture texture;
    friend bool operator==(const MaterialResourceInput&, const MaterialResourceInput&) = default;
};

struct MaterialShaderEmissionRequest {
    std::string stable_identity;
    ShaderTarget target{};
    DeviceFeatureSet features;
    std::vector<MaterialResourceInput> resources;
    LogicalTexture output;
    FilterMode requested_filter{FilterMode::linear};
    std::uint32_t vertex_count{3};
    friend bool operator==(const MaterialShaderEmissionRequest&,
                           const MaterialShaderEmissionRequest&) = default;
};

struct MaterialShaderEmission {
    PassPlan pass_plan;
    KongShaderProgram shader;
    std::vector<CapabilityWorkaround> workarounds;
    friend bool operator==(const MaterialShaderEmission&, const MaterialShaderEmission&) = default;
};

class MaterialEmissionError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

// Stable identifier a host-node callback can use when referring to one of its
// declared resource identifiers in a textual shader expression.
[[nodiscard]] std::string material_resource_name(std::string_view identifier);

[[nodiscard]] MaterialShaderEmission emit_material_shader(
    const graph::GraphDocument& graph, const graph::NodeTypeRegistry& registry,
    const MaterialShaderEmissionRequest& request);
[[nodiscard]] MaterialShaderEmission emit_material_shader(
    const graph::GraphDocument& graph, const MaterialShaderEmissionRequest& request);

[[nodiscard]] MaterialShaderEmission emit_workspace_material_shader(
    const graph::GraphWorkspace& workspace, std::string_view material_identifier,
    const graph::NodeTypeRegistry& registry, const MaterialShaderEmissionRequest& request);
[[nodiscard]] MaterialShaderEmission emit_workspace_material_shader(
    const graph::GraphWorkspace& workspace, std::string_view material_identifier,
    const MaterialShaderEmissionRequest& request);

}  // namespace ctex::emit

#endif
