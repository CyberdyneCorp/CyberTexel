#include <ctex/capi.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int expect(int condition) { return condition ? 1 : 0; }

static int near(double left, double right) { return fabs(left - right) < 1.0e-9; }

static ctex_stroke_input_sample sample(double x, double y, uint64_t timestamp) {
    const ctex_stroke_input_sample value = {
        .size = CTEX_STROKE_INPUT_SAMPLE_CURRENT_SIZE,
        .position = {x, y, 0.0},
        .frame = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}},
        .timestamp_nanoseconds = timestamp,
        .has_pressure = 0,
        .pressure = 0.0,
        .tilt = {0.0, 0.0},
    };
    return value;
}

static int defaults_and_caller_owned_buffers(void) {
    ctex_stroke_settings_descriptor settings;
    ctex_stroke_input_sample samples[2] = {sample(0.0, 0.0, 0), sample(2.0, 0.0, 2000000)};
    ctex_resolved_stroke_info info = {.size = CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE};
    ctex_resolved_stamp stamps[64];
    ctex_swept_segment segments[64];
    size_t stamp_count = 0;
    size_t segment_count = 0;

    if (!expect(ctex_stroke_settings_init(&settings) == CTEX_RESULT_SUCCESS) ||
        !expect(settings.reconstruction_version == 1 &&
                settings.tip_mode == CTEX_STROKE_TIP_CONTINUOUS_SWEEP &&
                near(settings.spacing_fraction, 0.1) && near(settings.radius, 1.0) &&
                settings.pressure_radius.enabled == 1 && settings.pressure_radius.points == NULL &&
                settings.pressure_radius.point_count == 0 &&
                strcmp(settings.tip_resource_identity, "builtin.circle") == 0) ||
        !expect(ctex_stroke_resolve(&settings, samples, 2, &info, NULL, 0, &stamp_count, NULL, 0,
                                    &segment_count) == CTEX_RESULT_SUCCESS) ||
        !expect(info.reconstruction_version == 1 && info.stamp_count == stamp_count &&
                info.swept_segment_count == segment_count && stamp_count > 2 &&
                segment_count == stamp_count - 1)) {
        return 0;
    }

    stamps[0].radius = 123.0;
    if (!expect(ctex_stroke_resolve(&settings, samples, 2, &info, stamps, stamp_count - 1,
                                    &stamp_count, segments, segment_count,
                                    &segment_count) == CTEX_RESULT_BUFFER_TOO_SMALL) ||
        !expect(stamps[0].radius == 123.0 &&
                ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL)) {
        return 0;
    }

    return expect(ctex_stroke_resolve(&settings, samples, 2, &info, stamps, 64, &stamp_count,
                                      segments, 64, &segment_count) == CTEX_RESULT_SUCCESS) &&
           expect(stamps[0].ordinal == 0 && stamps[stamp_count - 1].ordinal == stamp_count - 1 &&
                  near(stamps[0].position.x, 0.0) &&
                  near(stamps[stamp_count - 1].position.x, 2.0) &&
                  strcmp(stamps[0].tip_resource_identity, "builtin.circle") == 0 &&
                  segments[0].start_stamp_ordinal == 0 && segments[0].end_stamp_ordinal == 1);
}

static int mappings_taper_jitter_and_symmetry_are_reachable(void) {
    static const ctex_response_curve_point reverse_curve[2] = {{0.0, 1.0}, {1.0, 0.0}};
    ctex_stroke_settings_descriptor settings;
    ctex_stroke_input_sample input = sample(1.0, 2.0, 0);
    ctex_resolved_stroke_info info = {.size = CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE};
    ctex_resolved_stamp first[8];
    ctex_resolved_stamp second[8];
    size_t stamp_count = 0;
    size_t segment_count = 0;
    size_t repeat_stamp_count = 0;
    size_t repeat_segment_count = 0;
    size_t index = 0;

    if (!expect(ctex_stroke_settings_init(&settings) == CTEX_RESULT_SUCCESS)) {
        return 0;
    }
    settings.tip_mode = CTEX_STROKE_TIP_DISCRETE_ALPHA;
    settings.tip_resource_identity = "test.square";
    settings.radius = 2.0;
    settings.opacity = 0.8;
    settings.pressure_radius.points = reverse_curve;
    settings.pressure_radius.point_count = 2;
    settings.pressure_radius.minimum_output = 0.5;
    settings.pressure_radius.maximum_output = 1.0;
    settings.pressure_opacity.enabled = 1;
    settings.pressure_opacity.minimum_output = 0.4;
    settings.pressure_opacity.maximum_output = 1.0;
    settings.tilt_elongation.enabled = 1;
    settings.tilt_elongation.minimum_output = 1.0;
    settings.tilt_elongation.maximum_output = 3.0;
    settings.taper.entry.unit = CTEX_STROKE_TAPER_STAMP_COUNT;
    settings.taper.entry.extent = 2.0;
    settings.taper.floor = 0.25;
    settings.jitter.seed = 1234;
    settings.jitter.position_fraction = 0.2;
    settings.symmetry.mirror_x = 1;
    settings.symmetry.mirror_y = 1;
    settings.symmetry.mirror_z = 1;
    input.has_pressure = 1;
    input.pressure = 0.25;
    input.tilt.x = 0.5;

    if (!expect(ctex_stroke_resolve(&settings, &input, 1, &info, first, 8, &stamp_count, NULL, 0,
                                    &segment_count) == CTEX_RESULT_SUCCESS) ||
        !expect(stamp_count == 8 && segment_count == 0 && info.symmetry_instance_count == 8 &&
                info.tip_mode == CTEX_STROKE_TIP_DISCRETE_ALPHA) ||
        !expect(near(first[0].radius, 0.4375) && near(first[0].opacity, 0.11) &&
                near(first[0].elongation, 2.0) &&
                strcmp(first[0].tip_resource_identity, "test.square") == 0) ||
        !expect(ctex_stroke_resolve(&settings, &input, 1, &info, second, 8, &repeat_stamp_count,
                                    NULL, 0, &repeat_segment_count) == CTEX_RESULT_SUCCESS)) {
        return 0;
    }
    for (index = 0; index < stamp_count; ++index) {
        if (!expect(first[index].ordinal == index && first[index].symmetry_instance == index &&
                    near(first[index].position.x, second[index].position.x) &&
                    near(first[index].position.y, second[index].position.y) &&
                    near(first[index].radius, second[index].radius))) {
            return 0;
        }
    }
    return 1;
}

static int constraints_stabilizer_and_validation_are_reachable(void) {
    ctex_stroke_settings_descriptor settings;
    ctex_stroke_input_sample samples[2] = {sample(0.2, 0.4, 0), sample(2.2, 1.6, 2000000)};
    ctex_resolved_stroke_info info = {.size = CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE};
    ctex_resolved_stamp stamps[8];
    ctex_swept_segment segments[8];
    size_t stamp_count = 0;
    size_t segment_count = 0;

    if (!expect(ctex_stroke_settings_init(&settings) == CTEX_RESULT_SUCCESS)) {
        return 0;
    }
    settings.spacing_fraction = 4.0;
    settings.pressure_radius.enabled = 0;
    settings.constraint.mode = CTEX_STROKE_CONSTRAINT_GRID;
    settings.constraint.grid_step = 1.0;
    settings.stabilizer.radius = 0.25;
    settings.stabilizer.time_constant_seconds = 0.01;
    if (!expect(ctex_stroke_resolve(&settings, samples, 2, &info, stamps, 8, &stamp_count, segments,
                                    8, &segment_count) == CTEX_RESULT_SUCCESS) ||
        !expect(stamp_count >= 1 && near(stamps[0].position.x, 0.0) &&
                near(stamps[0].position.y, 0.0))) {
        return 0;
    }

    samples[1].timestamp_nanoseconds = 0;
    return expect(ctex_stroke_resolve(&settings, samples, 2, &info, stamps, 8, &stamp_count,
                                      segments, 8,
                                      &segment_count) == CTEX_RESULT_INVALID_ARGUMENT) &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_STROKE &&
                  strstr(ctex_get_last_diagnostic(), "timestamps") != NULL);
}

enum { PRESET_CAPACITY = 4096, PRESET_CURVE_CAPACITY = 32 };

static int configure_preset_settings(ctex_stroke_settings_descriptor* settings) {
    static const ctex_response_curve_point curve[3] = {{0.0, 0.1}, {0.4, 0.25}, {1.0, 0.9}};
    if (!expect(ctex_stroke_settings_init(settings) == CTEX_RESULT_SUCCESS)) {
        return 0;
    }
    settings->tip_mode = CTEX_STROKE_TIP_DISCRETE_ALPHA;
    settings->tip_resource_identity = "brushes/chalk\ttip";
    settings->radius = 3.5;
    settings->flow = 0.45;
    settings->pressure_radius.points = curve;
    settings->pressure_radius.point_count = 3;
    settings->pressure_radius.minimum_output = 0.2;
    settings->pressure_radius.maximum_output = 1.5;
    settings->jitter.seed = UINT64_C(0xfedcba9876543210);
    settings->jitter.flow = 0.3;
    settings->symmetry.mirror_x = 1;
    settings->symmetry.radial_count = 3;
    settings->symmetry.radial_axis = CTEX_STROKE_SYMMETRY_AXIS_Y;
    return 1;
}

static int serialize_preset(const ctex_stroke_settings_descriptor* settings, char* serialized,
                            size_t capacity, size_t* required_size) {
    return expect(ctex_stroke_preset_serialize("Chalk soft", settings, NULL, 0, required_size) ==
                  CTEX_RESULT_SUCCESS) &&
           expect(*required_size > 0 && *required_size < capacity) &&
           expect(ctex_stroke_preset_serialize("Chalk soft", settings, serialized,
                                               *required_size - 1,
                                               required_size) == CTEX_RESULT_BUFFER_TOO_SMALL) &&
           expect(ctex_stroke_preset_serialize("Chalk soft", settings, serialized, capacity,
                                               required_size) == CTEX_RESULT_SUCCESS);
}

static ctex_stroke_preset_buffers_descriptor preset_buffers(char* name, size_t name_size, char* tip,
                                                            size_t tip_size,
                                                            ctex_response_curve_point* points) {
    const ctex_stroke_preset_buffers_descriptor buffers = {
        .size = CTEX_STROKE_PRESET_BUFFERS_DESCRIPTOR_CURRENT_SIZE,
        .name_buffer = name,
        .name_buffer_size = name_size,
        .tip_resource_identity_buffer = tip,
        .tip_resource_identity_buffer_size = tip_size,
        .curve_points = points,
        .curve_point_capacity = PRESET_CURVE_CAPACITY,
    };
    return buffers;
}

static int current_preset_round_trip(void) {
    ctex_stroke_settings_descriptor settings;
    ctex_stroke_settings_descriptor restored = {
        .size = CTEX_STROKE_SETTINGS_DESCRIPTOR_CURRENT_SIZE, .radius = 123.0};
    ctex_stroke_preset_info info = {.size = CTEX_STROKE_PRESET_INFO_CURRENT_SIZE};
    ctex_response_curve_point points[PRESET_CURVE_CAPACITY];
    char name[64];
    char tip[64];
    char serialized[PRESET_CAPACITY] = {0};
    char repeated[PRESET_CAPACITY] = {0};
    size_t required_size = 0;
    size_t repeated_size = 0;
    ctex_stroke_preset_buffers_descriptor buffers =
        preset_buffers(name, sizeof(name), tip, sizeof(tip), points);

    if (!configure_preset_settings(&settings) ||
        !serialize_preset(&settings, serialized, sizeof(serialized), &required_size) ||
        !expect(ctex_stroke_preset_deserialize(serialized, required_size, &info, NULL, NULL) ==
                CTEX_RESULT_SUCCESS) ||
        !expect(info.schema_version == 2 && info.required_curve_point_count == 15 &&
                info.required_name_size == strlen("Chalk soft") + 1 &&
                info.required_tip_resource_identity_size == strlen("brushes/chalk\ttip") + 1)) {
        return 0;
    }

    buffers.name_buffer_size = 1;
    if (!expect(ctex_stroke_preset_deserialize(serialized, required_size, &info, &restored,
                                               &buffers) == CTEX_RESULT_BUFFER_TOO_SMALL) ||
        !expect(restored.radius == 123.0)) {
        return 0;
    }
    buffers.name_buffer_size = sizeof(name);
    if (!expect(ctex_stroke_preset_deserialize(serialized, required_size, &info, &restored,
                                               &buffers) == CTEX_RESULT_SUCCESS) ||
        !expect(strcmp(name, "Chalk soft") == 0 && strcmp(tip, "brushes/chalk\ttip") == 0 &&
                restored.tip_resource_identity == tip &&
                restored.pressure_radius.points == points &&
                restored.pressure_radius.point_count == 3 && near(restored.radius, 3.5) &&
                near(restored.jitter.flow, 0.3) && restored.symmetry.radial_count == 3) ||
        !expect(ctex_stroke_preset_serialize(name, &restored, repeated, sizeof(repeated),
                                             &repeated_size) == CTEX_RESULT_SUCCESS) ||
        !expect(repeated_size == required_size &&
                memcmp(repeated, serialized, required_size) == 0)) {
        return 0;
    }
    return 1;
}

static size_t remove_schema_one_flow_field(char* serialized, size_t serialized_size) {
    char* jitter = strstr(serialized, "\nJITTER\t");
    char* line_end = jitter == NULL ? NULL : strchr(jitter + 1, '\n');
    char* last_field = line_end;
    while (last_field != NULL && last_field > jitter && *last_field != '\t') {
        --last_field;
    }
    if (jitter == NULL || line_end == NULL || last_field == NULL || *last_field != '\t') {
        return 0;
    }
    memmove(last_field, line_end, serialized_size - (size_t)(line_end - serialized));
    return serialized_size - (size_t)(line_end - last_field);
}

static int older_preset_migrates(void) {
    ctex_stroke_settings_descriptor settings;
    ctex_stroke_settings_descriptor restored = {.size =
                                                    CTEX_STROKE_SETTINGS_DESCRIPTOR_CURRENT_SIZE};
    ctex_stroke_preset_info info = {.size = CTEX_STROKE_PRESET_INFO_CURRENT_SIZE};
    ctex_response_curve_point points[PRESET_CURVE_CAPACITY];
    char name[64];
    char tip[64];
    char migrated[PRESET_CAPACITY] = {0};
    size_t migrated_size = 0;
    ctex_stroke_preset_buffers_descriptor buffers =
        preset_buffers(name, sizeof(name), tip, sizeof(tip), points);

    if (!configure_preset_settings(&settings) ||
        !serialize_preset(&settings, migrated, sizeof(migrated), &migrated_size)) {
        return 0;
    }
    migrated[strlen("CTEX_STROKE_PRESET\t")] = '1';
    migrated_size = remove_schema_one_flow_field(migrated, migrated_size);
    if (!expect(migrated_size != 0)) {
        return 0;
    }
    return expect(ctex_stroke_preset_deserialize(migrated, migrated_size, &info, NULL, NULL) ==
                  CTEX_RESULT_SUCCESS) &&
           expect(ctex_stroke_preset_deserialize(migrated, migrated_size, &info, &restored,
                                                 &buffers) == CTEX_RESULT_SUCCESS) &&
           expect(info.schema_version == 2 && near(restored.jitter.flow, 0.0));
}

static int newer_preset_is_refused(void) {
    ctex_stroke_settings_descriptor settings;
    ctex_stroke_preset_info info = {.size = CTEX_STROKE_PRESET_INFO_CURRENT_SIZE};
    char serialized[PRESET_CAPACITY] = {0};
    size_t serialized_size = 0;
    if (!configure_preset_settings(&settings) ||
        !serialize_preset(&settings, serialized, sizeof(serialized), &serialized_size)) {
        return 0;
    }
    serialized[strlen("CTEX_STROKE_PRESET\t")] = '9';
    return expect(ctex_stroke_preset_deserialize(serialized, serialized_size, &info, NULL, NULL) ==
                  CTEX_RESULT_INVALID_ARGUMENT) &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_STROKE_PRESET &&
                  strstr(ctex_get_last_diagnostic(), "9") != NULL);
}

int main(void) {
    return defaults_and_caller_owned_buffers() &&
                   mappings_taper_jitter_and_symmetry_are_reachable() &&
                   constraints_stabilizer_and_validation_are_reachable() &&
                   current_preset_round_trip() && older_preset_migrates() &&
                   newer_preset_is_refused()
               ? 0
               : 1;
}
