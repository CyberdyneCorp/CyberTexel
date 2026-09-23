#ifndef CTEX_IO_OPERATION_RECORD_HPP
#define CTEX_IO_OPERATION_RECORD_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/image/color.hpp>
#include <ctex/image/pixel_format.hpp>
#include <ctex/io/project_container.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace ctex::io {

inline constexpr std::uint32_t current_operation_record_schema = 1;
inline constexpr std::string_view operation_record_asset_kind = "operation-record";

enum class OperationReplayClass : std::uint8_t {
    checkpoint_only,
    same_resolution,
    resolution_independent,
};

enum class OperationPayloadKind : std::uint8_t {
    resolved_stamps,
    editable_source_path,
    opaque_algorithm_data,
};

struct OperationChannelRecord {
    std::string semantic_id;
    image::PixelFormat format;
    image::ColorSpace color_space{image::ColorSpace::linear_rec709};
    std::vector<double> default_value;
    friend bool operator==(const OperationChannelRecord&, const OperationChannelRecord&) = default;
};

struct PinnedOperationResource {
    std::string role;
    std::string content_identity;
    std::vector<std::byte> bytes;
    friend bool operator==(const PinnedOperationResource&,
                           const PinnedOperationResource&) = default;
};

struct EditableOperationRecord {
    std::uint32_t schema_version{current_operation_record_schema};
    std::string identifier;
    std::string algorithm_identifier;
    std::uint32_t algorithm_version{};
    std::string preset_identifier;
    std::uint32_t preset_version{};
    OperationReplayClass replay_class{OperationReplayClass::checkpoint_only};
    ProjectRevision input_document_revision{};
    std::uint64_t seed{};
    std::string mesh_content_identity;
    std::array<double, 16> coordinate_frame{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                            0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    OperationPayloadKind payload_kind{OperationPayloadKind::opaque_algorithm_data};
    std::uint32_t payload_version{};
    std::vector<OperationChannelRecord> channels;
    std::vector<PinnedOperationResource> pinned_resources;
    std::vector<std::string> checkpoint_image_identifiers;
    std::vector<std::byte> payload;
    friend bool operator==(const EditableOperationRecord&,
                           const EditableOperationRecord&) = default;
};

struct OperationRecordReadLimits {
    std::size_t maximum_input_bytes{256ULL << 20};
    std::size_t maximum_total_payload_bytes{256ULL << 20};
    std::size_t maximum_string_bytes{1ULL << 20};
    std::size_t maximum_channels{1'024};
    std::size_t maximum_pinned_resources{65'536};
    std::size_t maximum_checkpoint_images{65'536};
};

class OperationRecordError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void validate_operation_record(const EditableOperationRecord& record);
[[nodiscard]] std::vector<std::byte> serialize_operation_record(
    const EditableOperationRecord& record);
[[nodiscard]] EditableOperationRecord deserialize_operation_record(
    std::span<const std::byte> serialized, OperationRecordReadLimits limits = {});

[[nodiscard]] StandaloneAsset package_operation_record(const EditableOperationRecord& record);
[[nodiscard]] EditableOperationRecord unpack_operation_record(
    const StandaloneAsset& asset, OperationRecordReadLimits limits = {});

}  // namespace ctex::io

#endif
