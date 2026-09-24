import Foundation

/// The bounded authored edit a painting host actually performs per frame.
///
/// `ctex_paint_preview_session_*` is a convenience: it allocates a whole-canvas
/// coverage buffer and finalizes against it, which costs 16 MiB and roughly
/// 16 ms for a single texel at 4096 square. Timing that measures the helper, not
/// painting.
///
/// The real path declares exactly the tiles a stamp touches and writes only
/// those: `ctex_paint_plan_work` reports the storage tiles a footprint reaches,
/// `ctex_texture_set_begin_transaction` declares that tile set up front, and the
/// transaction publishes one step.
enum Authoring {
    struct Failure: Error, CustomStringConvertible {
        let message: String
        var description: String { message }
    }

    static func check(_ result: ctex_result, _ what: String) throws {
        if result != CTEX_RESULT_SUCCESS {
            let diagnostic = ctex_get_last_diagnostic().map { String(cString: $0) } ?? "none"
            throw Failure(message: "\(what): \(diagnostic)")
        }
    }

    /// Storage tiles a square footprint reaches, from the library's planner.
    static func planTiles(extent: UInt32, minimumX: UInt32, minimumY: UInt32,
                          maximumX: UInt32, maximumY: UInt32)
        throws -> (tiles: [ctex_paint_tile_coordinate], canvasTiles: UInt64)
    {
        var work = ctex_paint_work_descriptor()
        try check(ctex_paint_work_init(&work), "work init")
        work.canvas_width = extent
        work.canvas_height = extent
        var footprint = ctex_paint_stamp_footprint(
            stamp_ordinal: 0, minimum_x: minimumX, minimum_y: minimumY,
            maximum_x: maximumX, maximum_y: maximumY)
        var info = ctex_paint_work_info()
        info.size = UInt32(MemoryLayout<ctex_paint_work_info>.size)
        var produced = 0
        return try withUnsafePointer(to: &footprint) { footprints in
            work.stamp_footprints = footprints
            work.stamp_footprint_count = 1
            try check(ctex_paint_plan_work(&work, &info, nil, 0, &produced), "plan sizing")
            var tiles = [ctex_paint_tile_coordinate](
                repeating: ctex_paint_tile_coordinate(), count: max(Int(produced), 1))
            try check(ctex_paint_plan_work(&work, &info, &tiles, tiles.count, &produced), "plan")
            return (Array(tiles.prefix(Int(produced))), info.canvas_tile_count)
        }
    }

    /// A transaction that edits texels declares no composite content, so both
    /// descriptor arrays take the documented empty form. The snapshot is still
    /// required: `begin_transaction` refuses a null one.
    static func makeSnapshot(extent: UInt32) throws -> OpaquePointer {
        var snapshot: OpaquePointer?
        try check(ctex_layer_snapshot_create(extent, extent, nil, 0, nil, 0, &snapshot),
                  "layer snapshot")
        guard let snapshot else { throw Failure(message: "layer snapshot returned no handle") }
        return snapshot
    }

    /// Commit one authored texel inside a declared tile set, as one undo step.
    static func commitEdit(document: OpaquePointer?, textureSetID: String, semantic: String,
                           tiles: [ctex_paint_tile_coordinate], x: UInt32, y: UInt32,
                           pixel: [UInt8], step: String,
                           snapshot: OpaquePointer) throws -> ctex_tile_history_commit_info {
        var info = ctex_tile_history_commit_info()
        info.size = UInt32(MemoryLayout<ctex_tile_history_commit_info>.size)
        try textureSetID.withCString { set in
            try semantic.withCString { semanticID in
                var targets = tiles.map { tile -> ctex_tile_history_target_descriptor in
                    var target = ctex_tile_history_target_descriptor()
                    target.size = UInt32(MemoryLayout<ctex_tile_history_target_descriptor>.size)
                    target.semantic_id = semanticID
                    target.tile_x = tile.x
                    target.tile_y = tile.y
                    return target
                }
                var transaction: OpaquePointer?
                try step.withCString { identifier in
                    try check(ctex_texture_set_begin_transaction(
                        document, set, identifier, &targets, targets.count, snapshot,
                        &transaction), "begin transaction")
                }
                var bytes = pixel
                try check(ctex_texture_set_transaction_write_pixel(
                    transaction, semanticID, x, y, &bytes, bytes.count), "transaction write")
                try check(ctex_texture_set_transaction_commit(transaction, &info),
                          "transaction commit")
            }
        }
        return info
    }
}
