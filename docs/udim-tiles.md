# UDIM tile storage

A texture set opts into UDIM storage with `TextureSetDescriptor::udim_tiling`.
It remains one texture set with one layer stack and one channel definition; its
UDIMs are sparse partitions of that set's UV space.

`udim_number({u, v})` uses the standard `1001 + u + 10 * v` mapping. The U
coordinate is restricted to `0..9`, numbers below 1001 are rejected, and
overflow is checked. `udim_coordinate()` provides the inverse mapping.

`TextureSet::write_udim_pixels()` accepts a batch of UV-addressed pixels. It
prevalidates every coordinate and pixel width, maps each sample to local tile
coordinates, and allocates channel storage only when a value differs from the
channel default. A batch crossing an integer U or V boundary therefore writes
both tiles and reports which tile numbers changed and which were newly
allocated.

`ensure_udim_tiles()` declares the sparse set discovered from mesh UV
occupancy. It validates and sorts unique standard numbers and creates only
those logical tiles. Declaring three tiles out of a possible hundred therefore
reports exactly those three to export planning while their constant default
pixels still require no physical tile allocation.

The texture set's `channels()` object is the channel-definition prototype for
UDIM storage. Descriptors, enablement, precision and defaults are copied when a
tile is first written. Configuration added later is synchronized before the
next write to an existing tile. `read_udim_pixel()` returns the configured
default from an absent tile without allocating it.

`occupied_udim_tiles()` returns logical mesh-occupied or authored tiles in
ascending numeric order. `memory_report()` separately counts only resident
physical pixel tiles, so the unoccupied portion of a UDIM range costs no image
storage. Clearing a texture set releases authored UDIM pixels while retaining
that logical occupancy for later painting and export.
