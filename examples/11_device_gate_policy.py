#!/usr/bin/env python3
"""Demonstrate the performance gate's decision rules without making a claim.

Capabilities: device-gate.
The example evaluates deterministic fixture measurements and asserts that only
a named reference-device result can pass; absent, unnamed and below-floor data
remain explicitly unmeasured, informational or unreachable.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path

import cybertexel


CAPABILITIES = ("device-gate",)


@dataclass(frozen=True)
class Budget:
    operation: str
    device: str
    ceiling_ms: float
    absolute_floor_ms: float
    baseline_ms: float
    allowed_regression: float


def decide(
    budget: Budget, *, measured_device: str | None, measurement_ms: float | None
) -> dict[str, object]:
    if measurement_ms is None:
        return {"operation": budget.operation, "status": "unmeasured"}
    if measured_device != budget.device:
        return {
            "operation": budget.operation,
            "status": "informational",
            "measurement_ms": measurement_ms,
        }
    if measurement_ms < budget.absolute_floor_ms:
        return {
            "operation": budget.operation,
            "status": "unreachable",
            "measurement_ms": measurement_ms,
        }
    regression_limit = budget.baseline_ms * (1.0 + budget.allowed_regression)
    status = (
        "passed"
        if measurement_ms <= budget.ceiling_ms and measurement_ms <= regression_limit
        else "failed"
    )
    return {
        "operation": budget.operation,
        "status": status,
        "measurement_ms": measurement_ms,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    stamp = Budget(
        operation="stamp",
        device="desktop-reference",
        ceiling_ms=8.0,
        absolute_floor_ms=0.05,
        baseline_ms=5.0,
        allowed_regression=0.20,
    )
    decisions = [
        decide(stamp, measured_device="desktop-reference", measurement_ms=5.5),
        decide(stamp, measured_device="developer-laptop", measurement_ms=4.0),
        decide(stamp, measured_device="desktop-reference", measurement_ms=0.001),
        decide(stamp, measured_device=None, measurement_ms=None),
        decide(stamp, measured_device="desktop-reference", measurement_ms=6.5),
    ]
    assert [decision["status"] for decision in decisions] == [
        "passed",
        "informational",
        "unreachable",
        "unmeasured",
        "failed",
    ]
    assert sum(decision["status"] in {"passed", "failed"} for decision in decisions) == 2

    arguments.output.mkdir(parents=True, exist_ok=True)
    summary = {
        "capabilities": list(CAPABILITIES),
        "decisions": decisions,
        "native_version": cybertexel.native_version(),
        "performance_claim": False,
        "reason": "deterministic policy fixtures are not reference-device measurements",
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
