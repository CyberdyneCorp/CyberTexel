# Tile-scoped history

`TextureSet` owns a history for its enabled channel images. Before changing
pixels, a caller supplies a step identifier and the unique semantic-channel and
tile coordinates the operation may write. `begin_tile_history_step()` validates
every target, snapshots its current storage owner, and refuses the operation if
the declared worst case cannot fit the configured byte ceiling.

After the writes, `commit_tile_history_step()` compares tile generations and
retains only targets that actually changed. An unchanged operation creates no
step. A successful new step clears redo history and, when necessary, discards
the oldest undo steps to stay within the ceiling. A single step larger than the
whole ceiling is refused before editing; reducing a ceiling below already
retained history is also refused.

Undo and redo are the same ownership exchange in opposite directions. The live
tile and retained snapshot swap exact storage owners, publish a new generation
and revision, and copy zero pixel bytes. Before exchanging anything, restore
validates every channel layout and expected tile generation. An external write,
revision-history reset, or layout replacement therefore produces a typed stale
error without partially restoring the step.

A [texture-set transaction](transactions.md) can add a reversible layer-stack
command to the same step. Layer commands retain only changed entries, removed
identities, and a full identity order when ordering changed. Layer revisions
detect edits made outside history before any tile or structure is restored.

`tile_history_budget_report()` exposes:

- configured, retained, and currently available bytes;
- undo and redo step counts; and
- the number of additional steps that fit at a caller-supplied proposed size.

Pixel accounting charges one complete physical tile for each retained changed
target; compact structural command records do not masquerade as pixel
snapshots. Pixel storage does not scale with the canvas: on the default layout,
a one-tile edit of a 16384×16384 channel retains one 64×64 tile. Complete shared
allocation accounting remains roadmap task 19.1. An empty undo and an empty
redo have distinct typed errors so a host can expose their availability without
parsing a message.
