#ifndef CTEX_DOC_LAYER_COMPOSITOR_HPP
#define CTEX_DOC_LAYER_COMPOSITOR_HPP

#include <cstdint>
#include <ctex/graph/document.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::doc {

class TextureSet;

struct LayerCompositeRaster {
    std::string entry_identifier;
    std::string semantic_id;
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<graph::ColourValue> pixels;
    // Optional independent coverage. Empty means fully covered.
    std::vector<float> coverage;
};

struct LayerCompositeMaskRaster {
    std::string mask_identifier;
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<double> values;
};

struct LayerCompositeRequest {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<LayerCompositeRaster> content;
    std::vector<LayerCompositeMaskRaster> masks;
};

struct LayerCompositeChannel {
    std::string semantic_id;
    std::uint8_t component_count{};
    std::vector<graph::ColourValue> pixels;
    friend bool operator==(const LayerCompositeChannel&, const LayerCompositeChannel&) = default;
};

struct LayerCompositeResult {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<LayerCompositeChannel> channels;
    [[nodiscard]] const LayerCompositeChannel& channel(std::string_view semantic_id) const;
    friend bool operator==(const LayerCompositeResult&, const LayerCompositeResult&) = default;
};

enum class LayerCompositeErrorCode : std::uint8_t {
    invalid_request,
    duplicate_input,
    unknown_input,
    missing_content,
    missing_mask,
    unevaluable_channel,
};

class LayerCompositeError final : public std::invalid_argument {
public:
    LayerCompositeError(LayerCompositeErrorCode code, std::string message);
    [[nodiscard]] LayerCompositeErrorCode code() const noexcept { return code_; }

private:
    LayerCompositeErrorCode code_;
};

// Deterministic device-free reference composition. Content rasters are the
// resolved outputs of paint/fill/editable entries and filter graphs; the
// document remains authoritative for stack order and all modulation.
[[nodiscard]] LayerCompositeResult composite_texture_set_cpu(const TextureSet& texture_set,
                                                             const LayerCompositeRequest& request);

}  // namespace ctex::doc

#endif
