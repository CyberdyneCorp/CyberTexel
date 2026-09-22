import CyberTexelC

public enum ResourceOwner: UInt32, Sendable {
  case library = 0
  case host = 1
}

public enum ResourceState: UInt32, Sendable {
  case shaderRead = 0
  case storageRead = 1
  case storageWrite = 2
  case renderTarget = 3
  case depthTarget = 4
}

public enum ReplaySemantics: UInt32, Sendable {
  case deterministic = 0
  case checkpointOnly = 1
}

public enum CompletionDisposition: UInt32, Sendable {
  case published = 0
  case awaitingRecovery = 1
  case stale = 2
  case cancelled = 3
  case failed = 4
  case rejected = 5
  case duplicate = 6
  case unknownToken = 7
}

public enum ReadbackStatus: UInt32, Sendable {
  case pending = 0
  case complete = 1
  case cancelled = 2
  case failed = 3
}

public enum ShaderTarget: UInt32, Sendable {
  case wgsl = 0
  case msl = 1
  case spirv = 2
  case hlsl = 3
}

public struct HostMaterialProgram: Equatable, Sendable {
  public let target: ShaderTarget
  public let vertexArtifact: [UInt8]
  public let fragmentArtifact: [UInt8]
  public let passPlan: String
  public let workaroundReport: String
}

public func emitDefaultHostMaterial(
  stableIdentity: String, outputIdentity: String, width: UInt32, height: UInt32,
  target: ShaderTarget = .wgsl
) throws -> HostMaterialProgram {
  var graphInfo = ctex_material_graph_info()
  graphInfo.size = UInt32(MemoryLayout<ctex_material_graph_info>.size)
  try Native.check(ctex_material_graph_create_default(&graphInfo, nil, 0, nil, 0))
  var graph = [UInt8](repeating: 0, count: graphInfo.canonical_size)
  try graph.withUnsafeMutableBytes { graph in
    try Native.check(
      ctex_material_graph_create_default(
        &graphInfo, graph.baseAddress, graph.count, nil, 0))
  }
  let formats: [UInt32] = [2]
  return try stableIdentity.withCString { stableIdentity in
    try outputIdentity.withCString { outputIdentity in
      try "material output".withCString { role in
        try formats.withUnsafeBufferPointer { formats in
          var request = ctex_shader_material_request(
            size: UInt32(MemoryLayout<ctex_shader_material_request>.size),
            stable_identity: stableIdentity,
            target: target.rawValue,
            features: ctex_shader_device_features_descriptor(
              size: UInt32(MemoryLayout<ctex_shader_device_features_descriptor>.size),
              binding_budget: 16,
              maximum_texture_dimension: 8192,
              supported_texture_formats: formats.baseAddress,
              supported_texture_format_count: formats.count,
              floating_point_filtering: 1,
              compute_available: 0
            ),
            resources: nil,
            resource_count: 0,
            output: ctex_shader_texture_descriptor(
              size: UInt32(MemoryLayout<ctex_shader_texture_descriptor>.size),
              logical_id: outputIdentity,
              generation: 1,
              role: role,
              format: 2,
              width: width,
              height: height,
              layers: 1,
              mip_levels: 1,
              tile_width: width,
              tile_height: height,
              externally_initialized: 0
            ),
            requested_filter: 1,
            vertex_count: 3
          )
          var info = ctex_shader_material_info()
          info.size = UInt32(MemoryLayout<ctex_shader_material_info>.size)
          try graph.withUnsafeBytes { graph in
            try Native.check(
              ctex_shader_emit_material(
                nil, graph.baseAddress, graph.count, &request, &info, nil, 0,
                nil, 0, nil, 0, nil, 0
              ))
          }
          var vertex = [UInt8](repeating: 0, count: info.vertex_artifact_size)
          var fragment = [UInt8](repeating: 0, count: info.fragment_artifact_size)
          var plan = [CChar](repeating: 0, count: info.pass_plan_size)
          var workarounds = [CChar](repeating: 0, count: info.workaround_report_size)
          try graph.withUnsafeBytes { graph in
            try vertex.withUnsafeMutableBytes { vertex in
              try fragment.withUnsafeMutableBytes { fragment in
                try plan.withUnsafeMutableBufferPointer { plan in
                  try workarounds.withUnsafeMutableBufferPointer { workarounds in
                    try Native.check(
                      ctex_shader_emit_material(
                        nil, graph.baseAddress, graph.count, &request, &info,
                        vertex.baseAddress, vertex.count,
                        fragment.baseAddress, fragment.count,
                        plan.baseAddress, plan.count,
                        workarounds.baseAddress, workarounds.count
                      ))
                  }
                }
              }
            }
          }
          return HostMaterialProgram(
            target: target,
            vertexArtifact: vertex,
            fragmentArtifact: fragment,
            passPlan: String(cString: plan),
            workaroundReport: String(cString: workarounds)
          )
        }
      }
    }
  }
}

public struct RevisionCursor: Equatable, Sendable {
  public let epoch: UInt64
  public let revision: UInt64

  public init(epoch: UInt64, revision: UInt64) {
    self.epoch = epoch
    self.revision = revision
  }
}

public struct HostResource: Equatable, Sendable {
  public let logicalID: String
  public let generation: UInt64
  public let role: String
  public let format: UInt32
  public let width: UInt32
  public let height: UInt32
  public let layers: UInt32
  public let mipLevels: UInt32
  public let tileWidth: UInt32
  public let tileHeight: UInt32
  public let externallyInitialized: Bool
  public let owner: ResourceOwner
  public let requiredState: ResourceState
  public let output: Bool

  public init(
    logicalID: String, generation: UInt64, role: String, format: UInt32,
    width: UInt32, height: UInt32, layers: UInt32 = 1, mipLevels: UInt32 = 1,
    tileWidth: UInt32 = 0, tileHeight: UInt32 = 0,
    externallyInitialized: Bool = true, owner: ResourceOwner = .host,
    requiredState: ResourceState = .shaderRead, output: Bool
  ) {
    self.logicalID = logicalID
    self.generation = generation
    self.role = role
    self.format = format
    self.width = width
    self.height = height
    self.layers = layers
    self.mipLevels = mipLevels
    self.tileWidth = tileWidth == 0 ? width : tileWidth
    self.tileHeight = tileHeight == 0 ? height : tileHeight
    self.externallyInitialized = externallyInitialized
    self.owner = owner
    self.requiredState = requiredState
    self.output = output
  }
}

public struct CompletedHostResource: Equatable, Sendable {
  public let logicalID: String
  public let generation: UInt64
  public let format: UInt32
  public let width: UInt32
  public let height: UInt32
  public let layers: UInt32

  public init(
    logicalID: String, generation: UInt64, format: UInt32, width: UInt32,
    height: UInt32, layers: UInt32 = 1
  ) {
    self.logicalID = logicalID
    self.generation = generation
    self.format = format
    self.width = width
    self.height = height
    self.layers = layers
  }
}

public struct HostRecovery: Equatable, Sendable {
  public let operationRecordVersion: String
  public let checkpointRevision: UInt64
  public let retainedBytes: Int
  public let inputsPinned: Bool

  public init(
    operationRecordVersion: String, checkpointRevision: UInt64,
    retainedBytes: Int = 0, inputsPinned: Bool = true
  ) {
    self.operationRecordVersion = operationRecordVersion
    self.checkpointRevision = checkpointRevision
    self.retainedBytes = retainedBytes
    self.inputsPinned = inputsPinned
  }
}

public struct ReleasedResource: Equatable, Sendable {
  public let logicalID: String
  public let generation: UInt64
}

public struct HostCompletionResult: Equatable, Sendable {
  public let disposition: CompletionDisposition
  public let completionToken: UInt64
  public let publishedRevision: UInt64?
  public let releasedResources: [ReleasedResource]
  public let message: String
}

private func withNativeResources<Result>(
  _ resources: [HostResource],
  _ body: (UnsafeBufferPointer<ctex_host_resource_descriptor>) throws -> Result
) rethrows -> Result {
  var native = Array(
    repeating: ctex_host_resource_descriptor(), count: resources.count)
  func visit(_ index: Int) throws -> Result {
    guard index < resources.count else {
      return try native.withUnsafeBufferPointer(body)
    }
    let resource = resources[index]
    return try resource.logicalID.withCString { logicalID in
      try resource.role.withCString { role in
        native[index] = ctex_host_resource_descriptor(
          size: UInt32(MemoryLayout<ctex_host_resource_descriptor>.size),
          logical_id: logicalID,
          generation: resource.generation,
          role: role,
          format: resource.format,
          width: resource.width,
          height: resource.height,
          layers: resource.layers,
          mip_levels: resource.mipLevels,
          tile_width: resource.tileWidth,
          tile_height: resource.tileHeight,
          externally_initialized: resource.externallyInitialized ? 1 : 0,
          owner: resource.owner.rawValue,
          required_state: resource.requiredState.rawValue,
          output: resource.output ? 1 : 0
        )
        return try visit(index + 1)
      }
    }
  }
  return try visit(0)
}

private func withNativeCompletedResources<Result>(
  _ resources: [CompletedHostResource],
  _ body: (UnsafeBufferPointer<ctex_host_completed_resource_descriptor>) throws -> Result
) rethrows -> Result {
  var native = Array(
    repeating: ctex_host_completed_resource_descriptor(), count: resources.count)
  func visit(_ index: Int) throws -> Result {
    guard index < resources.count else {
      return try native.withUnsafeBufferPointer(body)
    }
    let resource = resources[index]
    return try resource.logicalID.withCString { logicalID in
      native[index] = ctex_host_completed_resource_descriptor(
        size: UInt32(MemoryLayout<ctex_host_completed_resource_descriptor>.size),
        logical_id: logicalID,
        generation: resource.generation,
        format: resource.format,
        width: resource.width,
        height: resource.height,
        layers: resource.layers
      )
      return try visit(index + 1)
    }
  }
  return try visit(0)
}

public final class HostExecutionSession {
  private var handle: OpaquePointer?

  public init(initialRevision: UInt64 = 0) throws {
    try Native.checkABI()
    try Native.check(ctex_host_execution_session_create(initialRevision, &handle))
    guard handle != nil else {
      throw CyberTexelError.invalidNativeState("host session creation returned no handle")
    }
  }

  deinit { close() }

  public func close() {
    if let handle {
      ctex_host_execution_session_destroy(handle)
      self.handle = nil
    }
  }

  private func openHandle() throws -> OpaquePointer {
    guard let handle else {
      throw CyberTexelError.invalidNativeState("host execution session is closed")
    }
    return handle
  }

  public func submit(
    operation: String, baseRevision: UInt64, resources: [HostResource],
    replay: ReplaySemantics = .deterministic
  ) throws -> UInt64 {
    let handle = try openHandle()
    return try operation.withCString { operation in
      try withNativeResources(resources) { native in
        var descriptor = ctex_host_submission_descriptor(
          size: UInt32(MemoryLayout<ctex_host_submission_descriptor>.size),
          operation: operation,
          base_revision: baseRevision,
          resources: native.baseAddress,
          resource_count: native.count,
          replay_semantics: replay.rawValue
        )
        var info = ctex_host_submission_info()
        info.size = UInt32(MemoryLayout<ctex_host_submission_info>.size)
        try Native.check(ctex_host_execution_session_submit(handle, &descriptor, &info))
        return info.completion_token
      }
    }
  }

  public func complete(
    token: UInt64, outputs: [CompletedHostResource], recovery: HostRecovery? = nil
  ) throws -> HostCompletionResult {
    let handle = try openHandle()
    return try withNativeCompletedResources(outputs) { nativeOutputs in
      func invoke(_ recoveryPointer: UnsafePointer<ctex_host_recovery_descriptor>?) throws
        -> HostCompletionResult
      {
        var descriptor = ctex_host_completion_descriptor(
          size: UInt32(MemoryLayout<ctex_host_completion_descriptor>.size),
          completion_token: token,
          status: 0,
          outputs: nativeOutputs.baseAddress,
          output_count: nativeOutputs.count,
          recovery: recoveryPointer,
          detail: nil
        )
        var result: OpaquePointer?
        try Native.check(ctex_host_execution_session_complete(handle, &descriptor, &result))
        guard let result else {
          throw CyberTexelError.invalidNativeState("host completion returned no result")
        }
        defer { ctex_host_completion_result_destroy(result) }
        return try readCompletion(result)
      }
      guard let recovery else { return try invoke(nil) }
      return try recovery.operationRecordVersion.withCString { version in
        var nativeRecovery = ctex_host_recovery_descriptor(
          size: UInt32(MemoryLayout<ctex_host_recovery_descriptor>.size),
          kind: 1,
          checkpoint_complete: 1,
          checkpoint_revision: recovery.checkpointRevision,
          operation_record_version: version,
          inputs_pinned: recovery.inputsPinned ? 1 : 0,
          retained_bytes: recovery.retainedBytes
        )
        return try withUnsafePointer(to: &nativeRecovery, invoke)
      }
    }
  }

  public func committedGeneration(for logicalID: String) throws -> UInt64? {
    let handle = try openHandle()
    return try logicalID.withCString { logicalID in
      var found: UInt32 = 0
      var generation: UInt64 = 0
      try Native.check(
        ctex_host_execution_session_get_committed_resource(
          handle, logicalID, &found, &generation
        )
      )
      return found == 0 ? nil : generation
    }
  }

  public func resourceIsHeld(_ logicalID: String, generation: UInt64) throws -> Bool {
    let handle = try openHandle()
    return try logicalID.withCString { logicalID in
      var held: UInt32 = 0
      try Native.check(
        ctex_host_execution_session_resource_is_held(
          handle, logicalID, generation, &held
        )
      )
      return held != 0
    }
  }
}

private func readCompletion(_ handle: OpaquePointer) throws -> HostCompletionResult {
  var info = ctex_host_completion_result_info()
  info.size = UInt32(MemoryLayout<ctex_host_completion_result_info>.size)
  try Native.check(
    ctex_host_completion_result_get_info(handle, &info, nil, 0, nil, 0, nil, 0)
  )
  var released = Array(
    repeating: ctex_host_resource_version(), count: info.released_resource_count)
  var identities = [CChar](
    repeating: 0, count: max(1, info.required_released_identity_size))
  var message = [CChar](repeating: 0, count: max(1, info.required_message_size))
  try released.withUnsafeMutableBufferPointer { resources in
    try identities.withUnsafeMutableBufferPointer { identities in
      try message.withUnsafeMutableBufferPointer { message in
        try Native.check(
          ctex_host_completion_result_get_info(
            handle, &info, resources.baseAddress, resources.count,
            identities.baseAddress, info.required_released_identity_size,
            message.baseAddress, info.required_message_size
          )
        )
      }
    }
  }
  guard let disposition = CompletionDisposition(rawValue: info.disposition) else {
    throw CyberTexelError.invalidNativeState("native returned an unknown completion disposition")
  }
  let decoded = identities.withUnsafeBufferPointer { identities in
    released.map { resource in
      ReleasedResource(
        logicalID: String(
          cString: identities.baseAddress!.advanced(
            by: Int(resource.logical_id_offset))),
        generation: resource.generation
      )
    }
  }
  return HostCompletionResult(
    disposition: disposition,
    completionToken: info.completion_token,
    publishedRevision: info.has_published_revision == 0 ? nil : info.published_revision,
    releasedResources: decoded,
    message: String(cString: message)
  )
}

private final class SnapshotPoolStorage {
  let handle: OpaquePointer

  init(_ handle: OpaquePointer) { self.handle = handle }
  deinit { ctex_transport_snapshot_pool_destroy(handle) }
}

public final class SnapshotPool {
  private var storage: SnapshotPoolStorage?

  public init(budgetBytes: Int) throws {
    var handle: OpaquePointer?
    try Native.check(ctex_transport_snapshot_pool_create(budgetBytes, &handle))
    guard let handle else {
      throw CyberTexelError.invalidNativeState("snapshot pool creation returned no handle")
    }
    storage = SnapshotPoolStorage(handle)
  }

  public func close() { storage = nil }

  private func openStorage() throws -> SnapshotPoolStorage {
    guard let storage else {
      throw CyberTexelError.invalidNativeState("snapshot pool is closed")
    }
    return storage
  }

  public func currentCursor(
    document: Document, textureSet: TextureSet, semanticID: String
  ) throws -> RevisionCursor {
    var info = ctex_transport_delta_info()
    info.size = UInt32(MemoryLayout<ctex_transport_delta_info>.size)
    var cursor = ctex_transport_revision_cursor(epoch: 0, revision: 0)
    try textureSet.identifier.withCString { identifier in
      try semanticID.withCString { semantic in
        try Native.check(
          ctex_texture_set_query_channel_delta(
            document.storage.handle, identifier, semantic, cursor, nil, 0, &info
          )
        )
      }
    }
    cursor = info.current_cursor
    return RevisionCursor(epoch: cursor.epoch, revision: cursor.revision)
  }

  public func snapshot(
    document: Document, textureSet: TextureSet, semanticID: String,
    since: RevisionCursor
  ) throws -> TransportSnapshot {
    let storage = try openStorage()
    var handle: OpaquePointer?
    var info = ctex_transport_snapshot_query_info()
    info.size = UInt32(MemoryLayout<ctex_transport_snapshot_query_info>.size)
    let cursor = ctex_transport_revision_cursor(
      epoch: since.epoch, revision: since.revision)
    try textureSet.identifier.withCString { identifier in
      try semanticID.withCString { semantic in
        try Native.check(
          ctex_texture_set_query_channel_snapshot(
            storage.handle, document.storage.handle, identifier, semantic, cursor,
            &handle, &info
          )
        )
      }
    }
    guard let handle else {
      throw CyberTexelError.invalidNativeState("snapshot query returned no handle")
    }
    return TransportSnapshot(handle: handle, pool: storage)
  }
}

public final class TransportSnapshot {
  private var handle: OpaquePointer?
  private let pool: SnapshotPoolStorage

  fileprivate init(handle: OpaquePointer, pool: SnapshotPoolStorage) {
    self.handle = handle
    self.pool = pool
  }

  deinit { close() }

  public func close() {
    if let handle {
      ctex_transport_snapshot_destroy(handle)
      self.handle = nil
    }
  }

  public func beginHostReadback() throws -> HostReadback {
    guard let handle else {
      throw CyberTexelError.invalidNativeState("snapshot is closed")
    }
    var count = 0
    try Native.check(ctex_transport_snapshot_get_tile_versions(handle, nil, 0, &count))
    var versions = Array(repeating: ctex_transport_tile_version(), count: count)
    try versions.withUnsafeMutableBufferPointer { versions in
      try Native.check(
        ctex_transport_snapshot_get_tile_versions(
          handle, versions.baseAddress, versions.count, &count
        )
      )
    }
    var layouts = Array(repeating: ctex_transport_tile_memory_layout(), count: count)
    var buffers: [UnsafeMutableRawPointer] = []
    var destinations: [ctex_transport_tile_readback_destination] = []
    for index in versions.indices {
      layouts[index].size = UInt32(MemoryLayout<ctex_transport_tile_memory_layout>.size)
      try Native.check(
        ctex_transport_snapshot_get_tile_memory_layout(
          handle, versions[index], nil, &layouts[index]
        )
      )
      versions[index].residency = 1
      let output = UnsafeMutableRawPointer.allocate(
        byteCount: layouts[index].byte_size, alignment: 16)
      output.initializeMemory(as: UInt8.self, repeating: 0, count: layouts[index].byte_size)
      buffers.append(output)
      destinations.append(
        ctex_transport_tile_readback_destination(
          size: UInt32(MemoryLayout<ctex_transport_tile_readback_destination>.size),
          version: versions[index],
          layout: layouts[index],
          output: output,
          output_size: layouts[index].byte_size
        ))
    }
    var readback: OpaquePointer?
    do {
      try destinations.withUnsafeBufferPointer { destinations in
        try Native.check(
          ctex_transport_snapshot_begin_host_readback(
            handle, nil, destinations.baseAddress, destinations.count, &readback
          )
        )
      }
    } catch {
      buffers.forEach { $0.deallocate() }
      throw error
    }
    guard let readback else {
      buffers.forEach { $0.deallocate() }
      throw CyberTexelError.invalidNativeState("readback creation returned no handle")
    }
    return HostReadback(
      handle: readback, snapshot: self, versions: versions, layouts: layouts,
      buffers: buffers)
  }
}

public final class HostReadback {
  private var handle: OpaquePointer?
  private let snapshot: TransportSnapshot
  private let versions: [ctex_transport_tile_version]
  private let layouts: [ctex_transport_tile_memory_layout]
  private let buffers: [UnsafeMutableRawPointer]

  fileprivate init(
    handle: OpaquePointer, snapshot: TransportSnapshot,
    versions: [ctex_transport_tile_version],
    layouts: [ctex_transport_tile_memory_layout],
    buffers: [UnsafeMutableRawPointer]
  ) {
    self.handle = handle
    self.snapshot = snapshot
    self.versions = versions
    self.layouts = layouts
    self.buffers = buffers
  }

  deinit {
    close()
    buffers.forEach { $0.deallocate() }
  }

  public func close() {
    if let handle {
      ctex_transport_readback_destroy(handle)
      self.handle = nil
    }
  }

  private func info() throws -> ctex_transport_readback_info {
    guard let handle else {
      throw CyberTexelError.invalidNativeState("readback is closed")
    }
    var info = ctex_transport_readback_info()
    info.size = UInt32(MemoryLayout<ctex_transport_readback_info>.size)
    try Native.check(ctex_transport_readback_get_info(handle, &info, nil, 0))
    return info
  }

  public var status: ReadbackStatus {
    get throws {
      guard let status = ReadbackStatus(rawValue: try info().status) else {
        throw CyberTexelError.invalidNativeState("native returned an unknown readback status")
      }
      return status
    }
  }

  public var tileByteSizes: [Int] { layouts.map(\.byte_size) }

  public var tiles: [[UInt8]] {
    get throws {
      guard try info().output_readable != 0 else {
        throw CyberTexelError.invalidNativeState("readback output is not readable")
      }
      return zip(buffers, layouts).map { buffer, layout in
        Array(
          UnsafeBufferPointer(
            start: buffer.assumingMemoryBound(to: UInt8.self), count: layout.byte_size))
      }
    }
  }

  public func complete(tiles: [[UInt8]]) throws {
    guard let handle else {
      throw CyberTexelError.invalidNativeState("readback is closed")
    }
    guard tiles.count == versions.count else {
      throw CyberTexelError.invalidNativeState(
        "completion must contain one payload per requested tile")
    }
    let inputs = tiles.map { bytes -> UnsafeMutableRawPointer in
      let pointer = UnsafeMutableRawPointer.allocate(byteCount: bytes.count, alignment: 16)
      bytes.withUnsafeBytes { source in
        if let baseAddress = source.baseAddress {
          pointer.copyMemory(from: baseAddress, byteCount: source.count)
        }
      }
      return pointer
    }
    defer { inputs.forEach { $0.deallocate() } }
    let completions = versions.indices.map { index in
      ctex_transport_host_tile_completion(
        size: UInt32(MemoryLayout<ctex_transport_host_tile_completion>.size),
        version: versions[index],
        layout: layouts[index],
        bytes: UnsafeRawPointer(inputs[index]),
        byte_size: tiles[index].count
      )
    }
    try completions.withUnsafeBufferPointer { completions in
      try Native.check(
        ctex_transport_readback_complete_host(
          handle, completions.baseAddress, completions.count
        )
      )
    }
  }
}
