# Named reference-device CI

The [reference-device workflow](../.github/workflows/reference-device-gate.yml)
runs on pushes to `main` and by manual dispatch. Its self-hosted runner has the
`ctex-m3-pro-ipad-air` label and runs in the named MacBook Pro M3 Pro's logged-in
Aqua session, where a benchmark window can remain visible. The physical iPad
Air M3 must be connected, unlocked, awake and in Developer Mode for XCTest.
Pull requests never execute on this runner.

The runner is registered to this repository as
`macbook-pro-m3-pro-ipad-air`. On the current reference Mac, its program is in
`~/.local/share/cybertexel-actions-runner` and a user LaunchAgent
`~/Library/LaunchAgents/com.cybertexel.actions-runner.plist` keeps it online
in the Aqua session. Registration credentials and runner configuration stay
outside the repository. Check its availability in the repository's Actions
runner settings before dispatching the workflow.

`just gate-reference-devices`:

1. Checks the Mac and iPad model, memory, OS build, connection, Developer Mode
   and iPad lock state against `benchmarks/device_gate.json`.
2. Builds and runs the desktop WGSL host's visible 600-frame benchmark. A
   hidden window or absent adapter fails; the runner cannot substitute a CPU
   result for a device result.
3. Builds the iOS library, runs all seven probe tests on the iPad, and refuses
   failed or skipped tests. The suite includes the 20-minute sustained workload
   and recovery fixtures.
4. Extracts structured XCTest attachments and decides the desktop and tablet
   latency, memory and synchronous-readback budgets against
   `benchmarks/device_gate.json` and `benchmarks/baselines.json`.

The workflow uploads logs, `.xcresult` evidence and schema-1 measurements from
`build/reference-hosts/device-ci/`, including on failure. A disconnected or
locked iPad fails preflight explicitly. To diagnose one half locally, run
`just gate-reference-desktop` or `just gate-reference-ipad` on the named Mac.
