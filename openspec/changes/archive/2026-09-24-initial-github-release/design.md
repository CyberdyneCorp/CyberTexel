## Design

Use `VERSION` as the release version and a tag matching `v<VERSION>`. CI builds the Linux, macOS and iOS archives with the existing platform recipes and records their smoke outcomes. Fetch the artifacts from one verified commit, compare each embedded package manifest with the expected version, platform and smoke result, and publish exactly those three archives. Verify the published tag, assets and checksums after upload.

Repair CI failures at their sources: portable C++ threading, portable Rust lint compliance, sanitizer subprocess setup, deterministic test binary selection, formatting and readable examples. The physical-device gate must fail explicitly when the named iPad is unavailable; a green result requires the actual test suite and budget checks.

Calibrate desktop visible-latency baselines from the median of repeated runs on the named Mac, including CI runner runs, and retain their inputs in the repository. Run the visible desktop benchmark three times, require each run to pass the absolute ceilings, and use the middle p99 run for the 15% baseline decision. This avoids both a single exceptionally fast baseline and a single noisy CI tail as release decisions. Keep the independent device ceilings unchanged. Guard the iPad frame completion with a one-shot latch because two in-flight frames can both reach the sample target.

No Windows or Android job is a prerequisite for this release. The active `windows-android-release` change owns those deliverables.
