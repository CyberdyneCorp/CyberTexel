import CyberTexelC

public struct CyberTexelVersion: Equatable, Sendable {
  public let major: UInt32
  public let minor: UInt32
  public let patch: UInt32
  public let string: String

  public static var current: CyberTexelVersion {
    let value = ctex_get_version()
    return CyberTexelVersion(
      major: value.major,
      minor: value.minor,
      patch: value.patch,
      string: value.string.map(String.init(cString:))
        ?? "\(value.major).\(value.minor).\(value.patch)"
    )
  }
}

public enum ResultCode: UInt32, Equatable, Sendable {
  case success = 0
  case invalidArgument = 1
  case missingResource = 2
  case unsupportedOperation = 3
  case outOfMemory = 4
  case overBudget = 5
  case cancelled = 6
  case internalError = 7
  case bufferTooSmall = 8
  case noUndo = 9
  case noRedo = 10
  case staleState = 11
}

public enum CyberTexelError: Error, Equatable, Sendable {
  case native(result: ResultCode, diagnosticCode: UInt32, diagnostic: String)
  case incompatibleABI(expectedMajor: UInt32, native: String)
  case invalidNativeState(String)
}

enum Native {
  static let abiMajor: UInt32 = 0

  static func checkABI() throws {
    let value = ctex_get_abi_version()
    let version = CyberTexelVersion(
      major: value.major,
      minor: value.minor,
      patch: value.patch,
      string: value.string.map(String.init(cString:))
        ?? "\(value.major).\(value.minor).\(value.patch)"
    )
    try validateABI(version)
  }

  static func validateABI(_ version: CyberTexelVersion) throws {
    guard version.major == abiMajor else {
      throw CyberTexelError.incompatibleABI(
        expectedMajor: abiMajor, native: version.string)
    }
  }

  static func check(_ result: ctex_result) throws {
    guard result != CTEX_RESULT_SUCCESS else { return }
    let code = ResultCode(rawValue: UInt32(result.rawValue)) ?? .internalError
    let diagnostic =
      ctex_get_last_diagnostic().map(String.init(cString:))
      ?? "CyberTexel operation failed"
    throw CyberTexelError.native(
      result: code,
      diagnosticCode: UInt32(ctex_get_last_diagnostic_code().rawValue),
      diagnostic: diagnostic
    )
  }
}

final class DocumentStorage {
  let handle: OpaquePointer
  private let destroy: (OpaquePointer) -> Void

  init(handle: OpaquePointer, destroy: @escaping (OpaquePointer) -> Void = ctex_document_destroy) {
    self.handle = handle
    self.destroy = destroy
  }

  deinit {
    destroy(handle)
  }
}

public struct TextureSet: Equatable, Sendable {
  public let identifier: String
  public let width: UInt32
  public let height: UInt32
}

public struct Document {
  let storage: DocumentStorage

  public init() throws {
    try Native.checkABI()
    var handle: OpaquePointer?
    try Native.check(ctex_document_create(&handle))
    guard let handle else {
      throw CyberTexelError.invalidNativeState("document creation returned no handle")
    }
    storage = DocumentStorage(handle: handle)
  }

  init(storage: DocumentStorage) {
    self.storage = storage
  }

  public var textureSetIdentifiers: [String] {
    get throws {
      var required = 0
      var count = 0
      try Native.check(
        ctex_document_get_texture_set_ids(storage.handle, nil, 0, &required, &count)
      )
      var bytes = [CChar](repeating: 0, count: required)
      try bytes.withUnsafeMutableBufferPointer { buffer in
        try Native.check(
          ctex_document_get_texture_set_ids(
            storage.handle, buffer.baseAddress, buffer.count, &required, &count
          )
        )
      }
      return bytes.split(separator: 0).map { part in
        part.withUnsafeBufferPointer {
          String(decoding: $0.map(UInt8.init(bitPattern:)), as: UTF8.self)
        }
      }
    }
  }

  public func createTextureSet(
    displayName: String,
    partitionKey: String,
    uvSet: String = "uv0",
    width: UInt32,
    height: UInt32,
    defaultBitDepth: UInt8 = 8
  ) throws -> TextureSet {
    let previous = Set(try textureSetIdentifiers)
    try displayName.withCString { displayName in
      try partitionKey.withCString { partitionKey in
        try uvSet.withCString { uvSet in
          var descriptor = ctex_texture_set_descriptor(
            size: UInt32(MemoryLayout<ctex_texture_set_descriptor>.size),
            display_name: displayName,
            partition_kind: 0,
            partition_key: partitionKey,
            uv_set: uvSet,
            width: width,
            height: height,
            default_bit_depth: defaultBitDepth,
            udim_tiling: 0
          )
          try Native.check(
            ctex_document_create_texture_set(storage.handle, &descriptor)
          )
        }
      }
    }
    guard let identifier = try textureSetIdentifiers.first(where: { !previous.contains($0) }) else {
      throw CyberTexelError.invalidNativeState("created texture set has no identifier")
    }
    return TextureSet(identifier: identifier, width: width, height: height)
  }

  public func setChannelEnabled(
    _ semanticID: String,
    in textureSet: TextureSet,
    bitDepth: UInt32 = 0
  ) throws {
    try textureSet.identifier.withCString { identifier in
      try semanticID.withCString { semantic in
        try Native.check(
          ctex_texture_set_set_channel_enabled(
            storage.handle, identifier, semantic, 1, bitDepth
          )
        )
      }
    }
  }

  public func writeChannelPixel(
    _ pixel: [UInt8],
    x: UInt32,
    y: UInt32,
    semanticID: String,
    in textureSet: TextureSet
  ) throws {
    guard x < textureSet.width, y < textureSet.height else {
      throw CyberTexelError.invalidNativeState("pixel coordinate is outside the texture set")
    }
    var session: OpaquePointer?
    try textureSet.identifier.withCString { identifier in
      try semanticID.withCString { semantic in
        try Native.check(
          ctex_paint_preview_session_create(storage.handle, identifier, semantic, &session)
        )
      }
    }
    guard let session else {
      throw CyberTexelError.invalidNativeState("preview creation returned no handle")
    }
    defer { ctex_paint_preview_session_destroy(session) }
    try pixel.withUnsafeBytes { bytes in
      try Native.check(
        ctex_paint_preview_session_write_pixel(
          session, x, y, bytes.baseAddress, bytes.count
        )
      )
    }
    var coverage = [UInt8](
      repeating: 0, count: Int(textureSet.width * textureSet.height))
    coverage[Int(y * textureSet.width + x)] = 1
    var info = ctex_paint_preview_info()
    info.size = UInt32(MemoryLayout<ctex_paint_preview_info>.size)
    try coverage.withUnsafeBufferPointer { buffer in
      try Native.check(
        ctex_paint_preview_session_finalize(
          session, buffer.baseAddress, buffer.count, 0, &info
        )
      )
    }
    try Native.check(ctex_paint_preview_session_commit(session, &info))
  }
}
