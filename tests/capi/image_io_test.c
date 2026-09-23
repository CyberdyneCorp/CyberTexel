#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const unsigned char gray8_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
    0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00,
    0x00, 0xd1, 0x49, 0x20, 0x56, 0x00, 0x00, 0x00, 0x0b, 0x49, 0x44, 0x41, 0x54, 0x78,
    0x9c, 0x63, 0x10, 0x7c, 0x06, 0x00, 0x01, 0x0b, 0x00, 0xf8, 0x10, 0x14, 0xc2, 0x31,
    0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

static const unsigned char gray16_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
    0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x81, 0xd9, 0xfc, 0x15, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41, 0x54, 0x78,
    0x9c, 0x63, 0x10, 0x32, 0x59, 0x7d, 0x16, 0x00, 0x03, 0x0c, 0x01, 0xbf, 0x6e, 0xb9,
    0xc6, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

static int expect(int condition) { return condition ? 1 : 0; }

static uint32_t little_u32(const unsigned char* bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8u) | ((uint32_t)bytes[2] << 16u) |
           ((uint32_t)bytes[3] << 24u);
}

static void put_u16(unsigned char* bytes, size_t* offset, uint16_t value) {
    bytes[(*offset)++] = (unsigned char)(value >> 8u);
    bytes[(*offset)++] = (unsigned char)value;
}

static void put_i16(unsigned char* bytes, size_t* offset, int16_t value) {
    put_u16(bytes, offset, (uint16_t)value);
}

static void put_u32(unsigned char* bytes, size_t* offset, uint32_t value) {
    bytes[(*offset)++] = (unsigned char)(value >> 24u);
    bytes[(*offset)++] = (unsigned char)(value >> 16u);
    bytes[(*offset)++] = (unsigned char)(value >> 8u);
    bytes[(*offset)++] = (unsigned char)value;
}

static void put_i32(unsigned char* bytes, size_t* offset, int32_t value) {
    put_u32(bytes, offset, (uint32_t)value);
}

static void put_bytes(unsigned char* bytes, size_t* offset, const char* value, size_t size) {
    memcpy(bytes + *offset, value, size);
    *offset += size;
}

static void put_layer_record(unsigned char* bytes, size_t* offset, const char* name,
                             size_t name_size) {
    static const int16_t channels[] = {0, 1, 2, -1};
    size_t channel = 0;
    const size_t padded_name_size = ((name_size + 4) / 4) * 4;
    put_i32(bytes, offset, 0);
    put_i32(bytes, offset, 0);
    put_i32(bytes, offset, 1);
    put_i32(bytes, offset, 1);
    put_u16(bytes, offset, 4);
    for (channel = 0; channel < 4; ++channel) {
        put_i16(bytes, offset, channels[channel]);
        put_u32(bytes, offset, 3);
    }
    put_bytes(bytes, offset, "8BIMnorm", 8);
    bytes[(*offset)++] = 255;
    bytes[(*offset)++] = 0;
    bytes[(*offset)++] = 0;
    bytes[(*offset)++] = 0;
    put_u32(bytes, offset, (uint32_t)(8 + padded_name_size));
    put_u32(bytes, offset, 0);
    put_u32(bytes, offset, 0);
    bytes[(*offset)++] = (unsigned char)name_size;
    put_bytes(bytes, offset, name, name_size);
    memset(bytes + *offset, 0, padded_name_size - name_size - 1);
    *offset += padded_name_size - name_size - 1;
}

static void put_layer_pixels(unsigned char* bytes, size_t* offset, const unsigned char rgba[4]) {
    size_t channel = 0;
    for (channel = 0; channel < 4; ++channel) {
        put_u16(bytes, offset, 0);
        bytes[(*offset)++] = rgba[channel];
    }
}

static size_t make_layered_psd(unsigned char* bytes) {
    static const unsigned char top[] = {255, 0, 0, 128};
    static const unsigned char bottom[] = {0, 0, 255, 255};
    size_t offset = 0;
    put_bytes(bytes, &offset, "8BPS", 4);
    put_u16(bytes, &offset, 1);
    memset(bytes + offset, 0, 6);
    offset += 6;
    put_u16(bytes, &offset, 4);
    put_u32(bytes, &offset, 1);
    put_u32(bytes, &offset, 1);
    put_u16(bytes, &offset, 8);
    put_u16(bytes, &offset, 3);
    put_u32(bytes, &offset, 0);
    put_u32(bytes, &offset, 0);
    put_u32(bytes, &offset, 174);
    put_u32(bytes, &offset, 170);
    put_i16(bytes, &offset, 2);
    put_layer_record(bytes, &offset, "Top", 3);
    put_layer_record(bytes, &offset, "Bottom", 6);
    put_layer_pixels(bytes, &offset, top);
    put_layer_pixels(bytes, &offset, bottom);
    put_u16(bytes, &offset, 0);
    bytes[offset++] = 128;
    bytes[offset++] = 0;
    bytes[offset++] = 127;
    bytes[offset++] = 255;
    return offset;
}

static int decode_reports_content_and_caller_buffers(void) {
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    size_t required_size = 0;
    unsigned char short_buffer = 0xa5;
    unsigned char pixels[2] = {0};

    return expect(ctex_image_decode_memory(gray8_png, sizeof(gray8_png), "brush.jpg",
                                           CTEX_CHANNEL_SEMANTIC_ROUGHNESS,
                                           CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, NULL, 0,
                                           &required_size) == CTEX_RESULT_SUCCESS) &&
           expect(info.width == 2 && info.height == 1 && info.channel_count == 1 &&
                  info.scalar_representation == CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED &&
                  info.bit_depth == 8 && info.color_space == CTEX_COLOR_SPACE_LINEAR_REC709 &&
                  info.detected_format == CTEX_IMAGE_FILE_FORMAT_PNG &&
                  info.extension_mismatch == 1 &&
                  info.color_space_source == CTEX_COLOR_SPACE_SOURCE_AUTOMATIC_RULE &&
                  info.uninterpretable_profile == 0 && required_size == 2) &&
           expect(ctex_image_decode_memory(
                      gray8_png, sizeof(gray8_png), "brush.jpg", CTEX_CHANNEL_SEMANTIC_ROUGHNESS,
                      CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, &short_buffer, 1,
                      &required_size) == CTEX_RESULT_BUFFER_TOO_SMALL) &&
           expect(short_buffer == 0xa5 &&
                  ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL) &&
           expect(ctex_image_decode_memory(
                      gray8_png, sizeof(gray8_png), "brush.jpg", CTEX_CHANNEL_SEMANTIC_ROUGHNESS,
                      CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, pixels, sizeof(pixels),
                      &required_size) == CTEX_RESULT_SUCCESS) &&
           expect(pixels[0] == 17 && pixels[1] == 230);
}

static int decode_preserves_sixteen_bit_samples(void) {
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    unsigned char pixels[4] = {0};
    uint16_t first = 0;
    uint16_t second = 0;
    size_t required_size = 0;
    const ctex_result result = ctex_image_decode_memory(
        gray16_png, sizeof(gray16_png), "height.png", CTEX_CHANNEL_SEMANTIC_HEIGHT,
        CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, pixels, sizeof(pixels), &required_size);
    memcpy(&first, pixels, sizeof(first));
    memcpy(&second, pixels + sizeof(first), sizeof(second));
    return expect(result == CTEX_RESULT_SUCCESS && info.bit_depth == 16 && required_size == 4 &&
                  first == 0x1234 && second == 0xabcd);
}

static int channel_expansion_preserves_depth_and_reports_rule(void) {
    const uint16_t source[] = {0x1234, 0xabcd};
    const uint16_t expected[] = {0x1234, 0x1234, 0x1234, 0xabcd, 0xabcd, 0xabcd};
    uint16_t output[6] = {0};
    unsigned char short_output = 0xa5;
    const ctex_image_channel_expansion_descriptor descriptor = {
        .size = CTEX_IMAGE_CHANNEL_EXPANSION_DESCRIPTOR_CURRENT_SIZE,
        .width = 2,
        .height = 1,
        .source_channel_count = 1,
        .scalar_representation = CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
        .bit_depth = 16,
        .source_row_stride_bytes = 0,
        .target_channel_count = 3,
    };
    ctex_image_channel_expansion_info info = {
        .size = CTEX_IMAGE_CHANNEL_EXPANSION_INFO_CURRENT_SIZE,
    };
    return expect(ctex_image_expand_channels(source, sizeof(source), &descriptor, &info, NULL, 0) ==
                  CTEX_RESULT_SUCCESS) &&
           expect(info.channel_count == 3 &&
                  info.scalar_representation == CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED &&
                  info.bit_depth == 16 &&
                  info.rule == CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_TO_RGB &&
                  info.required_pixel_buffer_size == sizeof(output)) &&
           expect(ctex_image_expand_channels(source, sizeof(source), &descriptor, &info,
                                             &short_output, sizeof(short_output)) ==
                  CTEX_RESULT_BUFFER_TOO_SMALL) &&
           expect(short_output == 0xa5) &&
           expect(ctex_image_expand_channels(source, sizeof(source), &descriptor, &info, output,
                                             sizeof(output)) == CTEX_RESULT_SUCCESS) &&
           expect(memcmp(output, expected, sizeof(expected)) == 0);
}

static int image_resampling_selects_and_reports_filter(void) {
    const uint16_t source[] = {0, 1000, 2000, 3000};
    uint16_t output = 0;
    ctex_image_resample_descriptor descriptor = {
        .size = CTEX_IMAGE_RESAMPLE_DESCRIPTOR_CURRENT_SIZE,
        .source_width = 2,
        .source_height = 2,
        .channel_count = 1,
        .scalar_representation = CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
        .bit_depth = 16,
        .source_row_stride_bytes = 0,
        .output_width = 1,
        .output_height = 1,
        .filter = CTEX_IMAGE_RESAMPLE_FILTER_DEFAULT,
        .maximum_output_bytes = 0,
    };
    ctex_image_resample_info info = {.size = CTEX_IMAGE_RESAMPLE_INFO_CURRENT_SIZE};
    if (!expect(ctex_image_resample(source, sizeof(source), &descriptor, &info, &output,
                                    sizeof(output)) == CTEX_RESULT_SUCCESS) ||
        !expect(output == 1500 && info.width == 1 && info.height == 1 && info.channel_count == 1 &&
                info.bit_depth == 16 && info.filter == CTEX_IMAGE_RESAMPLE_FILTER_BILINEAR &&
                info.required_pixel_buffer_size == sizeof(output))) {
        return 0;
    }
    descriptor.filter = CTEX_IMAGE_RESAMPLE_FILTER_NEAREST;
    output = 0;
    if (!expect(ctex_image_resample(source, sizeof(source), &descriptor, &info, &output,
                                    sizeof(output)) == CTEX_RESULT_SUCCESS) ||
        !expect(output == 3000 && info.filter == CTEX_IMAGE_RESAMPLE_FILTER_NEAREST)) {
        return 0;
    }
    descriptor.filter = CTEX_IMAGE_RESAMPLE_FILTER_BILINEAR;
    descriptor.maximum_output_bytes = 1;
    return expect(ctex_image_resample(source, sizeof(source), &descriptor, &info, NULL, 0) ==
                  CTEX_RESULT_OVER_BUDGET) &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED);
}

static int sixteen_bit_png_round_trip_is_lossless(void) {
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    ctex_image_encode_descriptor descriptor = {
        CTEX_IMAGE_ENCODE_DESCRIPTOR_CURRENT_SIZE,
        2,
        1,
        1,
        CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
        16,
        0,
        CTEX_COLOR_SPACE_LINEAR_REC709,
        CTEX_IMAGE_FILE_FORMAT_PNG,
        16,
        0,
    };
    uint16_t source[2] = {0};
    uint16_t decoded[2] = {0};
    unsigned char encoded[4096];
    size_t source_size = 0;
    size_t encoded_size = 0;
    size_t decoded_size = 0;
    return expect(ctex_image_decode_memory(gray16_png, sizeof(gray16_png), "height.png",
                                           CTEX_CHANNEL_SEMANTIC_HEIGHT,
                                           CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, source,
                                           sizeof(source), &source_size) == CTEX_RESULT_SUCCESS) &&
           expect(ctex_image_encode_memory(source, source_size, &descriptor, encoded,
                                           sizeof(encoded),
                                           &encoded_size) == CTEX_RESULT_SUCCESS) &&
           expect(ctex_image_decode_memory(
                      encoded, encoded_size, "height.png", CTEX_CHANNEL_SEMANTIC_HEIGHT,
                      CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, decoded, sizeof(decoded),
                      &decoded_size) == CTEX_RESULT_SUCCESS) &&
           expect(decoded_size == sizeof(source) && memcmp(decoded, source, sizeof(source)) == 0);
}

static int decode_honours_color_and_resource_limits(void) {
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    ctex_image_decode_limits_descriptor limits = {
        CTEX_IMAGE_DECODE_LIMITS_DESCRIPTOR_CURRENT_SIZE,
        1,
        1,
        1,
    };
    size_t required_size = 0;
    if (!expect(ctex_image_decode_memory(gray8_png, sizeof(gray8_png), "base.png",
                                         CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                                         CTEX_INPUT_COLOR_SPACE_SRGB_REC709, NULL, &info, NULL, 0,
                                         &required_size) == CTEX_RESULT_SUCCESS) ||
        !expect(info.color_space == CTEX_COLOR_SPACE_SRGB_REC709 &&
                info.color_space_source == CTEX_COLOR_SPACE_SOURCE_CALLER)) {
        return 0;
    }
    return expect(ctex_image_decode_memory(gray8_png, sizeof(gray8_png), "large.png",
                                           CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                                           CTEX_INPUT_COLOR_SPACE_AUTOMATIC, &limits, &info, NULL,
                                           0, &required_size) == CTEX_RESULT_OVER_BUDGET) &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED) &&
           expect(strstr(ctex_get_last_diagnostic(), "2x1") != NULL &&
                  strstr(ctex_get_last_diagnostic(), "1x1") != NULL);
}

static int decode_refuses_unsupported_and_truncated_content(void) {
    static const unsigned char gif[] = {'G', 'I', 'F', '8', '9', 'a'};
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    size_t required_size = 0;
    return expect(ctex_image_decode_memory(gif, sizeof(gif), "animation.gif",
                                           CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                                           CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, NULL, 0,
                                           &required_size) == CTEX_RESULT_UNSUPPORTED_OPERATION) &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_FORMAT &&
                  strstr(ctex_get_last_diagnostic(), "supported") != NULL) &&
           expect(ctex_image_decode_memory(gray8_png, sizeof(gray8_png) / 2, "broken.png",
                                           CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                                           CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, NULL, 0,
                                           &required_size) == CTEX_RESULT_INVALID_ARGUMENT) &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA);
}

typedef struct decode_control_state {
    size_t callback_count;
    size_t peak_working_bytes;
    uint32_t cancel;
    uint32_t cancel_after_inspection;
} decode_control_state;

static uint32_t decode_is_cancelled(void* user_data) {
    return ((decode_control_state*)user_data)->cancel;
}

static void decode_progress(void* user_data, const ctex_image_decode_progress_info* progress) {
    decode_control_state* state = (decode_control_state*)user_data;
    ++state->callback_count;
    if (progress->estimated_peak_working_bytes > state->peak_working_bytes) {
        state->peak_working_bytes = progress->estimated_peak_working_bytes;
    }
    if (state->cancel_after_inspection != 0 &&
        progress->phase == CTEX_IMAGE_DECODE_PHASE_INSPECTION && progress->total_rows != 0) {
        state->cancel = 1;
    }
}

static int bounded_decode_reports_progress_budget_and_atomic_cancellation(void) {
    decode_control_state state = {0};
    ctex_image_decode_control_descriptor control = {
        .size = CTEX_IMAGE_DECODE_CONTROL_DESCRIPTOR_CURRENT_SIZE,
        .maximum_working_bytes = 2u << 20u,
        .progress_interval_rows = 1,
        .user_data = &state,
        .is_cancelled = decode_is_cancelled,
        .report_progress = decode_progress,
    };
    ctex_image_decode_execution_info execution = {
        .size = CTEX_IMAGE_DECODE_EXECUTION_INFO_CURRENT_SIZE,
    };
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    size_t required_size = 0;
    if (!expect(ctex_image_decode_memory_bounded(
                    gray8_png, sizeof(gray8_png), "bounded.png", CTEX_CHANNEL_SEMANTIC_ROUGHNESS,
                    CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &control, &execution, &info, NULL, 0,
                    &required_size) == CTEX_RESULT_SUCCESS) ||
        !expect(required_size == 2 && execution.cancelled == 0 &&
                execution.progress_event_count == state.callback_count &&
                execution.estimated_peak_working_bytes == state.peak_working_bytes &&
                execution.estimated_peak_working_bytes > 0)) {
        return 0;
    }

    control.maximum_working_bytes = execution.estimated_peak_working_bytes - 1;
    execution.size = CTEX_IMAGE_DECODE_EXECUTION_INFO_CURRENT_SIZE;
    if (!expect(ctex_image_decode_memory_bounded(gray8_png, sizeof(gray8_png), "over-budget.png",
                                                 CTEX_CHANNEL_SEMANTIC_ROUGHNESS,
                                                 CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &control,
                                                 &execution, &info, NULL, 0,
                                                 &required_size) == CTEX_RESULT_OVER_BUDGET) ||
        !expect(execution.estimated_peak_working_bytes > control.maximum_working_bytes &&
                ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED)) {
        return 0;
    }

    state = (decode_control_state){.cancel_after_inspection = 1};
    control.maximum_working_bytes = 2u << 20u;
    execution.size = CTEX_IMAGE_DECODE_EXECUTION_INFO_CURRENT_SIZE;
    info = (ctex_decoded_image_info){.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE, .width = 99};
    required_size = 77;
    unsigned char pixel = 0xa5;
    return expect(ctex_image_decode_memory_bounded(gray8_png, sizeof(gray8_png), "cancelled.png",
                                                   CTEX_CHANNEL_SEMANTIC_ROUGHNESS,
                                                   CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &control,
                                                   &execution, &info, &pixel, sizeof(pixel),
                                                   &required_size) == CTEX_RESULT_CANCELLED) &&
           expect(execution.cancelled == 1 && execution.progress_event_count == 2 &&
                  state.callback_count == 2) &&
           expect(info.width == 99 && required_size == 77 && pixel == 0xa5) &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_IMAGE_DECODE_CANCELLED);
}

static ctex_image_encode_descriptor rgba8_descriptor(uint32_t format) {
    ctex_image_encode_descriptor descriptor = {
        CTEX_IMAGE_ENCODE_DESCRIPTOR_CURRENT_SIZE,
        2,
        2,
        4,
        CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
        8,
        0,
        CTEX_COLOR_SPACE_SRGB_REC709,
        format,
        8,
        90,
    };
    return descriptor;
}

static int encoded_signature_is_valid(uint32_t format, const unsigned char* bytes, size_t size) {
    if (format == CTEX_IMAGE_FILE_FORMAT_PNG) {
        return size > 8 && bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' &&
               bytes[3] == 'G';
    }
    if (format == CTEX_IMAGE_FILE_FORMAT_JPEG) {
        return size > 4 && bytes[0] == 0xff && bytes[1] == 0xd8 && bytes[size - 2] == 0xff &&
               bytes[size - 1] == 0xd9;
    }
    if (format == CTEX_IMAGE_FILE_FORMAT_TGA) {
        return size > 18 && (bytes[2] == 2 || bytes[2] == 10) && bytes[16] == 32;
    }
    if (format == CTEX_IMAGE_FILE_FORMAT_TIFF) {
        return size > 8 && bytes[0] == 'I' && bytes[1] == 'I' && bytes[2] == 42 && bytes[3] == 0;
    }
    return format == CTEX_IMAGE_FILE_FORMAT_OPENEXR && size > 8 && little_u32(bytes) == 20000630u;
}

static int encode_supports_every_output_format(void) {
    static const unsigned char pixels[] = {
        255, 0, 0, 255, 0, 255, 0, 192, 0, 0, 255, 128, 64, 128, 192, 32,
    };
    static const uint32_t formats[] = {
        CTEX_IMAGE_FILE_FORMAT_PNG,  CTEX_IMAGE_FILE_FORMAT_JPEG,    CTEX_IMAGE_FILE_FORMAT_TGA,
        CTEX_IMAGE_FILE_FORMAT_TIFF, CTEX_IMAGE_FILE_FORMAT_OPENEXR,
    };
    unsigned char encoded[4096];
    size_t index = 0;
    for (index = 0; index < sizeof(formats) / sizeof(formats[0]); ++index) {
        ctex_image_encode_descriptor descriptor = rgba8_descriptor(formats[index]);
        ctex_decoded_image_info decoded = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
        size_t required_size = 0;
        size_t decoded_size = 0;
        if (formats[index] == CTEX_IMAGE_FILE_FORMAT_OPENEXR) {
            descriptor.output_bit_depth = 16;
        }
        if (!expect(ctex_image_encode_memory(pixels, sizeof(pixels), &descriptor, NULL, 0,
                                             &required_size) == CTEX_RESULT_SUCCESS) ||
            !expect(required_size > 0 && required_size <= sizeof(encoded)) ||
            !expect(ctex_image_encode_memory(pixels, sizeof(pixels), &descriptor, encoded,
                                             sizeof(encoded),
                                             &required_size) == CTEX_RESULT_SUCCESS) ||
            !expect(encoded_signature_is_valid(formats[index], encoded, required_size))) {
            return 0;
        }
        const ctex_result decode_result = ctex_image_decode_memory(
            encoded, required_size, "mislabelled.bin", CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
            CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &decoded, NULL, 0, &decoded_size);
        if (!expect(decode_result == CTEX_RESULT_SUCCESS) ||
            !expect(decoded.width == 2 && decoded.height == 2 &&
                    decoded.detected_format == formats[index] && decoded.extension_mismatch == 1 &&
                    decoded_size > 0)) {
            fprintf(stderr, "format %u decode result %u: %s\n", formats[index], decode_result,
                    ctex_get_last_diagnostic());
            return 0;
        }
    }
    return 1;
}

static int encode_round_trips_png_and_honours_stride(void) {
    static const unsigned char padded_pixels[] = {
        17,  34,  51,  255, 68,  85,  102, 128, 0xee, 0xee, 0xee, 0xee,
        119, 136, 153, 64,  170, 187, 204, 32,  0xee, 0xee, 0xee, 0xee,
    };
    static const unsigned char expected[] = {
        17, 34, 51, 255, 68, 85, 102, 128, 119, 136, 153, 64, 170, 187, 204, 32,
    };
    ctex_image_encode_descriptor descriptor = rgba8_descriptor(CTEX_IMAGE_FILE_FORMAT_PNG);
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    unsigned char encoded[4096];
    unsigned char decoded[sizeof(expected)];
    size_t encoded_size = 0;
    size_t decoded_size = 0;
    descriptor.row_stride_bytes = 12;
    return expect(ctex_image_encode_memory(padded_pixels, sizeof(padded_pixels), &descriptor,
                                           encoded, sizeof(encoded),
                                           &encoded_size) == CTEX_RESULT_SUCCESS) &&
           expect(ctex_image_decode_memory(
                      encoded, encoded_size, "roundtrip.png", CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                      CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, decoded, sizeof(decoded),
                      &decoded_size) == CTEX_RESULT_SUCCESS) &&
           expect(decoded_size == sizeof(expected) &&
                  memcmp(decoded, expected, sizeof(expected)) == 0);
}

static int encode_refuses_invalid_depth_and_small_output(void) {
    static const unsigned char pixels[] = {1, 2, 3, 4};
    unsigned char output = 0xa5;
    size_t required_size = 0;
    ctex_image_encode_descriptor descriptor = rgba8_descriptor(CTEX_IMAGE_FILE_FORMAT_PNG);
    descriptor.width = 1;
    descriptor.height = 1;
    descriptor.output_bit_depth = 32;
    if (!expect(ctex_image_encode_memory(pixels, sizeof(pixels), &descriptor, NULL, 0,
                                         &required_size) == CTEX_RESULT_UNSUPPORTED_OPERATION) ||
        !expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_COMBINATION) ||
        !expect(strstr(ctex_get_last_diagnostic(), "PNG") != NULL &&
                strstr(ctex_get_last_diagnostic(), "32") != NULL)) {
        return 0;
    }
    descriptor.output_bit_depth = 8;
    return expect(ctex_image_encode_memory(pixels, sizeof(pixels), &descriptor, &output, 1,
                                           &required_size) == CTEX_RESULT_BUFFER_TOO_SMALL) &&
           expect(output == 0xa5 && required_size > 1 &&
                  ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL);
}

static int encode_honours_jpeg_quality(void) {
    static const unsigned char pixels[] = {
        255, 0, 0, 255, 0, 255, 0, 192, 0, 0, 255, 128, 64, 128, 192, 32,
    };
    unsigned char low_quality[4096];
    unsigned char high_quality[4096];
    size_t low_size = 0;
    size_t high_size = 0;
    ctex_image_encode_descriptor descriptor = rgba8_descriptor(CTEX_IMAGE_FILE_FORMAT_JPEG);
    descriptor.jpeg_quality = 10;
    if (!expect(ctex_image_encode_memory(pixels, sizeof(pixels), &descriptor, low_quality,
                                         sizeof(low_quality), &low_size) == CTEX_RESULT_SUCCESS)) {
        return 0;
    }
    descriptor.jpeg_quality = 100;
    return expect(ctex_image_encode_memory(pixels, sizeof(pixels), &descriptor, high_quality,
                                           sizeof(high_quality),
                                           &high_size) == CTEX_RESULT_SUCCESS) &&
           expect(low_size != high_size || memcmp(low_quality, high_quality, low_size) != 0);
}

static int decode_preserves_radiance_hdr_values(void) {
    static const char header[] =
        "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\nPRIMARIES=0.6400 0.3300 0.3000 0.6000 "
        "0.1500 0.0600 0.3127 0.3290\n\n-Y 1 +X 2\n";
    static const char unsupported_header[] =
        "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\nPRIMARIES=0.7 0.3 0.2 0.7 0.1 0.1 "
        "0.3127 0.3290\n\n-Y 1 +X 2\n";
    static const unsigned char rgbe[] = {128, 64, 32, 130, 32, 64, 128, 129};
    unsigned char encoded[sizeof(header) - 1 + sizeof(rgbe)];
    unsigned char unsupported[sizeof(unsupported_header) - 1 + sizeof(rgbe)];
    float pixels[6] = {0};
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    size_t required_size = 0;
    memcpy(encoded, header, sizeof(header) - 1);
    memcpy(encoded + sizeof(header) - 1, rgbe, sizeof(rgbe));
    memcpy(unsupported, unsupported_header, sizeof(unsupported_header) - 1);
    memcpy(unsupported + sizeof(unsupported_header) - 1, rgbe, sizeof(rgbe));
    return expect(ctex_image_decode_memory(
                      encoded, sizeof(encoded), "environment.png", CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                      CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, pixels, sizeof(pixels),
                      &required_size) == CTEX_RESULT_SUCCESS) &&
           expect(info.detected_format == CTEX_IMAGE_FILE_FORMAT_RADIANCE_HDR &&
                  info.scalar_representation == CTEX_SCALAR_REPRESENTATION_FLOATING_POINT &&
                  info.bit_depth == 32 && info.channel_count == 3 &&
                  info.color_space == CTEX_COLOR_SPACE_LINEAR_REC709 &&
                  info.color_space_source == CTEX_COLOR_SPACE_SOURCE_EMBEDDED_PROFILE &&
                  info.uninterpretable_profile == 0 && info.extension_mismatch == 1 &&
                  required_size == sizeof(pixels)) &&
           expect(pixels[0] == 2.0f && pixels[1] == 1.0f && pixels[2] == 0.5f &&
                  pixels[3] == 0.25f && pixels[4] == 0.5f && pixels[5] == 1.0f) &&
           expect(ctex_image_decode_memory(unsupported, sizeof(unsupported), "wide.hdr",
                                           CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                                           CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, NULL, 0,
                                           &required_size) == CTEX_RESULT_SUCCESS) &&
           expect(info.color_space_source == CTEX_COLOR_SPACE_SOURCE_AUTOMATIC_RULE &&
                  info.uninterpretable_profile == 1);
}

static int openexr_round_trip_preserves_hdr_values(void) {
    static const float source[] = {4.0f, 2.0f, 0.5f, 1.0f};
    ctex_image_encode_descriptor descriptor = {
        CTEX_IMAGE_ENCODE_DESCRIPTOR_CURRENT_SIZE,
        1,
        1,
        4,
        CTEX_SCALAR_REPRESENTATION_FLOATING_POINT,
        32,
        0,
        CTEX_COLOR_SPACE_LINEAR_REC709,
        CTEX_IMAGE_FILE_FORMAT_OPENEXR,
        32,
        0,
    };
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    unsigned char encoded[4096];
    float decoded[4] = {0};
    size_t encoded_size = 0;
    size_t decoded_size = 0;
    return expect(ctex_image_encode_memory(source, sizeof(source), &descriptor, encoded,
                                           sizeof(encoded),
                                           &encoded_size) == CTEX_RESULT_SUCCESS) &&
           expect(ctex_image_decode_memory(
                      encoded, encoded_size, "environment.exr", CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                      CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, decoded, sizeof(decoded),
                      &decoded_size) == CTEX_RESULT_SUCCESS) &&
           expect(info.detected_format == CTEX_IMAGE_FILE_FORMAT_OPENEXR &&
                  info.scalar_representation == CTEX_SCALAR_REPRESENTATION_FLOATING_POINT &&
                  decoded_size == sizeof(decoded)) &&
           expect(decoded[0] == source[0] && decoded[1] == source[1] && decoded[2] == source[2] &&
                  decoded[3] == source[3]);
}

static int layered_decode_is_sized_named_and_atomic(void) {
    unsigned char encoded[256] = {0};
    const size_t encoded_size = make_layered_psd(encoded);
    const ctex_layered_image_decode_descriptor descriptor = {
        .size = CTEX_LAYERED_IMAGE_DECODE_DESCRIPTOR_CURRENT_SIZE,
        .mode = CTEX_LAYERED_IMAGE_DECODE_INDIVIDUAL,
        .intended_channel = CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
        .input_color_space = CTEX_INPUT_COLOR_SPACE_AUTOMATIC,
        .maximum_image_count = 8,
    };
    ctex_layered_image_decode_info info = {
        .size = CTEX_LAYERED_IMAGE_DECODE_INFO_CURRENT_SIZE,
    };
    ctex_layered_decoded_image_info image_infos[2] = {{0}};
    char names[11] = {0};
    unsigned char pixels[8] = {0};
    char short_names[10];
    memset(short_names, 0xa5, sizeof(short_names));

    if (!expect(ctex_image_decode_layered_memory(encoded, encoded_size, "layers.psd", &descriptor,
                                                 NULL, NULL, NULL, &info, NULL, 0, NULL, 0, NULL,
                                                 0) == CTEX_RESULT_SUCCESS) ||
        !expect(info.detected_format == CTEX_IMAGE_FILE_FORMAT_PSD &&
                info.source_was_layered == 1 && info.image_count == 2 &&
                info.required_image_info_count == 2 &&
                info.required_name_buffer_size == sizeof(names) &&
                info.required_pixel_buffer_size == sizeof(pixels))) {
        return 0;
    }
    image_infos[0].width = 77;
    if (!expect(ctex_image_decode_layered_memory(encoded, encoded_size, "layers.psd", &descriptor,
                                                 NULL, NULL, NULL, &info, image_infos, 2, NULL, 0,
                                                 NULL, 0) == CTEX_RESULT_INVALID_ARGUMENT) ||
        !expect(image_infos[0].width == 77 &&
                ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_NULL_ARGUMENT)) {
        return 0;
    }
    if (!expect(ctex_image_decode_layered_memory(encoded, encoded_size, "layers.psd", &descriptor,
                                                 NULL, NULL, NULL, &info, image_infos, 2,
                                                 short_names, sizeof(short_names), pixels,
                                                 sizeof(pixels)) == CTEX_RESULT_BUFFER_TOO_SMALL) ||
        !expect(image_infos[0].width == 77 && (unsigned char)short_names[0] == 0xa5 &&
                ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL)) {
        return 0;
    }
    if (!expect(ctex_image_decode_layered_memory(encoded, encoded_size, "layers.psd", &descriptor,
                                                 NULL, NULL, NULL, &info, image_infos, 2, names,
                                                 sizeof(names), pixels,
                                                 sizeof(pixels)) == CTEX_RESULT_SUCCESS)) {
        return 0;
    }
    return expect(image_infos[0].size == CTEX_LAYERED_DECODED_IMAGE_INFO_CURRENT_SIZE &&
                  image_infos[0].width == 1 && image_infos[0].height == 1 &&
                  image_infos[0].channel_count == 4 && image_infos[0].bit_depth == 8 &&
                  image_infos[0].name_offset == 0 && image_infos[0].name_size == 3 &&
                  image_infos[0].pixel_offset == 0 && image_infos[0].pixel_size == 4 &&
                  image_infos[1].name_offset == 4 && image_infos[1].name_size == 6 &&
                  image_infos[1].pixel_offset == 4 && image_infos[1].pixel_size == 4) &&
           expect(strcmp(names, "Top") == 0 && strcmp(names + 4, "Bottom") == 0) &&
           expect(pixels[0] == 255 && pixels[3] == 128 && pixels[6] == 255 && pixels[7] == 255);
}

int main(void) {
    return decode_reports_content_and_caller_buffers() && decode_preserves_sixteen_bit_samples() &&
                   channel_expansion_preserves_depth_and_reports_rule() &&
                   image_resampling_selects_and_reports_filter() &&
                   sixteen_bit_png_round_trip_is_lossless() &&
                   decode_honours_color_and_resource_limits() &&
                   decode_refuses_unsupported_and_truncated_content() &&
                   bounded_decode_reports_progress_budget_and_atomic_cancellation() &&
                   encode_supports_every_output_format() &&
                   encode_round_trips_png_and_honours_stride() &&
                   encode_refuses_invalid_depth_and_small_output() &&
                   encode_honours_jpeg_quality() && decode_preserves_radiance_hdr_values() &&
                   openexr_round_trip_preserves_hdr_values() &&
                   layered_decode_is_sized_named_and_atomic()
               ? 0
               : 1;
}
