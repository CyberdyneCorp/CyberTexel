#include <ctex/exec/emission_features.hpp>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

using ctex::emit::DeviceFeatureSet;
using ctex::emit::TextureFormat;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

DeviceFeatureSet features() {
    return {.binding_budget = 7,
            .maximum_texture_dimension = 2048,
            .supported_texture_formats = {TextureFormat::r8_unorm, TextureFormat::rgba16_float},
            .floating_point_filtering = false,
            .compute_available = true};
}

ctex::emit::LogicalTexture texture(std::string id, std::uint64_t generation, bool initialized) {
    return {.version = {std::move(id), generation},
            .role = "feature fixture",
            .format = TextureFormat::rgba16_float,
            .extent = {.width = 16, .height = 16, .layers = 1},
            .mip_levels = 1,
            .tile_shape = {.width = 16, .height = 16},
            .externally_initialized = initialized};
}

class FeatureExecutor final : public ctex::exec::Executor {
public:
    FeatureExecutor()
        : descriptor_{.identifier = "feature-fixture",
                      .display_name = "Feature fixture",
                      .device_name = "Fixture device",
                      .route = ctex::exec::ExecutorRoute::owned_gpu,
                      .availability = ctex::exec::ExecutorAvailability::available,
                      .features = features()} {}

    [[nodiscard]] const ctex::exec::ExecutorDescriptor& descriptor() const noexcept override {
        return descriptor_;
    }

private:
    ctex::exec::ExecutorDescriptor descriptor_;
};

bool every_emission_request_uses_the_selected_executor_report() {
    const FeatureExecutor executor;
    DeviceFeatureSet unrelated{.binding_budget = 99,
                               .maximum_texture_dimension = 99,
                               .supported_texture_formats = {TextureFormat::depth32_float},
                               .floating_point_filtering = true,
                               .compute_available = false};
    ctex::emit::LayerStackEmissionRequest layers;
    layers.stable_identity = "selected-executor-features";
    layers.target = ctex::emit::ShaderTarget::wgsl;
    layers.features = unrelated;
    for (std::uint64_t index = 0; index < 8; ++index) {
        layers.layers.push_back(
            {"layer-" + std::to_string(index), texture("layer-" + std::to_string(index), 1, true)});
    }
    layers.output = texture("output", 1, false);
    ctex::emit::MaterialShaderEmissionRequest material;
    material.features = unrelated;
    ctex::emit::PreviewEmissionRequest preview;
    preview.features = unrelated;

    const auto bound_layers = ctex::exec::emission_request_for(executor, std::move(layers));
    const auto bound_material = ctex::exec::emission_request_for(executor, std::move(material));
    const auto bound_preview = ctex::exec::emission_request_for(executor, std::move(preview));
    const auto emitted = ctex::emit::emit_layer_stack(bound_layers);
    return expect(bound_layers.features == executor.descriptor().features &&
                      bound_material.features == executor.descriptor().features &&
                      bound_preview.features == executor.descriptor().features,
                  "an emission request retained features unrelated to the selected executor") &&
           expect(emitted.pass_plan.passes().size() == 2 && emitted.workarounds.size() == 1 &&
                      emitted.workarounds[0].code == "nearest_float_sampling",
                  "emission did not apply the selected executor's budget and filtering report");
}

bool registry_refuses_an_executor_without_emission_capabilities() {
    class InvalidExecutor final : public ctex::exec::Executor {
    public:
        [[nodiscard]] const ctex::exec::ExecutorDescriptor& descriptor() const noexcept override {
            return descriptor_;
        }

    private:
        ctex::exec::ExecutorDescriptor descriptor_{
            .identifier = "invalid",
            .display_name = "Invalid",
            .device_name = "No capabilities",
            .route = ctex::exec::ExecutorRoute::owned_gpu,
            .availability = ctex::exec::ExecutorAvailability::available,
            .features = {}};
    };

    ctex::exec::ExecutorRegistry registry;
    bool refused = false;
    try {
        registry.add(std::make_shared<InvalidExecutor>());
    } catch (const ctex::exec::ExecutorRegistryError& error) {
        refused = std::string_view(error.what()).find("features") != std::string_view::npos;
    }
    return expect(refused, "executor without emission capabilities entered the registry");
}

}  // namespace

int main() {
    return every_emission_request_uses_the_selected_executor_report() &&
                   registry_refuses_an_executor_without_emission_capabilities()
               ? 0
               : 1;
}
