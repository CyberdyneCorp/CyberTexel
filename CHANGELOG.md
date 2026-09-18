# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Pre-implementation. The specification exists; no code does.

### Added

- Founding specification `bootstrap-v1-cybertexel`: 22 capabilities, 304
  requirements, 368 scenarios, 204 tasks.
- `just` as the single task-runner entry point for building, testing,
  formatting and every gate, specified in `build-packaging` rather than left as
  an undocumented convention. CI invokes the same recipes a contributor runs.
- Repository scaffolding: OpenSpec validation and the capability index gate in
  CI, lint configuration, contributor documentation, a roadmap with the
  decisions taken and the questions still open, and the third-party attribution
  table the licence audit checks against.
