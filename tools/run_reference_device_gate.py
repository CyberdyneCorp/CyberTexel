#!/usr/bin/env python3
"""Run the named Mac and iPad reference-device release gate in CI."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import shutil
import subprocess
import sys

import device_gate


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "build" / "reference-hosts" / "device-ci"
CONFIG = ROOT / "benchmarks" / "device_gate.json"
BASELINES = ROOT / "benchmarks" / "baselines.json"
DESKTOP_BUDGETS = (
    "desktop-visible-median", "desktop-visible-p95", "desktop-visible-p99",
    "desktop-paint-sync-readback", "desktop-undo-sync-readback",
)
IPAD_BUDGETS = (
    "tablet-visible-median", "tablet-visible-p95", "tablet-visible-p99",
    "tablet-sustained-final-p95", "tablet-sustained-memory",
    "tablet-paint-sync-readback", "tablet-undo-sync-readback",
)


def command(*args: str, output: Path | None = None, timeout: int = 3600) -> str:
    if output is None:
        return subprocess.run(args, cwd=ROOT, check=True, text=True, capture_output=True,
                              timeout=timeout).stdout
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8") as log:
        completed = subprocess.run(args, cwd=ROOT, text=True, stdout=log,
                                   stderr=subprocess.STDOUT, timeout=timeout)
    if completed.returncode:
        raise RuntimeError(f"{' '.join(args)} failed ({completed.returncode}); see {output}")
    return ""


def preflight(require_ipad: bool = True) -> str:
    config = device_gate.load_json(CONFIG)
    desktop = next(item for item in config["reference_devices"] if item["kind"] == "desktop")
    hardware = json.loads(command("system_profiler", "SPHardwareDataType", "-json"))[
        "SPHardwareDataType"][0]
    os_version = command("sw_vers", "-productVersion").strip()
    os_build = command("sw_vers", "-buildVersion").strip()
    if (hardware.get("machine_model") != "Mac15,7"
            or hardware.get("chip_type") != "Apple M3 Pro"
            or int(command("sysctl", "-n", "hw.memsize")) != desktop["memory_bytes"]
            or f"macOS {os_version} ({os_build})" != desktop["operating_system"]):
        raise RuntimeError("this runner is not the named MacBook Pro M3 Pro (Mac15,7)")
    if not require_ipad:
        return ""
    listing = OUTPUT / "devices.json"
    command("xcrun", "devicectl", "list", "devices", "--json-output", str(listing))
    devices = json.loads(listing.read_text(encoding="utf-8"))["result"]["devices"]
    ipads = [item for item in devices
             if item.get("properties", {}).get("hardware", {}).get("productType") == "iPad15,5"
             and item.get("properties", {}).get("hardware", {}).get("reality") == "physical"
             and item.get("properties", {}).get("connection", {}).get("state") == "connected"]
    if len(ipads) != 1:
        raise RuntimeError("exactly one connected physical iPad Air M3 (iPad15,5) is required")
    ipad = ipads[0]
    tablet = next(item for item in config["reference_devices"] if item["kind"] == "tablet")
    reported_os = ipad.get("deviceProperties", {})
    if (f"iPadOS {reported_os.get('osVersionNumber')} "
            f"({reported_os.get('osBuildUpdate')})" != tablet["operating_system"]):
        raise RuntimeError("the named iPad OS version differs from the reference manifest")
    if ipad.get("deviceProperties", {}).get("developerModeStatus") != "enabled":
        raise RuntimeError("Developer Mode is not enabled on the named iPad")
    if len(config["reference_devices"]) != 2:
        raise RuntimeError("reference-device manifest changed; update this gate")
    udid = str(ipad["properties"]["hardware"]["udid"])
    lock = OUTPUT / "ipad-lock.json"
    command("xcrun", "devicectl", "device", "info", "lockState", "--device", udid,
            "--json-output", str(lock))
    if json.loads(lock.read_text(encoding="utf-8"))["result"].get("passcodeRequired"):
        raise RuntimeError("the named iPad is locked; unlock it before the device gate")
    return udid


def measurement(budget_id: str, value: float | int) -> dict[str, object]:
    budget = next(item for item in device_gate.load_json(CONFIG)["budgets"]
                  if item["id"] == budget_id)
    return {"budget_id": budget_id, "configuration": budget["configuration"],
            "unit": budget["unit"], "batch_size": 1, "value": value}


def check_budgets(results: dict[str, object], required: tuple[str, ...]) -> None:
    problems = device_gate.validate_run_metadata(results)
    config = device_gate.load_json(CONFIG)
    baselines = device_gate.load_json(BASELINES)["baselines"]
    observed = {item["budget_id"]: item for item in results["measurements"]}
    budgets = {item["id"]: item for item in config["budgets"]}
    for identity in required:
        decision = device_gate.decide_budget(
            budgets[identity], observed.get(identity), results["device_id"],
            baselines.get(identity), config["baseline_regression_fraction"])
        print(f"{identity}: {decision.status}: {decision.detail}", flush=True)
        if decision.status != "passed":
            problems.append(f"{identity}: {decision.status}: {decision.detail}")
    problems.extend(device_gate.validate_interaction_trace(results["interaction"]))
    if "sustained_mobile" in results:
        problems.extend(device_gate.validate_sustained_mobile(results["sustained_mobile"]))
    if problems:
        raise RuntimeError("; ".join(problems))


def run_desktop() -> None:
    command("just", "host-desktop-build", output=OUTPUT / "desktop-build.log", timeout=1800)
    report = OUTPUT / "desktop-wgsl.json"
    result = OUTPUT / "desktop-measurements.json"
    environment = ["env", f"CTEX_RUN_DATE={datetime.now(timezone.utc).date().isoformat()}",
                   f"CTEX_RUN_COMMIT={command('git', 'rev-parse', 'HEAD').strip()}"]
    command(*environment, "cargo", "run", "--manifest-path", "hosts/desktop-wgpu/Cargo.toml",
            "--release", "--", "--benchmark", "--frames", "600", "--report", str(report),
            "--measurements", str(result), output=OUTPUT / "desktop-run.log", timeout=600)
    measured = device_gate.load_json(result)
    check_budgets(measured, DESKTOP_BUDGETS)


def attachment_payload(path: Path) -> dict[str, object] | None:
    if not path.is_file() or path.name == "manifest.json":
        return None
    try:
        with path.open("rb") as attachment:
            if attachment.read(1) != b"{":
                return None
        return device_gate.load_json(path)
    except (OSError, UnicodeError, ValueError, json.JSONDecodeError):
        return None


def attachments(bundle: Path) -> dict[str, dict[str, object]]:
    exported = OUTPUT / "ipad-attachments"
    if exported.exists():
        shutil.rmtree(exported)
    command("xcrun", "xcresulttool", "export", "attachments", "--path", str(bundle),
            "--output-path", str(exported))
    found: dict[str, dict[str, object]] = {}
    for path in exported.rglob("*"):
        value = attachment_payload(path)
        if value is None:
            continue
        for key in ("device", "metal", "residency", "interaction", "sustained"):
            if key in value:
                if key in found:
                    raise RuntimeError(f"duplicate iPad {key} attachment")
                found[key] = value
    missing = set(("device", "metal", "residency", "interaction", "sustained")) - found.keys()
    if missing:
        raise RuntimeError(f"iPad result bundle lacks attachments: {', '.join(sorted(missing))}")
    return found


def run_ipad(udid: str) -> None:
    command("cmake", "--preset", "ios-arm64", output=OUTPUT / "ipad-configure.log")
    command("cmake", "--build", "--preset", "ios-arm64", "--target", "cybertexel_c",
            output=OUTPUT / "ipad-build.log", timeout=1800)
    command("cmake", "--install", str(ROOT / "build" / "ios-arm64"), "--prefix",
            str(ROOT / "build" / "swift-ios-prefix"), output=OUTPUT / "ipad-install.log")
    bundle = OUTPUT / "ipad.xcresult"
    if bundle.exists():
        shutil.rmtree(bundle)
    command("xcodebuild", "test", "-project",
            "hosts/ios-probe/CyberTexelDeviceProbe.xcodeproj", "-scheme",
            "CyberTexelDeviceProbe", "-configuration", "Release", "-destination",
            f"platform=iOS,id={udid}", "-destination-timeout", "60",
            "-parallel-testing-enabled", "NO", "-resultBundlePath", str(bundle),
            "-derivedDataPath", str(OUTPUT / "ipad-derived"), "-allowProvisioningUpdates",
            "-allowProvisioningDeviceRegistration", "CODE_SIGN_STYLE=Automatic",
            output=OUTPUT / "ipad-test.log", timeout=2700)
    summary = json.loads(command("xcrun", "xcresulttool", "get", "test-results", "summary",
                                 "--path", str(bundle)))
    if summary.get("failedTests", 0) or summary.get("skippedTests", 0) or summary.get("passedTests") != 7:
        raise RuntimeError(f"iPad tests did not all pass: {summary}")
    reports = attachments(bundle)
    if reports["device"]["device"].get("model_identifier") != "iPad15,5":
        raise RuntimeError("XCTest ran on a different iPad")
    interaction = reports["interaction"]
    sustained = reports["sustained"]
    residency = reports["residency"]["residency"]
    if residency.get("undo_copied_pixel_bytes") != 0:
        raise RuntimeError("iPad undo copied pixels")
    results: dict[str, object] = {
        "schema": 1, "device_id": "ipad-air-13-m3-8gb",
        "date": datetime.now(timezone.utc).date().isoformat(),
        "commit": command("git", "rev-parse", "HEAD").strip(),
        "command": "just gate-reference-devices",
        "interaction": interaction["interaction"],
        "sustained_mobile": sustained["sustained"],
        "measurements": [
            measurement("tablet-visible-median", interaction["median_ms"]),
            measurement("tablet-visible-p95", interaction["p95_ms"]),
            measurement("tablet-visible-p99", interaction["p99_ms"]),
            measurement("tablet-sustained-final-p95", sustained["final_five_minute_p95_ms"]),
            measurement("tablet-sustained-memory", sustained["peak_physical_bytes"]),
            measurement("tablet-paint-sync-readback", residency["synchronous_readback_bytes"]),
            measurement("tablet-undo-sync-readback", residency["synchronous_readback_bytes"]),
        ],
    }
    result = OUTPUT / "ipad-measurements.json"
    result.write_text(json.dumps(results, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    check_budgets(results, IPAD_BUDGETS)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", choices=("preflight", "desktop", "ipad", "all"))
    target = parser.parse_args().target
    OUTPUT.mkdir(parents=True, exist_ok=True)
    try:
        udid = preflight(target != "desktop")
        if target in ("desktop", "all"):
            run_desktop()
        if target in ("ipad", "all"):
            run_ipad(udid)
        print(f"reference-device gate passed: {target}")
        return 0
    except (OSError, ValueError, KeyError, subprocess.SubprocessError, RuntimeError) as error:
        print(f"reference-device gate failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
