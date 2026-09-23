#include <algorithm>
#include <bit>
#include <cmath>
#include <ctex/io/operation_record.hpp>
#include <limits>
#include <set>
#include <string_view>
#include <utility>

namespace ctex::io {
namespace {

constexpr std::array<std::byte, 8> record_magic{std::byte{'C'}, std::byte{'T'}, std::byte{'E'},
                                                std::byte{'X'}, std::byte{'O'}, std::byte{'P'},
                                                std::byte{'R'}, std::byte{0}};

class Writer {
public:
    void u8(std::uint8_t value) { bytes_.push_back(static_cast<std::byte>(value)); }
    void u32(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            u8(static_cast<std::uint8_t>(value >> shift));
        }
    }
    void u64(std::uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8) {
            u8(static_cast<std::uint8_t>(value >> shift));
        }
    }
    void f64(double value) { u64(std::bit_cast<std::uint64_t>(value)); }
    void bytes(std::span<const std::byte> value) {
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }
    void string(std::string_view value) {
        if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw OperationRecordError("operation-record string exceeds the format limit");
        }
        u32(static_cast<std::uint32_t>(value.size()));
        bytes(std::as_bytes(std::span(value)));
    }
    [[nodiscard]] std::vector<std::byte> finish() && { return std::move(bytes_); }

private:
    std::vector<std::byte> bytes_;
};

class Reader {
public:
    Reader(std::span<const std::byte> bytes, const OperationRecordReadLimits& limits)
        : bytes_(bytes), limits_(limits) {}

    [[nodiscard]] std::uint8_t u8(std::string_view field) {
        return std::to_integer<std::uint8_t>(take(1, field).front());
    }
    [[nodiscard]] std::uint32_t u32(std::string_view field) {
        const auto value = take(4, field);
        std::uint32_t result = 0;
        for (unsigned index = 0; index < 4; ++index) {
            result |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value[index]))
                      << (index * 8U);
        }
        return result;
    }
    [[nodiscard]] std::uint64_t u64(std::string_view field) {
        const auto value = take(8, field);
        std::uint64_t result = 0;
        for (unsigned index = 0; index < 8; ++index) {
            result |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(value[index]))
                      << (index * 8U);
        }
        return result;
    }
    [[nodiscard]] double f64(std::string_view field) {
        const double result = std::bit_cast<double>(u64(field));
        if (!std::isfinite(result)) {
            throw OperationRecordError("operation record contains a non-finite " +
                                       std::string(field));
        }
        return result;
    }
    [[nodiscard]] std::string string(std::string_view field) {
        const std::size_t size = u32(field);
        if (size > limits_.maximum_string_bytes) {
            throw OperationRecordError("operation-record string exceeds the configured limit");
        }
        const auto value = take(size, field);
        return {reinterpret_cast<const char*>(value.data()), value.size()};
    }
    [[nodiscard]] std::vector<std::byte> blob(std::string_view field) {
        const std::uint64_t declared = u64(field);
        if (declared > std::numeric_limits<std::size_t>::max()) {
            throw OperationRecordError("operation-record payload exceeds the platform limit");
        }
        const std::size_t size = static_cast<std::size_t>(declared);
        if (size > limits_.maximum_total_payload_bytes -
                       std::min(payload_bytes_, limits_.maximum_total_payload_bytes)) {
            throw OperationRecordError(
                "operation-record payload exceeds the configured aggregate limit");
        }
        payload_bytes_ += size;
        const auto value = take(size, field);
        return {value.begin(), value.end()};
    }
    [[nodiscard]] std::span<const std::byte> take(std::size_t size, std::string_view field) {
        if (size > bytes_.size() - offset_) {
            throw OperationRecordError("operation record ends inside " + std::string(field));
        }
        const auto result = bytes_.subspan(offset_, size);
        offset_ += size;
        return result;
    }
    [[nodiscard]] bool empty() const noexcept { return offset_ == bytes_.size(); }

private:
    std::span<const std::byte> bytes_;
    const OperationRecordReadLimits& limits_;
    std::size_t offset_{};
    std::size_t payload_bytes_{};
};

void validate_count(std::size_t value, std::size_t maximum, std::string_view field) {
    if (value > maximum || value > std::numeric_limits<std::uint32_t>::max()) {
        throw OperationRecordError(std::string(field) + " exceeds the supported count");
    }
}

void validate_unique_string(std::set<std::string_view>& values, std::string_view value,
                            std::string_view field) {
    if (value.empty() || !values.insert(value).second) {
        throw OperationRecordError(std::string(field) + " is empty or duplicated");
    }
}

bool source_snapshot_operation(std::string_view algorithm) noexcept {
    return algorithm == "cybertexel.paint.clone" || algorithm == "cybertexel.paint.blur" ||
           algorithm == "cybertexel.paint.smear";
}

bool has_pinned_source_snapshot(const EditableOperationRecord& record) noexcept {
    return std::ranges::any_of(
        record.pinned_resources,
        [](const PinnedOperationResource& resource) { return resource.role == "source-snapshot"; });
}

void validate_algorithm_support(std::span<const OperationAlgorithmSupport> supported_algorithms) {
    std::set<std::string_view> identifiers;
    for (const OperationAlgorithmSupport& support : supported_algorithms) {
        if (support.identifier.empty() || support.minimum_version == 0 ||
            support.minimum_version > support.maximum_version ||
            !identifiers.insert(support.identifier).second) {
            throw OperationRecordError(
                "operation algorithm support ranges must be named, ordered and unique");
        }
    }
}

OperationChannelRecord read_channel(Reader& reader) {
    OperationChannelRecord channel;
    channel.semantic_id = reader.string("channel semantic identity");
    const std::uint8_t type = reader.u8("channel type");
    const std::uint8_t count = reader.u8("channel component count");
    static_cast<void>(reader.u8("channel reserved byte"));
    static_cast<void>(reader.u8("channel reserved byte"));
    if (type > static_cast<std::uint8_t>(image::ChannelType::float32)) {
        throw OperationRecordError("operation record contains an unknown channel type");
    }
    channel.format = {static_cast<image::ChannelType>(type), count};
    const std::uint32_t color_space = reader.u32("channel color space");
    if (color_space > static_cast<std::uint32_t>(image::ColorSpace::srgb_rec709)) {
        throw OperationRecordError("operation record contains an unknown color space");
    }
    channel.color_space = static_cast<image::ColorSpace>(color_space);
    const std::uint32_t default_count = reader.u32("channel default count");
    if (default_count > 4) {
        throw OperationRecordError("operation record channel default is too large");
    }
    channel.default_value.reserve(default_count);
    for (std::uint32_t index = 0; index < default_count; ++index) {
        channel.default_value.push_back(reader.f64("channel default value"));
    }
    return channel;
}

void write_channel(Writer& writer, const OperationChannelRecord& channel) {
    writer.string(channel.semantic_id);
    writer.u8(static_cast<std::uint8_t>(channel.format.channel_type));
    writer.u8(channel.format.channel_count);
    writer.u8(0);
    writer.u8(0);
    writer.u32(static_cast<std::uint32_t>(channel.color_space));
    writer.u32(static_cast<std::uint32_t>(channel.default_value.size()));
    for (double value : channel.default_value) writer.f64(value);
}

}  // namespace

void validate_operation_record(const EditableOperationRecord& record) {
    if (record.schema_version != current_operation_record_schema || record.identifier.empty() ||
        record.algorithm_identifier.empty() || record.algorithm_version == 0 ||
        record.preset_identifier.empty() || record.preset_version == 0 ||
        record.mesh_content_identity.empty() || record.payload_version == 0 ||
        record.payload.empty()) {
        throw OperationRecordError("operation record metadata is incomplete or unsupported");
    }
    if (static_cast<std::uint8_t>(record.replay_class) >
            static_cast<std::uint8_t>(OperationReplayClass::resolution_independent) ||
        static_cast<std::uint8_t>(record.payload_kind) >
            static_cast<std::uint8_t>(OperationPayloadKind::opaque_algorithm_data)) {
        throw OperationRecordError("operation record contains an unknown enum value");
    }
    if (!std::ranges::all_of(record.coordinate_frame,
                             [](double value) { return std::isfinite(value); })) {
        throw OperationRecordError("operation record coordinate frame must be finite");
    }
    std::set<std::string_view> channel_ids;
    for (const OperationChannelRecord& channel : record.channels) {
        validate_unique_string(channel_ids, channel.semantic_id, "operation channel identity");
        if (!channel.format.is_valid() ||
            channel.default_value.size() != channel.format.channel_count ||
            !std::ranges::all_of(channel.default_value,
                                 [](double value) { return std::isfinite(value); })) {
            throw OperationRecordError("operation channel descriptor is invalid");
        }
    }
    if (record.channels.empty()) {
        throw OperationRecordError("operation record requires at least one channel");
    }
    std::set<std::string_view> resource_roles;
    std::set<std::string_view> resource_content;
    for (const PinnedOperationResource& resource : record.pinned_resources) {
        validate_unique_string(resource_roles, resource.role, "pinned resource role");
        validate_unique_string(resource_content, resource.content_identity,
                               "pinned resource content identity");
        if (resource.bytes.empty()) {
            throw OperationRecordError("pinned resource payload is empty");
        }
    }
    std::set<std::string_view> checkpoints;
    for (const std::string& checkpoint : record.checkpoint_image_identifiers) {
        validate_unique_string(checkpoints, checkpoint, "checkpoint image identity");
    }
    if (record.replay_class == OperationReplayClass::checkpoint_only && checkpoints.empty()) {
        throw OperationRecordError("checkpoint-only operation has no raster checkpoint");
    }
    if (source_snapshot_operation(record.algorithm_identifier) &&
        record.replay_class != OperationReplayClass::checkpoint_only &&
        !has_pinned_source_snapshot(record)) {
        throw OperationRecordError(
            "clone, blur and smear replay requires a pinned source-snapshot resource");
    }
}

std::vector<std::byte> serialize_operation_record(const EditableOperationRecord& record) {
    validate_operation_record(record);
    validate_count(record.channels.size(), std::numeric_limits<std::uint32_t>::max(),
                   "operation channel count");
    validate_count(record.pinned_resources.size(), std::numeric_limits<std::uint32_t>::max(),
                   "pinned resource count");
    validate_count(record.checkpoint_image_identifiers.size(),
                   std::numeric_limits<std::uint32_t>::max(), "checkpoint image count");
    Writer writer;
    writer.bytes(record_magic);
    writer.u32(record.schema_version);
    writer.string(record.identifier);
    writer.string(record.algorithm_identifier);
    writer.u32(record.algorithm_version);
    writer.string(record.preset_identifier);
    writer.u32(record.preset_version);
    writer.u8(static_cast<std::uint8_t>(record.replay_class));
    writer.u8(static_cast<std::uint8_t>(record.payload_kind));
    writer.u8(0);
    writer.u8(0);
    writer.u32(record.payload_version);
    writer.u64(record.input_document_revision);
    writer.u64(record.seed);
    writer.string(record.mesh_content_identity);
    for (double value : record.coordinate_frame) writer.f64(value);
    writer.u32(static_cast<std::uint32_t>(record.channels.size()));
    writer.u32(static_cast<std::uint32_t>(record.pinned_resources.size()));
    writer.u32(static_cast<std::uint32_t>(record.checkpoint_image_identifiers.size()));
    for (const OperationChannelRecord& channel : record.channels) write_channel(writer, channel);
    for (const PinnedOperationResource& resource : record.pinned_resources) {
        writer.string(resource.role);
        writer.string(resource.content_identity);
        writer.u64(resource.bytes.size());
        writer.bytes(resource.bytes);
    }
    for (const std::string& checkpoint : record.checkpoint_image_identifiers) {
        writer.string(checkpoint);
    }
    writer.u64(record.payload.size());
    writer.bytes(record.payload);
    return std::move(writer).finish();
}

EditableOperationRecord deserialize_operation_record(std::span<const std::byte> serialized,
                                                     OperationRecordReadLimits limits) {
    if (serialized.size() > limits.maximum_input_bytes) {
        throw OperationRecordError("operation record exceeds the configured input limit");
    }
    Reader reader(serialized, limits);
    if (!std::ranges::equal(reader.take(record_magic.size(), "operation-record magic"),
                            record_magic)) {
        throw OperationRecordError("operation-record magic is invalid");
    }
    EditableOperationRecord record;
    record.schema_version = reader.u32("operation-record schema");
    record.identifier = reader.string("operation identity");
    record.algorithm_identifier = reader.string("algorithm identity");
    record.algorithm_version = reader.u32("algorithm version");
    record.preset_identifier = reader.string("preset identity");
    record.preset_version = reader.u32("preset version");
    const std::uint8_t replay_class = reader.u8("replay class");
    const std::uint8_t payload_kind = reader.u8("payload kind");
    static_cast<void>(reader.u8("operation reserved byte"));
    static_cast<void>(reader.u8("operation reserved byte"));
    record.replay_class = static_cast<OperationReplayClass>(replay_class);
    record.payload_kind = static_cast<OperationPayloadKind>(payload_kind);
    record.payload_version = reader.u32("payload version");
    record.input_document_revision = reader.u64("input document revision");
    record.seed = reader.u64("operation seed");
    record.mesh_content_identity = reader.string("mesh content identity");
    for (double& value : record.coordinate_frame) value = reader.f64("coordinate frame");
    const std::uint32_t channel_count = reader.u32("operation channel count");
    const std::uint32_t resource_count = reader.u32("pinned resource count");
    const std::uint32_t checkpoint_count = reader.u32("checkpoint image count");
    validate_count(channel_count, limits.maximum_channels, "operation channel count");
    validate_count(resource_count, limits.maximum_pinned_resources, "pinned resource count");
    validate_count(checkpoint_count, limits.maximum_checkpoint_images, "checkpoint image count");
    record.channels.reserve(channel_count);
    for (std::uint32_t index = 0; index < channel_count; ++index) {
        record.channels.push_back(read_channel(reader));
    }
    record.pinned_resources.reserve(resource_count);
    for (std::uint32_t index = 0; index < resource_count; ++index) {
        record.pinned_resources.push_back(
            {.role = reader.string("pinned resource role"),
             .content_identity = reader.string("pinned resource identity"),
             .bytes = reader.blob("pinned resource payload")});
    }
    record.checkpoint_image_identifiers.reserve(checkpoint_count);
    for (std::uint32_t index = 0; index < checkpoint_count; ++index) {
        record.checkpoint_image_identifiers.push_back(reader.string("checkpoint image identity"));
    }
    record.payload = reader.blob("operation payload");
    if (!reader.empty()) {
        throw OperationRecordError("operation record contains trailing bytes");
    }
    validate_operation_record(record);
    return record;
}

OperationReplayAssessment assess_operation_replay(
    const EditableOperationRecord& record,
    std::span<const OperationAlgorithmSupport> supported_algorithms,
    bool target_resolution_changed) {
    validate_operation_record(record);
    validate_algorithm_support(supported_algorithms);
    const bool checkpoint_available = !record.checkpoint_image_identifiers.empty();
    const auto supported =
        std::ranges::find_if(supported_algorithms, [&](const OperationAlgorithmSupport& candidate) {
            return candidate.identifier == record.algorithm_identifier &&
                   candidate.minimum_version != 0 &&
                   candidate.minimum_version <= record.algorithm_version &&
                   record.algorithm_version <= candidate.maximum_version;
        });
    if (supported == supported_algorithms.end()) {
        return {.disposition = OperationReplayDisposition::unsupported_algorithm,
                .replay_available = false,
                .checkpoint_available = checkpoint_available,
                .target_resolution_changed = target_resolution_changed,
                .diagnostic = "algorithm " + record.algorithm_identifier + " version " +
                              std::to_string(record.algorithm_version) +
                              " is unavailable; raster checkpoint retained"};
    }
    if (record.replay_class == OperationReplayClass::checkpoint_only) {
        return {.disposition = OperationReplayDisposition::checkpoint_only,
                .replay_available = false,
                .checkpoint_available = checkpoint_available,
                .target_resolution_changed = target_resolution_changed,
                .diagnostic = "operation declares checkpoint-only recovery"};
    }
    if (target_resolution_changed && record.replay_class == OperationReplayClass::same_resolution) {
        return {.disposition = OperationReplayDisposition::resample_checkpoint,
                .replay_available = false,
                .checkpoint_available = checkpoint_available,
                .target_resolution_changed = true,
                .diagnostic = "same-resolution operation requires explicit checkpoint resampling"};
    }
    return {
        .disposition = record.replay_class == OperationReplayClass::resolution_independent
                           ? OperationReplayDisposition::replay_resolution_independent
                           : OperationReplayDisposition::replay_same_resolution,
        .replay_available = true,
        .checkpoint_available = checkpoint_available,
        .target_resolution_changed = target_resolution_changed,
        .diagnostic = record.replay_class == OperationReplayClass::resolution_independent
                          ? "operation is eligible for resolution-independent replay"
                          : "operation is eligible for same-resolution recovery replay",
    };
}

StandaloneAsset package_operation_record(const EditableOperationRecord& record) {
    return {.identifier = record.identifier,
            .kind = std::string(operation_record_asset_kind),
            .format_version = record.schema_version,
            .resource_dependencies = {},
            .tiled_image_dependencies = record.checkpoint_image_identifiers,
            .payload = serialize_operation_record(record)};
}

EditableOperationRecord unpack_operation_record(const StandaloneAsset& asset,
                                                OperationRecordReadLimits limits) {
    if (asset.kind != operation_record_asset_kind ||
        asset.format_version != current_operation_record_schema) {
        throw OperationRecordError("standalone asset is not a supported operation record");
    }
    EditableOperationRecord record = deserialize_operation_record(asset.payload, limits);
    if (record.identifier != asset.identifier ||
        record.checkpoint_image_identifiers != asset.tiled_image_dependencies) {
        throw OperationRecordError("operation-record asset metadata disagrees with its payload");
    }
    return record;
}

}  // namespace ctex::io
