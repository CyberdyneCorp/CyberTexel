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

int main(void) {
    return defaults_and_caller_owned_buffers() &&
                   mappings_taper_jitter_and_symmetry_are_reachable() &&
                   constraints_stabilizer_and_validation_are_reachable()
               ? 0
               : 1;
}
