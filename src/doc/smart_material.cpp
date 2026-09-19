#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <ctex/doc/smart_material.hpp>
#include <limits>
#include <map>
#include <span>
#include <type_traits>
#include <utility>

namespace ctex::doc {
namespace {

constexpr char hex_digits[] = "0123456789abcdef";

[[noreturn]] void invalid(std::string message) {
    throw SmartMaterialError(SmartMaterialErrorCode::invalid_preset, std::move(message));
}

[[noreturn]] void malformed(std::string message) {
    throw SmartMaterialError(SmartMaterialErrorCode::malformed_serialization, std::move(message));
}

std::string encode_bytes(std::string_view value) {
    std::string result;
    result.reserve(value.size() * 2);
    for (const unsigned char byte : value) {
        result.push_back(hex_digits[byte >> 4U]);
        result.push_back(hex_digits[byte & 0x0fU]);
    }
    return result;
}

std::string encode_bytes(std::span<const std::byte> value) {
    std::string result;
    result.reserve(value.size() * 2);
    for (const std::byte byte : value) {
        const auto number = std::to_integer<std::uint8_t>(byte);
        result.push_back(hex_digits[number >> 4U]);
        result.push_back(hex_digits[number & 0x0fU]);
    }
    return result;
}

std::uint8_t hex_nibble(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<std::uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<std::uint8_t>(value - 'a' + 10);
    }
    malformed("smart material contains invalid hexadecimal data");
}

std::string decode_bytes(std::string_view encoded) {
    if (encoded.size() % 2 != 0) {
        malformed("smart material contains an odd hexadecimal string");
    }
    std::string result;
    result.reserve(encoded.size() / 2);
    for (std::size_t index = 0; index < encoded.size(); index += 2) {
        result.push_back(
            static_cast<char>((hex_nibble(encoded[index]) << 4U) | hex_nibble(encoded[index + 1])));
    }
    return result;
}

std::vector<std::byte> decode_pixel_bytes(std::string_view encoded) {
    const std::string decoded = decode_bytes(encoded);
    std::vector<std::byte> result;
    result.reserve(decoded.size());
    for (const unsigned char byte : decoded) {
        result.push_back(static_cast<std::byte>(byte));
    }
    return result;
}

template <typename Integer>
std::string fixed_hex(Integer value) {
    static_assert(std::is_unsigned_v<Integer>);
    std::string result(sizeof(Integer) * 2, '0');
    for (std::size_t index = result.size(); index != 0; --index) {
        result[index - 1] = hex_digits[value & 0x0fU];
        value >>= 4U;
    }
    return result;
}

template <typename Integer>
Integer parse_fixed_hex(std::string_view value) {
    static_assert(std::is_unsigned_v<Integer>);
    if (value.size() != sizeof(Integer) * 2) {
        malformed("smart material contains a malformed numeric value");
    }
    Integer result = 0;
    for (const char digit : value) {
        result = static_cast<Integer>((result << 4U) | hex_nibble(digit));
    }
    return result;
}

std::string double_hex(double value) { return fixed_hex(std::bit_cast<std::uint64_t>(value)); }
std::string float_hex(float value) { return fixed_hex(std::bit_cast<std::uint32_t>(value)); }
double parse_double(std::string_view value) {
    return std::bit_cast<double>(parse_fixed_hex<std::uint64_t>(value));
}
float parse_float(std::string_view value) {
    return std::bit_cast<float>(parse_fixed_hex<std::uint32_t>(value));
}

template <typename Integer>
Integer parse_decimal(std::string_view value, std::string_view subject) {
    static_assert(std::is_unsigned_v<Integer>);
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() ||
        parsed > std::numeric_limits<Integer>::max()) {
        malformed("smart material has an invalid " + std::string(subject));
    }
    return static_cast<Integer>(parsed);
}

std::vector<std::string_view> split(std::string_view value, char delimiter) {
    std::vector<std::string_view> result;
    std::size_t begin = 0;
    while (true) {
        const std::size_t end = value.find(delimiter, begin);
        if (end == std::string_view::npos) {
            result.push_back(value.substr(begin));
            return result;
        }
        result.push_back(value.substr(begin, end - begin));
        begin = end + 1;
    }
}

struct EncodedValue {
    std::string_view tag;
    std::string payload;
};

EncodedValue encode_value(const graph::SocketValue& value) {
    if (const auto* boolean = std::get_if<bool>(&value)) {
        return {"bool", *boolean ? "1" : "0"};
    }
    if (const auto* scalar = std::get_if<double>(&value)) {
        return {"scalar", double_hex(*scalar)};
    }
    if (const auto* vector = std::get_if<graph::VectorValue>(&value)) {
        return {"vector", float_hex(vector->x) + float_hex(vector->y) + float_hex(vector->z)};
    }
    if (const auto* colour = std::get_if<graph::ColourValue>(&value)) {
        return {"colour", float_hex(colour->r) + float_hex(colour->g) + float_hex(colour->b) +
                              float_hex(colour->a)};
    }
    if (const auto* text = std::get_if<std::string>(&value)) {
        return {"text", encode_bytes(*text)};
    }
    if (const auto* image = std::get_if<graph::ImageValue>(&value)) {
        return {"image", encode_bytes(image->resource_id)};
    }
    invalid("exposed parameter default must have a concrete value");
}

graph::SocketValue parse_value(std::string_view tag, std::string_view payload) {
    if (tag == "bool" && (payload == "0" || payload == "1")) {
        return payload == "1";
    }
    if (tag == "scalar") {
        return parse_double(payload);
    }
    if (tag == "vector" && payload.size() == 24) {
        return graph::VectorValue{parse_float(payload.substr(0, 8)),
                                  parse_float(payload.substr(8, 8)),
                                  parse_float(payload.substr(16, 8))};
    }
    if (tag == "colour" && payload.size() == 32) {
        return graph::ColourValue{
            parse_float(payload.substr(0, 8)), parse_float(payload.substr(8, 8)),
            parse_float(payload.substr(16, 8)), parse_float(payload.substr(24, 8))};
    }
    if (tag == "text") {
        return decode_bytes(payload);
    }
    if (tag == "image") {
        return graph::ImageValue{decode_bytes(payload)};
    }
    malformed("smart material has an invalid exposed parameter value");
}

bool finite_value(const graph::SocketValue& value) {
    if (const auto* scalar = std::get_if<double>(&value)) {
        return std::isfinite(*scalar);
    }
    if (const auto* vector = std::get_if<graph::VectorValue>(&value)) {
        return std::isfinite(vector->x) && std::isfinite(vector->y) && std::isfinite(vector->z);
    }
    if (const auto* colour = std::get_if<graph::ColourValue>(&value)) {
        return std::isfinite(colour->r) && std::isfinite(colour->g) && std::isfinite(colour->b) &&
               std::isfinite(colour->a);
    }
    return !std::holds_alternative<std::monostate>(value);
}

bool type_matches(graph::SocketType type, const graph::SocketValue& value) {
    switch (type) {
        case graph::SocketType::scalar:
            return std::holds_alternative<double>(value);
        case graph::SocketType::vector:
            return std::holds_alternative<graph::VectorValue>(value);
        case graph::SocketType::colour:
            return std::holds_alternative<graph::ColourValue>(value);
        case graph::SocketType::string:
            return std::holds_alternative<std::string>(value);
        case graph::SocketType::image:
            return std::holds_alternative<graph::ImageValue>(value);
        case graph::SocketType::boolean:
            return std::holds_alternative<bool>(value);
    }
    return false;
}

bool component_in_range(const graph::SocketValue& value, double minimum, double maximum) {
    const auto in_range = [&](double component) {
        return component >= minimum && component <= maximum;
    };
    if (const auto* scalar = std::get_if<double>(&value)) {
        return in_range(*scalar);
    }
    if (const auto* vector = std::get_if<graph::VectorValue>(&value)) {
        return in_range(vector->x) && in_range(vector->y) && in_range(vector->z);
    }
    if (const auto* colour = std::get_if<graph::ColourValue>(&value)) {
        return in_range(colour->r) && in_range(colour->g) && in_range(colour->b) &&
               in_range(colour->a);
    }
    return false;
}

void validate_parameter(const ExposedSmartMaterialParameter& parameter) {
    if (parameter.identifier.empty() || parameter.display_name.empty() ||
        parameter.display_group.empty()) {
        invalid("exposed parameter requires an identity, display name, and display group");
    }
    if (!type_matches(parameter.type, parameter.default_value) ||
        !finite_value(parameter.default_value)) {
        invalid("exposed parameter default does not match its finite declared type");
    }
    if (parameter.minimum.has_value() != parameter.maximum.has_value()) {
        invalid("exposed parameter range requires both minimum and maximum");
    }
    if (!parameter.minimum.has_value()) {
        return;
    }
    if (!std::isfinite(*parameter.minimum) || !std::isfinite(*parameter.maximum) ||
        *parameter.minimum > *parameter.maximum) {
        invalid("exposed parameter range is invalid");
    }
    if (!component_in_range(parameter.default_value, *parameter.minimum, *parameter.maximum)) {
        invalid("exposed parameter default lies outside its range or type has no numeric range");
    }
}

std::size_t expected_pixel_bytes(const SmartMaterialPixelPayload& payload) {
    if (payload.identifier.empty() || payload.width == 0 || payload.height == 0 ||
        !payload.format.is_valid()) {
        invalid("model-specific pixel payload metadata is invalid");
    }
    constexpr std::size_t maximum = std::numeric_limits<std::size_t>::max();
    if (payload.width > maximum / payload.height) {
        invalid("model-specific pixel payload dimensions overflow");
    }
    const std::size_t area = static_cast<std::size_t>(payload.width) * payload.height;
    if (area > maximum / payload.format.bytes_per_pixel()) {
        invalid("model-specific pixel payload byte size overflows");
    }
    return area * payload.format.bytes_per_pixel();
}

void validate_content(const SmartMaterialEntry& entry) {
    if (static_cast<std::uint8_t>(entry.content_kind) >
        static_cast<std::uint8_t>(SmartMaterialContentKind::model_specific)) {
        invalid("smart material entry has an unknown content kind");
    }
    if (entry.content_kind == SmartMaterialContentKind::derived) {
        if (!entry.pixel_payloads.empty()) {
            invalid("derived smart material content cannot carry rasterized output");
        }
        return;
    }
    if ((entry.kind != SmartMaterialEntryKind::layer &&
         entry.kind != SmartMaterialEntryKind::mask) ||
        entry.pixel_payloads.empty()) {
        invalid("model-specific content requires painted layer or mask pixels");
    }
    std::map<std::string_view, bool, std::less<>> payload_identifiers;
    for (const SmartMaterialPixelPayload& payload : entry.pixel_payloads) {
        if (payload.pixels.size() != expected_pixel_bytes(payload)) {
            invalid("model-specific pixel payload byte count does not match its format");
        }
        if (!payload_identifiers.emplace(payload.identifier, true).second) {
            invalid("model-specific content repeats pixel payload identity '" + payload.identifier +
                    "'");
        }
    }
}

SmartMaterialEntryKind parse_entry_kind(std::string_view value) {
    const auto parsed = parse_decimal<std::uint8_t>(value, "stack entry kind");
    if (parsed > static_cast<std::uint8_t>(SmartMaterialEntryKind::generator)) {
        malformed("smart material has an unknown stack entry kind");
    }
    return static_cast<SmartMaterialEntryKind>(parsed);
}

SmartMaterialContentKind parse_content_kind(std::string_view value) {
    const auto parsed = parse_decimal<std::uint8_t>(value, "content kind");
    if (parsed > static_cast<std::uint8_t>(SmartMaterialContentKind::model_specific)) {
        malformed("smart material has an unknown content kind");
    }
    return static_cast<SmartMaterialContentKind>(parsed);
}

graph::SocketType parse_parameter_type(std::string_view value) {
    const auto parsed = parse_decimal<std::uint8_t>(value, "parameter type");
    if (parsed > static_cast<std::uint8_t>(graph::SocketType::boolean)) {
        malformed("smart material has an unknown exposed parameter type");
    }
    return static_cast<graph::SocketType>(parsed);
}

std::optional<double> parse_optional_double(std::string_view value) {
    return value == "-" ? std::nullopt : std::optional<double>{parse_double(value)};
}

std::optional<graph::GraphDocument> parse_optional_graph(std::string_view value) {
    if (value == "-") {
        return std::nullopt;
    }
    try {
        return graph::deserialize_graph(decode_bytes(value));
    } catch (const SmartMaterialError&) {
        throw;
    } catch (const std::exception& error) {
        malformed("smart material contains an invalid graph: " + std::string(error.what()));
    }
}

void append_entry(std::string& output, const SmartMaterialEntry& entry) {
    output += "ENTRY\t" + std::to_string(static_cast<unsigned>(entry.kind));
    output += "\t" + encode_bytes(entry.identifier);
    output += "\t" + encode_bytes(entry.parent_identifier);
    output += "\t" + encode_bytes(entry.display_name);
    output += entry.enabled ? "\t1\t" : "\t0\t";
    output += double_hex(entry.opacity);
    output += "\t";
    output += entry.graph.has_value() ? encode_bytes(graph::serialize_graph(*entry.graph)) : "-";
    output += "\t" + std::to_string(static_cast<unsigned>(entry.content_kind));
    output.push_back('\n');
}

void append_pixels(std::string& output, const SmartMaterialEntry& entry,
                   const SmartMaterialPixelPayload& payload) {
    output += "PIXELS\t" + encode_bytes(entry.identifier);
    output += "\t" + encode_bytes(payload.identifier);
    output += "\t" + std::to_string(payload.width);
    output += "\t" + std::to_string(payload.height);
    output += "\t" + std::to_string(static_cast<unsigned>(payload.format.channel_type));
    output += "\t" + std::to_string(payload.format.channel_count);
    output += "\t" + encode_bytes(payload.pixels) + "\n";
}

void append_parameter(std::string& output, const ExposedSmartMaterialParameter& parameter) {
    const EncodedValue value = encode_value(parameter.default_value);
    output += "PARAM\t" + std::to_string(static_cast<unsigned>(parameter.type));
    output += "\t" + encode_bytes(parameter.identifier);
    output += "\t" + encode_bytes(parameter.display_name);
    output += "\t" + encode_bytes(parameter.display_group);
    output += "\t" + std::string(value.tag) + "\t" + value.payload;
    output +=
        "\t" + (parameter.minimum.has_value() ? double_hex(*parameter.minimum) : std::string("-"));
    output +=
        "\t" + (parameter.maximum.has_value() ? double_hex(*parameter.maximum) : std::string("-"));
    output.push_back('\n');
}

std::uint32_t parse_envelope(std::span<const std::string_view> records) {
    if (records.size() < 5 || !records.back().empty() || records[records.size() - 2] != "END") {
        malformed("smart material has a malformed envelope");
    }
    const std::vector<std::string_view> header = split(records.front(), '\t');
    if (header.size() != 2 || header[0] != "CTEX_SMART_MATERIAL") {
        malformed("smart material has a malformed header");
    }
    const std::uint32_t version = parse_decimal<std::uint32_t>(header[1], "schema version");
    if (version != current_smart_material_schema_version) {
        throw SmartMaterialError(
            SmartMaterialErrorCode::unsupported_version,
            "smart material schema version " + std::to_string(version) + " is unsupported");
    }
    return version;
}

SmartMaterialPreset parse_preset_record(std::string_view record, std::uint32_t version) {
    const std::vector<std::string_view> fields = split(record, '\t');
    if (fields.size() != 3 || fields[0] != "PRESET") {
        malformed("smart material has a malformed preset record");
    }
    return {.schema_version = version,
            .identifier = decode_bytes(fields[1]),
            .display_name = decode_bytes(fields[2]),
            .stack = {},
            .exposed_parameters = {}};
}

SmartMaterialEntry parse_entry_record(std::span<const std::string_view> fields) {
    if (fields.size() != 9 || fields[0] != "ENTRY") {
        malformed("smart material contains a malformed stack entry record");
    }
    if (fields[5] != "0" && fields[5] != "1") {
        malformed("smart material stack entry has an invalid enabled flag");
    }
    return {.identifier = decode_bytes(fields[2]),
            .parent_identifier = decode_bytes(fields[3]),
            .display_name = decode_bytes(fields[4]),
            .kind = parse_entry_kind(fields[1]),
            .enabled = fields[5] == "1",
            .opacity = parse_double(fields[6]),
            .graph = parse_optional_graph(fields[7]),
            .content_kind = parse_content_kind(fields[8]),
            .pixel_payloads = {}};
}

struct ParsedPixelPayload {
    std::string entry_identifier;
    SmartMaterialPixelPayload payload;
};

ParsedPixelPayload parse_pixel_record(std::span<const std::string_view> fields) {
    if (fields.size() != 8 || fields[0] != "PIXELS") {
        malformed("smart material contains a malformed pixel record");
    }
    const auto channel_type = parse_decimal<std::uint8_t>(fields[5], "pixel channel type");
    if (channel_type > static_cast<std::uint8_t>(image::ChannelType::float32)) {
        malformed("smart material has an unknown pixel channel type");
    }
    return {
        .entry_identifier = decode_bytes(fields[1]),
        .payload = {.identifier = decode_bytes(fields[2]),
                    .width = parse_decimal<std::uint32_t>(fields[3], "pixel width"),
                    .height = parse_decimal<std::uint32_t>(fields[4], "pixel height"),
                    .format = {static_cast<image::ChannelType>(channel_type),
                               parse_decimal<std::uint8_t>(fields[6], "pixel channel count")},
                    .pixels = decode_pixel_bytes(fields[7])},
    };
}

ExposedSmartMaterialParameter parse_parameter_record(std::span<const std::string_view> fields) {
    if (fields.size() != 9 || fields[0] != "PARAM") {
        malformed("smart material contains a malformed exposed parameter record");
    }
    return {.identifier = decode_bytes(fields[2]),
            .display_name = decode_bytes(fields[3]),
            .display_group = decode_bytes(fields[4]),
            .type = parse_parameter_type(fields[1]),
            .default_value = parse_value(fields[5], fields[6]),
            .minimum = parse_optional_double(fields[7]),
            .maximum = parse_optional_double(fields[8])};
}

void append_parsed_record(SmartMaterialPreset& preset, std::string_view record,
                          bool& saw_parameter) {
    const std::vector<std::string_view> fields = split(record, '\t');
    if (!fields.empty() && fields[0] == "ENTRY" && !saw_parameter) {
        preset.stack.push_back(parse_entry_record(fields));
        return;
    }
    if (!fields.empty() && fields[0] == "PIXELS" && !saw_parameter && !preset.stack.empty()) {
        ParsedPixelPayload parsed = parse_pixel_record(fields);
        if (parsed.entry_identifier != preset.stack.back().identifier) {
            malformed("smart material pixel record does not follow its owning entry");
        }
        preset.stack.back().pixel_payloads.push_back(std::move(parsed.payload));
        return;
    }
    if (!fields.empty() && fields[0] == "PARAM") {
        saw_parameter = true;
        preset.exposed_parameters.push_back(parse_parameter_record(fields));
        return;
    }
    malformed("smart material contains a malformed or out-of-order record");
}

void validate_deserialized(const SmartMaterialPreset& preset) {
    try {
        validate_smart_material(preset);
    } catch (const SmartMaterialError& error) {
        if (error.code() == SmartMaterialErrorCode::unsupported_version) {
            throw;
        }
        malformed("serialized smart material is invalid: " + std::string(error.what()));
    }
}

}  // namespace

SmartMaterialError::SmartMaterialError(SmartMaterialErrorCode code, std::string message)
    : std::invalid_argument(std::move(message)), code_(code) {}

void validate_smart_material(const SmartMaterialPreset& preset) {
    if (preset.schema_version != current_smart_material_schema_version) {
        throw SmartMaterialError(SmartMaterialErrorCode::unsupported_version,
                                 "smart material schema version is unsupported");
    }
    if (preset.identifier.empty() || preset.display_name.empty() || preset.stack.empty()) {
        invalid("smart material requires an identity, display name, and non-empty stack");
    }
    std::map<std::string_view, SmartMaterialEntryKind, std::less<>> entries;
    for (const SmartMaterialEntry& entry : preset.stack) {
        if (entry.identifier.empty() || entry.display_name.empty() ||
            !std::isfinite(entry.opacity) || entry.opacity < 0.0 || entry.opacity > 1.0 ||
            static_cast<std::uint8_t>(entry.kind) >
                static_cast<std::uint8_t>(SmartMaterialEntryKind::generator)) {
            invalid("smart material stack entry metadata is invalid");
        }
        if (!entry.parent_identifier.empty()) {
            const auto parent = entries.find(entry.parent_identifier);
            if (parent == entries.end()) {
                invalid("smart material stack parent must be an earlier entry");
            }
        }
        if (!entries.emplace(entry.identifier, entry.kind).second) {
            invalid("smart material repeats stack entry identity '" + entry.identifier + "'");
        }
        validate_content(entry);
    }
    std::map<std::string_view, bool, std::less<>> parameters;
    for (const ExposedSmartMaterialParameter& parameter : preset.exposed_parameters) {
        validate_parameter(parameter);
        if (!parameters.emplace(parameter.identifier, true).second) {
            invalid("smart material repeats exposed parameter identity '" + parameter.identifier +
                    "'");
        }
    }
}

SmartMaterialContentReport report_smart_material_content(const SmartMaterialPreset& preset) {
    validate_smart_material(preset);
    SmartMaterialContentReport report;
    report.entries.reserve(preset.stack.size());
    for (const SmartMaterialEntry& entry : preset.stack) {
        std::size_t stored_pixel_bytes = 0;
        for (const SmartMaterialPixelPayload& payload : entry.pixel_payloads) {
            stored_pixel_bytes += payload.pixels.size();
        }
        report.entries.push_back({.entry_identifier = entry.identifier,
                                  .content_kind = entry.content_kind,
                                  .pixel_payload_count = entry.pixel_payloads.size(),
                                  .stored_pixel_bytes = stored_pixel_bytes});
        if (entry.content_kind == SmartMaterialContentKind::derived) {
            ++report.derived_entry_count;
        } else {
            ++report.model_specific_entry_count;
            report.model_specific_pixel_bytes += stored_pixel_bytes;
        }
    }
    return report;
}

std::string serialize_smart_material(const SmartMaterialPreset& preset) {
    validate_smart_material(preset);
    std::string output = "CTEX_SMART_MATERIAL\t" + std::to_string(preset.schema_version) + "\n";
    output += "PRESET\t" + encode_bytes(preset.identifier) + "\t" +
              encode_bytes(preset.display_name) + "\n";
    for (const SmartMaterialEntry& entry : preset.stack) {
        append_entry(output, entry);
        for (const SmartMaterialPixelPayload& payload : entry.pixel_payloads) {
            append_pixels(output, entry, payload);
        }
    }
    for (const ExposedSmartMaterialParameter& parameter : preset.exposed_parameters) {
        append_parameter(output, parameter);
    }
    output += "END\n";
    return output;
}

SmartMaterialPreset deserialize_smart_material(std::string_view serialized) {
    const std::vector<std::string_view> records = split(serialized, '\n');
    const std::uint32_t version = parse_envelope(records);
    SmartMaterialPreset result = parse_preset_record(records[1], version);
    bool saw_parameter = false;
    for (std::size_t index = 2; index + 2 < records.size(); ++index) {
        append_parsed_record(result, records[index], saw_parameter);
    }
    validate_deserialized(result);
    return result;
}

}  // namespace ctex::doc
