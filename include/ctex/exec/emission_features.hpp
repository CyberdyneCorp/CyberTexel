#ifndef CTEX_EXEC_EMISSION_FEATURES_HPP
#define CTEX_EXEC_EMISSION_FEATURES_HPP

#include <ctex/emit/material_emission.hpp>
#include <ctex/emit/preview_emission.hpp>
#include <ctex/exec/executor.hpp>

namespace ctex::exec {

[[nodiscard]] emit::LayerStackEmissionRequest emission_request_for(
    const Executor& executor, emit::LayerStackEmissionRequest request);
[[nodiscard]] emit::MaterialShaderEmissionRequest emission_request_for(
    const Executor& executor, emit::MaterialShaderEmissionRequest request);
[[nodiscard]] emit::PreviewEmissionRequest emission_request_for(
    const Executor& executor, emit::PreviewEmissionRequest request);

}  // namespace ctex::exec

#endif
