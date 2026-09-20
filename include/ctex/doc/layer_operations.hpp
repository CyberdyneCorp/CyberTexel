#ifndef CTEX_DOC_LAYER_OPERATIONS_HPP
#define CTEX_DOC_LAYER_OPERATIONS_HPP

#include <cstddef>
#include <ctex/doc/layer_compositor.hpp>
#include <ctex/doc/layer_stack.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ctex::doc {

class TextureSet;

struct CreateLayerOperation {
    LayerEntry entry;
    std::vector<LayerCompositeRaster> content;
    std::vector<LayerCompositeMaskRaster> masks;
};

struct DuplicateLayerOperation {
    std::string identifier;
    std::string duplicate_identifier;
};

struct DeleteLayerOperation {
    std::string identifier;
    ReferencedSourceDeletionPolicy instance_policy{ReferencedSourceDeletionPolicy::refuse};
};

struct ReorderLayerOperation {
    std::string identifier;
    // Empty places the selected ownership subtree after its last sibling.
    std::string before_identifier;
};

struct ReparentLayerOperation {
    std::string identifier;
    // Empty selects the root scope.
    std::string parent_identifier;
    // Empty places the selected ownership subtree after its last new sibling.
    std::string before_identifier;
};

struct ClearLayerOperation {
    std::string identifier;
    // Empty clears every supplied channel on the entry.
    std::string semantic_id;
};

struct InvertLayerOperation {
    std::string identifier;
    // Empty inverts every supplied channel on the entry.
    std::string semantic_id;
};

struct MergeDownLayerOperation {
    std::string identifier;
    // Resolved replacement rasters keyed to the lower sibling's identity.
    std::vector<LayerCompositeRaster> replacement_content;
};

struct MergeGroupLayerOperation {
    std::string identifier;
    // Resolved replacement rasters keyed to the group's identity.
    std::vector<LayerCompositeRaster> replacement_content;
};

struct FlattenLayersOperation {
    LayerEntry output_entry;
    // Resolved replacement rasters keyed to output_entry.identifier.
    std::vector<LayerCompositeRaster> replacement_content;
};

struct ConvertLayerOperation {
    std::string identifier;
    LayerEntryKind target_kind{LayerEntryKind::paint_layer};
    std::optional<graph::GraphDocument> fill_graph;
};

struct ApplyMaskLayerOperation {
    std::string mask_identifier;
    // Resolved masked rasters keyed to the attachment target's identity.
    std::vector<LayerCompositeRaster> replacement_content;
};

using LayerOperation =
    std::variant<CreateLayerOperation, DuplicateLayerOperation, DeleteLayerOperation,
                 ReorderLayerOperation, ReparentLayerOperation, ClearLayerOperation,
                 InvertLayerOperation, MergeDownLayerOperation, MergeGroupLayerOperation,
                 FlattenLayersOperation, ConvertLayerOperation, ApplyMaskLayerOperation>;

struct LayerOperationRequest {
    LayerOperation operation;
    LayerCompositeRequest resolved_content;
    std::size_t maximum_output_bytes{1ULL << 30};
    float appearance_tolerance{1.0e-6F};
};

struct LayerOperationResult {
    LayerCompositeRequest resolved_content;
    std::vector<std::string> affected_identifiers;
};

enum class LayerOperationErrorCode : std::uint8_t {
    invalid_operation,
    invalid_content,
    allocation_limit,
    appearance_mismatch,
};

class LayerOperationError final : public std::invalid_argument {
public:
    LayerOperationError(LayerOperationErrorCode code, std::string message);
    [[nodiscard]] LayerOperationErrorCode code() const noexcept { return code_; }

private:
    LayerOperationErrorCode code_;
};

// Builds and validates the complete candidate before publishing the new stack.
// The request owns its resolved raster snapshot, so failure leaves both the
// TextureSet and the caller's original snapshot unchanged.
[[nodiscard]] LayerOperationResult apply_layer_operation(TextureSet& texture_set,
                                                         LayerOperationRequest request);

}  // namespace ctex::doc

#endif
