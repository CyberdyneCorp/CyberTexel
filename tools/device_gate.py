#!/usr/bin/env python3
"""Validate and evaluate CyberTexel reference-device performance budgets."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import math
from pathlib import Path
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / "benchmarks" / "device_gate.json"
DEFAULT_DOCUMENT = ROOT / "docs" / "performance-budgets.md"
RECORDED_RESULTS = ROOT / "benchmarks" / "results"
REQUIRED_TIME_OPERATIONS = {
    "stamp",
    "stroke",
    "composite",
    "emission",
    "generator",
    "delta-query",
    "tile-readback",
    "smart-material",
    "export",
}
REQUIRED_MEMORY_OPERATIONS = {"stroke", "composite", "smart-material", "export"}


@dataclass(frozen=True)
class Decision:
    budget_id: str
    status: str
    value: float | None
    detail: str


@dataclass(frozen=True)
class GateReport:
    device: str
    decisions: tuple[Decision, ...]
    decided: int
    total: int
    coverage: float
    passed: bool


def load_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path} must contain one JSON object")
    return value


def _unique(values: list[str], subject: str, failures: list[str]) -> None:
    duplicates = sorted({value for value in values if values.count(value) > 1})
    if duplicates:
        failures.append(f"duplicate {subject}: {', '.join(duplicates)}")


def _validate_devices(devices: list[Any], failures: list[str]) -> list[str]:
    device_ids = [str(device.get("id", "")) for device in devices if isinstance(device, dict)]
    _unique(device_ids, "reference device", failures)
    if {device.get("kind") for device in devices if isinstance(device, dict)} != {
        "desktop",
        "tablet",
    }:
        failures.append("reference devices must contain one desktop and one tablet kind")
    for device in devices:
        if not isinstance(device, dict) or any(
            not device.get(field)
            for field in ("id", "kind", "model", "cpu", "gpu", "memory_bytes", "operating_system", "host", "refresh_hz")
        ):
            failures.append("every reference device requires full model, CPU, GPU, memory, OS, host and refresh rate")
    return device_ids


def _validate_budget_record(
    budget: Any, device_ids: list[str], configuration_ids: list[str], failures: list[str]
) -> None:
    if not isinstance(budget, dict):
        failures.append("every budget must be an object")
        return
    if budget.get("device") not in device_ids:
        failures.append(f"budget {budget.get('id')!r} names an unknown device")
    if budget.get("configuration") not in configuration_ids:
        failures.append(f"budget {budget.get('id')!r} names an unknown configuration")
    for field in ("id", "operation", "metric", "unit"):
        if not budget.get(field):
            failures.append(f"budget {budget.get('id')!r} is missing {field}")
    for field in ("ceiling", "absolute_floor"):
        value = budget.get(field)
        if not isinstance(value, (int, float)) or not math.isfinite(value) or value < 0:
            failures.append(f"budget {budget.get('id')!r} has invalid {field}")


def _validate_budget_coverage(
    config: dict[str, Any], devices: list[dict[str, Any]], budgets: list[dict[str, Any]],
    device_ids: list[str], failures: list[str]
) -> None:
    time_operations = {budget.get("operation") for budget in budgets if budget.get("unit") == "ms"}
    missing_time = sorted(REQUIRED_TIME_OPERATIONS - time_operations)
    if missing_time:
        failures.append(f"missing time budgets: {', '.join(missing_time)}")
    for device_id in device_ids:
        memory_operations = {
            budget.get("operation")
            for budget in budgets
            if budget.get("device") == device_id and budget.get("metric") == "peak_working_bytes"
        }
        missing_memory = sorted(REQUIRED_MEMORY_OPERATIONS - memory_operations)
        if missing_memory:
            failures.append(f"{device_id} missing memory budgets: {', '.join(missing_memory)}")
    if not isinstance(config.get("stamp_scaling_maximum_ratio"), (int, float)):
        failures.append("stamp scaling maximum ratio is missing")
    if not isinstance(config.get("baseline_regression_fraction"), (int, float)):
        failures.append("baseline regression fraction is missing")
    for kind in ("desktop", "tablet"):
        device = next((item["id"] for item in devices if item.get("kind") == kind), None)
        metrics = {
            budget.get("metric")
            for budget in budgets
            if budget.get("device") == device and budget.get("operation") == "input-to-visible"
        }
        if metrics != {"median_ms", "p95_ms", "p99_ms"}:
            failures.append(f"{kind} input-to-visible budgets require median, p95 and p99")
    _validate_refresh_derived_latency(devices, budgets, failures)


def _validate_refresh_derived_latency(
    devices: list[dict[str, Any]], budgets: list[dict[str, Any]], failures: list[str]
) -> None:
    """Hold the two panels to the same pipeline depth.

    A frame cannot be visible before the next vsync, so an input-to-visible
    ceiling in milliseconds means nothing without the refresh rate it was written
    against: 20 ms is generous on a 120 Hz panel and below the hardware floor on a
    60 Hz one. The budget that actually transfers between devices is how many
    refresh periods the pipeline may take, so the desktop's ceilings fix those
    counts and every other device's follow from its own panel.
    """
    periods: dict[str, float] = {}
    by_id = {device.get("id"): device for device in devices}
    desktop = next((item for item in devices if item.get("kind") == "desktop"), None)
    if desktop is None or not desktop.get("refresh_hz"):
        return
    for budget in budgets:
        if budget.get("operation") == "input-to-visible" and budget.get("device") == desktop["id"]:
            periods[str(budget.get("metric"))] = (
                float(budget["ceiling"]) * float(desktop["refresh_hz"]) / 1000.0
            )
    for budget in budgets:
        if budget.get("operation") != "input-to-visible" or budget.get("device") == desktop["id"]:
            continue
        device = by_id.get(budget.get("device"))
        expected = periods.get(str(budget.get("metric")))
        if device is None or expected is None or not device.get("refresh_hz"):
            continue
        derived = round(expected * 1000.0 / float(device["refresh_hz"]), 1)
        if abs(float(budget["ceiling"]) - derived) > 0.05:
            failures.append(
                f"budget {budget.get('id')!r} ceiling {budget['ceiling']} ms does not match the "
                f"{derived} ms derived from {device['refresh_hz']} Hz and the desktop's "
                f"{expected:.2f} refresh periods"
            )


def validate_config(config: dict[str, Any]) -> list[str]:
    failures = [] if config.get("schema") == 1 else ["device gate config must use schema 1"]
    devices = config.get("reference_devices")
    configurations = config.get("configurations")
    budgets = config.get("budgets")
    for value, message in (
        (devices, "reference_devices must be a non-empty list"),
        (configurations, "configurations must be a non-empty list"),
        (budgets, "budgets must be a non-empty list"),
    ):
        if not isinstance(value, list) or not value:
            failures.append(message)
    if failures and any(not isinstance(value, list) or not value for value in (devices, configurations, budgets)):
        return failures
    assert isinstance(devices, list) and isinstance(configurations, list) and isinstance(budgets, list)
    device_ids = _validate_devices(devices, failures)
    configuration_ids = [str(item.get("id", "")) for item in configurations if isinstance(item, dict)]
    budget_ids = [str(item.get("id", "")) for item in budgets if isinstance(item, dict)]
    _unique(configuration_ids, "configuration", failures)
    _unique(budget_ids, "budget", failures)
    for budget in budgets:
        _validate_budget_record(budget, device_ids, configuration_ids, failures)
    valid_budgets = [budget for budget in budgets if isinstance(budget, dict)]
    valid_devices = [device for device in devices if isinstance(device, dict)]
    _validate_budget_coverage(config, valid_devices, valid_budgets, device_ids, failures)
    return failures


def _measurement_map(results: dict[str, Any]) -> dict[str, dict[str, Any]]:
    measurements = results.get("measurements", [])
    if not isinstance(measurements, list):
        raise ValueError("measurements must be a list")
    mapped: dict[str, dict[str, Any]] = {}
    for measurement in measurements:
        if not isinstance(measurement, dict) or not measurement.get("budget_id"):
            raise ValueError("every measurement requires budget_id")
        identity = str(measurement["budget_id"])
        if identity in mapped:
            raise ValueError(f"duplicate measurement: {identity}")
        mapped[identity] = measurement
    return mapped


def decide_budget(
    budget: dict[str, Any],
    measurement: dict[str, Any] | None,
    measured_device: str | None,
    baseline: float | None,
    regression_fraction: float,
) -> Decision:
    identity = str(budget["id"])
    if measurement is None:
        return Decision(identity, "unmeasured", None, "case did not run")
    raw_value = measurement.get("value")
    value = float(raw_value) if isinstance(raw_value, (int, float)) else math.nan
    if measured_device != budget["device"]:
        return Decision(identity, "informational", value, "measurement is from another device")
    if not math.isfinite(value) or value < 0:
        return Decision(identity, "failed", value, "measurement is not a finite non-negative value")
    if measurement.get("unit") != budget["unit"]:
        return Decision(identity, "failed", value, "measurement unit does not match the budget")
    if measurement.get("configuration") != budget["configuration"]:
        return Decision(identity, "unmeasured", value, "document configuration does not match")
    batch_size = measurement.get("batch_size", 1)
    if not isinstance(batch_size, int) or batch_size < 1:
        return Decision(identity, "failed", value, "batch size is invalid")
    if batch_size > 1 and measurement.get("fixed_batch_cost_excluded") is not True:
        return Decision(identity, "failed", value, "batch fixed cost was not excluded")
    if value < float(budget["absolute_floor"]):
        return Decision(identity, "unreachable", value, "measurement is below the absolute floor")
    if value > float(budget["ceiling"]):
        return Decision(identity, "failed", value, "measurement exceeds the budget")
    if baseline is not None and value > baseline * (1.0 + regression_fraction):
        return Decision(identity, "failed", value, f"measurement regressed from baseline {baseline:g}")
    return Decision(identity, "passed", value, "measurement satisfies budget and baseline")


def evaluate_gate(
    config: dict[str, Any],
    results: dict[str, Any],
    baselines: dict[str, float] | None = None,
) -> GateReport:
    failures = validate_config(config)
    if failures:
        raise ValueError("; ".join(failures))
    measured_device = results.get("device_id")
    if not isinstance(measured_device, str) or not measured_device:
        measured_device = None
    measurements = _measurement_map(results)
    baseline_values = baselines or {}
    applicable = [budget for budget in config["budgets"] if budget["device"] == measured_device]
    if not applicable:
        decisions = tuple(
            decide_budget(
                budget,
                measurements.get(str(budget["id"])),
                measured_device,
                baseline_values.get(str(budget["id"])),
                float(config["baseline_regression_fraction"]),
            )
            for budget in config["budgets"]
            if str(budget["id"]) in measurements
        )
        return GateReport(measured_device or "unnamed", decisions, 0, 0, 0.0, False)
    decisions = tuple(
        decide_budget(
            budget,
            measurements.get(str(budget["id"])),
            measured_device,
            baseline_values.get(str(budget["id"])),
            float(config["baseline_regression_fraction"]),
        )
        for budget in applicable
    )
    decided = sum(decision.status in {"passed", "failed"} for decision in decisions)
    total = len(decisions)
    return GateReport(
        measured_device,
        decisions,
        decided,
        total,
        decided / total if total else 0.0,
        total > 0 and all(decision.status == "passed" for decision in decisions),
    )


def evaluate_stamp_scaling(config: dict[str, Any], results: dict[str, Any]) -> Decision:
    scaling = results.get("stamp_scaling")
    if not isinstance(scaling, dict):
        return Decision("stamp-scaling", "unmeasured", None, "scaling case did not run")
    small = scaling.get("resolution_2048_ms")
    large = scaling.get("resolution_16384_ms")
    if not isinstance(small, (int, float)) or not isinstance(large, (int, float)) or small <= 0:
        return Decision("stamp-scaling", "failed", None, "scaling measurements are invalid")
    ratio = float(large) / float(small)
    ceiling = float(config["stamp_scaling_maximum_ratio"])
    return Decision(
        "stamp-scaling",
        "passed" if ratio <= ceiling else "failed",
        ratio,
        f"large-to-small touched-area ratio is {ratio:.3f}; ceiling is {ceiling:.3f}",
    )


def validate_run_metadata(results: dict[str, Any]) -> list[str]:
    failures = []
    if results.get("schema") != 1:
        failures.append("measurement run must use schema 1")
    for field in ("device_id", "date", "commit", "command"):
        if not results.get(field):
            failures.append(f"measurement run is missing {field}")
    return failures


def validate_interaction_trace(trace: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    stages = trace.get("stages_ms")
    total = trace.get("input_to_visible_ms")
    required = {"input", "queue", "upload", "execution", "presentation"}
    if not isinstance(stages, dict) or set(stages) != required:
        return ["interaction trace must contain all five pipeline stages"]
    if not isinstance(total, (int, float)) or abs(sum(stages.values()) - total) > 1.0e-9:
        failures.append("interaction stage timings must sum to input-to-visible latency")
    for field in ("input_rate_hz", "refresh_rate_hz", "mesh_triangles", "layers", "channels", "residency"):
        if field not in trace:
            failures.append(f"interaction trace is missing {field}")
    return failures


def interaction_bottleneck(trace: dict[str, Any]) -> str:
    failures = validate_interaction_trace(trace)
    if failures:
        raise ValueError("; ".join(failures))
    return max(trace["stages_ms"], key=trace["stages_ms"].get)


def validate_sustained_mobile(run: dict[str, Any]) -> list[str]:
    failures = []
    if run.get("duration_minutes", 0) < 20:
        failures.append("mobile workload must run for at least twenty minutes")
    if run.get("final_window_minutes", 0) < 5:
        failures.append("mobile budgets must use at least the final five minutes")
    for field in (
        "initial_latency_ms",
        "final_latency_ms",
        "peak_physical_bytes",
        "thermal_state",
        "memory_pressure_passed",
        "suspend_resume_passed",
        "device_loss_passed",
    ):
        if field not in run:
            failures.append(f"mobile workload is missing {field}")
    return failures


def render_document(
    config: dict[str, Any],
    result_sets: tuple[dict[str, Any], ...] = (),
    baselines: dict[str, float] | None = None,
) -> str:
    failures = validate_config(config)
    if failures:
        raise ValueError("; ".join(failures))
    latest: dict[str, tuple[Decision, dict[str, Any]]] = {}
    for results in result_sets:
        for decision in evaluate_gate(config, results, baselines).decisions:
            latest[decision.budget_id] = (decision, results)
    lines = [
        "# Reference-device performance budgets",
        "",
        "This file is generated from `benchmarks/device_gate.json` by",
        "`python3 tools/device_gate.py document --write`. Targets are not measurements.",
        "A row remains **unmeasured** until a dated, commit-pinned run from the exact named",
        "device is recorded; developer-machine timings are informational only.",
        "",
        "## Reference devices",
        "",
        "| ID | Kind | Model | CPU | GPU | Memory | Operating system | Host |",
        "| --- | --- | --- | --- | --- | --- | ---: | --- |",
    ]
    for device in config["reference_devices"]:
        lines.append(
            f"| `{device['id']}` | {device['kind']} | {device['model']} | {device['cpu']} | "
            f"{device['gpu']} | {device['memory_bytes']} bytes | {device['operating_system']} | {device['host']} |"
        )
    lines.extend(
        [
            "",
            "## Configurations",
            "",
            "| ID | Description |",
            "| --- | --- |",
        ]
    )
    for configuration in config["configurations"]:
        lines.append(f"| `{configuration['id']}` | {configuration['description']} |")
    lines.extend(
        [
            "",
            "## Budgets",
            "",
            "| ID | Operation | Metric | Ceiling | Device | Configuration | Latest |",
            "| --- | --- | --- | ---: | --- | --- | --- |",
        ]
    )
    for budget in config["budgets"]:
        ceiling = (
            str(int(budget["ceiling"]))
            if budget["unit"] == "bytes"
            else f"{budget['ceiling']:g}"
        )
        recorded = latest.get(str(budget["id"]))
        latest_text = "**unmeasured**"
        if recorded is not None:
            decision, results = recorded
            value = "n/a" if decision.value is None else f"{decision.value:g} {budget['unit']}"
            latest_text = (
                f"**{decision.status}** — {value}; {results.get('date', 'undated')}; "
                f"`{results.get('commit', 'unknown')}`"
            )
        lines.append(
            f"| `{budget['id']}` | {budget['operation']} | {budget['metric']} | "
            f"{ceiling} {budget['unit']} | `{budget['device']}` | "
            f"`{budget['configuration']}` | {latest_text} |"
        )
    lines.extend(
        [
            "",
            "## Recording a run",
            "",
            "A reference host writes schema-1 JSON containing `device_id`, `date`, `commit`,",
            "`command` and one measurement per `budget_id`. Reproduce and decide it with:",
            "",
            "```sh",
            "CTEX_DEVICE_GATE_RESULTS=benchmarks/results/<run>.json just gate-budgets",
            "python3 tools/device_gate.py document --results benchmarks/results/<run>.json --write",
            "```",
            "",
            "Each measurement records its unit, configuration, batch size and whether fixed batch",
            "cost was excluded. The gate refuses incomplete attribution and never substitutes one",
            "device for another.",
            "",
            "## Gate rules",
            "",
            f"- Stamp scaling from 2048 to 16384 may increase by at most {config['stamp_scaling_maximum_ratio']:g}× for identical touched area.",
            f"- A result beyond {config['baseline_regression_fraction'] * 100:g}% of its recorded baseline is a regression.",
            "- Batch measurements must remove fixed batch cost before reporting a per-operation value.",
            "- Coverage counts only passed or failed decisions; informational, unmeasured and unreachable rows are uncovered.",
            "- Resident paint and undo have zero-byte synchronous-readback ceilings; save and export traffic is attributed separately.",
            "- The mobile workload lasts at least twenty minutes and enforces sustained budgets over the final five minutes.",
            "",
        ]
    )
    return "\n".join(lines)


def recorded_result_paths(directory: Path = RECORDED_RESULTS) -> tuple[Path, ...]:
    """Every committed measurement run, oldest first.

    The generated document reflects what has actually been recorded, so a run
    that is committed is a run the document accounts for.
    """

    if not directory.is_dir():
        return ()
    return tuple(sorted(directory.glob("*.json")))


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("validate", "document", "gate"))
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--document", type=Path, default=DEFAULT_DOCUMENT)
    parser.add_argument("--results", type=Path, action="append", default=[])
    parser.add_argument("--baselines", type=Path)
    parser.add_argument("--write", action="store_true")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    config = load_json(arguments.config)
    failures = validate_config(config)
    if failures:
        for failure in failures:
            print(failure, file=sys.stderr)
        return 1
    if arguments.command == "validate":
        print(f"ok: {len(config['budgets'])} budgets and {len(config['reference_devices'])} reference devices")
        return 0
    if arguments.command == "document":
        baselines = (
            load_json(arguments.baselines).get("baselines", {}) if arguments.baselines else {}
        )
        paths = tuple(arguments.results) or recorded_result_paths()
        result_sets = tuple(load_json(path) for path in paths)
        for results in result_sets:
            metadata_failures = validate_run_metadata(results)
            if metadata_failures:
                for failure in metadata_failures:
                    print(failure, file=sys.stderr)
                return 1
        rendered = render_document(config, result_sets, baselines)
        if arguments.write:
            arguments.document.write_text(rendered, encoding="utf-8")
            print(f"updated {arguments.document}")
            return 0
        if not arguments.document.is_file() or arguments.document.read_text(encoding="utf-8") != rendered:
            print("performance budget document is stale; run device_gate.py document --write", file=sys.stderr)
            return 1
        print("performance budget document is current")
        return 0
    if len(arguments.results) != 1:
        print("gate requires exactly one --results", file=sys.stderr)
        return 2
    results = load_json(arguments.results[0])
    metadata_failures = validate_run_metadata(results)
    if metadata_failures:
        for failure in metadata_failures:
            print(failure, file=sys.stderr)
        return 1
    baselines = load_json(arguments.baselines).get("baselines", {}) if arguments.baselines else {}
    report = evaluate_gate(config, results, baselines)
    for decision in report.decisions:
        print(f"{decision.budget_id}: {decision.status}: {decision.detail}")
    print(f"coverage: {report.decided}/{report.total} ({report.coverage:.1%})")
    return 0 if report.passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
