#include <ctex/paint/stroke_preset.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace {

using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

StrokePreset rich_preset() {
    StrokePreset preset;
    preset.name = "Chalk\tsoft\nUTF-8: \xc3\xa7";
    StrokeSettings& settings = preset.settings;
    settings.tip_mode = TipMode::discrete_alpha;
    settings.spacing_fraction = 0.75;
    settings.radius = 3.5;
    settings.opacity = 0.8;
    settings.hardness = 0.3;
    settings.rotation_radians = -0.0;
    settings.elongation = 1.75;
    settings.flow = 0.45;
    settings.tip_resource_identity = "brushes/chalk\ttip";
    settings.stabilizer = {.radius = 2.0, .time_constant_seconds = 0.125};
    const ResponseCurve curve{{{0.0, 0.1}, {0.4, 0.25}, {1.0, 0.9}}};
    settings.input_mapping.pressure_radius = {true, curve, 0.2, 1.5};
    settings.input_mapping.pressure_opacity = {true, curve, 0.1, 0.9};
    settings.input_mapping.pressure_hardness = {true, curve, 0.2, 0.8};
    settings.input_mapping.pressure_flow = {true, curve, 0.3, 0.7};
    settings.input_mapping.pressure_rotation = {true, curve, -1.0, 2.0};
    settings.input_mapping.tilt_rotation = {true, curve, 0.0, 0.75};
    settings.input_mapping.tilt_elongation = {true, curve, 1.0, 2.5};
    settings.jitter = {
        .seed = 0xfedcba9876543210ULL,
        .position_fraction = 0.15,
        .radius_fraction = 0.2,
        .rotation_radians = 0.35,
        .opacity = 0.25,
        .flow = 0.3,
    };
    settings.taper = {
        .entry = {.unit = TaperUnit::stamp_count, .extent = 10.0},
        .exit = {.unit = TaperUnit::distance, .extent = 5.0},
        .floor = 0.1,
        .affect_radius = true,
        .affect_opacity = false,
    };
    settings.constraint = {.mode = ConstraintMode::grid, .grid_step = 2.25};
    settings.symmetry = {
        .mirror_x = true,
        .mirror_y = false,
        .mirror_z = true,
        .radial_count = 3,
        .radial_axis = SymmetryAxis::y,
    };
    return preset;
}

std::string schema_one_fixture() {
    StrokePreset preset = rich_preset();
    std::string serialized = serialize_stroke_preset(preset);
    serialized.replace(0, std::string_view("CTEX_STROKE_PRESET\t2").size(),
                       "CTEX_STROKE_PRESET\t1");
    const std::size_t jitter = serialized.find("\nJITTER\t") + 1;
    const std::size_t line_end = serialized.find('\n', jitter);
    const std::size_t last_field = serialized.rfind('\t', line_end);
    serialized.erase(last_field, line_end - last_field);
    return serialized;
}

bool canonical_round_trip_preserves_every_setting() {
    const StrokePreset source = rich_preset();
    const std::string serialized = serialize_stroke_preset(source);
    const StrokePreset restored = deserialize_stroke_preset(serialized);
    return expect(restored == source, "stroke preset changed during a complete round trip") &&
           expect(serialize_stroke_preset(restored) == serialized,
                  "stroke preset serialization was not byte-canonical") &&
           expect(serialized.find(source.name) == std::string::npos,
                  "stroke preset name was not encoded safely");
}

bool older_schema_uses_documented_new_field_defaults() {
    const StrokePreset restored = deserialize_stroke_preset(schema_one_fixture());
    const StrokePreset current = rich_preset();
    return expect(restored.schema_version == current_stroke_preset_schema_version,
                  "older stroke preset was not migrated to the current schema") &&
           expect(restored.settings.jitter.flow == JitterSettings{}.flow,
                  "schema one did not default its absent flow jitter") &&
           expect(restored.settings.jitter.seed == current.settings.jitter.seed &&
                      restored.settings.jitter.opacity == current.settings.jitter.opacity &&
                      restored.settings.symmetry == current.settings.symmetry &&
                      restored.name == current.name,
                  "schema one migration changed fields that were present");
}

bool newer_schema_is_named_and_transactionally_refused() {
    StrokePreset destination = rich_preset();
    const StrokePreset before = destination;
    std::string future = serialize_stroke_preset(destination);
    future.replace(0, std::string_view("CTEX_STROKE_PRESET\t2").size(), "CTEX_STROKE_PRESET\t99");
    future.insert(future.rfind("END\n"), "FUTURE_FIELD\tcontents\n");
    bool named_version = false;
    try {
        destination = deserialize_stroke_preset(future);
    } catch (const StrokePresetError& error) {
        named_version = std::string_view(error.what()).find("99") != std::string_view::npos;
    }
    return expect(named_version, "newer stroke preset refusal did not name its schema version") &&
           expect(destination == before, "newer stroke preset was partially applied");
}

bool malformed_and_invalid_presets_are_refused() {
    const auto refused = [](std::string_view serialized) {
        try {
            static_cast<void>(deserialize_stroke_preset(serialized));
        } catch (const StrokePresetError&) {
            return true;
        }
        return false;
    };
    const auto write_refused = [](const StrokePreset& preset) {
        try {
            static_cast<void>(serialize_stroke_preset(preset));
        } catch (const StrokePresetError&) {
            return true;
        }
        return false;
    };
    std::string wrong_mapping = serialize_stroke_preset(rich_preset());
    wrong_mapping.replace(wrong_mapping.find("pressure_radius"),
                          std::string_view("pressure_radius").size(), "wrong_mapping__");
    std::string missing_end = serialize_stroke_preset(rich_preset());
    missing_end.erase(missing_end.rfind("END\n"));

    StrokePreset unnamed = rich_preset();
    unnamed.name.clear();
    StrokePreset old_for_write = rich_preset();
    old_for_write.schema_version = 1;
    StrokePreset invalid_settings = rich_preset();
    invalid_settings.settings.spacing_fraction = 0.0;
    return expect(refused(wrong_mapping) && refused(missing_end),
                  "malformed stroke preset records were accepted") &&
           expect(write_refused(unnamed) && write_refused(old_for_write) &&
                      write_refused(invalid_settings),
                  "invalid or non-current stroke preset was serialized");
}

}  // namespace

int main() {
    return canonical_round_trip_preserves_every_setting() &&
                   older_schema_uses_documented_new_field_defaults() &&
                   newer_schema_is_named_and_transactionally_refused() &&
                   malformed_and_invalid_presets_are_refused()
               ? 0
               : 1;
}
