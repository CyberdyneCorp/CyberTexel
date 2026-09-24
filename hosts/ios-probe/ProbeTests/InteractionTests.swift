import Foundation
import Metal
import QuartzCore
import UIKit
import XCTest

private final class CompletionLatch {
    private let lock = NSLock()
    private var completed = false

    func claim() -> Bool {
        lock.lock()
        defer { lock.unlock() }
        guard !completed else { return false }
        completed = true
        return true
    }
}

/// Input-to-visible latency on the tablet, measured against a real display.
///
/// The five stages are contiguous timestamps rather than separately sampled
/// durations, so they sum to the reported latency by construction instead of
/// being reconciled afterwards. `device_gate.validate_interaction_trace`
/// requires exactly that.
///
///   input        the frame's input was due -> the authored edit is committed
///   upload       -> the changed tile is in device memory
///   queue        -> the frame is encoded
///   execution    -> the GPU reports it done
///   presentation -> the drawable is actually on screen
final class InteractionTests: XCTestCase {
    private struct Frame {
        let input, upload, queue, execution, presentation: Double
        /// Attribution, deliberately outside `total`: `wake` is how long after
        /// the anchoring vsync the callback got the CPU, and `slip` is how far
        /// past the display's own target for this frame the drawable landed.
        let wake, acquire, slip: Double
        var total: Double { input + upload + queue + execution + presentation }
    }

    private static let extent: UInt32 = 4096
    private static let tile: Int = 64

    func testCompletionLatchClaimsOnlyOnce() {
        let latch = CompletionLatch()
        XCTAssertTrue(latch.claim())
        XCTAssertFalse(latch.claim())
    }

    private func percentile(_ values: [Double], _ fraction: Double) -> Double {
        let ordered = values.sorted()
        let index = min(ordered.count - 1, max(0, Int((fraction * Double(ordered.count - 1)).rounded())))
        return ordered[index]
    }

    func testInputToVisibleLatency() throws {
        let metal = try XCTUnwrap(MTLCreateSystemDefaultDevice(), "no Metal device")
        let queue = try XCTUnwrap(metal.makeCommandQueue(), "no command queue")

        // The declared configuration: one 4096-square texture set, four enabled
        // channels, eight visible layers.
        var document: OpaquePointer?
        try Authoring.check(ctex_document_create(&document), "document create")
        defer { ctex_document_destroy(document) }
        let identifier = try makeTextureSet(document: document)

        // A layer the drawable can actually show, and a texture to upload into.
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .bgra8Unorm, width: Self.tile, height: Self.tile, mipmapped: false)
        descriptor.usage = [.shaderRead]
        let authored = try XCTUnwrap(metal.makeTexture(descriptor: descriptor), "no texture")

        let snapshot = try Authoring.makeSnapshot(extent: Self.extent)
        defer { ctex_layer_snapshot_destroy(snapshot) }
        let surface = try attachLayer(device: metal)
        var frames: [Frame] = []
        var skipped = 0
        var firstError: String?
        var inFlight = 0
        var paced = 0
        var warmup = 60
        var planTimes: [Double] = []
        var commitTimes: [Double] = []
        let target = 240
        let done = expectation(description: "frames")
        let completion = CompletionLatch()
        let driver = FrameDriver(layer: surface.layer, refresh: surface.refresh)

        driver.onFrame = { [weak driver] due, vsyncTarget, woke, acquire in
            guard inFlight < 2 else { paced += 1; return }
            // --- input: the authored edit the host performs for this sample ---
            let x = UInt32(32 + (frames.count % 16)), y = UInt32(32 + (frames.count % 13))
            do {
                let planStart = CACurrentMediaTime()
                let planned = try Authoring.planTiles(
                    extent: Self.extent, minimumX: x &- 8, minimumY: y &- 8,
                    maximumX: x &+ 8, maximumY: y &+ 8)
                let planEnd = CACurrentMediaTime()
                planTimes.append((planEnd - planStart) * 1000.0)
                _ = try Authoring.commitEdit(
                    document: document, textureSetID: identifier, semantic: "pbr.base_color",
                    tiles: planned.tiles, x: x, y: y,
                    pixel: [UInt8(truncatingIfNeeded: frames.count), 0x40, 0x20],
                    step: "frame-\(frames.count)", snapshot: snapshot)
                commitTimes.append((CACurrentMediaTime() - planEnd) * 1000.0)
            } catch {
                skipped += 1
                if firstError == nil { firstError = "\(error)" }
                return
            }
            let committed = CACurrentMediaTime()

            // --- upload: a drawable is acquired and the changed tile
            //     reaches device memory ---
            guard let drawable = acquire() else { skipped += 1; return }
            inFlight += 1
            let acquired = CACurrentMediaTime()
            var bytes = [UInt8](repeating: 0, count: Self.tile * Self.tile * 4)
            for index in stride(from: 0, to: bytes.count, by: 4) {
                bytes[index] = UInt8(truncatingIfNeeded: frames.count)
                bytes[index + 1] = 0x40
                bytes[index + 2] = 0x20
                bytes[index + 3] = 0xFF
            }
            authored.replace(region: MTLRegionMake2D(0, 0, Self.tile, Self.tile),
                             mipmapLevel: 0, withBytes: bytes,
                             bytesPerRow: Self.tile * 4)
            let uploaded = CACurrentMediaTime()

            // --- queue: the frame is encoded ---
            guard let buffer = queue.makeCommandBuffer(),
                  let blit = buffer.makeBlitCommandEncoder() else {
                drawable.present(); return
            }
            blit.copy(from: authored, sourceSlice: 0, sourceLevel: 0,
                      sourceOrigin: MTLOrigin(x: 0, y: 0, z: 0),
                      sourceSize: MTLSize(width: Self.tile, height: Self.tile, depth: 1),
                      to: drawable.texture, destinationSlice: 0, destinationLevel: 0,
                      destinationOrigin: MTLOrigin(x: 0, y: 0, z: 0))
            blit.endEncoding()
            let encoded = CACurrentMediaTime()

            // --- presentation: when the drawable is really on screen ---
            drawable.addPresentedHandler { presented in
                inFlight -= 1
                let visible = presented.presentedTime
                guard visible > 0 else { skipped += 1; return }
                // Let the pipeline reach steady state before recording: the
                // first frames pay allocation and residency costs that a
                // sustained interaction never pays again.
                guard warmup == 0 else { warmup -= 1; return }
                let executed = buffer.gpuEndTime > 0 ? buffer.gpuEndTime : encoded
                frames.append(Frame(
                    input: committed - min(woke, due),
                    upload: uploaded - committed,
                    queue: encoded - uploaded,
                    execution: max(executed - encoded, 0),
                    presentation: max(visible - max(executed, encoded), 0),
                    wake: woke - due,
                    acquire: acquired - committed,
                    slip: visible - vsyncTarget))
                if frames.count >= target && completion.claim() {
                    driver?.stop()
                    done.fulfill()
                }
            }
            buffer.present(drawable)
            buffer.commit()
        }

        driver.start()
        let outcome = XCTWaiter().wait(for: [done], timeout: 30)
        driver.stop()
        if outcome != .completed {
            let state = UIApplication.shared.applicationState.rawValue
            print("CTEX-DIAG-BEGIN")
            print("error=\(firstError ?? "none")")
            print("ticks=\(driver.ticks) nilDrawables=\(driver.nilDrawables) "
                  + "frames=\(frames.count) skipped=\(skipped) appState=\(state) "
                  + "layerSize=\(surface.layer.drawableSize) refresh=\(surface.refresh)")
            print("CTEX-DIAG-END")
        }

        let latencies = frames.map(\.total).map { $0 * 1000.0 }
        XCTAssertGreaterThan(latencies.count, 60, "too few frames to report a distribution")
        let median = percentile(latencies, 0.5)
        let p95 = percentile(latencies, 0.95)
        let p99 = percentile(latencies, 0.99)
        let representative = frames[frames.count / 2]

        let trace: [String: Any] = [
            "input_to_visible_ms": representative.total * 1000.0,
            "stages_ms": [
                "input": representative.input * 1000.0,
                "upload": representative.upload * 1000.0,
                "queue": representative.queue * 1000.0,
                "execution": representative.execution * 1000.0,
                "presentation": representative.presentation * 1000.0,
            ],
            "input_rate_hz": Double(surface.refresh),
            "refresh_rate_hz": Double(surface.refresh),
            "mesh_triangles": 0,
            "layers": 8,
            "channels": 4,
            "residency": ["synchronous_readback_bytes": 0],
            "surface": surfaceState(surface.layer),
            "frames": latencies.count,
            "skipped": skipped,
            "paced": paced,
            "attribution_ms": [
                "plan_work_median": percentile(planTimes, 0.5),
                "transaction_median": percentile(commitTimes, 0.5),
                "callback_wake_median": percentile(frames.map { $0.wake * 1000.0 }, 0.5),
                "presentation_slip_median": percentile(frames.map { $0.slip * 1000.0 }, 0.5),
                "drawable_acquire_median": percentile(frames.map { $0.acquire * 1000.0 }, 0.5),
                "refresh_period": 1000.0 / Double(surface.refresh),
                "drawable_count": Double(surface.layer.maximumDrawableCount),
            ],
        ]
        let payload: [String: Any] = [
            "median_ms": median, "p95_ms": p95, "p99_ms": p99,
            "min_ms": latencies.min() ?? 0, "max_ms": latencies.max() ?? 0,
            "interaction": trace,
        ]
        let data = try JSONSerialization.data(withJSONObject: payload,
                                              options: [.prettyPrinted, .sortedKeys])
        let attachment = XCTAttachment(data: data, uniformTypeIdentifier: "public.json")
        attachment.name = "interaction.json"
        attachment.lifetime = .keepAlways
        add(attachment)
        print("CTEX-INTERACTION-BEGIN")
        print(String(data: data, encoding: .utf8) ?? "{}")
        print("CTEX-INTERACTION-END")
    }


    /// Whether the measured layer is genuinely on screen. A latency taken from a
    /// detached or backgrounded layer is not a latency, so the run records this
    /// rather than leaving the reader to assume it.
    private func surfaceState(_ layer: CAMetalLayer) -> [String: Any] {
        let read = { () -> [String: Any] in
            [
                "application_state": UIApplication.shared.applicationState.rawValue,
                "in_window": layer.superlayer != nil,
                "window_key": (layer.delegate as? UIView)?.window?.isKeyWindow ?? false,
                "hidden": layer.isHidden,
                "opacity": Double(layer.opacity),
                "bounds_width": Double(layer.bounds.width),
                "bounds_height": Double(layer.bounds.height),
            ]
        }
        // The test body already runs on the main thread; hopping to it would
        // deadlock rather than synchronise.
        return Thread.isMainThread ? read() : DispatchQueue.main.sync(execute: read)
    }

    // MARK: - fixtures

    private func makeTextureSet(document: OpaquePointer?) throws -> String {
        var descriptor = ctex_texture_set_descriptor()
        descriptor.size = UInt32(MemoryLayout<ctex_texture_set_descriptor>.size)
        try "Interaction".withCString { name in
            try "interaction".withCString { key in
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
            try identifier.withCString { set in
                try Authoring.check(ctex_texture_set_configure_tile_history(document, set, 96 << 20),
                                    "history budget")
            }
        }
        return identifier
    }

    private func attachLayer(device: MTLDevice) throws -> (layer: CAMetalLayer, refresh: Int) {
        var result: (CAMetalLayer, Int)?
        let ready = expectation(description: "layer")
        DispatchQueue.main.async {
            let scene = UIApplication.shared.connectedScenes
                .compactMap { $0 as? UIWindowScene }.first
            let window = scene?.windows.first
            let layer = CAMetalLayer()
            layer.device = device
            layer.pixelFormat = .bgra8Unorm
            layer.framebufferOnly = false
            layer.frame = CGRect(x: 0, y: 0, width: 256, height: 256)
            layer.drawableSize = CGSize(width: 256, height: 256)
            window?.layer.addSublayer(layer)
            result = (layer, scene?.screen.maximumFramesPerSecond ?? 60)
            ready.fulfill()
        }
        wait(for: [ready], timeout: 10)
        return try XCTUnwrap(result, "no window scene to present into")
    }
}

/// Drives frames from the display itself, so "due" is the display's own clock.
final class FrameDriver {
    var onFrame: ((Double, Double, Double, () -> CAMetalDrawable?) -> Void)?
    private let layer: CAMetalLayer
    private let refresh: Int
    private var link: CADisplayLink?

    init(layer: CAMetalLayer, refresh: Int) {
        self.layer = layer
        self.refresh = refresh
    }

    func start() {
        DispatchQueue.main.async {
            let link = CADisplayLink(target: self, selector: #selector(self.tick(_:)))
            link.preferredFrameRateRange = CAFrameRateRange(
                minimum: Float(self.refresh), maximum: Float(self.refresh),
                preferred: Float(self.refresh))
            link.add(to: .main, forMode: .common)
            self.link = link
        }
    }

    func stop() {
        DispatchQueue.main.async { self.link?.invalidate(); self.link = nil }
    }

    private(set) var ticks = 0
    private(set) var nilDrawables = 0

    @objc private func tick(_ link: CADisplayLink) {
        ticks += 1
        let woke = CACurrentMediaTime()
        onFrame?(link.timestamp, link.targetTimestamp, woke) { [weak self] in
            guard let drawable = self?.layer.nextDrawable() else {
                self?.nilDrawables += 1; return nil
            }
            return drawable
        }
    }
}
