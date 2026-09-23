//! End-to-end sys-binding workflow: document, paint, material, mesh map,
//! smart material and export planning.

use std::ffi::{c_void, CStr};
use std::ptr::{null, null_mut};

use cybertexel_sys as sys;

fn size_of<T>() -> u32 {
    std::mem::size_of::<T>() as u32
}

unsafe fn check(result: sys::ctex_result, operation: &str) {
    if result != sys::CTEX_RESULT_SUCCESS {
        let diagnostic = CStr::from_ptr(sys::ctex_get_last_diagnostic()).to_string_lossy();
        panic!("{operation} failed: {diagnostic}");
    }
}

fn hex(bytes: &[u8]) -> String {
    const DIGITS: &[u8; 16] = b"0123456789abcdef";
    let mut result = String::with_capacity(bytes.len() * 2);
    for byte in bytes {
        result.push(DIGITS[(byte >> 4) as usize] as char);
        result.push(DIGITS[(byte & 0xf) as usize] as char);
    }
    result
}

unsafe extern "C" fn receive_report(
    json: *const std::os::raw::c_char,
    size: usize,
    state: *mut c_void,
) -> sys::ctex_result {
    let bytes = std::slice::from_raw_parts(json.cast::<u8>(), size);
    let marker = b"\"dry_run\":true";
    *state.cast::<bool>() = bytes.windows(marker.len()).any(|part| part == marker);
    sys::CTEX_RESULT_SUCCESS
}

unsafe fn texture_set_identifier(document: *mut sys::ctex_document) -> Vec<i8> {
    let mut required = 0;
    let mut count = 0;
    check(
        sys::ctex_document_get_texture_set_ids(document, null_mut(), 0, &mut required, &mut count),
        "texture-set identifier sizing",
    );
    assert_eq!(count, 1);
    let mut bytes = vec![0_i8; required];
    check(
        sys::ctex_document_get_texture_set_ids(
            document,
            bytes.as_mut_ptr(),
            bytes.len(),
            &mut required,
            &mut count,
        ),
        "texture-set identifier query",
    );
    bytes
}

unsafe fn paint(document: *mut sys::ctex_document, texture_set: *const i8) {
    let mut validation = sys::ctex_paint_parameter_validation_info {
        size: size_of::<sys::ctex_paint_parameter_validation_info>(),
        ..Default::default()
    };
    check(
        sys::ctex_paint_validate_parameter(
            c"stroke.radius".as_ptr(),
            sys::ctex_paint_parameter_context_CTEX_PAINT_PARAMETER_CONTEXT_GENERAL,
            2_000_000.0,
            &mut validation,
        ),
        "paint parameter validation",
    );
    assert_eq!(validation.resolved, 1_000_000.0);
    assert_eq!(validation.clamped, 1);
    check(
        sys::ctex_texture_set_set_channel_enabled(
            document,
            texture_set,
            c"pbr.base_color".as_ptr(),
            1,
            8,
        ),
        "enable paint channel",
    );
    let mut session = null_mut();
    check(
        sys::ctex_paint_preview_session_create(
            document,
            texture_set,
            c"pbr.base_color".as_ptr(),
            &mut session,
        ),
        "paint preview creation",
    );
    let pixel = [25_u8, 50, 75];
    check(
        sys::ctex_paint_preview_session_write_pixel(
            session,
            1,
            1,
            pixel.as_ptr().cast(),
            pixel.len(),
        ),
        "paint pixel",
    );
    let mut coverage = [0_u8; 16];
    coverage[5] = 1;
    let mut info = sys::ctex_paint_preview_info {
        size: size_of::<sys::ctex_paint_preview_info>(),
        ..Default::default()
    };
    check(
        sys::ctex_paint_preview_session_finalize(
            session,
            coverage.as_ptr(),
            coverage.len(),
            0,
            &mut info,
        ),
        "paint finalize",
    );
    check(
        sys::ctex_paint_preview_session_commit(session, &mut info),
        "paint commit",
    );
    sys::ctex_paint_preview_session_destroy(session);
}

unsafe fn author_material() -> Vec<u8> {
    let mut info = sys::ctex_material_graph_info {
        size: size_of::<sys::ctex_material_graph_info>(),
        ..Default::default()
    };
    check(
        sys::ctex_material_graph_create_default(&mut info, null_mut(), 0, null_mut(), 0),
        "material graph sizing",
    );
    let mut graph = vec![0_u8; info.canonical_size];
    check(
        sys::ctex_material_graph_create_default(
            &mut info,
            graph.as_mut_ptr().cast(),
            graph.len(),
            null_mut(),
            0,
        ),
        "material graph creation",
    );
    assert_eq!(info.output_channel_count, 9);
    graph
}

unsafe fn bind_mesh_map(document: *mut sys::ctex_document, texture_set: *const i8) {
    let positions = [
        sys::ctex_vec3f {
            x: 0.0,
            y: 0.0,
            z: 0.0,
        },
        sys::ctex_vec3f {
            x: 1.0,
            y: 0.0,
            z: 0.0,
        },
        sys::ctex_vec3f {
            x: 0.0,
            y: 1.0,
            z: 0.0,
        },
    ];
    let normals = [sys::ctex_vec3f {
        x: 0.0,
        y: 0.0,
        z: 1.0,
    }; 3];
    let uv_values = [
        sys::ctex_vec2f { x: 0.0, y: 0.0 },
        sys::ctex_vec2f { x: 1.0, y: 0.0 },
        sys::ctex_vec2f { x: 0.0, y: 1.0 },
    ];
    let triangles = [0_u32, 1, 2];
    let face_partition = [0_u32];
    let face_material = [1_u32];
    let uv = sys::ctex_uv_set_descriptor {
        size: size_of::<sys::ctex_uv_set_descriptor>(),
        name: c"uv0".as_ptr(),
        values: uv_values.as_ptr(),
        value_count: uv_values.len(),
    };
    let partition = sys::ctex_mesh_partition_descriptor {
        size: size_of::<sys::ctex_mesh_partition_descriptor>(),
        kind: sys::ctex_partition_source_kind_CTEX_PARTITION_SOURCE_MATERIAL,
        stable_key: c"body".as_ptr(),
        display_name: c"Body".as_ptr(),
    };
    let descriptor = sys::ctex_mesh_descriptor {
        size: size_of::<sys::ctex_mesh_descriptor>(),
        positions: positions.as_ptr(),
        position_count: positions.len(),
        normals: normals.as_ptr(),
        normal_count: normals.len(),
        vertex_colors: null(),
        vertex_color_count: 0,
        triangle_indices: triangles.as_ptr(),
        triangle_index_count: triangles.len(),
        uv_sets: &uv,
        uv_set_count: 1,
        default_uv_set: c"uv0".as_ptr(),
        partitions: &partition,
        partition_count: 1,
        face_partition_indices: face_partition.as_ptr(),
        face_partition_index_count: 1,
        face_material_ids: face_material.as_ptr(),
        face_material_id_count: 1,
    };
    let mut mesh = null_mut();
    check(
        sys::ctex_mesh_create(&descriptor, &mut mesh),
        "mesh creation",
    );
    let mut maps = null_mut();
    check(
        sys::ctex_mesh_map_set_create(document, texture_set, mesh, &mut maps),
        "mesh-map set creation",
    );
    let pixels = [0_u8, 64, 128, 255];
    let descriptor = sys::ctex_mesh_map_import_descriptor {
        size: size_of::<sys::ctex_mesh_map_import_descriptor>(),
        kind: sys::ctex_mesh_map_kind_CTEX_MESH_MAP_AMBIENT_OCCLUSION,
        channel_meaning: sys::ctex_mesh_map_channel_meaning_CTEX_MESH_MAP_SCALAR_DATA,
        color_space: sys::ctex_color_space_CTEX_COLOR_SPACE_LINEAR_REC709,
        has_normal_convention: 0,
        normal_convention: 0,
        tangent_frame: null(),
        buffer: sys::ctex_mesh_map_pixel_buffer_descriptor {
            size: size_of::<sys::ctex_mesh_map_pixel_buffer_descriptor>(),
            width: 2,
            height: 2,
            component_type: sys::ctex_transport_component_type_CTEX_TRANSPORT_COMPONENT_UINT8_UNORM,
            component_count: 1,
            row_stride_bytes: 2,
            pixels: pixels.as_ptr().cast(),
            pixel_bytes: pixels.len(),
        },
    };
    let mut info = sys::ctex_mesh_map_import_info {
        size: size_of::<sys::ctex_mesh_map_import_info>(),
        ..Default::default()
    };
    check(
        sys::ctex_mesh_map_set_import_external(maps, &descriptor, &mut info),
        "mesh-map import",
    );
    assert_eq!(info.map_width, 2);
    sys::ctex_mesh_map_set_destroy(maps);
    sys::ctex_mesh_destroy(mesh);
}

unsafe fn apply_smart_material(
    document: *mut sys::ctex_document,
    texture_set: *const i8,
    graph: &[u8],
) {
    let material = format!(
        "CTEX_SMART_MATERIAL\t6\nPRESET\t{}\t{}\nENTRY\t0\t{}\t\t{}\t1\t3ff0000000000000\t{}\t0\nEND\n",
        hex(b"examples/rust-material"),
        hex(b"Rust material"),
        hex(b"surface"),
        hex(b"Surface"),
        hex(graph),
    );
    let mut info = sys::ctex_preset_application_info {
        size: size_of::<sys::ctex_preset_application_info>(),
        ..Default::default()
    };
    check(
        sys::ctex_texture_set_apply_smart_material(
            document,
            texture_set,
            material.as_ptr().cast(),
            material.len(),
            c"rust-example".as_ptr(),
            &mut info,
        ),
        "smart material application",
    );
    assert_eq!(info.entry_count, 1);
}

unsafe fn plan_export() {
    let layer = sys::ctex_texture_export_layer_source_descriptor {
        size: size_of::<sys::ctex_texture_export_layer_source_descriptor>(),
        identifier: c"surface".as_ptr(),
        display_name: c"Surface".as_ptr(),
        parent_identifier: c"".as_ptr(),
        kind: sys::ctex_texture_export_layer_kind_CTEX_TEXTURE_EXPORT_LAYER_CONTENT,
        visible: 1,
    };
    let source = sys::ctex_texture_export_texture_set_source_descriptor {
        size: size_of::<sys::ctex_texture_export_texture_set_source_descriptor>(),
        identifier: c"body".as_ptr(),
        display_name: c"Body".as_ptr(),
        width: 4,
        height: 4,
        occupied_udim_tiles: null(),
        occupied_udim_tile_count: 0,
        layers: &layer,
        layer_count: 1,
    };
    let catalogue = sys::ctex_texture_export_catalogue_descriptor {
        size: size_of::<sys::ctex_texture_export_catalogue_descriptor>(),
        project_name: c"binding-example".as_ptr(),
        texture_sets: &source,
        texture_set_count: 1,
        atlases: null(),
        atlas_count: 0,
    };
    let preset = sys::ctex_texture_export_preset_descriptor {
        size: size_of::<sys::ctex_texture_export_preset_descriptor>(),
        identifier: c"pbr-individual".as_ptr(),
        display_name: null(),
        textures: null(),
        texture_count: 0,
    };
    let options = sys::ctex_texture_export_options_descriptor {
        size: size_of::<sys::ctex_texture_export_options_descriptor>(),
        plan: null(),
        padding_radius: 0,
        jpeg_quality: 90,
        dry_run: 1,
    };
    let mut report_received = false;
    let callbacks = sys::ctex_texture_export_callbacks_descriptor {
        size: size_of::<sys::ctex_texture_export_callbacks_descriptor>(),
        source: None,
        output: None,
        report: Some(receive_report),
        progress: None,
        cancel: None,
        user_data: (&mut report_received as *mut bool).cast(),
    };
    let mut info = sys::ctex_texture_export_info {
        size: size_of::<sys::ctex_texture_export_info>(),
        ..Default::default()
    };
    check(
        sys::ctex_texture_export_run(&catalogue, &preset, &options, &callbacks, &mut info),
        "texture export dry run",
    );
    assert!(report_received && info.planned_output_count > 0 && info.encoded_output_count == 0);
}

fn main() {
    unsafe {
        let mut document = null_mut();
        check(
            sys::ctex_document_create(&mut document),
            "document creation",
        );
        let descriptor = sys::ctex_texture_set_descriptor {
            size: size_of::<sys::ctex_texture_set_descriptor>(),
            display_name: c"Body".as_ptr(),
            partition_kind: sys::ctex_partition_source_kind_CTEX_PARTITION_SOURCE_MATERIAL,
            partition_key: c"body".as_ptr(),
            uv_set: c"uv0".as_ptr(),
            width: 4,
            height: 4,
            default_bit_depth: 8,
            udim_tiling: 0,
        };
        check(
            sys::ctex_document_create_texture_set(document, &descriptor),
            "texture-set creation",
        );
        let texture_set_identifier = texture_set_identifier(document);
        let texture_set = texture_set_identifier.as_ptr();
        paint(document, texture_set);
        let graph = author_material();
        bind_mesh_map(document, texture_set);
        apply_smart_material(document, texture_set, &graph);
        plan_export();
        sys::ctex_document_destroy(document);
    }
    println!("ok: Rust binding workflow example completed");
}
