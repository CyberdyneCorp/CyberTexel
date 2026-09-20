#include <ctex/capi.h>
#include <stdint.h>
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
    static const unsigned char jpeg[] = {0xff, 0xd8, 0xff, 0x00};
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    size_t required_size = 0;
    return expect(ctex_image_decode_memory(jpeg, sizeof(jpeg), "photo.bin",
                                           CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                                           CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, NULL, 0,
                                           &required_size) == CTEX_RESULT_UNSUPPORTED_OPERATION) &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_FORMAT &&
                  strstr(ctex_get_last_diagnostic(), "JPEG") != NULL) &&
           expect(ctex_image_decode_memory(gray8_png, sizeof(gray8_png) / 2, "broken.png",
                                           CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                                           CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, NULL, 0,
                                           &required_size) == CTEX_RESULT_INVALID_ARGUMENT) &&
           expect(ctex_get_last_diagnostic_code() == CTEX_DIAGNOSTIC_INVALID_IMAGE_DATA);
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
        size_t required_size = 0;
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
    static const char header[] = "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 1 +X 2\n";
    static const unsigned char rgbe[] = {128, 64, 32, 130, 32, 64, 128, 129};
    unsigned char encoded[sizeof(header) - 1 + sizeof(rgbe)];
    float pixels[6] = {0};
    ctex_decoded_image_info info = {.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE};
    size_t required_size = 0;
    memcpy(encoded, header, sizeof(header) - 1);
    memcpy(encoded + sizeof(header) - 1, rgbe, sizeof(rgbe));
    return expect(ctex_image_decode_memory(
                      encoded, sizeof(encoded), "environment.png", CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                      CTEX_INPUT_COLOR_SPACE_AUTOMATIC, NULL, &info, pixels, sizeof(pixels),
                      &required_size) == CTEX_RESULT_SUCCESS) &&
           expect(info.detected_format == CTEX_IMAGE_FILE_FORMAT_RADIANCE_HDR &&
                  info.scalar_representation == CTEX_SCALAR_REPRESENTATION_FLOATING_POINT &&
                  info.bit_depth == 32 && info.channel_count == 3 &&
                  info.color_space == CTEX_COLOR_SPACE_LINEAR_REC709 &&
                  info.extension_mismatch == 1 && required_size == sizeof(pixels)) &&
           expect(pixels[0] == 2.0f && pixels[1] == 1.0f && pixels[2] == 0.5f &&
                  pixels[3] == 0.25f && pixels[4] == 0.5f && pixels[5] == 1.0f);
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

int main(void) {
    return decode_reports_content_and_caller_buffers() && decode_preserves_sixteen_bit_samples() &&
                   channel_expansion_preserves_depth_and_reports_rule() &&
                   image_resampling_selects_and_reports_filter() &&
                   sixteen_bit_png_round_trip_is_lossless() &&
                   decode_honours_color_and_resource_limits() &&
                   decode_refuses_unsupported_and_truncated_content() &&
                   encode_supports_every_output_format() &&
                   encode_round_trips_png_and_honours_stride() &&
                   encode_refuses_invalid_depth_and_small_output() &&
                   encode_honours_jpeg_quality() && decode_preserves_radiance_hdr_values() &&
                   openexr_round_trip_preserves_hdr_values()
               ? 0
               : 1;
}
