## ADDED Requirements

### Requirement: Published initial GitHub release
The first GitHub release SHALL be tagged from a commit whose in-scope CI checks pass. The tag SHALL match `v<VERSION>`. The release SHALL contain exactly the Linux x64, macOS universal and iOS arm64 package archives built from that commit, with package manifests matching the version, platform and recorded smoke result. Its notes SHALL identify the supported platforms and any known limitations.

#### Scenario: Verified release artifacts
- **WHEN** the initial release is published
- **THEN** each of its three package archives SHALL be traceable to the tagged commit and pass manifest and archive-content validation

#### Scenario: CI or device gate fails
- **WHEN** an in-scope CI job or the named reference-device gate fails
- **THEN** the release SHALL not be published as verified

#### Scenario: Deferred platforms
- **WHEN** the initial release is assembled
- **THEN** Windows and Android SHALL remain tracked in the active follow-up change and SHALL not be represented by release assets
