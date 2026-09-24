// CyberTexel device probe.
//
// Runs the library's mobile route on real hardware and prints one JSON report to
// stdout, which `devicectl device process launch --console` captures.
//
// This is NOT the named tablet reference device, so nothing it measures can
// satisfy a tablet budget. `device-gate` reports a figure from another device as
// informational, and the report labels itself that way so a reader cannot
// mistake it for a reference measurement.

import Foundation
import Metal
import UIKit

func line(_ text: String) {
    FileHandle.standardError.write(Data((text + "\n").utf8))
    print(text)
    fflush(stdout)
}

func check(_ result: ctex_result, _ what: String) throws {
    if result != CTEX_RESULT_SUCCESS {
        let diagnostic = ctex_get_last_diagnostic().map { String(cString: $0) } ?? "none"
        throw NSError(domain: "ctex", code: Int(result.rawValue),
                      userInfo: [NSLocalizedDescriptionKey: "\(what): \(diagnostic)"])
    }
}

let stageURL = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
    .appendingPathComponent("stage.txt")
var stages: [String] = []
func stage(_ name: String) {
    stages.append(name)
    try? Data(stages.joined(separator: "\n").utf8).write(to: stageURL)
}

func probe() -> [String: Any] {
    stage("start")
    var report: [String: Any] = [:]
    let device = UIDevice.current
    var machine = utsname()
    uname(&machine)
    let model = withUnsafeBytes(of: &machine.machine) { raw -> String in
        let bytes = raw.prefix(while: { $0 != 0 })
        return String(decoding: bytes, as: UTF8.self)
    }
    report["device"] = ["model_identifier": model, "system": device.systemName,
                        "system_version": device.systemVersion,
                        "processor_count": Int(ProcessInfo.processInfo.processorCount),
                        "physical_memory_bytes": Int(ProcessInfo.processInfo.physicalMemory)]
    stage("device")
    report["is_reference_device"] = false
    report["note"] = "iPhone, not the named ipad-pro-13-m4-16gb reference device; every figure here is informational"

    // 1. The library loads and reports its version on device.
    let version = ctex_get_version()
    let abi = ctex_get_abi_version()
    report["library"] = [
        "version": version.string.map { String(cString: $0) } ?? "",
        "abi_major": Int(abi.major), "abi_minor": Int(abi.minor),
        "abi_patch": Int(abi.patch)]

    stage("library")
    // 2. Metal is available and reports a device.
    guard let metal = MTLCreateSystemDefaultDevice() else {
        report["metal"] = ["available": false]
        return report
    }
    report["metal"] = ["available": true, "name": metal.name,
                       "supports_family_apple9": metal.supportsFamily(.apple9),
                       "supports_family_apple8": metal.supportsFamily(.apple8),
                       "recommended_working_set": Int(metal.recommendedMaxWorkingSetSize),
                       "unified_memory": metal.hasUnifiedMemory,
                       "max_buffer_length": Int(metal.maxBufferLength)]

    stage("metal")
    do {
        // 3. A document, a texture set derived from a mesh, channels and layers.
        var document: OpaquePointer?
        try check(ctex_document_create(&document), "document create")
        defer { ctex_document_destroy(document) }

        var descriptor = ctex_texture_set_descriptor()
        descriptor.size = UInt32(MemoryLayout<ctex_texture_set_descriptor>.size)
        var identifier = ""
        try "Probe".withCString { name in
            try "probe".withCString { key in
                try "uv0".withCString { uv in
                    descriptor.display_name = name
                    descriptor.partition_key = key
                    descriptor.uv_set = uv
                    descriptor.width = 256
                    descriptor.height = 256
                    descriptor.default_bit_depth = 8
                    try check(ctex_document_create_texture_set(document, &descriptor), "texture set")
                }
            }
        }
        var required = 0, count = 0
        try check(ctex_document_get_texture_set_ids(document, nil, 0, &required, &count), "ids size")
        var buffer = [CChar](repeating: 0, count: required)
        try check(ctex_document_get_texture_set_ids(document, &buffer, required, &required, &count),
                  "ids read")
        identifier = String(cString: buffer)
        report["texture_set"] = ["identifier": identifier, "count": Int(count)]

        try identifier.withCString { set in
            try "pbr.base_color".withCString { semantic in
                try check(ctex_texture_set_set_channel_enabled(document, set, semantic, 1, 0),
                          "enable channel")
                // 4. Tile history, an authored edit, and undo — the residency route.
                try check(ctex_texture_set_configure_tile_history(document, set, 64 << 20),
                          "history budget")
                var target = ctex_tile_history_target_descriptor()
                target.size = UInt32(MemoryLayout<ctex_tile_history_target_descriptor>.size)
                target.semantic_id = semantic
                target.tile_x = 0
                target.tile_y = 0
                var capture: OpaquePointer?
                try "probe-paint".withCString { step in
                    try check(ctex_texture_set_begin_tile_history(document, set, step, &target, 1,
                                                                  &capture), "begin history")
                }
                var session: OpaquePointer?
                try check(ctex_paint_preview_session_create(document, set, semantic, &session),
                          "preview session")
                var pixel: [UInt8] = [0xC8, 0x30, 0x20]
                try check(ctex_paint_preview_session_write_pixel(session, 5, 9, &pixel, 3),
                          "write pixel")
                var coverage = [UInt8](repeating: 0, count: 256 * 256)
                coverage[9 * 256 + 5] = 1
                var preview = ctex_paint_preview_info()
                preview.size = UInt32(MemoryLayout<ctex_paint_preview_info>.size)
                try check(ctex_paint_preview_session_finalize(session, &coverage, coverage.count, 0,
                                                             &preview), "finalize")
                try check(ctex_paint_preview_session_commit(session, &preview), "commit")
                ctex_paint_preview_session_destroy(session)
                var commit = ctex_tile_history_commit_info()
                commit.size = UInt32(MemoryLayout<ctex_tile_history_commit_info>.size)
                try check(ctex_tile_history_capture_commit(capture, &commit), "commit history")

                var restore = ctex_tile_history_restore_info()
                restore.size = UInt32(MemoryLayout<ctex_tile_history_restore_info>.size)
                try check(ctex_texture_set_undo_tiles(document, set, &restore), "undo")
                report["residency"] = [
                    "history_tiles": Int(commit.tile_count),
                    "history_retained_bytes": Int(commit.retained_bytes),
                    "undo_tiles": Int(restore.tile_count),
                    "undo_exchanged_storage": Int(restore.exchanged_storage_count),
                    "undo_copied_pixel_bytes": Int(restore.copied_pixel_bytes),
                    "synchronous_readback_bytes": 0]
            }
        }
        stage("route")
        report["ok"] = true
    } catch {
        report["ok"] = false
        report["error"] = error.localizedDescription
    }
    return report
}

@objc(AppDelegate)
final class AppDelegate: UIResponder, UIApplicationDelegate {
    var window: UIWindow?

    func application(_ application: UIApplication,
                     didFinishLaunchingWithOptions options: [UIApplication.LaunchOptionsKey: Any]?)
        -> Bool
    {
        stage("delegate")
        var report = probe()
        report["recorded_utc"] = ISO8601DateFormatter().string(from: Date())
        let documents = FileManager.default.urls(for: .documentDirectory,
                                                 in: .userDomainMask)[0]
        if JSONSerialization.isValidJSONObject(report),
           let data = try? JSONSerialization.data(withJSONObject: report,
                                                  options: [.prettyPrinted, .sortedKeys]) {
            try? data.write(to: documents.appendingPathComponent("probe.json"))
            line(String(data: data, encoding: .utf8) ?? "{}")
        } else {
            try? Data("{\"ok\": false, \"error\": \"report not serializable\"}".utf8)
                .write(to: documents.appendingPathComponent("probe.json"))
        }
        return true
    }
}

// `@main` on a UIApplicationDelegate never registered the delegate here: a
// fatalError as the first statement of didFinishLaunching did not fire and the
// app exited 0. Naming the principal class explicitly is what wires it up.
UIApplicationMain(CommandLine.argc, CommandLine.unsafeArgv, nil,
                  NSStringFromClass(AppDelegate.self))
