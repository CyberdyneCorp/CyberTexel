# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Pre-implementation. The specification exists; no code does.

### Added

- Founding specification `bootstrap-v1-cybertexel`: 24 capabilities, 332
  requirements, 400 scenarios, 222 tasks.
- `just` as the single task-runner entry point for building, testing,
  formatting and every gate, specified in `build-packaging` rather than left as
  an undocumented convention. CI invokes the same recipes a contributor runs.
- Repository scaffolding: OpenSpec validation and the capability index gate in
  CI, lint configuration, contributor documentation, a roadmap with the
  decisions taken and the questions still open, and the third-party attribution
  table the licence audit checks against.

### Changed

- Prioritize a real desktop/mobile painting slice before the full v1 catalogue;
  retain task IDs and bring reference hosts, bindings and budgets forward.
- Specify GPU-resident completion, asynchronous snapshots, recovery, full resource
  accounting and mobile lifecycle behavior.
- Separate brush coverage from build-up deposition and specify sample invariance,
  seam filtering and tangent-frame compatibility.
- Add extensible channel descriptors, CPU semantics for custom nodes, versioned
  bake requests, replay eligibility and persistent editable authoring.
