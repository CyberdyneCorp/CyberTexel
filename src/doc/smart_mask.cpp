#include <algorithm>
#include <ctex/doc/smart_mask.hpp>
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

std::uint8_t hex_nibble(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<std::uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<std::uint8_t>(value - 'a' + 10);
    }
    malformed("smart mask contains invalid hexadecimal data");
}

std::string decode_bytes(std::string_view value) {
    if (value.size() % 2 != 0) {
        malformed("smart mask contains an odd hexadecimal string");
    }
    std::string result;
    result.reserve(value.size() / 2);
    for (std::size_t index = 0; index < value.size(); index += 2) {
        result.push_back(
            static_cast<char>((hex_nibble(value[index]) << 4U) | hex_nibble(value[index + 1])));
    }
    return result;
}

std::vector<std::string_view> split_lines(std::string_view value) {
    std::vector<std::string_view> result;
    std::size_t begin = 0;
    while (true) {
        const std::size_t end = value.find('\n', begin);
        if (end == std::string_view::npos) {
            result.push_back(value.substr(begin));
            return result;
        }
        result.push_back(value.substr(begin, end - begin));
        begin = end + 1;
    }
}

bool allowed_entry_kind(SmartMaterialEntryKind kind) {
    return kind == SmartMaterialEntryKind::mask || kind == SmartMaterialEntryKind::filter ||
           kind == SmartMaterialEntryKind::generator;
}

void validate_mask_fragment(const SmartMaterialPreset& definition) {
    validate_smart_material(definition);
    if (definition.stack.front().kind != SmartMaterialEntryKind::mask ||
        !definition.stack.front().parent_identifier.empty()) {
        invalid("smart mask requires one root mask entry");
    }
    bool has_graph = false;
    for (std::size_t index = 0; index < definition.stack.size(); ++index) {
        const SmartMaterialEntry& entry = definition.stack[index];
        if (!allowed_entry_kind(entry.kind) ||
            entry.content_kind != SmartMaterialContentKind::derived ||
            (index != 0 && entry.parent_identifier.empty())) {
            invalid("smart mask entries must be one derived mask hierarchy");
        }
        has_graph = has_graph || entry.graph.has_value();
    }
    if (!has_graph) {
        invalid("smart mask requires a generator, filter, or mask graph");
    }
}

SmartMaskParameterValue& parameter_value(SmartMaskInstance& instance, std::string_view identifier) {
    const auto found =
        std::find_if(instance.parameter_values.begin(), instance.parameter_values.end(),
                     [&](const SmartMaskParameterValue& value) {
                         return value.parameter_identifier == identifier;
                     });
    if (found == instance.parameter_values.end()) {
        throw SmartMaterialError(
            SmartMaterialErrorCode::unknown_parameter,
            "smart mask parameter '" + std::string(identifier) + "' does not exist");
    }
    return *found;
}

}  // namespace

void validate_smart_mask(const SmartMaskPreset& preset) {
    if (preset.schema_version != current_smart_mask_schema_version) {
        throw SmartMaterialError(SmartMaterialErrorCode::unsupported_version,
                                 "smart mask schema version is unsupported");
    }
    validate_mask_fragment(preset.definition);
}

void validate_smart_mask_instance(const SmartMaskInstance& instance) {
    if (instance.identifier.empty() || instance.target_entry_identifier.empty() ||
        instance.origin_preset_identifier.empty() ||
        instance.origin_preset_schema_version != current_smart_mask_schema_version ||
        static_cast<std::uint8_t>(instance.target_kind) >
            static_cast<std::uint8_t>(SmartMaskTargetKind::group) ||
        instance.origin_preset_identifier != instance.fragment.identifier) {
        invalid("smart mask instance metadata is invalid");
    }
    validate_mask_fragment(instance.fragment);
    if (instance.parameter_values.size() != instance.fragment.exposed_parameters.size()) {
        invalid("smart mask instance parameter state is incomplete");
    }
    SmartMaterialPreset expected = instance.fragment;
    for (std::size_t index = 0; index < instance.parameter_values.size(); ++index) {
        const SmartMaskParameterValue& value = instance.parameter_values[index];
        if (value.parameter_identifier != expected.exposed_parameters[index].identifier) {
            invalid("smart mask instance parameter order does not match its definition");
        }
        static_cast<void>(
            set_smart_material_parameter_value(expected, value.parameter_identifier, value.value));
    }
    if (expected != instance.fragment) {
        invalid("smart mask instance graph values disagree with its parameter state");
    }
}

SmartMaskInstance instantiate_smart_mask(const SmartMaskPreset& preset,
                                         std::string instance_identifier,
                                         std::string target_entry_identifier,
                                         SmartMaskTargetKind target_kind) {
    validate_smart_mask(preset);
    if (instance_identifier.empty() || target_entry_identifier.empty() ||
        static_cast<std::uint8_t>(target_kind) >
            static_cast<std::uint8_t>(SmartMaskTargetKind::group)) {
        invalid("smart mask instance target metadata is invalid");
    }
    SmartMaskInstance result{.identifier = std::move(instance_identifier),
                             .target_entry_identifier = std::move(target_entry_identifier),
                             .target_kind = target_kind,
                             .origin_preset_identifier = preset.definition.identifier,
                             .origin_preset_schema_version = preset.schema_version,
                             .fragment = preset.definition,
                             .parameter_values = {}};
    result.parameter_values.reserve(result.fragment.exposed_parameters.size());
    for (std::size_t index = 0; index < result.fragment.exposed_parameters.size(); ++index) {
        const std::string parameter_identifier =
            result.fragment.exposed_parameters[index].identifier;
        const graph::SocketValue default_value =
            result.fragment.exposed_parameters[index].default_value;
        static_cast<void>(set_smart_material_parameter_value(result.fragment, parameter_identifier,
                                                             default_value));
        result.parameter_values.push_back(
            {.parameter_identifier = parameter_identifier, .value = default_value});
    }
    validate_smart_mask_instance(result);
    return result;
}

SmartMaterialParameterUpdate set_smart_mask_parameter_value(SmartMaskInstance& instance,
                                                            std::string_view parameter_identifier,
                                                            graph::SocketValue value) {
    validate_smart_mask_instance(instance);
    SmartMaskInstance updated = instance;
    SmartMaterialParameterUpdate report =
        set_smart_material_parameter_value(updated.fragment, parameter_identifier, value);
    parameter_value(updated, parameter_identifier).value = std::move(value);
    validate_smart_mask_instance(updated);
    instance = std::move(updated);
    return report;
}

std::string serialize_smart_mask(const SmartMaskPreset& preset) {
    validate_smart_mask(preset);
    return "CTEX_SMART_MASK\t" + std::to_string(preset.schema_version) + "\nDEFINITION\t" +
           encode_bytes(serialize_smart_material(preset.definition)) + "\nEND\n";
}

SmartMaskPreset deserialize_smart_mask(std::string_view serialized) {
    const std::vector<std::string_view> lines = split_lines(serialized);
    if (lines.size() != 4 || lines[0].substr(0, 16) != "CTEX_SMART_MASK\t" || lines[2] != "END" ||
        !lines[3].empty()) {
        malformed("smart mask has a malformed envelope");
    }
    const std::string_view version = lines[0].substr(16);
    if (version != "1") {
        throw SmartMaterialError(
            SmartMaterialErrorCode::unsupported_version,
            "smart mask schema version '" + std::string(version) + "' is unsupported");
    }
    if (lines[1].substr(0, 11) != "DEFINITION\t") {
        malformed("smart mask has a malformed definition record");
    }
    SmartMaskPreset result{
        .schema_version = current_smart_mask_schema_version,
        .definition = deserialize_smart_material(decode_bytes(lines[1].substr(11)))};
    try {
        validate_smart_mask(result);
    } catch (const SmartMaterialError& error) {
        if (error.code() == SmartMaterialErrorCode::unsupported_version) {
            throw;
        }
        malformed("serialized smart mask is invalid: " + std::string(error.what()));
    }
    return result;
}

}  // namespace ctex::doc
