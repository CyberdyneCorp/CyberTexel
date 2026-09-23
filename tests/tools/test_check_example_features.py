from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_example_features.py"
SPEC = importlib.util.spec_from_file_location("check_example_features", SCRIPT)
assert SPEC and SPEC.loader
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class ExampleFeatureCoverageTests(unittest.TestCase):
    def make_root(self, directory: str, symbols: list[str], deferred: dict[str, str]) -> Path:
        root = Path(directory) / "root"
        (root / "abi").mkdir(parents=True)
        (root / "examples").mkdir(parents=True)
        (root / "abi" / "capi-capabilities.json").write_text(
            json.dumps(
                {
                    "schema": 1,
                    "capabilities": [
                        {
                            "name": "image-io",
                            "kind": "runtime",
                            "requirements": [{"requirement": "Decode", "symbols": symbols}],
                        }
                    ],
                }
            ),
            encoding="utf-8",
        )
        (root / "examples" / "feature_coverage.json").write_text(
            json.dumps({"schema": 1, "deferred": deferred}), encoding="utf-8"
        )
        (root / "examples" / "01_io.py").write_text("CAPABILITIES = ('image-io',)\n", encoding="utf-8")
        return root

    def make_traces(self, directory: str, called: list[str], example: str = "01_io") -> Path:
        traces = Path(directory) / "traces"
        traces.mkdir(parents=True)
        (traces / f"{example}.json").write_text(
            json.dumps({"schema": 1, "example": example, "symbols": called}), encoding="utf-8"
        )
        return traces

    def test_fully_exercised_surface_passes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["ctex_a", "ctex_b"], {})
            traces = self.make_traces(directory, ["ctex_a", "ctex_b"])
            self.assertEqual(CHECK.failures(traces, root), [])

    def test_declared_deferral_covers_an_unexercised_symbol(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["ctex_a", "ctex_b"], {"ctex_b": "task 16.12"})
            traces = self.make_traces(directory, ["ctex_a"])
            self.assertEqual(CHECK.failures(traces, root), [])

    def test_undeclared_unexercised_symbol_is_named(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["ctex_a", "ctex_b"], {})
            traces = self.make_traces(directory, ["ctex_a"])
            problems = CHECK.failures(traces, root)
            self.assertTrue(any("ctex_b" in problem for problem in problems), problems)

    def test_stale_deferral_must_be_removed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["ctex_a"], {"ctex_a": "task 16.12"})
            traces = self.make_traces(directory, ["ctex_a"])
            problems = CHECK.failures(traces, root)
            self.assertTrue(any("now exercised" in problem for problem in problems), problems)

    def test_unknown_deferral_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["ctex_a"], {"ctex_ghost": "task 16.12"})
            traces = self.make_traces(directory, ["ctex_a"])
            problems = CHECK.failures(traces, root)
            self.assertTrue(any("ctex_ghost" in problem for problem in problems), problems)

    def test_example_without_a_trace_is_named(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["ctex_a"], {})
            traces = self.make_traces(directory, ["ctex_a"], example="02_other")
            problems = CHECK.failures(traces, root)
            self.assertTrue(any("01_io" in problem for problem in problems), problems)

    def test_deferral_without_a_reason_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["ctex_a", "ctex_b"], {"ctex_b": "  "})
            traces = self.make_traces(directory, ["ctex_a"])
            with self.assertRaises(ValueError):
                CHECK.failures(traces, root)


if __name__ == "__main__":
    unittest.main()
