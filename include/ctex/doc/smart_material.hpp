#ifndef CTEX_DOC_SMART_MATERIAL_HPP
#define CTEX_DOC_SMART_MATERIAL_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/graph/document.hpp>
#include <ctex/image/pixel_format.hpp>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::doc {

inline constexpr std::uint32_t current_smart_material_schema_version = 6;

enum class SmartMaterialEntryKind : std::uint8_t { layer, group, mask, filter, generator };
enum class SmartMaterialContentKind : std::uint8_t { derived, model_specific };

struct SmartMaterialPixelPayload {
    std::string identifier;
    std::uint32_t width{};
    std::uint32_t height{};
    image::PixelFormat format{};
    // Tightly packed, row-major pixels in the declared format.
    std::vector<std::byte> pixels;
    friend bool operator==(const SmartMaterialPixelPayload&,
                           const SmartMaterialPixelPayload&) = default;
};

struct SmartMaterialEntry {
    std::string identifier;
    std::string parent_identifier;
    std::string display_name;
    SmartMaterialEntryKind kind{SmartMaterialEntryKind::layer};
    bool enabled{true};
    double opacity{1.0};
    std::optional<graph::GraphDocument> graph;
    SmartMaterialContentKind content_kind{SmartMaterialContentKind::derived};
    std::vector<SmartMaterialPixelPayload> pixel_payloads;
    friend bool operator==(const SmartMaterialEntry&, const SmartMaterialEntry&) = default;
};

enum class SmartMaterialBindingTargetKind : std::uint8_t { input, property };
enum class SmartMaterialParameterBindingState : std::uint8_t { bound, legacy_unbound };

struct SmartMaterialParameterBinding {
    std::string entry_identifier;
    graph::NodeId node_id{};
    SmartMaterialBindingTargetKind target_kind{SmartMaterialBindingTargetKind::input};
    std::string target_identifier;
    friend bool operator==(const SmartMaterialParameterBinding&,
                           const SmartMaterialParameterBinding&) = default;
};

struct ExposedSmartMaterialParameter {
    std::string identifier;
    std::string display_name;
    std::string display_group;
    graph::SocketType type{graph::SocketType::scalar};
    graph::SocketValue default_value{0.0};
    std::optional<double> minimum;
    std::optional<double> maximum;
    SmartMaterialParameterBindingState binding_state{SmartMaterialParameterBindingState::bound};
    std::vector<SmartMaterialParameterBinding> bindings;
    friend bool operator==(const ExposedSmartMaterialParameter&,
                           const ExposedSmartMaterialParameter&) = default;
};

struct SmartMaterialAnchorReference {
    std::string anchor_entry_identifier;
    std::string consumer_entry_identifier;
    graph::NodeId consumer_node_id{};
    std::string consumer_input_identifier;
    friend bool operator==(const SmartMaterialAnchorReference&,
                           const SmartMaterialAnchorReference&) = default;
};

struct SmartMaterialResourceReference {
    std::string identifier;
    std::string kind;
    friend bool operator==(const SmartMaterialResourceReference&,
                           const SmartMaterialResourceReference&) = default;
};

struct SmartMaterialPreset {
    std::uint32_t schema_version{current_smart_material_schema_version};
    std::string identifier;
    std::string display_name;
    std::vector<SmartMaterialEntry> stack;
    std::vector<ExposedSmartMaterialParameter> exposed_parameters;
    std::vector<std::string> anchor_entries;
    std::vector<SmartMaterialAnchorReference> anchor_references;
    std::vector<SmartMaterialResourceReference> resource_references;
    friend bool operator==(const SmartMaterialPreset&, const SmartMaterialPreset&) = default;
};

struct SmartMaterialContentReportEntry {
    std::string entry_identifier;
    SmartMaterialContentKind content_kind{SmartMaterialContentKind::derived};
    std::size_t pixel_payload_count{};
    std::size_t stored_pixel_bytes{};
    friend bool operator==(const SmartMaterialContentReportEntry&,
                           const SmartMaterialContentReportEntry&) = default;
};

struct SmartMaterialContentReport {
    std::vector<SmartMaterialContentReportEntry> entries;
    std::size_t derived_entry_count{};
    std::size_t model_specific_entry_count{};
    std::size_t model_specific_pixel_bytes{};

    [[nodiscard]] bool contains_model_specific_content() const noexcept {
        return model_specific_entry_count != 0;
    }
    friend bool operator==(const SmartMaterialContentReport&,
                           const SmartMaterialContentReport&) = default;
};

struct SmartMaterialParameterUpdate {
    std::string parameter_identifier;
    std::vector<SmartMaterialParameterBinding> updated_bindings;
    friend bool operator==(const SmartMaterialParameterUpdate&,
                           const SmartMaterialParameterUpdate&) = default;
};

struct SmartMaterialAnchorEvaluationPlan {
    std::vector<std::string> entry_identifiers;
    friend bool operator==(const SmartMaterialAnchorEvaluationPlan&,
                           const SmartMaterialAnchorEvaluationPlan&) = default;
};

enum class SmartMaterialResourceLocation : std::uint8_t { input, output, property };

struct SmartMaterialImageResourceUse {
    std::string entry_identifier;
    graph::NodeId node_id{};
    SmartMaterialResourceLocation location{SmartMaterialResourceLocation::input};
    std::string target_identifier;
    std::string resource_identifier;
    friend bool operator==(const SmartMaterialImageResourceUse&,
                           const SmartMaterialImageResourceUse&) = default;
};

enum class SmartMaterialErrorCode : std::uint8_t {
    invalid_preset,
    invalid_parameter_value,
    malformed_serialization,
    unknown_parameter,
    read_only_parameter,
    unsupported_version,
    invalid_anchor_reference,
    anchor_ordering_violation,
    anchor_cycle,
    invalid_resource_reference,
};

class SmartMaterialError final : public std::invalid_argument {
public:
    SmartMaterialError(SmartMaterialErrorCode code, std::string message);
    [[nodiscard]] SmartMaterialErrorCode code() const noexcept { return code_; }

private:
    SmartMaterialErrorCode code_;
};

void validate_smart_material(const SmartMaterialPreset& preset);
[[nodiscard]] SmartMaterialContentReport report_smart_material_content(
    const SmartMaterialPreset& preset);
[[nodiscard]] SmartMaterialParameterUpdate set_smart_material_parameter_value(
    SmartMaterialPreset& preset, std::string_view parameter_identifier, graph::SocketValue value);
void set_smart_material_anchor(SmartMaterialPreset& preset, std::string_view entry_identifier,
                               bool marked);
void add_smart_material_anchor_reference(SmartMaterialPreset& preset,
                                         SmartMaterialAnchorReference reference);
[[nodiscard]] SmartMaterialAnchorEvaluationPlan plan_smart_material_anchor_evaluation(
    const SmartMaterialPreset& preset, std::span<const std::string_view> changed_anchor_entries);

// Public so validation can be composed by smart-material containers.
void validate_smart_material_anchors(const SmartMaterialPreset& preset);
void validate_smart_material_resources(const SmartMaterialPreset& preset);
[[nodiscard]] std::vector<SmartMaterialImageResourceUse> list_smart_material_image_resource_uses(
    const SmartMaterialPreset& preset);

// Canonical, versioned, device-independent representation. Stack order is
// retained because it is part of the material's compositing semantics.
[[nodiscard]] std::string serialize_smart_material(const SmartMaterialPreset& preset);
[[nodiscard]] SmartMaterialPreset deserialize_smart_material(std::string_view serialized);

}  // namespace ctex::doc

#endif
