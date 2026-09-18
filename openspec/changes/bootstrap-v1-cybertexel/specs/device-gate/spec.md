# device-gate — What A Performance Number May Claim

## ADDED Requirements

### Requirement: A named reference device
Performance claims SHALL be made against named reference devices — at minimum one desktop and one tablet — recorded with their CPU, GPU, memory and operating system version. A figure measured anywhere else SHALL NOT be reported as meeting a budget.

#### Scenario: Figure from an unnamed machine
- **WHEN** a benchmark runs on a machine that is not a declared reference device
- **THEN** its figures SHALL be recorded as informational and SHALL NOT satisfy a budget

### Requirement: Budgeted operations
Budgets SHALL be declared for: applying one stamp, resolving and applying a complete stroke, compositing a texture set, emitting a material, evaluating a generator, a delta query, a tile readback, applying a smart material, and exporting a texture set.

#### Scenario: Every budget is named
- **WHEN** the budget table is read
- **THEN** each operation SHALL carry a numeric budget, the reference device it applies to, and the document configuration it was measured at

### Requirement: The interactive budget is stated in milliseconds
The per-stamp budget SHALL be expressed in milliseconds on the reference device at a stated texture set resolution, and SHALL be low enough that a stroke sustains an interactive rate on that device. The rate the budget targets SHALL be stated rather than implied.

#### Scenario: A stamp is within budget
- **WHEN** the stamp benchmark runs at the stated resolution on the reference desktop
- **THEN** its median and its 95th percentile SHALL both be compared against the budget

### Requirement: Cost scales with what is touched
The specification SHALL state, and the gate SHALL verify, that a stamp's cost scales with the area it touches rather than with the texture set's resolution.

#### Scenario: Same stamp, larger canvas
- **WHEN** the same stamp is applied to a 2048 and to a 16384 texture set
- **THEN** the measured costs SHALL be within a stated factor of each other, and a linear growth with resolution SHALL fail the gate

### Requirement: A gate that cannot fail protects nothing
A budget SHALL be compared against an absolute floor. A case counts as measured only where its figure clears the floor the gate compares against; a budget that no configuration can reach SHALL be reported as unreachable rather than recorded as passed.

#### Scenario: Unreachable budget
- **WHEN** a declared budget is one that no configuration meets
- **THEN** the gate SHALL report it as unreachable and SHALL NOT record a pass

### Requirement: A batch may not stand in for the operation it names
Where an operation is measured inside a batch, the figure SHALL attribute the cost of the operation the budget names rather than the cost of the batch around it.

#### Scenario: Stamps measured in a batch
- **WHEN** a hundred stamps are applied in one call to amortize the boundary crossing
- **THEN** the per-stamp figure SHALL exclude the batch's fixed cost, or the budget SHALL be declared as a per-batch budget instead

### Requirement: Coverage is counted over what the gate decides
Coverage SHALL be reported as the proportion of budgeted operations the gate actually decided on, not the proportion it happened to time. An operation that ran but was not compared SHALL count as uncovered.

#### Scenario: Reporting coverage
- **WHEN** the gate reports coverage
- **THEN** operations that were timed without being compared against a budget SHALL be counted as uncovered

### Requirement: Unmeasurable cases are reported, not skipped
A budget that could not be measured — because the device was absent, the backend was not compiled in, or the case did not run — SHALL be reported as unmeasured. It SHALL NOT be counted as a pass and SHALL NOT be silently omitted.

#### Scenario: Tablet reference device unavailable
- **WHEN** the tablet reference device is not available to CI
- **THEN** its budgets SHALL be reported as unmeasured and the desktop figures SHALL NOT be substituted for them

### Requirement: Memory budgets alongside time
Budgets SHALL cover peak working memory as well as time, for a stroke, a composite, an export and a smart material application, at stated document configurations.

#### Scenario: Memory budget on the tablet
- **WHEN** a stroke runs on the tablet reference device
- **THEN** its peak working memory SHALL be compared against the declared ceiling

### Requirement: Figures are dated and reproducible
Every recorded figure SHALL carry the date, the commit, the device and the configuration it was measured at, and the command that produces it SHALL be in the repository.

#### Scenario: Reproducing a claim
- **WHEN** a reader wants to check a published figure
- **THEN** the repository SHALL contain the command, and re-running it on the named device SHALL reproduce the figure within a stated variance

### Requirement: Regressions fail the build
A measured regression beyond a stated variance against the recorded baseline SHALL fail CI, naming the operation, the baseline and the measurement.

#### Scenario: A change makes strokes slower
- **WHEN** the stroke benchmark regresses beyond the stated variance
- **THEN** CI SHALL fail naming the operation and both figures

### Requirement: The budget table is a document, not a comment
The budgets, the reference devices and the current measurements SHALL live in a file in the repository that is updated by the gate rather than by hand.

#### Scenario: Reading current performance
- **WHEN** the budget document is read
- **THEN** it SHALL show every budgeted operation with its budget, its latest measurement, its device, its date and whether it is measured, unmeasured or unreachable
