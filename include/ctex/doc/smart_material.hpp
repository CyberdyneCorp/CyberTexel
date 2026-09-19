#ifndef CTEX_DOC_SMART_MATERIAL_HPP
#define CTEX_DOC_SMART_MATERIAL_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/graph/document.hpp>
#include <ctex/image/pixel_format.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::doc {

inline constexpr std::uint32_t current_smart_material_schema_version = 2;

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

struct ExposedSmartMaterialParameter {
    std::string identifier;
    std::string display_name;
    std::string display_group;
    graph::SocketType type{graph::SocketType::scalar};
    graph::SocketValue default_value{0.0};
    std::optional<double> minimum;
    std::optional<double> maximum;
    friend bool operator==(const ExposedSmartMaterialParameter&,
                           const ExposedSmartMaterialParameter&) = default;
};

struct SmartMaterialPreset {
    std::uint32_t schema_version{current_smart_material_schema_version};
    std::string identifier;
    std::string display_name;
    std::vector<SmartMaterialEntry> stack;
    std::vector<ExposedSmartMaterialParameter> exposed_parameters;
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

enum class SmartMaterialErrorCode : std::uint8_t {
    invalid_preset,
    malformed_serialization,
    unsupported_version,
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

// Canonical, versioned, device-independent representation. Stack order is
// retained because it is part of the material's compositing semantics.
[[nodiscard]] std::string serialize_smart_material(const SmartMaterialPreset& preset);
[[nodiscard]] SmartMaterialPreset deserialize_smart_material(std::string_view serialized);

}  // namespace ctex::doc

#endif
