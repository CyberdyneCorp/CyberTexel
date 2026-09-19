# Picking scenario coverage

`just test-picking-scenarios` runs every CTest carrying the
`picking-scenario` label. The suite maps the OpenSpec scenarios to executable
headless coverage as follows.

| OpenSpec scenario | CTest | Covered behavior |
|---|---|---|
| Orthographic pick | `picking-ray-construction` | Parallel view direction and near-plane origin |
| Everything a tool needs from one call | `picking-hit-record` | Every hit field, including texture set and material |
| Interpolated and geometric normals differ | `picking-hit-record` | Independent smooth and face normals |
| Clicking the background | `picking-hit-record` | Empty optional miss and distance-limited miss |
| Picking through the mesh | `picking-occlusion` | Ordered intersections through two walls |
| Ignoring back faces | `picking-occlusion` | Per-call front-face retention in both directions |
| Painting in a 2D view | `picking-uv-space` | UV ownership, surface reconstruction, and explicit miss |
| Snapping a placed decal | `picking-surface-snap` | Nearest surface, bounded miss, edges, and degenerates |
| Lasso selection | `picking-region-queries` | Concave lasso and inclusive partial coverage |
| Large mesh | `picking-spatial-index` | Bounded-leaf traversal over 2,097,152 indexed triangles |
| Structure is rebuilt on mesh change | `picking-spatial-index` | Revision-triggered rebuild and subsequent reuse |
| Picking exactly on an edge | `picking-boundary-determinism` | Repeated lowest-index edge and vertex ownership |
| Headless picking | all labeled tests | Every ray, UV, snap, region, and batch query runs without a device |
| Stroke resolved in one call | `picking-batch` | Ordered optional hit record per input sample |
| Cancelling a large batch | `picking-batch` | 5,000-ray cancellation with no partial hit array |

The large-mesh fixture shares vertices on a 1024 by 1024 grid so the test
exercises the specified multi-million-triangle scale without wasting memory on
three duplicate vertices per face.
