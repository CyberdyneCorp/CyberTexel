# Image storage

The `image` module stores pixels in independently allocated tiles. New images
use 64×64 tiles by default; callers may explicitly select another tile size.
Task 3.9 fixed 64×64 as the v1 default after the history path demonstrated that
a one-tile edit on a 16384×16384 channel retains one 64×64 physical tile. Host
delta publication uses the same tile coordinates and revisions, so storage,
history and upload work share one granularity.

An unmodified tile is represented by one clear pixel and allocates no pixel
payload. The first write materializes that tile, initializes every pixel to the
clear value, writes the requested pixel and marks the tile dirty. Dirty queries
are unique and deterministic in row-major tile order. Clearing dirty state does
not release or alter pixel data.

Formats contain one to four channels of 8-bit unsigned normalized, 16-bit
unsigned normalized or 32-bit floating-point data. Storage preserves the raw
channel bytes; interpretation and colour conversion belong to later
color-management work.

History can hold an opaque storage-owner snapshot and exchange it with the live
tile. The exchange publishes a new tile generation and image revision but does
not copy pixel bytes. Snapshot ownership is intentionally not exposed as a
mutable pixel buffer.
