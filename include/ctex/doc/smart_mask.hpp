#ifndef CTEX_DOC_SMART_MASK_HPP
#define CTEX_DOC_SMART_MASK_HPP

#include <cstdint>
#include <ctex/doc/smart_material.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::doc {

inline constexpr std::uint32_t current_smart_mask_schema_version = 1;

enum class SmartMaskTargetKind : std::uint8_t { layer, group };

struct SmartMaskPreset {
    std::uint32_t schema_version{current_smart_mask_schema_version};
    SmartMaterialPreset definition;
    friend bool operator==(const SmartMaskPreset&, const SmartMaskPreset&) = default;
};

struct SmartMaskParameterValue {
    std::string parameter_identifier;
    graph::SocketValue value;
    friend bool operator==(const SmartMaskParameterValue&,
                           const SmartMaskParameterValue&) = default;
};

struct SmartMaskInstance {
    std::string identifier;
    std::string target_entry_identifier;
    SmartMaskTargetKind target_kind{SmartMaskTargetKind::layer};
    std::string origin_preset_identifier;
    std::uint32_t origin_preset_schema_version{};
    SmartMaterialPreset fragment;
    std::vector<SmartMaskParameterValue> parameter_values;
    friend bool operator==(const SmartMaskInstance&, const SmartMaskInstance&) = default;
};

void validate_smart_mask(const SmartMaskPreset& preset);
void validate_smart_mask_instance(const SmartMaskInstance& instance);

[[nodiscard]] SmartMaskInstance instantiate_smart_mask(const SmartMaskPreset& preset,
                                                       std::string instance_identifier,
                                                       std::string target_entry_identifier,
                                                       SmartMaskTargetKind target_kind);
[[nodiscard]] SmartMaterialParameterUpdate set_smart_mask_parameter_value(
    SmartMaskInstance& instance, std::string_view parameter_identifier, graph::SocketValue value);

[[nodiscard]] std::string serialize_smart_mask(const SmartMaskPreset& preset);
[[nodiscard]] SmartMaskPreset deserialize_smart_mask(std::string_view serialized);

}  // namespace ctex::doc

#endif
