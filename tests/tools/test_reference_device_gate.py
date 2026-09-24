from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import run_reference_device_gate as gate  # noqa: E402


class ReferenceDeviceGateTest(unittest.TestCase):
    def test_ipad_attachments_require_every_measured_part(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.object(gate, "OUTPUT", Path(directory)):
                runs = 0

                def fake_export(*_: str, **__: object) -> str:
                    nonlocal runs
                    runs += 1
                    exported = Path(directory) / "ipad-attachments"
                    exported.mkdir()
                    for name in ("device", "metal", "residency", "interaction", "sustained"):
                        if runs == 2 and name == "sustained":
                            continue
                        (exported / f"{name}.data").write_text(json.dumps({name: {}}))
                    (exported / "screenshot.png").write_bytes(b"\x89PNG")
                    return ""

                with mock.patch.object(gate, "command", side_effect=fake_export):
                    reports = gate.attachments(Path(directory) / "fake.xcresult")
                    self.assertEqual(set(reports),
                                     {"device", "metal", "residency", "interaction", "sustained"})
                    with self.assertRaisesRegex(RuntimeError, "lacks attachments: sustained"):
                        gate.attachments(Path(directory) / "fake.xcresult")

    def test_missing_measurement_fails_instead_of_passing(self) -> None:
        baseline = gate.device_gate.load_json(gate.BASELINES)["baselines"]
        results = {
            "schema": 1, "device_id": "macbook-pro-m3-pro-18gpu-36gb",
            "date": "2026-09-24", "commit": "test", "command": "test",
            "interaction": {
                "stages_ms": {"input": 1.0, "upload": 1.0, "queue": 1.0,
                              "execution": 1.0, "presentation": 1.0},
                "input_to_visible_ms": 5.0, "input_rate_hz": 120,
                "refresh_rate_hz": 120, "mesh_triangles": 250632,
                "layers": 8, "channels": 4, "residency": {},
            },
            "measurements": [
                gate.measurement(identity, baseline[identity])
                for identity in gate.DESKTOP_BUDGETS if identity in baseline
            ],
        }
        gate.check_budgets(results, gate.DESKTOP_BUDGETS)
        results["measurements"] = [item for item in results["measurements"]
                                   if item["budget_id"] != "desktop-visible-p95"]
        with self.assertRaisesRegex(RuntimeError, "desktop-visible-p95: unmeasured"):
            gate.check_budgets(results, gate.DESKTOP_BUDGETS)

    def test_locked_named_ipad_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.object(gate, "OUTPUT", Path(directory)):
                def fake_command(*args: str, **_: object) -> str:
                    if args[0] == "system_profiler":
                        return json.dumps({"SPHardwareDataType": [
                            {"machine_model": "Mac15,7", "chip_type": "Apple M3 Pro",
                             "physical_memory": "36 GB"}]})
                    if args[0] == "sw_vers":
                        return "27.0" if args[1] == "-productVersion" else "26A428"
                    if args[0] == "sysctl":
                        return "38654705664"
                    destination = Path(args[args.index("--json-output") + 1])
                    if "lockState" in args:
                        destination.write_text(json.dumps({"result": {"passcodeRequired": True}}))
                    else:
                        destination.write_text(json.dumps({"result": {"devices": [{
                            "properties": {
                                "hardware": {"productType": "iPad15,5", "reality": "physical",
                                             "udid": "named-ipad"},
                                "connection": {"state": "connected"}},
                            "deviceProperties": {"developerModeStatus": "enabled",
                                                 "osVersionNumber": "27.0",
                                                 "osBuildUpdate": "24A437"},
                        }]}}))
                    return ""

                with mock.patch.object(gate, "command", side_effect=fake_command):
                    with self.assertRaisesRegex(RuntimeError, "iPad is locked"):
                        gate.preflight()


if __name__ == "__main__":
    unittest.main()
