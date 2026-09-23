// Mobile MSL reference host.
//
// CyberTexel does not own a GPU device. It emits shader source and an ordered
// pass plan, and the host runs them on the device it already has. This is the
// smallest program that is honestly such a host on Metal: it asks the library
// for an MSL material program, executes the plan on a real MTLDevice, reports
// completion so the library publishes the revision, and then drives the
// explicit transport path — cursor, snapshot, tile layout, host readback.
//
// It builds for iPadOS and for macOS. The iPad is the named mobile reference
// device; macOS lets CI run the same route rather than only link it.
//
// Exit codes:
//   0  the route ran on a device and every assertion held
//   2  the route failed
//   3  no Metal device was available, so the run is unmeasured (never a pass)

import CyberTexel
import Foundation
import Metal

let stableIdentity = "reference-host/mobile-msl"
let outputIdentity = "material-output"
let semantic = "pbr.base_color"
let unmeasured: Int32 = 3

struct HostFailure: Error {
  let message: String
  let isUnmeasured: Bool

  init(_ message: String, unmeasured: Bool = false) {
    self.message = message
    self.isUnmeasured = unmeasured
  }
}

/// A text shader artifact crosses the C ABI as bytes with a terminating NUL.
/// The Metal compiler rejects that byte, so the host trims it rather than
/// handing the driver a string the library never meant as source.
func shaderSource(_ artifact: [UInt8], stage: String) throws -> String {
  var bytes = artifact
  while bytes.last == 0 { bytes.removeLast() }
  guard let text = String(bytes: bytes, encoding: .utf8) else {
    throw HostFailure("\(stage) artifact is not UTF-8 MSL")
  }
  return text
}

func metalFormat(_ name: String) throws -> MTLPixelFormat {
  switch name {
  case "rgba8_unorm": return .rgba8Unorm
  case "rgba8_unorm_srgb": return .rgba8Unorm_srgb
  case "rgba16_float": return .rgba16Float
  case "rgba32_float": return .rgba32Float
  case "r8_unorm": return .r8Unorm
  case "r16_float": return .r16Float
  case "r32_float": return .r32Float
  default:
    throw HostFailure("pass plan names a texture format this host does not carry: \(name)")
  }
}

func metalVertexFormat(_ name: String) throws -> MTLVertexFormat {
  switch name {
  case "float32": return .float
  case "float32x2": return .float2
  case "float32x3": return .float3
  case "float32x4": return .float4
  default:
    throw HostFailure("pass plan names a vertex format this host does not carry: \(name)")
  }
}

func metalPrimitive(_ name: String) throws -> MTLPrimitiveType {
  switch name {
  case "triangle_list": return .triangle
  case "triangle_strip": return .triangleStrip
  case "line_list": return .line
  case "point_list": return .point
  default:
    throw HostFailure("pass plan names a topology this host does not carry: \(name)")
  }
}

/// Execute one render pass of the plan and return the rendered texels.
func execute(
  device: MTLDevice, queue: MTLCommandQueue, plan: PassPlan, source: String
) throws -> (pixels: [UInt8], width: Int, height: Int) {
  guard let pass = plan.passes.first else {
    throw HostFailure("pass plan carries no pass")
  }
  guard pass.kind == "render" else {
    throw HostFailure("this host runs render passes; the plan names \(pass.kind)")
  }
  guard pass.command.kind == "draw", !pass.command.indexed else {
    throw HostFailure("this host runs non-indexed draws; the plan names \(pass.command.kind)")
  }
  guard pass.textureBindings.isEmpty, pass.samplerBindings.isEmpty, pass.uniformBlocks.isEmpty
  else {
    throw HostFailure("this host does not carry bound textures, samplers or uniforms yet")
  }
  guard pass.depthTarget == nil else {
    throw HostFailure("this host does not carry a depth target yet")
  }
  guard let target = pass.renderTargets.first else {
    throw HostFailure("render pass names no render target")
  }
  guard
    let resource = plan.resources.first(where: {
      $0.version.logicalID == target.resource.logicalID
    })
  else {
    throw HostFailure("no resource declares \(target.resource.logicalID)")
  }
  guard resource.extent.width == target.width, resource.extent.height == target.height else {
    throw HostFailure("resource extent and render target size disagree")
  }
  guard resource.format == target.format else {
    throw HostFailure("resource format and render target format disagree")
  }

  // The library emits one MSL module carrying both stages.
  let library = try device.makeLibrary(source: source, options: nil)
  guard let vertexFunction = library.makeFunction(name: pass.entryPoints.vertex) else {
    throw HostFailure("MSL module has no vertex entry point \(pass.entryPoints.vertex)")
  }
  guard let fragmentFunction = library.makeFunction(name: pass.entryPoints.fragment) else {
    throw HostFailure("MSL module has no fragment entry point \(pass.entryPoints.fragment)")
  }

  // The plan declares the vertex layout; the host owns the geometry. A material
  // preview is one full-screen triangle in clip space, which the emitted vertex
  // stage maps to the 0..1 material coordinate.
  guard pass.vertexBuffers.count == 1 else {
    throw HostFailure(
      "this host supplies one vertex buffer; the plan declares \(pass.vertexBuffers.count)")
  }
  let declared = pass.vertexBuffers[0]
  guard declared.attributes.contains(where: { $0.semantic == "position" }) else {
    throw HostFailure("the declared vertex layout carries no position attribute")
  }
  guard declared.stride == 8 else {
    throw HostFailure(
      "this host supplies a vec2 position stream; the plan declares stride \(declared.stride)")
  }
  let descriptor = MTLVertexDescriptor()
  for attribute in declared.attributes {
    let index = Int(attribute.location)
    descriptor.attributes[index].format = try metalVertexFormat(attribute.format)
    descriptor.attributes[index].offset = attribute.offset
    descriptor.attributes[index].bufferIndex = declared.slot
  }
  descriptor.layouts[declared.slot].stride = declared.stride
  descriptor.layouts[declared.slot].stepFunction =
    declared.stepMode == "instance" ? .perInstance : .perVertex

  let format = try metalFormat(target.format)
  let pipelineDescriptor = MTLRenderPipelineDescriptor()
  pipelineDescriptor.label = pass.identifier
  pipelineDescriptor.vertexFunction = vertexFunction
  pipelineDescriptor.fragmentFunction = fragmentFunction
  pipelineDescriptor.vertexDescriptor = descriptor
  pipelineDescriptor.colorAttachments[0].pixelFormat = format
  let pipeline = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)

  let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(
    pixelFormat: format, width: Int(target.width), height: Int(target.height), mipmapped: false)
  textureDescriptor.usage = [.renderTarget, .shaderRead]
  #if os(macOS)
    textureDescriptor.storageMode = .managed
  #else
    textureDescriptor.storageMode = .shared
  #endif
  guard let texture = device.makeTexture(descriptor: textureDescriptor) else {
    throw HostFailure("device refused the declared render target")
  }

  let geometry: [Float] = [-1, -1, 3, -1, -1, 3]
  guard
    let vertices = device.makeBuffer(
      bytes: geometry, length: MemoryLayout<Float>.stride * geometry.count, options: [])
  else {
    throw HostFailure("device refused the vertex buffer")
  }

  let renderPass = MTLRenderPassDescriptor()
  renderPass.colorAttachments[0].texture = texture
  renderPass.colorAttachments[0].loadAction = target.load == "clear" ? .clear : .load
  renderPass.colorAttachments[0].storeAction = .store
  renderPass.colorAttachments[0].clearColor = MTLClearColor(
    red: target.clearColour[0], green: target.clearColour[1],
    blue: target.clearColour[2], alpha: target.clearColour[3])

  guard let buffer = queue.makeCommandBuffer(),
    let encoder = buffer.makeRenderCommandEncoder(descriptor: renderPass)
  else {
    throw HostFailure("device refused a command encoder")
  }
  encoder.setRenderPipelineState(pipeline)
  encoder.setVertexBuffer(vertices, offset: 0, index: declared.slot)
  encoder.drawPrimitives(
    type: try metalPrimitive(pass.command.topology),
    vertexStart: Int(pass.command.firstVertex),
    vertexCount: Int(pass.command.vertexCount),
    instanceCount: Int(pass.command.instanceCount))
  encoder.endEncoding()
  #if os(macOS)
    if let blit = buffer.makeBlitCommandEncoder() {
      blit.synchronize(resource: texture)
      blit.endEncoding()
    }
  #endif
  buffer.commit()
  buffer.waitUntilCompleted()
  if let error = buffer.error {
    throw HostFailure("command buffer failed: \(error.localizedDescription)")
  }

  let width = Int(target.width)
  let height = Int(target.height)
  var pixels = [UInt8](repeating: 0, count: width * height * 4)
  pixels.withUnsafeMutableBytes { raw in
    texture.getBytes(
      raw.baseAddress!, bytesPerRow: width * 4,
      from: MTLRegionMake2D(0, 0, width, height), mipmapLevel: 0)
  }
  return (pixels, width, height)
}

func run() throws -> [String: Any] {
  guard let device = MTLCreateSystemDefaultDevice() else {
    throw HostFailure("no Metal device is available", unmeasured: true)
  }
  guard let queue = device.makeCommandQueue() else {
    throw HostFailure("Metal device refused a command queue", unmeasured: true)
  }

  // 1. The library emits MSL and a device-independent plan; it never sees the
  //    MTLDevice above.
  let program = try emitDefaultHostMaterial(
    stableIdentity: stableIdentity, outputIdentity: outputIdentity,
    width: 64, height: 64, target: .msl)
  guard let planData = program.passPlan.data(using: .utf8) else {
    throw HostFailure("pass plan is not UTF-8")
  }
  let plan = try JSONDecoder().decode(PassPlan.self, from: planData)
  guard plan.stableIdentity == stableIdentity else {
    throw HostFailure("pass plan carries a different stable identity")
  }
  // MSL is a unified module: one artifact carries both stages and the other
  // is empty. Assert that rather than assuming which one it is.
  let unified = try shaderSource(program.vertexArtifact, stage: "vertex")
  let companion = try shaderSource(program.fragmentArtifact, stage: "fragment")
  guard companion.isEmpty else {
    throw HostFailure("the MSL target emitted two modules; this host expects one")
  }
  let source = unified

  // 2. The host executes the plan on its own device.
  let rendered = try execute(device: device, queue: queue, plan: plan, source: source)
  let first = Array(rendered.pixels[0..<4])
  for index in stride(from: 0, to: rendered.pixels.count, by: 4) {
    guard Array(rendered.pixels[index..<index + 4]) == first else {
      throw HostFailure("the constant material did not render a constant image")
    }
  }

  // 3. The host reports completion; the library publishes the revision without
  //    the pixels ever leaving the device.
  let resource = plan.resources[0]
  let session = try HostExecutionSession()
  defer { session.close() }
  let token = try session.submit(
    operation: "material-graph", baseRevision: 0,
    resources: [
      HostResource(
        logicalID: outputIdentity, generation: resource.version.generation,
        role: resource.role, format: 2,
        width: UInt32(rendered.width), height: UInt32(rendered.height),
        layers: resource.extent.layers, mipLevels: resource.mipLevels,
        tileWidth: resource.tileShape.width, tileHeight: resource.tileShape.height,
        externallyInitialized: resource.externallyInitialized,
        owner: .host, requiredState: .renderTarget, output: true)
    ])
  let completion = try session.complete(
    token: token,
    outputs: [
      CompletedHostResource(
        logicalID: outputIdentity, generation: resource.version.generation,
        format: 2, width: UInt32(rendered.width), height: UInt32(rendered.height),
        layers: resource.extent.layers)
    ],
    recovery: HostRecovery(
      operationRecordVersion: "material-graph-v1", checkpointRevision: 0,
      retainedBytes: rendered.pixels.count, inputsPinned: true))
  guard completion.disposition == .published else {
    throw HostFailure("completion was not published: \(completion.message)")
  }

  // 4. The explicit transport path: what changed, pinned, laid out, read back.
  let document = try Document()
  let textureSet = try document.createTextureSet(
    displayName: "Reference", partitionKey: "reference", width: 64, height: 64)
  try document.setChannelEnabled(semantic, in: textureSet)
  let pool = try SnapshotPool(budgetBytes: 4 << 20)
  defer { pool.close() }
  let before = try pool.currentCursor(
    document: document, textureSet: textureSet, semanticID: semantic)

  // A write matching the stored value produces no delta by design, and the
  // emitted material is a constant that may equal the channel's default. The
  // host writes the device colour and its complement so a tile is genuinely
  // dirty.
  let deviceTexel = Array(first[0..<3])
  try document.writeChannelPixel(
    deviceTexel, x: 2, y: 3, semanticID: semantic, in: textureSet)
  try document.writeChannelPixel(
    deviceTexel.map { ~$0 }, x: 4, y: 5, semanticID: semantic, in: textureSet)

  let snapshot = try pool.snapshot(
    document: document, textureSet: textureSet, semanticID: semantic, since: before)
  let readback = try snapshot.beginHostReadback()
  defer { readback.close() }
  let sizes = readback.tileByteSizes
  guard !sizes.isEmpty else {
    throw HostFailure("the snapshot reported no changed tile to read back")
  }
  let initial = try readback.status
  guard initial == .pending else {
    throw HostFailure("a fresh host readback must be pending, not \(initial)")
  }
  try readback.complete(tiles: sizes.map { [UInt8](repeating: 0, count: $0) })
  guard try readback.status == .complete else {
    throw HostFailure("a completed host readback must report complete")
  }

  return [
    "host": "mobile-msl",
    "device": device.name,
    "stable_identity": plan.stableIdentity,
    "pass_count": plan.passes.count,
    "target": [
      "width": rendered.width, "height": rendered.height, "format": resource.format,
    ],
    "rendered_texel": deviceTexel,
    "published_revision": completion.publishedRevision ?? 0,
    "transport_tile_count": sizes.count,
    "transport_tile_bytes": sizes,
    "synchronous_pixel_readbacks": 0,
  ]
}

let arguments = CommandLine.arguments
let reportPath = arguments.firstIndex(of: "--report").flatMap { index -> String? in
  index + 1 < arguments.count ? arguments[index + 1] : nil
}

do {
  let report = try run()
  let data = try JSONSerialization.data(
    withJSONObject: report, options: [.prettyPrinted, .sortedKeys])
  if let path = reportPath {
    try FileManager.default.createDirectory(
      atPath: (path as NSString).deletingLastPathComponent,
      withIntermediateDirectories: true)
    try data.write(to: URL(fileURLWithPath: path))
  }
  print(String(data: data, encoding: .utf8) ?? "{}")
  print("ok: the mobile MSL reference host ran the pass plan on a device")
} catch let failure as HostFailure where failure.isUnmeasured {
  FileHandle.standardError.write(Data("unmeasured: \(failure.message)\n".utf8))
  exit(unmeasured)
} catch let failure as HostFailure {
  FileHandle.standardError.write(Data("mobile reference host failed: \(failure.message)\n".utf8))
  exit(2)
} catch {
  FileHandle.standardError.write(
    Data("mobile reference host failed: \(error)\n".utf8))
  exit(2)
}
