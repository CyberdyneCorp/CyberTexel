import XCTest

@testable import CyberTexel

final class CyberTexelTests: XCTestCase {
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
}
