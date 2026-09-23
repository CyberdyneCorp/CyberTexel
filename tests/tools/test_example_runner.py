from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "examples" / "run_all.py"
SPEC = importlib.util.spec_from_file_location("example_runner", SCRIPT)
assert SPEC and SPEC.loader
RUNNER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUNNER)


EXAMPLE = '''
import argparse
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument("--output", type=Path, required=True)
p.add_argument("--executor", required=True)
a = p.parse_args()
assert a.executor == "cpu"
a.output.mkdir(parents=True, exist_ok=True)
(a.output / "result.txt").write_text("stable\\n")
'''


class ExampleRunnerTests(unittest.TestCase):
    def test_discovery_requires_numbered_examples(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(RUNNER.ExampleFailure, "no numbered examples"):
                RUNNER.discover_examples(Path(directory))

    def test_assert_compare_and_update_modes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            script = root / "01_fixture.py"
            outputs = root / "outputs"
            script.write_text(EXAMPLE, encoding="utf-8")
            RUNNER.run_example(script, "assert", "cpu", outputs)
            RUNNER.run_example(script, "update", "cpu", outputs)
            RUNNER.run_example(script, "compare", "cpu", outputs)
            (outputs / script.stem / "result.txt").write_text("drift\n", encoding="utf-8")
            with self.assertRaisesRegex(RUNNER.ExampleFailure, "committed output changed"):
                RUNNER.run_example(script, "compare", "cpu", outputs)

    def test_non_cpu_output_update_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            script = root / "01_fixture.py"
            script.write_text(EXAMPLE.replace('"cpu"', '"host"'), encoding="utf-8")
            with self.assertRaisesRegex(RUNNER.ExampleFailure, "only be updated"):
                RUNNER.run_example(script, "update", "host", root / "outputs")


if __name__ == "__main__":
    unittest.main()
