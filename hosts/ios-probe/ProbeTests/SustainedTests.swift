import Foundation
import Metal
import QuartzCore
import UIKit
import XCTest

#if canImport(CyberTexel)
import CyberTexel
#endif

/// Task 17.13: the twenty-minute mobile workload.
///
/// The budgets apply to the final five minutes, so the point of the run is not
/// the average but the drift: whether latency, resident memory or thermal state
/// move over twenty minutes of continuous authoring. A short run cannot show
/// that, which is why `TEST_RUNNER_CTEX_SUSTAINED_MINUTES` only shortens the
/// run for developing the fixtures and the recorded result always uses the full
/// duration.
final class SustainedTests: XCTestCase {
    private static let extent: UInt32 = 4096
    private static let tile: Int = 64

    /// One recorded frame: when it became visible, and how long that took.
    private struct Sample {
        let at: CFTimeInterval
        let latency: Double
    }

    func testTwentyMinuteMobileWorkload() throws {
        // xcodebuild only forwards variables prefixed TEST_RUNNER_ to a test
        // runner on a device, so an unprefixed name silently leaves the default
        // in place and a "short" run is really the full twenty minutes.
        let minutes = ProcessInfo.processInfo.environment["TEST_RUNNER_CTEX_SUSTAINED_MINUTES"]
            .flatMap(Double.init) ?? 20.0
        let finalWindow = min(5.0, minutes / 4.0)

        let device = try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
        print("CTEX-STEP device")
        let queue = try XCTUnwrap(device.makeCommandQueue(), "no command queue")
        print("CTEX-STEP queue")
        let surface = try attachLayer(device: device)
        print("CTEX-STEP layer")
        var document: OpaquePointer?
        try Authoring.check(ctex_document_create(&document), "document create")
        defer { ctex_document_destroy(document) }
        let textureSetID = try makeTextureSet(document: document)
        print("CTEX-STEP textureset")
        let snapshot = try Authoring.makeSnapshot(extent: Self.extent)
        print("CTEX-STEP snapshot")
        defer { ctex_layer_snapshot_destroy(snapshot) }

        let authored = try XCTUnwrap(device.makeTexture(descriptor: {
            let d = MTLTextureDescriptor.texture2DDescriptor(
                pixelFormat: .bgra8Unorm, width: Self.tile, height: Self.tile, mipmapped: false)
            d.usage = [.shaderRead]
            return d
        }()), "no authored texture")

        var samples: [Sample] = []
        var peakPhysical: UInt64 = 0
        var thermal = ProcessInfo.processInfo.thermalState
        var inFlight = 0
        var paced = 0
        var skipped = 0
        var firstError: String?
        var warmup = 60
        var finished = false

        let started = CACurrentMediaTime()
        let deadline = started + minutes * 60.0
        let done = expectation(description: "sustained")
        let driver = FrameDriver(layer: surface.layer, refresh: surface.refresh)

        driver.onFrame = { [weak driver] due, _, woke, acquire in
            guard !finished else { return }
            guard inFlight < 2 else { paced += 1; return }
            let anchor = min(woke, due)
            do {
                let x = UInt32(32 + (samples.count % 16)), y = UInt32(32 + (samples.count % 13))
                let planned = try Authoring.planTiles(
                    extent: Self.extent, minimumX: x, minimumY: y,
                    maximumX: x + UInt32(Self.tile), maximumY: y + UInt32(Self.tile))
                try Authoring.commitEdit(
                    document: document, textureSetID: textureSetID, semantic: "pbr.base_color",
                    tiles: planned.tiles, x: x, y: y,
                    pixel: [UInt8(truncatingIfNeeded: samples.count), 0x40, 0x20],
                    step: "sustained-\(samples.count)", snapshot: snapshot)
            } catch {
                skipped += 1
                if firstError == nil { firstError = "\(error)" }
                return
            }
            guard let drawable = acquire() else { skipped += 1; return }
            inFlight += 1

            var bytes = [UInt8](repeating: 0, count: Self.tile * Self.tile * 4)
            for index in stride(from: 0, to: bytes.count, by: 4) {
                bytes[index] = UInt8(truncatingIfNeeded: samples.count)
                bytes[index + 1] = 0x40; bytes[index + 2] = 0x20; bytes[index + 3] = 0xFF
            }
            authored.replace(region: MTLRegionMake2D(0, 0, Self.tile, Self.tile),
                             mipmapLevel: 0, withBytes: bytes, bytesPerRow: Self.tile * 4)

            guard let buffer = queue.makeCommandBuffer(),
                  let blit = buffer.makeBlitCommandEncoder() else {
                inFlight -= 1; skipped += 1; return
            }
            blit.copy(from: authored, sourceSlice: 0, sourceLevel: 0,
                      sourceOrigin: MTLOrigin(x: 0, y: 0, z: 0),
                      sourceSize: MTLSize(width: Self.tile, height: Self.tile, depth: 1),
                      to: drawable.texture, destinationSlice: 0, destinationLevel: 0,
                      destinationOrigin: MTLOrigin(x: 0, y: 0, z: 0))
            blit.endEncoding()

            drawable.addPresentedHandler { presented in
                inFlight -= 1
                let visible = presented.presentedTime
                guard visible > 0 else { skipped += 1; return }
                guard warmup == 0 else { warmup -= 1; return }
                samples.append(Sample(at: visible, latency: (visible - anchor) * 1000.0))
                if samples.count % 120 == 0 {
                    let footprint = Self.physicalFootprint()
                    peakPhysical = max(peakPhysical, footprint)
                    let now = ProcessInfo.processInfo.thermalState
                    if now.rawValue > thermal.rawValue { thermal = now }
                    print("CTEX-PROGRESS frames=\(samples.count) "
                          + "elapsed=\(Int(visible - started))s "
                          + "footprint=\(footprint / (1024 * 1024))MB "
                          + "thermal=\(Self.name(now)) paced=\(paced) skipped=\(skipped)")
                }
                if visible >= deadline && !finished {
                    finished = true
                    driver?.stop()
                    done.fulfill()
                }
            }
            buffer.present(drawable)
            buffer.commit()
        }

        print("CTEX-STEP driver-start")
        driver.start()
        let outcome = XCTWaiter().wait(for: [done], timeout: minutes * 60.0 + 120.0)
        driver.stop()
        XCTAssertEqual(outcome, .completed, "sustained run did not finish: \(firstError ?? "none")")
        peakPhysical = max(peakPhysical, Self.physicalFootprint())

        // The budgets are about drift, so compare the first minute with the
        // final window rather than reporting one figure for the whole run.
        let first = samples.filter { $0.at < started + 60.0 }.map(\.latency)
        let last = samples.filter { $0.at >= deadline - finalWindow * 60.0 }.map(\.latency)
        XCTAssertGreaterThan(first.count, 60, "too few frames in the opening minute")
        XCTAssertGreaterThan(last.count, 60, "too few frames in the final window")

        print("CTEX-STEP loop-done frames=\(samples.count)")
        let pressure = try exerciseMemoryPressure()
        let lifecycle = try exerciseSuspendResume()
        let loss = try exerciseDeviceLoss()

        let run: [String: Any] = [
            "duration_minutes": (samples.last!.at - started) / 60.0,
            "final_window_minutes": finalWindow,
            "initial_latency_ms": percentile(first, 0.95),
            "final_latency_ms": percentile(last, 0.95),
            "peak_physical_bytes": Int(peakPhysical),
            "thermal_state": Self.name(thermal),
            "memory_pressure_passed": pressure.passed,
            "suspend_resume_passed": lifecycle.passed,
            "device_loss_passed": loss.passed,
            "frames": samples.count,
            "paced": paced,
            "skipped": skipped,
            "input_rate_hz": Double(surface.refresh),
            "refresh_rate_hz": Double(surface.refresh),
            "fixtures": [
                "memory_pressure": pressure.detail,
                "suspend_resume": lifecycle.detail,
                "device_loss": loss.detail,
            ],
        ]
        let payload: [String: Any] = [
            "final_five_minute_p95_ms": percentile(last, 0.95),
            "peak_physical_bytes": Int(peakPhysical),
            "sustained": run,
        ]
        let data = try JSONSerialization.data(withJSONObject: payload,
                                              options: [.prettyPrinted, .sortedKeys])
        let attachment = XCTAttachment(data: data, uniformTypeIdentifier: "public.json")
        attachment.name = "sustained.json"
        attachment.lifetime = .keepAlways
        add(attachment)
        print("CTEX-SUSTAINED-BEGIN")
        print(String(data: data, encoding: .utf8) ?? "{}")
        print("CTEX-SUSTAINED-END")
    }

    /// The three recovery fixtures on their own, so they can be developed and
    /// checked in seconds rather than behind the twenty-minute workload.
    func testRecoveryFixtures() throws {
        let pressure = try exerciseMemoryPressure()
        let lifecycle = try exerciseSuspendResume()
        let loss = try exerciseDeviceLoss()
        for (name, outcome) in [("memory_pressure", pressure),
                                ("suspend_resume", lifecycle),
                                ("device_loss", loss)] {
            print("CTEX-FIXTURE \(name) passed=\(outcome.passed) \(outcome.detail)")
        }
        XCTAssertTrue(pressure.passed, "memory pressure: \(pressure.detail)")
        XCTAssertTrue(lifecycle.passed, "suspend/resume: \(lifecycle.detail)")
        XCTAssertTrue(loss.passed, "device loss: \(loss.detail)")
    }

    // MARK: - recovery fixtures

    /// Memory pressure: with cache resident and an operation admitted under
    /// limits too tight to hold it, the ledger must shed cache rather than
    /// exceed the budget, and it must name what it shed.
    private func exerciseMemoryPressure() throws -> (passed: Bool, detail: [String: Any]) {
        var ledger: OpaquePointer?
        try Authoring.check(ctex_resource_ledger_create(&ledger), "ledger_create")
        defer { ctex_resource_ledger_destroy(ledger) }

        let sink = EvictionBox()
        let box = Unmanaged.passRetained(sink)
        defer { box.release() }
        try Authoring.check(ctex_resource_ledger_set_cache_eviction_callback(
            ledger,
            { identity, user in
                guard let user else { return }
                Unmanaged<EvictionBox>.fromOpaque(user).takeUnretainedValue().evict(identity)
            },
            box.toOpaque()), "set_cache_eviction_callback")

        let cacheBytes = 512 * 1024
        for identity in 0..<UInt64(8) {
            var allocation = ctex_resource_allocation_descriptor()
            allocation.size = UInt32(MemoryLayout<ctex_resource_allocation_descriptor>.size)
            allocation.allocation_identity = identity + 1
            allocation.category = UInt32(CTEX_RESOURCE_CACHE.rawValue)
            allocation.physical_bytes = cacheBytes
            allocation.roles = UInt32(CTEX_RESOURCE_CPU_RESIDENT.rawValue)
            try Authoring.check(ctex_resource_ledger_upsert(ledger, &allocation), "upsert")
        }

        var before = ctex_resource_accounting_report()
        before.size = UInt32(MemoryLayout<ctex_resource_accounting_report>.size)
        try Authoring.check(
            ctex_resource_ledger_get_report(ledger, &before, nil, 0), "get_report")

        var requirement = ctex_resource_requirement()
        requirement.size = UInt32(MemoryLayout<ctex_resource_requirement>.size)
        requirement.category = UInt32(CTEX_RESOURCE_DOCUMENT_STORAGE.rawValue)
        requirement.physical_bytes = 2 * 1024 * 1024
        requirement.roles = UInt32(CTEX_RESOURCE_CPU_RESIDENT.rawValue)

        var admission = ctex_resource_admission_descriptor()
        admission.size = UInt32(MemoryLayout<ctex_resource_admission_descriptor>.size)
        admission.limits.size = UInt32(MemoryLayout<ctex_resource_budget_limits>.size)
        // Four megabytes of cache are already resident and the operation needs
        // two more, so it fits only if the ledger sheds cache first.
        admission.limits.cpu_bytes = 4 * 1024 * 1024
        admission.limits.gpu_bytes = 64 * 1024 * 1024
        admission.limits.backing_store_bytes = 64 * 1024 * 1024
        admission.limits.temporary_bytes = 64 * 1024 * 1024

        var reservation: OpaquePointer?
        var report = ctex_resource_admission_report()
        report.size = UInt32(MemoryLayout<ctex_resource_admission_report>.size)
        let admitted: ctex_result = "sustained-pressure".withCString { operation in
            admission.operation = operation
            return withUnsafePointer(to: &requirement) { fixed in
                admission.fixed_requirements = fixed
                admission.fixed_requirement_count = 1
                return ctex_resource_ledger_admit(ledger, &admission, &reservation, &report)
            }
        }
        try Authoring.check(admitted, "ledger_admit")
        defer { if reservation != nil { ctex_resource_reservation_destroy(reservation) } }

        var after = ctex_resource_accounting_report()
        after.size = UInt32(MemoryLayout<ctex_resource_accounting_report>.size)
        try Authoring.check(
            ctex_resource_ledger_get_report(ledger, &after, nil, 0), "get_report")

        let passed = report.status == UInt32(CTEX_RESOURCE_ADMITTED_WHOLE.rawValue)
            && report.evicted_allocation_count > 0
            && sink.count == report.evicted_allocation_count
            && after.cpu_resident_bytes < before.cpu_resident_bytes
        return (passed, [
            "evicted_allocations": report.evicted_allocation_count,
            "eviction_callbacks": sink.count,
            "cpu_resident_before": before.cpu_resident_bytes,
            "cpu_resident_after": after.cpu_resident_bytes,
            "limit_bytes": admission.limits.cpu_bytes,
            "admission_status": Int(report.status),
        ])
    }

    /// Suspend and resume: quiescing must stop admission, and resuming must
    /// restore it. This is the app being backgrounded and brought back.
    private func exerciseSuspendResume() throws -> (passed: Bool, detail: [String: Any]) {
        var ledger: OpaquePointer?
        try Authoring.check(ctex_resource_ledger_create(&ledger), "ledger_create")
        defer { ctex_resource_ledger_destroy(ledger) }

        // Quiescing checkpoints through an autosave session, so suspend/resume
        // is only meaningful with a real one behind it.
        let recovery = FileManager.default.temporaryDirectory
            .appendingPathComponent("ctex-sustained-recovery", isDirectory: true)
        try FileManager.default.createDirectory(
            at: recovery, withIntermediateDirectories: true)
        var autosave: OpaquePointer?
        try recovery.path.withCString { directory in
            try "sustained".withCString { key in
                var config = ctex_project_autosave_config_descriptor()
                config.size =
                    UInt32(MemoryLayout<ctex_project_autosave_config_descriptor>.size)
                config.recovery_directory = directory
                config.recovery_key = key
                config.interval_milliseconds = 60_000
                try Authoring.check(
                    ctex_project_autosave_session_create(&config, &autosave),
                    "autosave_session_create")
            }
        }
        defer { ctex_project_autosave_session_destroy(autosave) }

        let admittedBefore = !admissionRefused(ledger)

        var descriptor = ctex_project_quiesce_descriptor()
        descriptor.size = UInt32(MemoryLayout<ctex_project_quiesce_descriptor>.size)
        descriptor.current_revision = 1
        descriptor.deadline_milliseconds = 1000
        var quiesce = ctex_project_quiesce_report()
        quiesce.size = UInt32(MemoryLayout<ctex_project_quiesce_report>.size)
        try Authoring.check(
            ctex_project_lifecycle_quiesce(ledger, autosave, &descriptor, &quiesce), "quiesce")

        let refusedWhileQuiesced = admissionRefused(ledger)
        try Authoring.check(ctex_project_lifecycle_resume(ledger), "lifecycle_resume")
        let readmitted = !admissionRefused(ledger)

        let passed = admittedBefore && quiesce.admissions_stopped == 1
            && quiesce.work_drained == 1 && refusedWhileQuiesced && readmitted
        return (passed, [
            "admitted_before_suspend": admittedBefore,
            "admissions_stopped": Int(quiesce.admissions_stopped),
            "work_drained": Int(quiesce.work_drained),
            "active_operations": quiesce.active_operation_count,
            "refused_while_quiesced": refusedWhileQuiesced,
            "readmitted_after_resume": readmitted,
            "quiesce_status": Int(quiesce.status),
        ])
    }

    /// Device loss: reporting it must restore the session to its last revision
    /// and leave nothing in flight.
    private func exerciseDeviceLoss() throws -> (passed: Bool, detail: [String: Any]) {
        var session: OpaquePointer?
        try Authoring.check(
            ctex_host_execution_session_create(7, &session), "session_create")
        defer { ctex_host_execution_session_destroy(session) }

        var report: OpaquePointer?
        try Authoring.check(
            ctex_host_execution_session_report_device_loss(session, &report), "report_device_loss")
        defer { ctex_host_recovery_report_destroy(report) }

        var loss = ctex_host_device_loss_info()
        loss.size = UInt32(MemoryLayout<ctex_host_device_loss_info>.size)
        try Authoring.check(
            ctex_host_recovery_report_get_info(report, &loss, nil, 0, nil, 0, nil, 0),
            "recovery_report_get_info")

        var after = ctex_host_execution_session_info()
        after.size = UInt32(MemoryLayout<ctex_host_execution_session_info>.size)
        try Authoring.check(
            ctex_host_execution_session_get_info(session, &after), "session_get_info")

        let passed = loss.restored == 1 && loss.recovered_revision == 7
            && after.active_submission_count == 0 && after.revision == 7
        return (passed, [
            "restored": Int(loss.restored),
            "recovered_revision": Int(loss.recovered_revision),
            "cancelled_submissions": loss.cancelled_submission_count,
            "released_resources": loss.released_resource_count,
            "revision_after_recovery": Int(after.revision),
            "active_after_recovery": after.active_submission_count,
        ])
    }

    private func admissionRefused(_ ledger: OpaquePointer?) -> Bool {
        var admission = ctex_resource_admission_descriptor()
        admission.size = UInt32(MemoryLayout<ctex_resource_admission_descriptor>.size)
        admission.limits.size = UInt32(MemoryLayout<ctex_resource_budget_limits>.size)
        admission.per_work_item.size = UInt32(MemoryLayout<ctex_resource_requirement>.size)
        admission.per_work_item.category = UInt32(CTEX_RESOURCE_TEMPORARY.rawValue)
        admission.per_work_item.physical_bytes = 64 * 1024
        admission.per_work_item.roles = UInt32(CTEX_RESOURCE_CPU_RESIDENT.rawValue)
        admission.work_item_count = 1
        admission.limits.cpu_bytes = 8 * 1024 * 1024
        admission.limits.temporary_bytes = 8 * 1024 * 1024
        var reservation: OpaquePointer?
        var report = ctex_resource_admission_report()
        report.size = UInt32(MemoryLayout<ctex_resource_admission_report>.size)
        let result = "sustained-lifecycle".withCString { operation in
            admission.operation = operation
            return ctex_resource_ledger_admit(ledger, &admission, &reservation, &report)
        }
        if reservation != nil { ctex_resource_reservation_destroy(reservation) }
        return result != CTEX_RESULT_SUCCESS || reservation == nil
    }

    // MARK: - fixtures

    /// Counts eviction callbacks. A ctypes-style callback cannot assert, so it
    /// records and the assertion happens after the call returns.
    private final class EvictionBox {
        private(set) var evicted: [UInt64] = []
        var count: Int { evicted.count }
        func evict(_ identity: UInt64) { evicted.append(identity) }
    }

    /// The app's own physical footprint, which is what iOS holds against the
    /// memory limit, rather than anything the library reports about itself.
    private static func physicalFootprint() -> UInt64 {
        var info = task_vm_info_data_t()
        var count = mach_msg_type_number_t(MemoryLayout<task_vm_info_data_t>.size
                                           / MemoryLayout<natural_t>.size)
        let result = withUnsafeMutablePointer(to: &info) {
            $0.withMemoryRebound(to: integer_t.self, capacity: Int(count)) {
                task_info(mach_task_self_, task_flavor_t(TASK_VM_INFO), $0, &count)
            }
        }
        return result == KERN_SUCCESS ? info.phys_footprint : 0
    }

    private static func name(_ state: ProcessInfo.ThermalState) -> String {
        switch state {
        case .nominal: return "nominal"
        case .fair: return "fair"
        case .serious: return "serious"
        case .critical: return "critical"
        @unknown default: return "unknown"
        }
    }

    private func percentile(_ values: [Double], _ fraction: Double) -> Double {
        guard !values.isEmpty else { return 0 }
        let sorted = values.sorted()
        let rank = Int((Double(sorted.count - 1) * fraction).rounded())
        return sorted[rank]
    }

    private func makeTextureSet(document: OpaquePointer?) throws -> String {
        var descriptor = ctex_texture_set_descriptor()
        descriptor.size = UInt32(MemoryLayout<ctex_texture_set_descriptor>.size)
        try "Sustained".withCString { name in
            try "sustained".withCString { key in
                try "uv0".withCString { uv in
                    descriptor.display_name = name
                    descriptor.partition_key = key
                    descriptor.uv_set = uv
                    descriptor.width = Self.extent
                    descriptor.height = Self.extent
                    descriptor.default_bit_depth = 8
                    try Authoring.check(
                        ctex_document_create_texture_set(document, &descriptor), "texture set")
                }
            }
        }
        var required = 0, count = 0
        try Authoring.check(ctex_document_get_texture_set_ids(document, nil, 0, &required, &count),
                            "ids size")
        var raw = [CChar](repeating: 0, count: required)
        try Authoring.check(
            ctex_document_get_texture_set_ids(document, &raw, required, &required, &count),
            "ids read")
        let identifier = String(cString: raw)
        for semantic in ["pbr.base_color", "pbr.roughness", "pbr.metallic", "pbr.normal"] {
            try identifier.withCString { set in
                try semantic.withCString { name in
                    try Authoring.check(
                        ctex_texture_set_set_channel_enabled(document, set, name, 1, 0),
                        "enable \(semantic)")
                }
            }
        }
        try identifier.withCString { set in
            try Authoring.check(ctex_texture_set_configure_tile_history(document, set, 96 << 20),
                                "history budget")
        }
        return identifier
    }

    private func attachLayer(device: MTLDevice) throws -> (layer: CAMetalLayer, refresh: Int) {
        var result: (CAMetalLayer, Int)?
        let ready = expectation(description: "layer")
        DispatchQueue.main.async {
            let scene = UIApplication.shared.connectedScenes.first as? UIWindowScene
            let window = scene?.windows.first ?? UIApplication.shared.windows.first
            let layer = CAMetalLayer()
            layer.device = device
            layer.pixelFormat = .bgra8Unorm
            layer.framebufferOnly = false
            layer.frame = CGRect(x: 0, y: 0, width: 256, height: 256)
            layer.drawableSize = CGSize(width: 256, height: 256)
            window?.layer.addSublayer(layer)
            result = (layer, scene?.screen.maximumFramesPerSecond
                      ?? UIScreen.main.maximumFramesPerSecond)
            ready.fulfill()
        }
        wait(for: [ready], timeout: 10)
        let (layer, refresh) = try XCTUnwrap(result, "no layer")
        return (layer, refresh)
    }
}
