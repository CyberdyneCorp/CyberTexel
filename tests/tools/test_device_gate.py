from __future__ import annotations

import copy
import importlib.util
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "device_gate.py"
SPEC = importlib.util.spec_from_file_location("device_gate", SCRIPT)
assert SPEC and SPEC.loader
GATE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = GATE
SPEC.loader.exec_module(GATE)


class DeviceGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.config = GATE.load_json(ROOT / "benchmarks" / "device_gate.json")

    def budget(self, identity: str) -> dict[str, object]:
        return next(item for item in self.config["budgets"] if item["id"] == identity)

    def measurement(
        self, identity: str, value: float, *, batch_size: int = 1, excluded: bool = False
    ) -> dict[str, object]:
        budget = self.budget(identity)
        return {
            "budget_id": identity,
            "value": value,
            "unit": budget["unit"],
            "configuration": budget["configuration"],
            "batch_size": batch_size,
            "fixed_batch_cost_excluded": excluded,
        }

    def results(self, device: str, measurements: list[dict[str, object]]) -> dict[str, object]:
        return {
            "schema": 1,
            "device_id": device,
            "date": "2026-09-23",
            "commit": "0123456789abcdef",
            "command": "just gate-budgets",
            "measurements": measurements,
        }

    def test_declares_full_desktop_and_tablet_and_every_budget(self) -> None:
        self.assertEqual(GATE.validate_config(self.config), [])
        devices = self.config["reference_devices"]
        self.assertEqual({device["kind"] for device in devices}, {"desktop", "tablet"})
        operations = {
            item["operation"] for item in self.config["budgets"] if item["unit"] == "ms"
        }
        self.assertTrue(GATE.REQUIRED_TIME_OPERATIONS <= operations)
        for device in devices:
            memory = {
                item["operation"]
                for item in self.config["budgets"]
                if item["device"] == device["id"] and item["metric"] == "peak_working_bytes"
            }
            self.assertTrue(GATE.REQUIRED_MEMORY_OPERATIONS <= memory)

    def test_unnamed_device_is_informational_and_cannot_pass(self) -> None:
        measurement = self.measurement("desktop-stamp-median", 2.0)
        decision = GATE.decide_budget(
            self.budget("desktop-stamp-median"), measurement, "developer-machine", None, 0.15
        )
        self.assertEqual(decision.status, "informational")
        report = GATE.evaluate_gate(self.config, self.results("developer-machine", [measurement]))
        self.assertFalse(report.passed)
        self.assertEqual((report.decided, report.total, report.coverage), (0, 0, 0.0))

    def test_stamp_median_and_p95_are_independent_decisions(self) -> None:
        device = "macbook-pro-m3-pro-18gpu-36gb"
        measurements = [
            self.measurement("desktop-stamp-median", 3.0),
            self.measurement("desktop-stamp-p95", 7.0),
        ]
        report = GATE.evaluate_gate(self.config, self.results(device, measurements))
        decisions = {item.budget_id: item.status for item in report.decisions}
        self.assertEqual(decisions["desktop-stamp-median"], "passed")
        self.assertEqual(decisions["desktop-stamp-p95"], "passed")
        self.assertFalse(report.passed)
        self.assertLess(report.coverage, 1.0)

    def test_touched_area_scaling_rejects_canvas_linear_growth(self) -> None:
        passed = GATE.evaluate_stamp_scaling(
            self.config,
            {"stamp_scaling": {"resolution_2048_ms": 2.0, "resolution_16384_ms": 2.8}},
        )
        failed = GATE.evaluate_stamp_scaling(
            self.config,
            {"stamp_scaling": {"resolution_2048_ms": 2.0, "resolution_16384_ms": 16.0}},
        )
        self.assertEqual((passed.status, round(passed.value, 1)), ("passed", 1.4))
        self.assertEqual(failed.status, "failed")

    def test_absolute_floor_and_budget_both_decide(self) -> None:
        budget = self.budget("desktop-stamp-median")
        below = self.measurement("desktop-stamp-median", 0.001)
        over = self.measurement("desktop-stamp-median", 5.0)
        self.assertEqual(GATE.decide_budget(budget, below, budget["device"], None, 0.15).status, "unreachable")
        self.assertEqual(GATE.decide_budget(budget, over, budget["device"], None, 0.15).status, "failed")

    def test_batch_must_exclude_fixed_cost(self) -> None:
        budget = self.budget("desktop-stamp-median")
        included = self.measurement("desktop-stamp-median", 2.0, batch_size=100)
        excluded = self.measurement(
            "desktop-stamp-median", 2.0, batch_size=100, excluded=True
        )
        self.assertEqual(GATE.decide_budget(budget, included, budget["device"], None, 0.15).status, "failed")
        self.assertEqual(GATE.decide_budget(budget, excluded, budget["device"], None, 0.15).status, "passed")

    def test_coverage_counts_decisions_not_timings(self) -> None:
        device = "macbook-pro-m3-pro-18gpu-36gb"
        measurement = self.measurement("desktop-stamp-median", 3.0)
        report = GATE.evaluate_gate(self.config, self.results(device, [measurement]))
        self.assertEqual(report.decided, 1)
        self.assertGreater(report.total, report.decided)
        self.assertAlmostEqual(report.coverage, 1 / report.total)
        self.assertTrue(any(item.status == "unmeasured" for item in report.decisions))

    def test_unavailable_tablet_is_unmeasured_without_desktop_substitution(self) -> None:
        device = "ipad-pro-13-m4-16gb"
        report = GATE.evaluate_gate(self.config, self.results(device, []))
        self.assertFalse(report.passed)
        self.assertEqual(report.decided, 0)
        self.assertTrue(report.decisions)
        self.assertTrue(all(item.status == "unmeasured" for item in report.decisions))

    def test_regression_names_baseline_and_fails(self) -> None:
        budget = self.budget("desktop-stamp-median")
        measurement = self.measurement("desktop-stamp-median", 3.6)
        decision = GATE.decide_budget(budget, measurement, budget["device"], 3.0, 0.15)
        self.assertEqual(decision.status, "failed")
        self.assertIn("baseline 3", decision.detail)

    def test_run_metadata_is_required_for_reproduction(self) -> None:
        self.assertEqual(
            GATE.validate_run_metadata({}),
            [
                "measurement run must use schema 1",
                "measurement run is missing device_id",
                "measurement run is missing date",
                "measurement run is missing commit",
                "measurement run is missing command",
            ],
        )
        self.assertEqual(GATE.validate_run_metadata(self.results("reference", [])), [])

    def test_input_to_visible_requires_every_stage_and_configuration(self) -> None:
        trace = {
            "stages_ms": {
                "input": 1.0,
                "queue": 2.0,
                "upload": 3.0,
                "execution": 4.0,
                "presentation": 5.0,
            },
            "input_to_visible_ms": 15.0,
            "input_rate_hz": 120,
            "refresh_rate_hz": 120,
            "mesh_triangles": 250000,
            "layers": 8,
            "channels": 4,
            "residency": "resident",
        }
        self.assertEqual(GATE.validate_interaction_trace(trace), [])
        delayed = copy.deepcopy(trace)
        delayed["stages_ms"]["queue"] = 20.0
        delayed["input_to_visible_ms"] = 33.0
        self.assertEqual(GATE.interaction_bottleneck(delayed), "queue")
        budget = self.budget("desktop-visible-p95")
        measurement = self.measurement("desktop-visible-p95", 33.0)
        decision = GATE.decide_budget(budget, measurement, budget["device"], None, 0.15)
        self.assertEqual(decision.status, "failed")

    def test_sustained_mobile_requires_final_window_and_recovery_fixtures(self) -> None:
        run = {
            "duration_minutes": 20,
            "final_window_minutes": 5,
            "initial_latency_ms": {"median": 12.0, "p95": 20.0, "p99": 25.0},
            "final_latency_ms": {"median": 14.0, "p95": 24.0, "p99": 30.0},
            "peak_physical_bytes": 500000000,
            "thermal_state": "nominal",
            "memory_pressure_passed": True,
            "suspend_resume_passed": True,
            "device_loss_passed": True,
        }
        self.assertEqual(GATE.validate_sustained_mobile(run), [])
        budget = self.budget("tablet-sustained-final-p95")
        measurement = self.measurement("tablet-sustained-final-p95", 40.0)
        self.assertLess(run["initial_latency_ms"]["p95"], budget["ceiling"])
        self.assertEqual(
            GATE.decide_budget(budget, measurement, budget["device"], None, 0.15).status,
            "failed",
        )
        run["duration_minutes"] = 19
        self.assertIn("twenty minutes", GATE.validate_sustained_mobile(run)[0])

    def test_residency_zero_budget_rejects_hidden_round_trip(self) -> None:
        budget = self.budget("desktop-paint-sync-readback")
        zero = self.measurement("desktop-paint-sync-readback", 0)
        hidden = self.measurement("desktop-paint-sync-readback", 4096)
        self.assertEqual(GATE.decide_budget(budget, zero, budget["device"], None, 0.15).status, "passed")
        self.assertEqual(GATE.decide_budget(budget, hidden, budget["device"], None, 0.15).status, "failed")

    def test_generated_document_is_current(self) -> None:
        expected = GATE.render_document(self.config)
        actual = (ROOT / "docs" / "performance-budgets.md").read_text(encoding="utf-8")
        self.assertEqual(actual, expected)
        recorded = GATE.render_document(
            self.config,
            (
                self.results(
                    "macbook-pro-m3-pro-18gpu-36gb",
                    [self.measurement("desktop-stamp-median", 3.0)],
                ),
            ),
        )
        self.assertIn("**passed** — 3 ms; 2026-09-23; `0123456789abcdef`", recorded)


if __name__ == "__main__":
    unittest.main()
