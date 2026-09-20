#include <ctex/capi.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
    }
    return condition;
}

typedef struct fixture {
    ctex_document* document;
    ctex_mesh* mesh;
    ctex_mesh_map_set* maps;
    char texture_set_id[128];
} fixture;

static int create_fixture(fixture* value) {
    static const ctex_vec3f positions[] = {
        {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}};
    static const ctex_vec3f normals[] = {
        {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}};
    static const ctex_vec2f uv_values[] = {{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}};
    static const uint32_t triangle_indices[] = {0, 1, 2};
    static const uint32_t face_partitions[] = {0};
    static const uint32_t face_materials[] = {1};
    static const ctex_uv_set_descriptor uv_sets[] = {{
        CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE,
        "uv0",
        uv_values,
        3,
    }};
    static const ctex_mesh_partition_descriptor partitions[] = {{
        CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE,
        CTEX_PARTITION_SOURCE_MATERIAL,
        "body",
        "Body",
    }};
    const ctex_texture_set_descriptor texture_set = {
        .size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE,
        .display_name = "Body",
        .partition_kind = CTEX_PARTITION_SOURCE_MATERIAL,
        .partition_key = "body",
        .uv_set = "uv0",
        .width = 4,
        .height = 4,
        .default_bit_depth = 8,
    };
    const ctex_mesh_descriptor mesh = {
        CTEX_MESH_DESCRIPTOR_CURRENT_SIZE,
        positions,
        3,
        normals,
        3,
        NULL,
        0,
        triangle_indices,
        3,
        uv_sets,
        1,
        "uv0",
        partitions,
        1,
        face_partitions,
        1,
        face_materials,
        1,
    };
    size_t required_size = 0;
    size_t count = 0;

    return ctex_document_create(&value->document) == CTEX_RESULT_SUCCESS &&
           ctex_document_create_texture_set(value->document, &texture_set) == CTEX_RESULT_SUCCESS &&
           ctex_document_get_texture_set_ids(value->document, NULL, 0, &required_size, &count) ==
               CTEX_RESULT_SUCCESS &&
           count == 1 && required_size <= sizeof(value->texture_set_id) &&
           ctex_document_get_texture_set_ids(value->document, value->texture_set_id,
                                             sizeof(value->texture_set_id), &required_size,
                                             &count) == CTEX_RESULT_SUCCESS &&
           ctex_mesh_create(&mesh, &value->mesh) == CTEX_RESULT_SUCCESS &&
           ctex_mesh_map_set_create(value->document, value->texture_set_id, value->mesh,
                                    &value->maps) == CTEX_RESULT_SUCCESS;
}

static void destroy_fixture(fixture* value) {
    ctex_mesh_map_set_destroy(value->maps);
    ctex_mesh_destroy(value->mesh);
    ctex_document_destroy(value->document);
}

static int import_float_map(fixture* value, uint32_t kind, uint32_t meaning, const float* pixels,
                            uint32_t component_count) {
    const ctex_mesh_map_import_descriptor descriptor = {
        .size = CTEX_MESH_MAP_IMPORT_DESCRIPTOR_CURRENT_SIZE,
        .kind = kind,
        .channel_meaning = meaning,
        .color_space = CTEX_COLOR_SPACE_LINEAR_REC709,
        .buffer =
            {
                .size = CTEX_MESH_MAP_PIXEL_BUFFER_DESCRIPTOR_CURRENT_SIZE,
                .width = 1,
                .height = 1,
                .component_type = CTEX_TRANSPORT_COMPONENT_FLOAT32,
                .component_count = component_count,
                .row_stride_bytes = component_count * sizeof(float),
                .pixels = pixels,
                .pixel_bytes = component_count * sizeof(float),
            },
    };
    ctex_mesh_map_import_info info = {.size = CTEX_MESH_MAP_IMPORT_INFO_CURRENT_SIZE};
    return ctex_mesh_map_set_import_external(value->maps, &descriptor, &info) ==
           CTEX_RESULT_SUCCESS;
}

static int generator_catalogue_is_public(void) {
    static const char* names[] = {
        "ambient-occlusion",     "curvature", "thickness", "position-gradient",
        "world-space-direction", "dirt",      "edge-wear", "scratches"};
    uint32_t kind = 0;
    for (kind = 0; kind < 8; ++kind) {
        ctex_mesh_map_generator_info info = {.size = CTEX_MESH_MAP_GENERATOR_INFO_CURRENT_SIZE};
        uint32_t maps[4] = {0};
        ctex_mesh_map_generator_parameter_descriptor parameters[8] = {{0}};
        char strings[1024] = {0};
        if (!expect(ctex_mesh_map_generator_get_info(kind, &info, NULL, 0, NULL, 0, NULL, 0) ==
                        CTEX_RESULT_SUCCESS,
                    "generator catalogue sizing failed") ||
            !expect(info.required_map_count > 0 && info.parameter_count > 0 &&
                        info.required_map_count <= 4 && info.parameter_count <= 8 &&
                        info.required_string_size <= sizeof(strings),
                    "generator catalogue counts are invalid") ||
            !expect(ctex_mesh_map_generator_get_info(kind, &info, maps, 4, parameters, 8, strings,
                                                     sizeof(strings)) == CTEX_RESULT_SUCCESS,
                    "generator catalogue copy failed") ||
            !expect(strcmp(strings + info.name_offset, names[kind]) == 0 &&
                        parameters[0].size ==
                            CTEX_MESH_MAP_GENERATOR_PARAMETER_DESCRIPTOR_CURRENT_SIZE &&
                        parameters[0].minimum <= parameters[0].default_value &&
                        parameters[0].default_value <= parameters[0].maximum &&
                        parameters[0].name_size > 1 && parameters[0].meaning_size > 1,
                    "generator catalogue metadata is incomplete")) {
            return 0;
        }
    }
    return expect(ctex_mesh_map_generator_get_info(99, NULL, NULL, 0, NULL, 0, NULL, 0) ==
                      CTEX_RESULT_INVALID_ARGUMENT,
                  "invalid generator kind was accepted");
}

static int generator_output_is_bounded_and_deterministic(fixture* value) {
    static const float ao[] = {0.75F};
    const ctex_mesh_map_generator_parameter parameters[] = {
        {.size = CTEX_MESH_MAP_GENERATOR_PARAMETER_CURRENT_SIZE, .name = "strength", .value = 3.0},
        {.size = CTEX_MESH_MAP_GENERATOR_PARAMETER_CURRENT_SIZE, .name = "contrast", .value = 2.0},
    };
    ctex_mesh_map_generator_result_info sizing = {
        .size = CTEX_MESH_MAP_GENERATOR_RESULT_INFO_CURRENT_SIZE};
    ctex_mesh_map_generator_result_info result = {
        .size = CTEX_MESH_MAP_GENERATOR_RESULT_INFO_CURRENT_SIZE};
    ctex_mesh_map_generator_resolved_parameter resolved[8] = {{0}};
    ctex_mesh_map_generator_parameter_clamp clamps[8] = {{0}};
    ctex_mesh_map_staleness stale[2] = {{0}};
    float first[1] = {0.0F};
    float second[1] = {0.0F};
    char strings[512] = {0};

    if (!expect(import_float_map(value, CTEX_MESH_MAP_AMBIENT_OCCLUSION, CTEX_MESH_MAP_SCALAR_DATA,
                                 ao, 1),
                "generator AO fixture import failed") ||
        !expect(ctex_mesh_map_generator_generate(
                    value->maps, CTEX_MESH_MAP_GENERATOR_AMBIENT_OCCLUSION, 1, 1, parameters, 2,
                    &sizing, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0) == CTEX_RESULT_SUCCESS,
                "generator output sizing failed") ||
        !expect(sizing.required_mask_value_count == 1 &&
                    sizing.required_resolved_parameter_count == 2 &&
                    sizing.required_parameter_clamp_count == 1 &&
                    sizing.required_string_size <= sizeof(strings),
                "generator output counts are incorrect") ||
        !expect(ctex_mesh_map_generator_generate(
                    value->maps, CTEX_MESH_MAP_GENERATOR_AMBIENT_OCCLUSION, 1, 1, parameters, 2,
                    &result, first, 1, resolved, 8, clamps, 8, stale, 2, strings,
                    sizeof(strings)) == CTEX_RESULT_SUCCESS,
                "generator evaluation failed") ||
        !expect(fabs(first[0] - 0.125F) < 0.00001F && clamps[0].supplied == 3.0 &&
                    clamps[0].resolved == 2.0 &&
                    strcmp(strings + clamps[0].name_offset, "strength") == 0,
                "generator did not apply or report parameter bounds") ||
        !expect(ctex_mesh_map_generator_generate(
                    value->maps, CTEX_MESH_MAP_GENERATOR_AMBIENT_OCCLUSION, 1, 1, parameters, 2,
                    &result, second, 1, resolved, 8, clamps, 8, stale, 2, strings,
                    sizeof(strings)) == CTEX_RESULT_SUCCESS &&
                    memcmp(first, second, sizeof(first)) == 0,
                "generator output is not deterministic")) {
        return 0;
    }
    result.size = CTEX_MESH_MAP_GENERATOR_RESULT_INFO_CURRENT_SIZE;
    return expect(ctex_mesh_map_generator_generate(
                      value->maps, CTEX_MESH_MAP_GENERATOR_SCRATCHES, 1, 1, NULL, 0, &result, NULL,
                      0, NULL, 0, NULL, 0, NULL, 0, NULL, 0) == CTEX_RESULT_MISSING_RESOURCE,
                  "missing generator inputs did not fail loudly");
}

typedef struct provider_state {
    uint8_t pixels[4];
    size_t request_count;
    uint64_t settings_revision;
    uint64_t request_generation;
    double last_progress;
    uint32_t cancel;
    uint32_t fail;
    uint32_t invalid_status;
} provider_state;

static uint32_t provider_can_produce(void* user_data, uint32_t kind) {
    (void)user_data;
    return kind == CTEX_MESH_MAP_AMBIENT_OCCLUSION || kind == CTEX_MESH_MAP_CURVATURE;
}

static uint32_t control_cancelled(void* user_data) { return ((provider_state*)user_data)->cancel; }

static void control_progress(void* user_data, double fraction) {
    ((provider_state*)user_data)->last_progress = fraction;
}

static uint32_t provider_request(void* user_data,
                                 const ctex_mesh_map_bake_request_descriptor* request,
                                 const ctex_mesh_map_bake_control* control,
                                 ctex_mesh_map_bake_output_descriptor* output) {
    provider_state* state = (provider_state*)user_data;
    ++state->request_count;
    state->settings_revision = request->bake_settings_revision;
    state->request_generation = request->request_generation;
    control->report_progress(control->user_data, 0.5);
    if (control->is_cancelled(control->user_data)) {
        return CTEX_MESH_MAP_BAKE_PROVIDER_CANCELLED;
    }
    if (state->fail) {
        output->detail = "fixture provider failure";
        return CTEX_MESH_MAP_BAKE_PROVIDER_FAILED;
    }
    if (state->invalid_status) {
        return 256;
    }
    output->buffer.width = request->width;
    output->buffer.height = request->height;
    output->buffer.component_type = CTEX_TRANSPORT_COMPONENT_UINT8_UNORM;
    output->buffer.component_count = 1;
    output->buffer.row_stride_bytes = request->width;
    output->buffer.pixels = state->pixels;
    output->buffer.pixel_bytes = sizeof(state->pixels);
    return CTEX_MESH_MAP_BAKE_PROVIDER_COMPLETED;
}

static ctex_mesh_map_bake_output_descriptor bake_output(const uint8_t* pixels, uint32_t width,
                                                        uint32_t height) {
    ctex_mesh_map_bake_output_descriptor result = {
        .size = CTEX_MESH_MAP_BAKE_OUTPUT_DESCRIPTOR_CURRENT_SIZE,
        .buffer =
            {
                .size = CTEX_MESH_MAP_PIXEL_BUFFER_DESCRIPTOR_CURRENT_SIZE,
                .width = width,
                .height = height,
                .component_type = CTEX_TRANSPORT_COMPONENT_UINT8_UNORM,
                .component_count = 1,
                .row_stride_bytes = width,
                .pixels = pixels,
                .pixel_bytes = (size_t)width * height,
            },
    };
    return result;
}

static int synchronous_provider_is_host_owned(fixture* value) {
    provider_state state = {.pixels = {0, 64, 128, 255}};
    const ctex_mesh_map_bake_provider_descriptor provider = {
        .size = CTEX_MESH_MAP_BAKE_PROVIDER_DESCRIPTOR_CURRENT_SIZE,
        .name = "fixture-baker",
        .user_data = &state,
        .can_produce = provider_can_produce,
        .request = provider_request,
    };
    const ctex_mesh_map_bake_control_descriptor control = {
        .size = CTEX_MESH_MAP_BAKE_CONTROL_DESCRIPTOR_CURRENT_SIZE,
        .user_data = &state,
        .is_cancelled = control_cancelled,
        .report_progress = control_progress,
    };
    ctex_mesh_map_bake_result_info info = {.size = CTEX_MESH_MAP_BAKE_RESULT_INFO_CURRENT_SIZE};
    ctex_mesh_map_sample_info sample = {.size = CTEX_MESH_MAP_SAMPLE_INFO_CURRENT_SIZE};

    if (!expect(ctex_mesh_map_set_request_bake(value->maps, &provider, CTEX_MESH_MAP_CURVATURE, 2,
                                               2, 17, 23, &control, &info) == CTEX_RESULT_SUCCESS,
                "host bake provider request failed") ||
        !expect(info.status == CTEX_MESH_MAP_BAKE_COMPLETED && info.has_binding == 1 &&
                    info.resolution_mismatch == 1 && state.request_count == 1 &&
                    state.settings_revision == 17 && state.request_generation == 23 &&
                    state.last_progress == 1.0,
                "host bake request metadata or progress is incorrect")) {
        return 0;
    }
    state.pixels[3] = 0;
    if (!expect(ctex_mesh_map_set_sample(value->maps, CTEX_MESH_MAP_CURVATURE, 1.0, 0.0, &sample) ==
                        CTEX_RESULT_SUCCESS &&
                    sample.values[0] == 1.0,
                "host bake output was not copied") ||
        !expect(ctex_mesh_map_set_request_bake(value->maps, &provider, CTEX_MESH_MAP_THICKNESS, 2,
                                               2, 17, 24, &control,
                                               &info) == CTEX_RESULT_UNSUPPORTED_OPERATION &&
                    state.request_count == 1,
                "unsupported bake invoked the provider")) {
        return 0;
    }
    state.cancel = 1;
    info.size = CTEX_MESH_MAP_BAKE_RESULT_INFO_CURRENT_SIZE;
    if (!expect(ctex_mesh_map_set_request_bake(value->maps, &provider,
                                               CTEX_MESH_MAP_AMBIENT_OCCLUSION, 2, 2, 17, 25,
                                               &control, &info) == CTEX_RESULT_CANCELLED &&
                    info.status == CTEX_MESH_MAP_BAKE_CANCELLED,
                "cancelled provider request published output")) {
        return 0;
    }
    state.cancel = 0;
    state.invalid_status = 1;
    info.size = CTEX_MESH_MAP_BAKE_RESULT_INFO_CURRENT_SIZE;
    return expect(
        ctex_mesh_map_set_request_bake(value->maps, &provider, CTEX_MESH_MAP_AMBIENT_OCCLUSION, 2,
                                       2, 17, 26, &control, &info) == CTEX_RESULT_INTERNAL_ERROR &&
            info.status == CTEX_MESH_MAP_BAKE_REQUEST_PROVIDER_FAILED,
        "out-of-range provider status was accepted or truncated");
}

static int complete_request(ctex_mesh_map_bake_session* session,
                            ctex_mesh_map_bake_request_token* token,
                            const ctex_mesh_map_bake_output_descriptor* output,
                            uint32_t disposition) {
    ctex_mesh_map_bake_completion_info info = {.size =
                                                   CTEX_MESH_MAP_BAKE_COMPLETION_INFO_CURRENT_SIZE};
    return ctex_mesh_map_bake_session_complete(session, token, output, &info) ==
               CTEX_RESULT_SUCCESS &&
           info.disposition == disposition;
}

static int asynchronous_tokens_are_versioned(fixture* value) {
    static const uint8_t dark[] = {32};
    static const uint8_t light[] = {255};
    const ctex_mesh_map_bake_output_descriptor dark_output = bake_output(dark, 1, 1);
    const ctex_mesh_map_bake_output_descriptor light_output = bake_output(light, 1, 1);
    const ctex_mesh_map_bake_output_descriptor invalid_output = bake_output(light, 2, 1);
    ctex_mesh_map_bake_session* session = NULL;
    ctex_mesh_map_bake_request_token* old = NULL;
    ctex_mesh_map_bake_request_token* current = NULL;
    ctex_mesh_map_bake_request_token* cancelled = NULL;
    ctex_mesh_map_bake_request_token* replacement = NULL;
    ctex_mesh_map_bake_request_token* malformed = NULL;
    ctex_mesh_map_bake_token_info token_info = {.size = CTEX_MESH_MAP_BAKE_TOKEN_INFO_CURRENT_SIZE};
    ctex_mesh_map_bake_session_info session_info = {
        .size = CTEX_MESH_MAP_BAKE_SESSION_INFO_CURRENT_SIZE};
    ctex_mesh_map_bake_settings_edit_info edit = {
        .size = CTEX_MESH_MAP_BAKE_SETTINGS_EDIT_INFO_CURRENT_SIZE};
    ctex_mesh_map_bake_settings_undo_info undo = {
        .size = CTEX_MESH_MAP_BAKE_SETTINGS_UNDO_INFO_CURRENT_SIZE};
    char texture_set_id[128] = {0};
    char uv_set[16] = {0};
    uint32_t was_cancelled = 0;
    int passed = 0;

    if (!expect(ctex_mesh_map_bake_session_create(value->maps, 7, &session) == CTEX_RESULT_SUCCESS,
                "async bake session creation failed") ||
        !expect(ctex_mesh_map_bake_session_begin(session, CTEX_MESH_MAP_AMBIENT_OCCLUSION, 1, 1,
                                                 &old) == CTEX_RESULT_SUCCESS &&
                    ctex_mesh_map_bake_session_begin(session, CTEX_MESH_MAP_AMBIENT_OCCLUSION, 1, 1,
                                                     &current) == CTEX_RESULT_SUCCESS,
                "async bake request creation failed") ||
        !expect(ctex_mesh_map_bake_request_token_get_info(
                    current, &token_info, texture_set_id, sizeof(texture_set_id), uv_set,
                    sizeof(uv_set), NULL, 0) == CTEX_RESULT_SUCCESS &&
                    token_info.session_identity != 0 && token_info.bake_settings_revision == 7 &&
                    token_info.request_generation != 0 &&
                    strcmp(texture_set_id, value->texture_set_id) == 0 &&
                    strcmp(uv_set, "uv0") == 0,
                "async bake token omitted its identity") ||
        !expect(
            complete_request(session, old, &dark_output, CTEX_MESH_MAP_BAKE_STALE) &&
                complete_request(session, current, &light_output, CTEX_MESH_MAP_BAKE_BOUND) &&
                complete_request(session, current, &dark_output, CTEX_MESH_MAP_BAKE_UNKNOWN_TOKEN),
            "superseded or duplicate async completion was accepted") ||
        !expect(ctex_mesh_map_bake_session_begin(session, CTEX_MESH_MAP_THICKNESS, 1, 1,
                                                 &cancelled) == CTEX_RESULT_SUCCESS &&
                    ctex_mesh_map_bake_session_cancel(session, cancelled, &was_cancelled) ==
                        CTEX_RESULT_SUCCESS &&
                    was_cancelled == 1 &&
                    complete_request(session, cancelled, &dark_output,
                                     CTEX_MESH_MAP_BAKE_COMPLETION_CANCELLED),
                "cancelled async completion was published") ||
        !expect(
            ctex_mesh_map_bake_session_edit_settings(session, 8, &edit) == CTEX_RESULT_SUCCESS &&
                edit.previous_revision == 7 && edit.current_revision == 8 &&
                ctex_mesh_map_bake_session_begin(session, CTEX_MESH_MAP_CURVATURE, 1, 1,
                                                 &replacement) == CTEX_RESULT_SUCCESS &&
                complete_request(session, replacement, &dark_output, CTEX_MESH_MAP_BAKE_BOUND),
            "settings edit did not accept its current completion") ||
        !expect(ctex_mesh_map_bake_session_begin(session, CTEX_MESH_MAP_UV_DENSITY, 1, 1,
                                                 &malformed) == CTEX_RESULT_SUCCESS &&
                    complete_request(session, malformed, &invalid_output,
                                     CTEX_MESH_MAP_BAKE_INVALID_OUTPUT),
                "malformed async output was bound") ||
        !expect(ctex_mesh_map_bake_session_undo_settings(session, &undo) == CTEX_RESULT_SUCCESS,
                "settings/map undo failed") ||
        !expect(undo.restored == 1 && undo.previous_revision == 8 && undo.restored_revision == 7 &&
                    undo.restored_map_count == 2,
                "settings/map undo report is incorrect") ||
        !expect(
            ctex_mesh_map_bake_session_get_info(session, &session_info) == CTEX_RESULT_SUCCESS &&
                session_info.settings_revision == 7 && session_info.pending_request_count == 0,
            "settings/map undo left incorrect session state")) {
        goto cleanup;
    }
    passed = 1;

cleanup:
    ctex_mesh_map_bake_request_token_destroy(malformed);
    ctex_mesh_map_bake_request_token_destroy(replacement);
    ctex_mesh_map_bake_request_token_destroy(cancelled);
    ctex_mesh_map_bake_request_token_destroy(current);
    ctex_mesh_map_bake_request_token_destroy(old);
    ctex_mesh_map_bake_session_destroy(session);
    return passed;
}

int main(void) {
    fixture value = {0};
    int passed = expect(create_fixture(&value), "mesh-map execution fixture creation failed");
    if (passed) {
        passed = generator_catalogue_is_public() &&
                 generator_output_is_bounded_and_deterministic(&value) &&
                 synchronous_provider_is_host_owned(&value) &&
                 asynchronous_tokens_are_versioned(&value);
    }
    destroy_fixture(&value);
    return passed ? 0 : 1;
}
