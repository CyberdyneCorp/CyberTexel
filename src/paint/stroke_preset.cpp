#include <bit>
#include <charconv>
#include <ctex/paint/stroke_preset.hpp>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace ctex::paint {
namespace {

constexpr char hex_digits[] = "0123456789abcdef";

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
    throw StrokePresetError("stroke preset contains invalid hexadecimal data");
}

std::string decode_bytes(std::string_view encoded) {
    if (encoded.size() % 2 != 0) {
        throw StrokePresetError("stroke preset contains an odd hexadecimal string");
    }
    std::string result;
    result.reserve(encoded.size() / 2);
    for (std::size_t index = 0; index < encoded.size(); index += 2) {
        result.push_back(
            static_cast<char>((hex_nibble(encoded[index]) << 4U) | hex_nibble(encoded[index + 1])));
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

std::string double_hex(double value) { return fixed_hex(std::bit_cast<std::uint64_t>(value)); }

template <typename Integer>
Integer parse_decimal(std::string_view field, std::string_view name) {
    static_assert(std::is_unsigned_v<Integer>);
    std::uint64_t parsed{};
    const auto [end, error] = std::from_chars(field.data(), field.data() + field.size(), parsed);
    if (error != std::errc{} || end != field.data() + field.size() ||
        parsed > std::numeric_limits<Integer>::max()) {
        throw StrokePresetError("stroke preset has an invalid " + std::string(name));
    }
    return static_cast<Integer>(parsed);
}

template <typename Integer>
Integer parse_fixed_hex(std::string_view field) {
    static_assert(std::is_unsigned_v<Integer>);
    if (field.size() != sizeof(Integer) * 2) {
        throw StrokePresetError("stroke preset has a malformed numeric payload");
    }
    Integer result{};
    for (const char digit : field) {
        result = static_cast<Integer>((result << 4U) | hex_nibble(digit));
    }
    return result;
}

double parse_double(std::string_view field) {
    return std::bit_cast<double>(parse_fixed_hex<std::uint64_t>(field));
}

bool parse_bool(std::string_view field, std::string_view name) {
    if (field == "0" || field == "1") {
        return field == "1";
    }
    throw StrokePresetError("stroke preset has an invalid " + std::string(name));
}

std::vector<std::string_view> split(std::string_view value, char delimiter) {
    std::vector<std::string_view> result;
    std::size_t begin{};
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

std::vector<std::string_view> preset_lines(std::string_view serialized) {
    std::vector<std::string_view> result = split(serialized, '\n');
    if (result.empty() || !result.back().empty()) {
        throw StrokePresetError("stroke preset must end with a newline");
    }
    result.pop_back();
    for (const std::string_view line : result) {
        if (line.empty()) {
            throw StrokePresetError("stroke preset contains an empty record");
        }
    }
    return result;
}

void append_field(std::string& output, std::string_view field) {
    output.push_back('\t');
    output.append(field);
}

void append_double(std::string& output, double value) { append_field(output, double_hex(value)); }

void append_bool(std::string& output, bool value) { append_field(output, value ? "1" : "0"); }

void append_mapping(std::string& output, std::string_view identifier,
                    const ResponseMapping& mapping) {
    output.append("MAP");
    append_field(output, identifier);
    append_bool(output, mapping.enabled);
    append_double(output, mapping.minimum_output);
    append_double(output, mapping.maximum_output);
    for (const ResponseCurvePoint point : mapping.curve.points) {
        append_double(output, point.input);
        append_double(output, point.output);
    }
    output.push_back('\n');
}

void append_base(std::string& output, const StrokeSettings& settings) {
    output.append("BASE");
    append_field(output, std::to_string(settings.reconstruction_version));
    append_field(output, std::to_string(static_cast<unsigned>(settings.tip_mode)));
    append_double(output, settings.spacing_fraction);
    append_double(output, settings.radius);
    append_double(output, settings.opacity);
    append_double(output, settings.hardness);
    append_double(output, settings.rotation_radians);
    append_double(output, settings.elongation);
    append_double(output, settings.flow);
    append_field(output, encode_bytes(settings.tip_resource_identity));
    output.push_back('\n');
}

void append_settings(std::string& output, const StrokeSettings& settings) {
    append_base(output, settings);
    output.append("STABILIZER");
    append_double(output, settings.stabilizer.radius);
    append_double(output, settings.stabilizer.time_constant_seconds);
    output.push_back('\n');
    append_mapping(output, "pressure_radius", settings.input_mapping.pressure_radius);
    append_mapping(output, "pressure_opacity", settings.input_mapping.pressure_opacity);
    append_mapping(output, "pressure_hardness", settings.input_mapping.pressure_hardness);
    append_mapping(output, "pressure_flow", settings.input_mapping.pressure_flow);
    append_mapping(output, "pressure_rotation", settings.input_mapping.pressure_rotation);
    append_mapping(output, "tilt_rotation", settings.input_mapping.tilt_rotation);
    append_mapping(output, "tilt_elongation", settings.input_mapping.tilt_elongation);
    output.append("JITTER");
    append_field(output, std::to_string(settings.jitter.seed));
    append_double(output, settings.jitter.position_fraction);
    append_double(output, settings.jitter.radius_fraction);
    append_double(output, settings.jitter.rotation_radians);
    append_double(output, settings.jitter.opacity);
    append_double(output, settings.jitter.flow);
    output.push_back('\n');
    output.append("TAPER");
    append_field(output, std::to_string(static_cast<unsigned>(settings.taper.entry.unit)));
    append_double(output, settings.taper.entry.extent);
    append_field(output, std::to_string(static_cast<unsigned>(settings.taper.exit.unit)));
    append_double(output, settings.taper.exit.extent);
    append_double(output, settings.taper.floor);
    append_bool(output, settings.taper.affect_radius);
    append_bool(output, settings.taper.affect_opacity);
    output.push_back('\n');
    output.append("CONSTRAINT");
    append_field(output, std::to_string(static_cast<unsigned>(settings.constraint.mode)));
    append_double(output, settings.constraint.grid_step);
    output.push_back('\n');
    output.append("SYMMETRY");
    append_bool(output, settings.symmetry.mirror_x);
    append_bool(output, settings.symmetry.mirror_y);
    append_bool(output, settings.symmetry.mirror_z);
    append_field(output, std::to_string(settings.symmetry.radial_count));
    append_field(output, std::to_string(static_cast<unsigned>(settings.symmetry.radial_axis)));
    output.push_back('\n');
}

std::vector<std::string_view> fields_for(const std::vector<std::string_view>& lines,
                                         std::size_t index, std::string_view record) {
    std::vector<std::string_view> fields = split(lines.at(index), '\t');
    if (fields.empty() || fields.front() != record) {
        throw StrokePresetError("stroke preset expected " + std::string(record) + " record");
    }
    return fields;
}

TipMode parse_tip_mode(std::string_view field) {
    const auto value = parse_decimal<std::uint8_t>(field, "tip mode");
    if (value > static_cast<std::uint8_t>(TipMode::discrete_alpha)) {
        throw StrokePresetError("stroke preset has an unknown tip mode");
    }
    return static_cast<TipMode>(value);
}

TaperUnit parse_taper_unit(std::string_view field) {
    const auto value = parse_decimal<std::uint8_t>(field, "taper unit");
    if (value > static_cast<std::uint8_t>(TaperUnit::distance)) {
        throw StrokePresetError("stroke preset has an unknown taper unit");
    }
    return static_cast<TaperUnit>(value);
}

ConstraintMode parse_constraint_mode(std::string_view field) {
    const auto value = parse_decimal<std::uint8_t>(field, "constraint mode");
    if (value > static_cast<std::uint8_t>(ConstraintMode::grid)) {
        throw StrokePresetError("stroke preset has an unknown constraint mode");
    }
    return static_cast<ConstraintMode>(value);
}

SymmetryAxis parse_symmetry_axis(std::string_view field) {
    const auto value = parse_decimal<std::uint8_t>(field, "symmetry axis");
    if (value > static_cast<std::uint8_t>(SymmetryAxis::z)) {
        throw StrokePresetError("stroke preset has an unknown symmetry axis");
    }
    return static_cast<SymmetryAxis>(value);
}

void parse_base(const std::vector<std::string_view>& fields, StrokeSettings& settings) {
    if (fields.size() != 11) {
        throw StrokePresetError("stroke preset has a malformed BASE record");
    }
    settings.reconstruction_version =
        parse_decimal<std::uint32_t>(fields[1], "reconstruction version");
    settings.tip_mode = parse_tip_mode(fields[2]);
    settings.spacing_fraction = parse_double(fields[3]);
    settings.radius = parse_double(fields[4]);
    settings.opacity = parse_double(fields[5]);
    settings.hardness = parse_double(fields[6]);
    settings.rotation_radians = parse_double(fields[7]);
    settings.elongation = parse_double(fields[8]);
    settings.flow = parse_double(fields[9]);
    settings.tip_resource_identity = decode_bytes(fields[10]);
}

ResponseMapping parse_mapping(const std::vector<std::string_view>& fields,
                              std::string_view expected_identifier) {
    if (fields.size() < 9 || (fields.size() - 5) % 2 != 0 || fields[1] != expected_identifier) {
        throw StrokePresetError("stroke preset has a malformed " +
                                std::string(expected_identifier) + " mapping");
    }
    ResponseMapping result{
        .enabled = parse_bool(fields[2], "mapping enable flag"),
        .curve = {.points = {}},
        .minimum_output = parse_double(fields[3]),
        .maximum_output = parse_double(fields[4]),
    };
    result.curve.points.reserve((fields.size() - 5) / 2);
    for (std::size_t index = 5; index < fields.size(); index += 2) {
        result.curve.points.push_back(
            {parse_double(fields[index]), parse_double(fields[index + 1])});
    }
    return result;
}

void parse_stabilizer(const std::vector<std::string_view>& fields, StrokeSettings& settings) {
    if (fields.size() != 3) {
        throw StrokePresetError("stroke preset has a malformed STABILIZER record");
    }
    settings.stabilizer = {parse_double(fields[1]), parse_double(fields[2])};
}

void parse_jitter(const std::vector<std::string_view>& fields, std::uint32_t schema_version,
                  StrokeSettings& settings) {
    const std::size_t expected_size = schema_version == 1 ? 6 : 7;
    if (fields.size() != expected_size) {
        throw StrokePresetError("stroke preset has a malformed JITTER record for schema version " +
                                std::to_string(schema_version));
    }
    settings.jitter.seed = parse_decimal<std::uint64_t>(fields[1], "jitter seed");
    settings.jitter.position_fraction = parse_double(fields[2]);
    settings.jitter.radius_fraction = parse_double(fields[3]);
    settings.jitter.rotation_radians = parse_double(fields[4]);
    settings.jitter.opacity = parse_double(fields[5]);
    if (schema_version >= 2) {
        settings.jitter.flow = parse_double(fields[6]);
    }
}

void parse_taper(const std::vector<std::string_view>& fields, StrokeSettings& settings) {
    if (fields.size() != 8) {
        throw StrokePresetError("stroke preset has a malformed TAPER record");
    }
    settings.taper.entry = {parse_taper_unit(fields[1]), parse_double(fields[2])};
    settings.taper.exit = {parse_taper_unit(fields[3]), parse_double(fields[4])};
    settings.taper.floor = parse_double(fields[5]);
    settings.taper.affect_radius = parse_bool(fields[6], "taper radius flag");
    settings.taper.affect_opacity = parse_bool(fields[7], "taper opacity flag");
}

void parse_constraint(const std::vector<std::string_view>& fields, StrokeSettings& settings) {
    if (fields.size() != 3) {
        throw StrokePresetError("stroke preset has a malformed CONSTRAINT record");
    }
    settings.constraint = {parse_constraint_mode(fields[1]), parse_double(fields[2])};
}

void parse_symmetry(const std::vector<std::string_view>& fields, StrokeSettings& settings) {
    if (fields.size() != 6) {
        throw StrokePresetError("stroke preset has a malformed SYMMETRY record");
    }
    settings.symmetry = {
        .mirror_x = parse_bool(fields[1], "mirror X flag"),
        .mirror_y = parse_bool(fields[2], "mirror Y flag"),
        .mirror_z = parse_bool(fields[3], "mirror Z flag"),
        .radial_count = parse_decimal<std::uint32_t>(fields[4], "radial count"),
        .radial_axis = parse_symmetry_axis(fields[5]),
    };
}

void validate_preset(const StrokePreset& preset) {
    if (preset.name.empty()) {
        throw StrokePresetError("stroke preset requires a name");
    }
    try {
        const StrokeResolver resolver(preset.settings);
        if (!resolver.parameter_report().clamps.empty()) {
            throw StrokePresetError(
                "stroke preset settings require normalization before serialization");
        }
    } catch (const StrokeResolutionError& error) {
        throw StrokePresetError("stroke preset settings are invalid: " + std::string(error.what()));
    }
}

std::uint32_t parse_header(std::string_view line) {
    const std::vector<std::string_view> fields = split(line, '\t');
    if (fields.size() != 2 || fields[0] != "CTEX_STROKE_PRESET") {
        throw StrokePresetError("stroke preset has a malformed header");
    }
    const std::uint32_t version = parse_decimal<std::uint32_t>(fields[1], "schema version");
    if (version == 0 || version > current_stroke_preset_schema_version) {
        throw StrokePresetError("unsupported stroke preset schema version " +
                                std::to_string(version));
    }
    return version;
}

}  // namespace

std::string serialize_stroke_preset(const StrokePreset& preset) {
    if (preset.schema_version != current_stroke_preset_schema_version) {
        throw StrokePresetError("cannot serialize stroke preset schema version " +
                                std::to_string(preset.schema_version));
    }
    validate_preset(preset);
    std::string output = "CTEX_STROKE_PRESET\t" + std::to_string(preset.schema_version) + "\nNAME";
    append_field(output, encode_bytes(preset.name));
    output.push_back('\n');
    append_settings(output, preset.settings);
    output.append("END\n");
    return output;
}

StrokePreset deserialize_stroke_preset(std::string_view serialized) {
    const std::vector<std::string_view> lines = preset_lines(serialized);
    if (lines.empty()) {
        throw StrokePresetError("stroke preset is empty");
    }
    const std::uint32_t source_version = parse_header(lines[0]);
    if (lines.size() != 16 || lines.back() != "END") {
        throw StrokePresetError("stroke preset has a malformed record envelope");
    }
    const auto name_fields = fields_for(lines, 1, "NAME");
    if (name_fields.size() != 2) {
        throw StrokePresetError("stroke preset has a malformed NAME record");
    }
    StrokePreset result{
        .schema_version = current_stroke_preset_schema_version,
        .name = decode_bytes(name_fields[1]),
        .settings = {},
    };
    parse_base(fields_for(lines, 2, "BASE"), result.settings);
    parse_stabilizer(fields_for(lines, 3, "STABILIZER"), result.settings);
    result.settings.input_mapping.pressure_radius =
        parse_mapping(fields_for(lines, 4, "MAP"), "pressure_radius");
    result.settings.input_mapping.pressure_opacity =
        parse_mapping(fields_for(lines, 5, "MAP"), "pressure_opacity");
    result.settings.input_mapping.pressure_hardness =
        parse_mapping(fields_for(lines, 6, "MAP"), "pressure_hardness");
    result.settings.input_mapping.pressure_flow =
        parse_mapping(fields_for(lines, 7, "MAP"), "pressure_flow");
    result.settings.input_mapping.pressure_rotation =
        parse_mapping(fields_for(lines, 8, "MAP"), "pressure_rotation");
    result.settings.input_mapping.tilt_rotation =
        parse_mapping(fields_for(lines, 9, "MAP"), "tilt_rotation");
    result.settings.input_mapping.tilt_elongation =
        parse_mapping(fields_for(lines, 10, "MAP"), "tilt_elongation");
    parse_jitter(fields_for(lines, 11, "JITTER"), source_version, result.settings);
    parse_taper(fields_for(lines, 12, "TAPER"), result.settings);
    parse_constraint(fields_for(lines, 13, "CONSTRAINT"), result.settings);
    parse_symmetry(fields_for(lines, 14, "SYMMETRY"), result.settings);
    validate_preset(result);
    return result;
}

}  // namespace ctex::paint
