# Bounded paint work

`<ctex/paint/work.hpp>` turns resolved stamp footprints into the only storage
tiles a paint operation may process. The projection/raster front end supplies
one or more exact, half-open `StampTexelFootprint` rectangles in canvas texel
coordinates. The scheduler expands each rectangle by the configured seam
dilation radius, clips it to the canvas, converts it to storage-tile
coordinates, and deduplicates the result in deterministic row-major order.

`plan_paint_work` returns a `PaintWorkReport` without running an operation.
`process_paint_work` validates the complete request first, builds the same
workset, and invokes its callback exactly once for each reported tile. The
report contains:

- the total canvas tile count as metadata;
- the number of supplied stamp footprints;
- candidate tile visits before overlap deduplication; and
- the exact ordered `processed_tiles` set.

Planning stores only tiles intersecting expanded footprints. It does not
allocate a canvas-sized bitmap or iterate over the canvas grid. Its cost is
therefore proportional to the stamp and dilation footprints, including when
the canvas is 16384 by 16384 or its dimensions approach the 32-bit limit.
Overlapping footprints from different stamps or frames contribute candidate
visits but produce one processing callback per tile. A dilation radius of zero
uses the stamp rectangles unchanged.

Footprints must already be clipped to and non-empty inside the canvas. This
makes the projection stage's reach calculation explicit and prevents a silent
full-canvas fallback. Invalid dimensions, tile sizes, footprints, or callbacks
are refused before any tile processor is invoked.
