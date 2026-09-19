#include <ctex/exec/emission_features.hpp>
#include <utility>

namespace ctex::exec {

emit::LayerStackEmissionRequest emission_request_for(const Executor& executor,
                                                     emit::LayerStackEmissionRequest request) {
    request.features = executor.descriptor().features;
    return request;
}

emit::MaterialShaderEmissionRequest emission_request_for(
    const Executor& executor, emit::MaterialShaderEmissionRequest request) {
    request.features = executor.descriptor().features;
    return request;
}

emit::PreviewEmissionRequest emission_request_for(const Executor& executor,
                                                  emit::PreviewEmissionRequest request) {
    request.features = executor.descriptor().features;
    return request;
}

}  // namespace ctex::exec
