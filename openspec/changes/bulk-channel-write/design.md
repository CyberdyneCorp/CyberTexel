# Design

The host declares every tile intersecting the rectangle when it begins a transaction. The writer validates bounds, packed pixel size and row pitch, then all targets before mutating staged pixels. Complete tiles use `TiledImage::write_tile`; partial tiles use `write_pixel`. Commit retains only changed tiles and exchanges storage owners for undo and redo. The C ABI takes a size-tagged region descriptor and byte span. Bindings create an empty layer snapshot for pixel-only transactions.
