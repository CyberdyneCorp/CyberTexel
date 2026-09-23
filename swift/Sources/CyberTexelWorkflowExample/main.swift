import CyberTexelC
import Foundation

private enum WorkflowError: Error, CustomStringConvertible {
  case native(operation: String, diagnostic: String)

  var description: String {
    switch self {
    case let .native(operation, diagnostic):
      return "\(operation) failed: \(diagnostic)"
    }
  }
}

private final class CStringPool {
  private var pointers: [UnsafeMutablePointer<CChar>] = []

  func add(_ value: String) -> UnsafePointer<CChar> {
    guard let pointer = strdup(value) else { fatalError("unable to allocate C string") }
    pointers.append(pointer)
    return UnsafePointer(pointer)
  }

  deinit {
    for pointer in pointers { free(pointer) }
  }
}

private func sizeOf<T>(_ type: T.Type) -> UInt32 {
  UInt32(MemoryLayout<T>.size)
}

private func check(_ result: ctex_result, _ operation: String) throws {
  guard result == CTEX_RESULT_SUCCESS else {
    let diagnostic = ctex_get_last_diagnostic().map(String.init(cString:)) ?? "unknown error"
    throw WorkflowError.native(operation: operation, diagnostic: diagnostic)
  }
}

private func textureSetIdentifier(_ document: OpaquePointer) throws -> String {
  var required = 0
  var count = 0
  try check(
    ctex_document_get_texture_set_ids(document, nil, 0, &required, &count),
    "texture-set identifier sizing")
  precondition(count == 1)
  var bytes = [CChar](repeating: 0, count: required)
  try bytes.withUnsafeMutableBufferPointer { buffer in
    try check(
      ctex_document_get_texture_set_ids(
        document, buffer.baseAddress, buffer.count, &required, &count),
      "texture-set identifier query")
  }
  return String(cString: bytes)
}

private func paint(
  document: OpaquePointer, textureSet: UnsafePointer<CChar>, strings: CStringPool
) throws {
  var validation = ctex_paint_parameter_validation_info()
  validation.size = sizeOf(ctex_paint_parameter_validation_info.self)
  try check(
    ctex_paint_validate_parameter(
      strings.add("stroke.radius"), 0, 2_000_000, &validation),
    "paint parameter validation")
  precondition(validation.resolved == 1_000_000 && validation.clamped == 1)
  try check(
    ctex_texture_set_set_channel_enabled(
      document, textureSet, strings.add("pbr.base_color"), 1, 8),
    "enable paint channel")
  var session: OpaquePointer?
  try check(
    ctex_paint_preview_session_create(
      document, textureSet, strings.add("pbr.base_color"), &session),
    "paint preview creation")
  guard let session else { fatalError("paint preview returned no handle") }
  defer { ctex_paint_preview_session_destroy(session) }

  let pixel: [UInt8] = [25, 50, 75]
  try pixel.withUnsafeBytes { bytes in
    try check(
      ctex_paint_preview_session_write_pixel(
        session, 1, 1, bytes.baseAddress, bytes.count),
      "paint pixel")
  }
  var coverage = [UInt8](repeating: 0, count: 16)
  coverage[5] = 1
  var info = ctex_paint_preview_info()
  info.size = sizeOf(ctex_paint_preview_info.self)
  try coverage.withUnsafeBufferPointer { buffer in
    try check(
      ctex_paint_preview_session_finalize(
        session, buffer.baseAddress, buffer.count, 0, &info),
      "paint finalize")
  }
  try check(ctex_paint_preview_session_commit(session, &info), "paint commit")
}

private func authorMaterial() throws -> [UInt8] {
  var info = ctex_material_graph_info()
  info.size = sizeOf(ctex_material_graph_info.self)
  try check(
    ctex_material_graph_create_default(&info, nil, 0, nil, 0),
    "material graph sizing")
  var graph = [UInt8](repeating: 0, count: info.canonical_size)
  try graph.withUnsafeMutableBytes { bytes in
    try check(
      ctex_material_graph_create_default(
        &info, bytes.baseAddress, bytes.count, nil, 0),
      "material graph creation")
  }
  precondition(info.output_channel_count == 9)
  return graph
}

private func bindMeshMap(
  document: OpaquePointer, textureSet: UnsafePointer<CChar>, strings: CStringPool
) throws {
  let positions = [
    ctex_vec3f(x: 0, y: 0, z: 0), ctex_vec3f(x: 1, y: 0, z: 0),
    ctex_vec3f(x: 0, y: 1, z: 0),
  ]
  let normals = Array(repeating: ctex_vec3f(x: 0, y: 0, z: 1), count: 3)
  let uvValues = [
    ctex_vec2f(x: 0, y: 0), ctex_vec2f(x: 1, y: 0), ctex_vec2f(x: 0, y: 1),
  ]
  let triangles: [UInt32] = [0, 1, 2]
  let facePartition: [UInt32] = [0]
  let faceMaterial: [UInt32] = [1]
  var mesh: OpaquePointer?

  try positions.withUnsafeBufferPointer { positions in
    try normals.withUnsafeBufferPointer { normals in
      try uvValues.withUnsafeBufferPointer { uvValues in
        try triangles.withUnsafeBufferPointer { triangles in
          try facePartition.withUnsafeBufferPointer { facePartition in
            try faceMaterial.withUnsafeBufferPointer { faceMaterial in
              var uv = ctex_uv_set_descriptor()
              uv.size = sizeOf(ctex_uv_set_descriptor.self)
              uv.name = strings.add("uv0")
              uv.values = uvValues.baseAddress
              uv.value_count = uvValues.count
              var partition = ctex_mesh_partition_descriptor()
              partition.size = sizeOf(ctex_mesh_partition_descriptor.self)
              partition.kind = 0
              partition.stable_key = strings.add("body")
              partition.display_name = strings.add("Body")
              try withUnsafePointer(to: &uv) { uv in
                try withUnsafePointer(to: &partition) { partition in
                  var descriptor = ctex_mesh_descriptor()
                  descriptor.size = sizeOf(ctex_mesh_descriptor.self)
                  descriptor.positions = positions.baseAddress
                  descriptor.position_count = positions.count
                  descriptor.normals = normals.baseAddress
                  descriptor.normal_count = normals.count
                  descriptor.triangle_indices = triangles.baseAddress
                  descriptor.triangle_index_count = triangles.count
                  descriptor.uv_sets = uv
                  descriptor.uv_set_count = 1
                  descriptor.default_uv_set = strings.add("uv0")
                  descriptor.partitions = partition
                  descriptor.partition_count = 1
                  descriptor.face_partition_indices = facePartition.baseAddress
                  descriptor.face_partition_index_count = facePartition.count
                  descriptor.face_material_ids = faceMaterial.baseAddress
                  descriptor.face_material_id_count = faceMaterial.count
                  try check(ctex_mesh_create(&descriptor, &mesh), "mesh creation")
                }
              }
            }
          }
        }
      }
    }
  }
  guard let mesh else { fatalError("mesh creation returned no handle") }
  defer { ctex_mesh_destroy(mesh) }

  var maps: OpaquePointer?
  try check(
    ctex_mesh_map_set_create(document, textureSet, mesh, &maps),
    "mesh-map set creation")
  guard let maps else { fatalError("mesh-map set creation returned no handle") }
  defer { ctex_mesh_map_set_destroy(maps) }
  let pixels: [UInt8] = [0, 64, 128, 255]
  try pixels.withUnsafeBytes { bytes in
    var descriptor = ctex_mesh_map_import_descriptor()
    descriptor.size = sizeOf(ctex_mesh_map_import_descriptor.self)
    descriptor.kind = 3
    descriptor.channel_meaning = 0
    descriptor.color_space = 0
    descriptor.buffer.size = sizeOf(ctex_mesh_map_pixel_buffer_descriptor.self)
    descriptor.buffer.width = 2
    descriptor.buffer.height = 2
    descriptor.buffer.component_type = 0
    descriptor.buffer.component_count = 1
    descriptor.buffer.row_stride_bytes = 2
    descriptor.buffer.pixels = bytes.baseAddress
    descriptor.buffer.pixel_bytes = bytes.count
    var info = ctex_mesh_map_import_info()
    info.size = sizeOf(ctex_mesh_map_import_info.self)
    try check(
      ctex_mesh_map_set_import_external(maps, &descriptor, &info),
      "mesh-map import")
    precondition(info.map_width == 2 && info.map_height == 2)
  }
}

private func hex(_ bytes: [UInt8]) -> String {
  bytes.map { String(format: "%02x", $0) }.joined()
}

private func applySmartMaterial(
  document: OpaquePointer, textureSet: UnsafePointer<CChar>, graph: [UInt8],
  strings: CStringPool
) throws {
  let material =
    "CTEX_SMART_MATERIAL\t6\nPRESET\t\(hex(Array("examples/swift-material".utf8)))\t"
    + "\(hex(Array("Swift material".utf8)))\nENTRY\t0\t\(hex(Array("surface".utf8)))\t\t"
    + "\(hex(Array("Surface".utf8)))\t1\t3ff0000000000000\t\(hex(graph))\t0\nEND\n"
  let bytes = Array(material.utf8)
  var info = ctex_preset_application_info()
  info.size = sizeOf(ctex_preset_application_info.self)
  try bytes.withUnsafeBytes { bytes in
    try check(
      ctex_texture_set_apply_smart_material(
        document, textureSet, bytes.baseAddress, bytes.count,
        strings.add("swift-example"), &info),
      "smart material application")
  }
  precondition(info.entry_count == 1)
}

private func receiveReport(
  _ json: UnsafePointer<CChar>?, _ size: Int, _ state: UnsafeMutableRawPointer?
) -> ctex_result {
  guard let json, let state else { return CTEX_RESULT_INVALID_ARGUMENT }
  let report = String(decoding: UnsafeRawBufferPointer(start: json, count: size), as: UTF8.self)
  state.assumingMemoryBound(to: Bool.self).pointee = report.contains("\"dry_run\":true")
  return CTEX_RESULT_SUCCESS
}

private func planExport(strings: CStringPool) throws {
  var layer = ctex_texture_export_layer_source_descriptor()
  layer.size = sizeOf(ctex_texture_export_layer_source_descriptor.self)
  layer.identifier = strings.add("surface")
  layer.display_name = strings.add("Surface")
  layer.parent_identifier = strings.add("")
  layer.kind = 0
  layer.visible = 1
  try withUnsafePointer(to: &layer) { layer in
    var source = ctex_texture_export_texture_set_source_descriptor()
    source.size = sizeOf(ctex_texture_export_texture_set_source_descriptor.self)
    source.identifier = strings.add("body")
    source.display_name = strings.add("Body")
    source.width = 4
    source.height = 4
    source.layers = layer
    source.layer_count = 1
    try withUnsafePointer(to: &source) { source in
      var catalogue = ctex_texture_export_catalogue_descriptor()
      catalogue.size = sizeOf(ctex_texture_export_catalogue_descriptor.self)
      catalogue.project_name = strings.add("binding-example")
      catalogue.texture_sets = source
      catalogue.texture_set_count = 1
      var preset = ctex_texture_export_preset_descriptor()
      preset.size = sizeOf(ctex_texture_export_preset_descriptor.self)
      preset.identifier = strings.add("pbr-individual")
      var options = ctex_texture_export_options_descriptor()
      options.size = sizeOf(ctex_texture_export_options_descriptor.self)
      options.jpeg_quality = 90
      options.dry_run = 1
      var reportReceived = false
      var info = ctex_texture_export_info()
      info.size = sizeOf(ctex_texture_export_info.self)
      try withUnsafeMutablePointer(to: &reportReceived) { state in
        var callbacks = ctex_texture_export_callbacks_descriptor()
        callbacks.size = sizeOf(ctex_texture_export_callbacks_descriptor.self)
        callbacks.report = receiveReport
        callbacks.user_data = UnsafeMutableRawPointer(state)
        try check(
          ctex_texture_export_run(
            &catalogue, &preset, &options, &callbacks, &info),
          "texture export dry run")
      }
      precondition(
        reportReceived && info.planned_output_count > 0 && info.encoded_output_count == 0)
    }
  }
}

private func runWorkflow() throws {
  let strings = CStringPool()
  var document: OpaquePointer?
  try check(ctex_document_create(&document), "document creation")
  guard let document else { fatalError("document creation returned no handle") }
  defer { ctex_document_destroy(document) }
  var descriptor = ctex_texture_set_descriptor()
  descriptor.size = sizeOf(ctex_texture_set_descriptor.self)
  descriptor.display_name = strings.add("Body")
  descriptor.partition_kind = 0
  descriptor.partition_key = strings.add("body")
  descriptor.uv_set = strings.add("uv0")
  descriptor.width = 4
  descriptor.height = 4
  descriptor.default_bit_depth = 8
  try check(
    ctex_document_create_texture_set(document, &descriptor),
    "texture-set creation")
  let textureSet = strings.add(try textureSetIdentifier(document))
  try paint(document: document, textureSet: textureSet, strings: strings)
  let graph = try authorMaterial()
  try bindMeshMap(document: document, textureSet: textureSet, strings: strings)
  try applySmartMaterial(
    document: document, textureSet: textureSet, graph: graph, strings: strings)
  try planExport(strings: strings)
}

do {
  try runWorkflow()
  print("ok: Swift binding workflow example completed")
} catch {
  fputs("error: \(error)\n", stderr)
  exit(EXIT_FAILURE)
}
