#include <ctex/capi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct export_state {
    size_t source_count;
    size_t output_count;
    size_t progress_count;
    size_t report_count;
    size_t report_size;
    int report_dry_run;
    int report_cancelled;
    int cancel_immediately;
    int valid;
} export_state;

static int contains_bytes(const char* bytes, size_t byte_count, const char* needle) {
    const size_t needle_size = strlen(needle);
    size_t index = 0;
    if (needle_size > byte_count) {
        return 0;
    }
    for (index = 0; index + needle_size <= byte_count; ++index) {
        if (memcmp(bytes + index, needle, needle_size) == 0) {
            return 1;
        }
    }
    return 0;
}

static ctex_result sample_pixel(uint32_t x, uint32_t y, ctex_texture_export_sample* out_sample,
                                void* user_data) {
    static const ctex_texture_export_named_value custom = {
        CTEX_TEXTURE_EXPORT_NAMED_VALUE_CURRENT_SIZE, "custom", 2, {0.25, 0.75, 0.0, 0.0}};
    export_state* state = (export_state*)user_data;
    if (out_sample == NULL || out_sample->size != CTEX_TEXTURE_EXPORT_SAMPLE_CURRENT_SIZE ||
        x >= 2 || y >= 2) {
        state->valid = 0;
        return CTEX_RESULT_INVALID_ARGUMENT;
    }
    out_sample->base_color[0] = (double)x;
    out_sample->base_color[1] = (double)y;
    out_sample->base_color[2] = 0.5;
    out_sample->opacity = 1.0;
    out_sample->roughness = 0.25;
    out_sample->metallic = 0.5;
    out_sample->normal[0] = 0.5;
    out_sample->normal[1] = 0.25;
    out_sample->normal[2] = 1.0;
    out_sample->emission[0] = 0.2;
    out_sample->emission[1] = 0.4;
    out_sample->emission[2] = 0.6;
    out_sample->registered_channels = &custom;
    out_sample->registered_channel_count = 1;
    return CTEX_RESULT_SUCCESS;
}

static ctex_result provide_source(const ctex_texture_export_planned_output* output,
                                  ctex_texture_export_pixel_source_descriptor* out_source,
                                  void* user_data) {
    static const uint8_t coverage[4] = {1, 1, 1, 1};
    export_state* state = (export_state*)user_data;
    ++state->source_count;
    if (output == NULL || output->size != CTEX_TEXTURE_EXPORT_PLANNED_OUTPUT_CURRENT_SIZE ||
        output->relative_path == NULL || output->texture_set_identifier_count != 1 ||
        output->width != 2 || output->height != 2 || output->format != CTEX_IMAGE_FILE_FORMAT_PNG ||
        out_source == NULL ||
        out_source->size != CTEX_TEXTURE_EXPORT_PIXEL_SOURCE_DESCRIPTOR_CURRENT_SIZE) {
        state->valid = 0;
        return CTEX_RESULT_INVALID_ARGUMENT;
    }
    *out_source = (ctex_texture_export_pixel_source_descriptor){
        CTEX_TEXTURE_EXPORT_PIXEL_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        2,
        2,
        sample_pixel,
        user_data,
        coverage,
        4,
    };
    return CTEX_RESULT_SUCCESS;
}

static ctex_result receive_output(const ctex_texture_export_encoded_output* output,
                                  void* user_data) {
    static const uint8_t png_signature[8] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    export_state* state = (export_state*)user_data;
    ctex_decoded_image_info decoded = {0};
    uint8_t pixels[16] = {0};
    size_t pixel_size = 0;
    decoded.size = CTEX_DECODED_IMAGE_INFO_CURRENT_SIZE;
    ++state->output_count;
    if (output == NULL || output->size != CTEX_TEXTURE_EXPORT_ENCODED_OUTPUT_CURRENT_SIZE ||
        output->report_entry_index != 0 || output->relative_path == NULL || output->width != 2 ||
        output->height != 2 || output->format != CTEX_IMAGE_FILE_FORMAT_PNG ||
        output->bit_depth != 8 || output->color_space != CTEX_COLOR_SPACE_LINEAR_REC709 ||
        output->byte_count < sizeof(png_signature) ||
        memcmp(output->bytes, png_signature, sizeof(png_signature)) != 0) {
        state->valid = 0;
        return CTEX_RESULT_INVALID_ARGUMENT;
    }
    if (ctex_image_decode_memory(output->bytes, output->byte_count, output->relative_path,
                                 CTEX_CHANNEL_SEMANTIC_ROUGHNESS, CTEX_INPUT_COLOR_SPACE_AUTOMATIC,
                                 NULL, &decoded, NULL, 0, &pixel_size) != CTEX_RESULT_SUCCESS ||
        pixel_size != sizeof(pixels) || decoded.channel_count != 4 || decoded.bit_depth != 8 ||
        ctex_image_decode_memory(output->bytes, output->byte_count, output->relative_path,
                                 CTEX_CHANNEL_SEMANTIC_ROUGHNESS, CTEX_INPUT_COLOR_SPACE_AUTOMATIC,
                                 NULL, &decoded, pixels, sizeof(pixels),
                                 &pixel_size) != CTEX_RESULT_SUCCESS ||
        pixels[0] != 191 || pixels[1] != 95 || pixels[2] != 191 || pixels[3] != 191) {
        state->valid = 0;
        return CTEX_RESULT_INVALID_ARGUMENT;
    }
    return CTEX_RESULT_SUCCESS;
}

static ctex_result receive_report(const char* json, size_t json_size, void* user_data) {
    export_state* state = (export_state*)user_data;
    ++state->report_count;
    state->report_size = json_size;
    state->report_dry_run = contains_bytes(json, json_size, "\"dry_run\":true");
    state->report_cancelled = contains_bytes(json, json_size, "\"cancelled\":true");
    if (json == NULL || !contains_bytes(json, json_size, "\"outputs\":")) {
        state->valid = 0;
        return CTEX_RESULT_INVALID_ARGUMENT;
    }
    return CTEX_RESULT_SUCCESS;
}

static void receive_progress(size_t completed, size_t total, const char* relative_path,
                             void* user_data) {
    export_state* state = (export_state*)user_data;
    ++state->progress_count;
    if (completed != 1 || total != 1 || relative_path == NULL) {
        state->valid = 0;
    }
}

static uint32_t cancel_export(void* user_data) {
    const export_state* state = (const export_state*)user_data;
    return state->cancel_immediately != 0 ? 1U : 0U;
}

static ctex_texture_export_callbacks_descriptor callbacks(export_state* state) {
    return (ctex_texture_export_callbacks_descriptor){
        CTEX_TEXTURE_EXPORT_CALLBACKS_DESCRIPTOR_CURRENT_SIZE,
        provide_source,
        receive_output,
        receive_report,
        receive_progress,
        cancel_export,
        state,
    };
}

static ctex_texture_export_catalogue_descriptor catalogue(
    ctex_texture_export_texture_set_source_descriptor* texture_set,
    ctex_texture_export_atlas_source_descriptor* atlas,
    ctex_texture_export_layer_source_descriptor* layers) {
    static const uint32_t udim_tiles[2] = {1001, 1002};
    static const char* atlas_members[1] = {"body"};
    layers[0] = (ctex_texture_export_layer_source_descriptor){
        CTEX_TEXTURE_EXPORT_LAYER_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        "group",
        "Group",
        "",
        CTEX_TEXTURE_EXPORT_LAYER_GROUP,
        1,
    };
    layers[1] = (ctex_texture_export_layer_source_descriptor){
        CTEX_TEXTURE_EXPORT_LAYER_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        "paint",
        "Paint",
        "group",
        CTEX_TEXTURE_EXPORT_LAYER_CONTENT,
        1,
    };
    *texture_set = (ctex_texture_export_texture_set_source_descriptor){
        CTEX_TEXTURE_EXPORT_TEXTURE_SET_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        "body",
        "Body",
        2,
        2,
        udim_tiles,
        2,
        layers,
        2,
    };
    *atlas = (ctex_texture_export_atlas_source_descriptor){
        CTEX_TEXTURE_EXPORT_ATLAS_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        "main",
        "Main",
        4,
        4,
        atlas_members,
        1,
    };
    return (ctex_texture_export_catalogue_descriptor){
        CTEX_TEXTURE_EXPORT_CATALOGUE_DESCRIPTOR_CURRENT_SIZE, "demo", texture_set, 1, atlas, 1,
    };
}

static ctex_texture_export_preset_descriptor custom_preset(
    ctex_texture_export_texture_descriptor* texture) {
    *texture = (ctex_texture_export_texture_descriptor){
        CTEX_TEXTURE_EXPORT_TEXTURE_DESCRIPTOR_CURRENT_SIZE,
        "_Packed",
        {"smoothness", "emission", "channel:custom:1", "normal.directx_y"},
        CTEX_COLOR_SPACE_LINEAR_REC709,
        8,
        CTEX_IMAGE_FILE_FORMAT_PNG,
    };
    return (ctex_texture_export_preset_descriptor){
        CTEX_TEXTURE_EXPORT_PRESET_DESCRIPTOR_CURRENT_SIZE,
        "custom-packed",
        "Custom packed",
        texture,
        1,
    };
}

static int built_in_presets_are_enumerable(void) {
    char identifiers[256] = {0};
    char too_small[1] = {'X'};
    size_t required = 0;
    size_t count = 0;
    if (ctex_texture_export_get_built_in_preset_ids(NULL, 0, &required, &count) !=
            CTEX_RESULT_SUCCESS ||
        required == 0 || count < 6 ||
        ctex_texture_export_get_built_in_preset_ids(too_small, sizeof(too_small), &required,
                                                    &count) != CTEX_RESULT_BUFFER_TOO_SMALL ||
        too_small[0] != 'X' ||
        ctex_texture_export_get_built_in_preset_ids(identifiers, sizeof(identifiers), &required,
                                                    &count) != CTEX_RESULT_SUCCESS ||
        strcmp(identifiers, "pbr-individual") != 0) {
        return 0;
    }
    return 1;
}

static int built_in_preset_can_be_selected(
    const ctex_texture_export_catalogue_descriptor* sources) {
    ctex_texture_export_preset_descriptor preset = {
        CTEX_TEXTURE_EXPORT_PRESET_DESCRIPTOR_CURRENT_SIZE, "pbr-individual", NULL, NULL, 0,
    };
    ctex_texture_export_options_descriptor options = {
        CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE, NULL, 2, 90, 1};
    export_state state = {0, 0, 0, 0, 0, 0, 0, 0, 1};
    ctex_texture_export_callbacks_descriptor call = callbacks(&state);
    ctex_texture_export_info info = {CTEX_TEXTURE_EXPORT_INFO_CURRENT_SIZE, 0, 0, 0, 0};
    return ctex_texture_export_run(sources, &preset, &options, &call, &info) ==
               CTEX_RESULT_SUCCESS &&
           info.planned_output_count == 9 && info.encoded_output_count == 0 &&
           state.report_count == 1 && state.report_dry_run && state.valid;
}

static int dry_run_covers_scopes(const ctex_texture_export_catalogue_descriptor* sources,
                                 const ctex_texture_export_preset_descriptor* preset) {
    static const char* selected_sets[1] = {"body"};
    static const char* selected_layer_ids[1] = {"group"};
    ctex_texture_export_layer_selection_descriptor selected_layers = {
        CTEX_TEXTURE_EXPORT_LAYER_SELECTION_DESCRIPTOR_CURRENT_SIZE,
        "body",
        selected_layer_ids,
        1,
    };
    const uint32_t scopes[3] = {CTEX_TEXTURE_EXPORT_SCOPE_TEXTURE_SET,
                                CTEX_TEXTURE_EXPORT_SCOPE_UDIM, CTEX_TEXTURE_EXPORT_SCOPE_ATLAS};
    size_t index = 0;
    for (index = 0; index < 3; ++index) {
        export_state state = {0, 0, 0, 0, 0, 0, 0, 0, 1};
        ctex_texture_export_plan_descriptor plan = {
            CTEX_TEXTURE_EXPORT_PLAN_DESCRIPTOR_CURRENT_SIZE,
            CTEX_TEXTURE_EXPORT_TEXTURE_SET_SELECTED,
            selected_sets,
            1,
            scopes[index],
            CTEX_TEXTURE_EXPORT_LAYER_EACH_SELECTED,
            &selected_layers,
            1,
            4,
            4,
            "{project}_{texture_set}{suffix}_{udim}_{layer}.{extension}",
        };
        ctex_texture_export_options_descriptor options = {
            CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE, &plan, 2, 90, 1};
        ctex_texture_export_callbacks_descriptor call = callbacks(&state);
        ctex_texture_export_info info = {CTEX_TEXTURE_EXPORT_INFO_CURRENT_SIZE, 0, 0, 0, 0};
        if (ctex_texture_export_run(sources, preset, &options, &call, &info) !=
                CTEX_RESULT_SUCCESS ||
            info.planned_output_count == 0 || info.encoded_output_count != 0 || info.dry_run != 1 ||
            info.cancelled != 0 || state.source_count != 0 || state.output_count != 0 ||
            state.progress_count != 0 || state.report_count != 1 || !state.report_dry_run ||
            !state.valid) {
            return 0;
        }
    }
    return 1;
}

static int actual_export_uses_callbacks(const ctex_texture_export_catalogue_descriptor* sources,
                                        const ctex_texture_export_preset_descriptor* preset) {
    export_state state = {0, 0, 0, 0, 0, 0, 0, 0, 1};
    ctex_texture_export_plan_descriptor plan = {
        CTEX_TEXTURE_EXPORT_PLAN_DESCRIPTOR_CURRENT_SIZE,
        CTEX_TEXTURE_EXPORT_TEXTURE_SET_ALL,
        NULL,
        0,
        CTEX_TEXTURE_EXPORT_SCOPE_TEXTURE_SET,
        CTEX_TEXTURE_EXPORT_LAYER_FLATTEN_VISIBLE,
        NULL,
        0,
        2,
        2,
        "{project}_{texture_set}{suffix}.{extension}",
    };
    ctex_texture_export_options_descriptor options = {
        CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE, &plan, 0, 90, 0};
    ctex_texture_export_callbacks_descriptor call = callbacks(&state);
    ctex_texture_export_info info = {CTEX_TEXTURE_EXPORT_INFO_CURRENT_SIZE, 0, 0, 0, 0};
    if (ctex_texture_export_run(sources, preset, &options, &call, &info) != CTEX_RESULT_SUCCESS ||
        info.planned_output_count != 1 || info.encoded_output_count != 1 || info.dry_run != 0 ||
        info.cancelled != 0 || state.source_count != 1 || state.output_count != 1 ||
        state.progress_count != 1 || state.report_count != 1 || state.report_size == 0 ||
        state.report_dry_run || state.report_cancelled || !state.valid) {
        return 0;
    }
    return 1;
}

static int cancellation_is_reported(const ctex_texture_export_catalogue_descriptor* sources,
                                    const ctex_texture_export_preset_descriptor* preset) {
    export_state state = {0, 0, 0, 0, 0, 0, 0, 1, 1};
    ctex_texture_export_options_descriptor options = {
        CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE, NULL, 0, 90, 0};
    ctex_texture_export_callbacks_descriptor call = callbacks(&state);
    ctex_texture_export_info info = {CTEX_TEXTURE_EXPORT_INFO_CURRENT_SIZE, 0, 0, 0, 0};
    if (ctex_texture_export_run(sources, preset, &options, &call, &info) != CTEX_RESULT_CANCELLED ||
        info.planned_output_count != 1 || info.encoded_output_count != 0 || info.cancelled != 1 ||
        state.source_count != 0 || state.output_count != 0 || state.report_count != 1 ||
        !state.report_cancelled || !state.valid) {
        return 0;
    }
    return 1;
}

static int invalid_format_is_refused(const ctex_texture_export_catalogue_descriptor* sources,
                                     ctex_texture_export_preset_descriptor* preset,
                                     ctex_texture_export_texture_descriptor* texture) {
    export_state state = {0, 0, 0, 0, 0, 0, 0, 0, 1};
    ctex_texture_export_options_descriptor options = {
        CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE, NULL, 0, 90, 1};
    ctex_texture_export_callbacks_descriptor call = callbacks(&state);
    ctex_texture_export_info info = {CTEX_TEXTURE_EXPORT_INFO_CURRENT_SIZE, 0, 0, 0, 0};
    texture->format = CTEX_IMAGE_FILE_FORMAT_BMP;
    if (ctex_texture_export_run(sources, preset, &options, &call, &info) !=
            CTEX_RESULT_UNSUPPORTED_OPERATION ||
        ctex_get_last_diagnostic_code() != CTEX_DIAGNOSTIC_UNSUPPORTED_IMAGE_FORMAT) {
        return 0;
    }
    texture->format = CTEX_IMAGE_FILE_FORMAT_PNG;
    return 1;
}

int main(void) {
    ctex_texture_export_layer_source_descriptor layers[2];
    ctex_texture_export_texture_set_source_descriptor texture_set;
    ctex_texture_export_atlas_source_descriptor atlas;
    ctex_texture_export_texture_descriptor texture;
    ctex_texture_export_catalogue_descriptor sources = catalogue(&texture_set, &atlas, layers);
    ctex_texture_export_preset_descriptor preset = custom_preset(&texture);
    if (!built_in_presets_are_enumerable() || !built_in_preset_can_be_selected(&sources) ||
        !dry_run_covers_scopes(&sources, &preset) ||
        !actual_export_uses_callbacks(&sources, &preset) ||
        !cancellation_is_reported(&sources, &preset) ||
        !invalid_format_is_refused(&sources, &preset, &texture)) {
        fprintf(stderr, "texture export C ABI regression failed: %s\n", ctex_get_last_diagnostic());
        return 1;
    }
    return 0;
}
