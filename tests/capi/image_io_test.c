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

int main(void) {
    return decode_reports_content_and_caller_buffers() && decode_preserves_sixteen_bit_samples() &&
                   decode_honours_color_and_resource_limits() &&
                   decode_refuses_unsupported_and_truncated_content()
               ? 0
               : 1;
}
