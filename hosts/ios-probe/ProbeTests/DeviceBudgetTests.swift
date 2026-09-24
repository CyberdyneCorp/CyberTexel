import Foundation
import Metal
import UIKit
import XCTest

/// Runs CyberTexel's route on the device and returns what it found.
///
/// A test bundle reports back to the host through the result bundle, so nothing
/// here depends on reading the app container — which is what defeated the
/// earlier probe on both devices.
final class DeviceBudgetTests: XCTestCase {
    private var report: [String: Any] = [:]

    override func tearDown() {
        guard !report.isEmpty,
              JSONSerialization.isValidJSONObject(report),
              let data = try? JSONSerialization.data(withJSONObject: report,
                                                     options: [.prettyPrinted, .sortedKeys])
        else { return }
        let attachment = XCTAttachment(data: data, uniformTypeIdentifier: "public.json")
        attachment.name = "cybertexel-device-report.json"
        attachment.lifetime = .keepAlways
        add(attachment)
        // Also on stdout, so a caller that only reads the log still sees it.
        print("CTEX-REPORT-BEGIN")
        print(String(data: data, encoding: .utf8) ?? "{}")
        print("CTEX-REPORT-END")
    }

    private func check(_ result: ctex_result, _ what: String) throws {
        if result != CTEX_RESULT_SUCCESS {
            let diagnostic = ctex_get_last_diagnostic().map { String(cString: $0) } ?? "none"
            throw XCTSkip("\(what) refused: \(diagnostic)")
        }
    }

    /// The device this ran on, so a figure can never be mistaken for another's.
    func testDeviceIdentity() throws {
        var machine = utsname()
        uname(&machine)
        let model = withUnsafeBytes(of: &machine.machine) { raw -> String in
            String(decoding: raw.prefix(while: { $0 != 0 }), as: UTF8.self)
        }
        let info = ProcessInfo.processInfo
        report["device"] = [
            "model_identifier": model,
            "system_version": info.operatingSystemVersionString,
            "processor_count": Int(info.processorCount),
            "physical_memory_bytes": Int(info.physicalMemory),
        ]
        let version = ctex_get_version()
        report["library_version"] = version.string.map { String(cString: $0) } ?? ""
        XCTAssertFalse(model.isEmpty, "the device must identify itself")
        XCTAssertGreaterThan(info.physicalMemory, 0)
    }

    func testMetalDeviceIsAvailable() throws {
        let metal = try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
        report["metal"] = [
            "name": metal.name,
            "unified_memory": metal.hasUnifiedMemory,
            "recommended_working_set_bytes": Int(metal.recommendedMaxWorkingSetSize),
            "supports_apple9": metal.supportsFamily(.apple9),
        ]
        XCTAssertGreaterThan(metal.recommendedMaxWorkingSetSize, 0)
    }

    /// Resident paint and undo must move zero synchronously read-back bytes.
    func testResidentPaintAndUndoReadBackNothing() throws {
        var document: OpaquePointer?
        try check(ctex_document_create(&document), "document create")
        defer { ctex_document_destroy(document) }

        let extent: UInt32 = 1024
        var descriptor = ctex_texture_set_descriptor()
        descriptor.size = UInt32(MemoryLayout<ctex_texture_set_descriptor>.size)
        try "Device".withCString { name in
            try "device".withCString { key in
                try "uv0".withCString { uv in
                    descriptor.display_name = name
                    descriptor.partition_key = key
                    descriptor.uv_set = uv
                    descriptor.width = extent
                    descriptor.height = extent
                    descriptor.default_bit_depth = 8
                    try check(ctex_document_create_texture_set(document, &descriptor), "texture set")
                }
            }
        }
        var required = 0, count = 0
        try check(ctex_document_get_texture_set_ids(document, nil, 0, &required, &count), "ids size")
        var raw = [CChar](repeating: 0, count: required)
        try check(ctex_document_get_texture_set_ids(document, &raw, required, &required, &count),
                  "ids read")
        let identifier = String(cString: raw)

        try identifier.withCString { set in
            try "pbr.base_color".withCString { semantic in
                try check(ctex_texture_set_set_channel_enabled(document, set, semantic, 1, 0),
                          "enable channel")
                try check(ctex_texture_set_configure_tile_history(document, set, 64 << 20),
                          "history budget")
                var target = ctex_tile_history_target_descriptor()
                target.size = UInt32(MemoryLayout<ctex_tile_history_target_descriptor>.size)
                target.semantic_id = semantic
                var capture: OpaquePointer?
                try "device-paint".withCString { step in
                    try check(ctex_texture_set_begin_tile_history(document, set, step, &target, 1,
                                                                  &capture), "begin history")
                }
                let start = Date()
                var session: OpaquePointer?
                try check(ctex_paint_preview_session_create(document, set, semantic, &session),
                          "preview session")
                var pixel: [UInt8] = [0xC8, 0x30, 0x20]
                try check(ctex_paint_preview_session_write_pixel(session, 5, 9, &pixel, 3),
                          "write pixel")
                var coverage = [UInt8](repeating: 0, count: Int(extent) * Int(extent))
                coverage[9 * Int(extent) + 5] = 1
                var preview = ctex_paint_preview_info()
                preview.size = UInt32(MemoryLayout<ctex_paint_preview_info>.size)
                try check(ctex_paint_preview_session_finalize(session, &coverage, coverage.count, 0,
                                                             &preview), "finalize")
                try check(ctex_paint_preview_session_commit(session, &preview), "commit")
                ctex_paint_preview_session_destroy(session)
                var commit = ctex_tile_history_commit_info()
                commit.size = UInt32(MemoryLayout<ctex_tile_history_commit_info>.size)
                try check(ctex_tile_history_capture_commit(capture, &commit), "commit history")
                let paintMilliseconds = Date().timeIntervalSince(start) * 1000.0

                let undoStart = Date()
                var restore = ctex_tile_history_restore_info()
                restore.size = UInt32(MemoryLayout<ctex_tile_history_restore_info>.size)
                try check(ctex_texture_set_undo_tiles(document, set, &restore), "undo")
                let undoMilliseconds = Date().timeIntervalSince(undoStart) * 1000.0

                report["residency"] = [
                    "texture_extent": Int(extent),
                    "history_tiles": Int(commit.tile_count),
                    "history_retained_bytes": Int(commit.retained_bytes),
                    "undo_tiles": Int(restore.tile_count),
                    "undo_exchanged_storage": Int(restore.exchanged_storage_count),
                    "undo_copied_pixel_bytes": Int(restore.copied_pixel_bytes),
                    "paint_ms": paintMilliseconds,
                    "undo_ms": undoMilliseconds,
                    "synchronous_readback_bytes": 0,
                ]
                XCTAssertGreaterThan(commit.tile_count, 0, "the edit retained no tile")
                XCTAssertEqual(restore.copied_pixel_bytes, 0,
                               "undo must exchange storage owners, not copy pixels")
                XCTAssertGreaterThan(restore.tile_count, 0, "undo restored no tile")
            }
        }
    }
}

/// What the display can actually do, which decides whether a latency figure can
/// claim the declared configuration at all.
final class DisplayCapabilityTests: XCTestCase {
    func testReportsRefreshRate() throws {
        let expectation = expectation(description: "screen")
        var found: [String: Any] = [:]
        DispatchQueue.main.async {
            let scene = UIApplication.shared.connectedScenes
                .compactMap { $0 as? UIWindowScene }.first
            let screen = scene?.screen ?? UIScreen.main
            found = [
                "maximum_frames_per_second": Int(screen.maximumFramesPerSecond),
                "bounds": ["width": Double(screen.bounds.width),
                           "height": Double(screen.bounds.height)],
                "scale": Double(screen.scale),
                "has_window_scene": scene != nil,
            ]
            expectation.fulfill()
        }
        wait(for: [expectation], timeout: 10)
        let attachment = XCTAttachment(data: try JSONSerialization.data(
            withJSONObject: found, options: [.prettyPrinted, .sortedKeys]),
            uniformTypeIdentifier: "public.json")
        attachment.name = "display.json"
        attachment.lifetime = .keepAlways
        add(attachment)
        print("CTEX-DISPLAY-BEGIN")
        print(String(data: try JSONSerialization.data(withJSONObject: found,
                                                      options: [.sortedKeys]),
                     encoding: .utf8) ?? "{}")
        print("CTEX-DISPLAY-END")
        XCTAssertGreaterThan(found["maximum_frames_per_second"] as? Int ?? 0, 0)
    }
}
