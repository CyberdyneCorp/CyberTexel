## MODIFIED Requirements

### Requirement: Platform packages
The project SHALL produce packages for macOS, Linux, iOS, Windows and Android, each containing the C header, the library, the licence and the attribution file, and each SHALL be smoke-tested by a program that links it. Android runtime validation SHALL run on a named physical device/API matrix; an iOS device result SHALL NOT substitute for it.

#### Scenario: Unexercised package
- **WHEN** a package is produced but its smoke test does not run
- **THEN** the release gate SHALL fail rather than publish an unexercised binary

#### Scenario: Android device matrix
- **WHEN** the Android release is gated
- **THEN** each declared device/API configuration SHALL run the native smoke test on physical hardware
- **AND** any missing configuration SHALL be reported as unmeasured rather than passed
