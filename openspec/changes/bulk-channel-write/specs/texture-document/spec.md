## ADDED Requirements

### Requirement: Bulk channel write
The system SHALL let a host write a rectangular region, including a whole channel, in one undoable texture-set transaction. It SHALL validate the region, byte layout and every intersecting declared channel tile before modifying staged pixels. Commit SHALL retain only changed tiles under the declared history budget; undo and redo SHALL exchange tile storage owners.

#### Scenario: Full-channel write
- **WHEN** a host writes a rectangle covering the channel extent and commits
- **THEN** reading the channel SHALL return the supplied pixel bytes in row order
- **AND** undo SHALL restore the prior channel bytes

#### Scenario: Partial-tile region
- **WHEN** a region intersects only part of a tile
- **THEN** only pixels inside the region SHALL change

#### Scenario: Write outside declared targets
- **WHEN** any tile intersecting a region was not declared at transaction start
- **THEN** the write SHALL be refused before changing staged pixels

#### Scenario: Size or format mismatch
- **WHEN** the supplied byte count or row pitch does not match the channel format and region
- **THEN** the write SHALL be refused before changing staged pixels
