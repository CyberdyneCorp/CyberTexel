# Image storage

The `image` module stores pixels in independently allocated tiles. New images
use 64×64 tiles by default; callers may select another tile size while the
slice-A history and upload measurements are collected. The final transport
layout is not frozen until task 3.9 measures undo granularity alongside upload
cost.

An unmodified tile is represented by one clear pixel and allocates no pixel
payload. The first write materializes that tile, initializes every pixel to the
clear value, writes the requested pixel and marks the tile dirty. Dirty queries
are unique and deterministic in row-major tile order. Clearing dirty state does
not release or alter pixel data.

Formats contain one to four channels of 8-bit unsigned normalized, 16-bit
unsigned normalized or 32-bit floating-point data. Storage preserves the raw
channel bytes; interpretation and colour conversion belong to later
color-management work.
