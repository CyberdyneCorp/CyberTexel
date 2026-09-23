import XCTest

@testable import CyberTexel

final class CyberTexelTests: XCTestCase {
  func testIncompatibleNativeABINamesBothVersions() {
    XCTAssertThrowsError(
      try Native.validateABI(
        CyberTexelVersion(major: 7, minor: 2, patch: 1, string: "7.2.1-test"))
    ) { error in
      XCTAssertEqual(
        error as? CyberTexelError,
        .incompatibleABI(expectedMajor: 0, native: "7.2.1-test"))
    }
  }

  func testVersionAndDocumentRoundTrip() throws {
    XCTAssertEqual(CyberTexelVersion.current.major, 0)
    XCTAssertFalse(CyberTexelVersion.current.string.isEmpty)
    let document = try Document()
    let textureSet = try document.createTextureSet(
      displayName: "Body", partitionKey: "body", width: 8, height: 4
    )
    XCTAssertEqual(textureSet.width, 8)
    XCTAssertEqual(try document.textureSetIdentifiers, [textureSet.identifier])
  }

  func testNativeFailureIsTyped() throws {
    let document = try Document()
    XCTAssertThrowsError(
      try document.createTextureSet(
        displayName: "", partitionKey: "body", width: 8, height: 4
      )
    ) { error in
      guard case CyberTexelError.native(let result, let code, let diagnostic) = error else {
        return XCTFail("unexpected error: \(error)")
      }
      XCTAssertEqual(result, .invalidArgument)
      XCTAssertGreaterThan(code, 0)
      XCTAssertTrue(diagnostic.contains("display_name"))
    }
  }

  func testDocumentStorageDestroysExactlyOnce() {
    var destroyCount = 0
    do {
      let storage = DocumentStorage(handle: OpaquePointer(bitPattern: 1)!) { _ in
        destroyCount += 1
      }
      let first = Document(storage: storage)
      let second = first
      _ = second
    }
    XCTAssertEqual(destroyCount, 1)
  }

  func testHostExecutionRetainsResidentResultWithoutReadback() throws {
    let program = try emitDefaultHostMaterial(
      stableIdentity: "binding/paint", outputIdentity: "paint", width: 64,
      height: 32)
    XCTAssertTrue(String(decoding: program.vertexArtifact, as: UTF8.self).contains("@vertex"))
    XCTAssertTrue(program.passPlan.contains("\"logical_id\":\"paint\""))
    let session = try HostExecutionSession()
    let source = HostResource(
      logicalID: "source", generation: 1, role: "input", format: 2,
      width: 64, height: 32, output: false)
    let output = HostResource(
      logicalID: "paint", generation: 1, role: "output", format: 2,
      width: 64, height: 32, externallyInitialized: false,
      requiredState: .renderTarget, output: true)
    let token = try session.submit(
      operation: "paint", baseRevision: 0, resources: [source, output])
    let result = try session.complete(
      token: token,
      outputs: [
        CompletedHostResource(
          logicalID: "paint", generation: 1, format: 2, width: 64, height: 32)
      ],
      recovery: HostRecovery(
        operationRecordVersion: "paint-v1", checkpointRevision: 0,
        retainedBytes: 96)
    )
    XCTAssertEqual(result.disposition, .published)
    XCTAssertEqual(result.publishedRevision, 1)
    XCTAssertEqual(try session.committedGeneration(for: "paint"), 1)
    XCTAssertTrue(try session.resourceIsHeld("paint", generation: 1))
  }

  func testExplicitHostReadbackPublishesOnlyAfterCompletion() throws {
    let document = try Document()
    let textureSet = try document.createTextureSet(
      displayName: "Readback", partitionKey: "readback", width: 8, height: 4)
    try document.setChannelEnabled("pbr.base_color", in: textureSet)
    let pool = try SnapshotPool(budgetBytes: 64 * 64 * 3)
    let cursor = try pool.currentCursor(
      document: document, textureSet: textureSet, semanticID: "pbr.base_color")
    try document.writeChannelPixel(
      [7, 11, 13], x: 1, y: 2, semanticID: "pbr.base_color", in: textureSet)
    let snapshot = try pool.snapshot(
      document: document, textureSet: textureSet, semanticID: "pbr.base_color",
      since: cursor)
    let readback = try snapshot.beginHostReadback()
    XCTAssertEqual(try readback.status, .pending)
    XCTAssertThrowsError(try readback.tiles)
    let payloads = readback.tileByteSizes.map { [UInt8](repeating: 42, count: $0) }
    try readback.complete(tiles: payloads)
    XCTAssertEqual(try readback.status, .complete)
    XCTAssertEqual(try readback.tiles, payloads)
    snapshot.close()
    readback.close()
    pool.close()
  }
}
