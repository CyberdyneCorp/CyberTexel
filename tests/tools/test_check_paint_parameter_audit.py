import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).parents[2] / "tools" / "check_paint_parameter_audit.py"
SPEC = importlib.util.spec_from_file_location("check_paint_parameter_audit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


class PaintParameterAuditTests(unittest.TestCase):
    def fixture(self, table: str, implementation: str, tests: str) -> Path:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        (root / "docs").mkdir()
        (root / "include" / "ctex" / "paint").mkdir(parents=True)
        (root / "src" / "paint").mkdir(parents=True)
        (root / "tests" / "paint").mkdir(parents=True)
        (root / "docs" / "paint-tool-parameters.md").write_text(table, encoding="utf-8")
        (root / "include" / "ctex" / "paint" / "fixture.hpp").write_text(
            implementation, encoding="utf-8"
        )
        (root / "tests" / "paint" / "fixture_test.cpp").write_text(tests, encoding="utf-8")
        return root

    def test_accepts_one_implementation_and_evidence_marker_per_parameter(self) -> None:
        root = self.fixture(
            "| Parameter | Default | Minimum | Maximum | Unit |\n"
            "|---|---:|---:|---:|---|\n"
            "| `paint.amount` | 1 | 0 | 2 | ratio |\n",
            'ToolParameterDescriptor descriptor{"paint.amount", 1, 0, 2};\n',
            "// parameter-audit: paint.amount\n",
        )
        self.assertEqual(AUDIT.audit(root), [])

    def test_rejects_empty_parameter_table(self) -> None:
        root = self.fixture("# Parameters\n", "", "")
        self.assertIn(
            "parameter table contains no documented parameters", AUDIT.audit(root)
        )

    def test_rejects_missing_and_duplicate_inventory_entries(self) -> None:
        root = self.fixture(
            "| `paint.amount` | 1 | 0 | 2 | ratio |\n"
            "| `paint.amount` | 1 | 0 | 2 | ratio |\n"
            "| `paint.missing` | 1 | 0 | 2 | ratio |\n",
            'ToolParameterDescriptor amount{"paint.amount", 1, 0, 2};\n'
            'ToolParameterDescriptor extra{"paint.extra", 1, 0, 2};\n',
            "// parameter-audit: paint.amount\n"
            "// parameter-audit: paint.amount\n"
            "// parameter-audit: paint.extra\n",
        )
        failures = AUDIT.audit(root)
        self.assertTrue(any("appears 2 times" in failure for failure in failures))
        self.assertTrue(any("0 implementation descriptors" in failure for failure in failures))
        self.assertTrue(any("2 behavioural" in failure for failure in failures))
        self.assertTrue(any("descriptor 'paint.extra'" in failure for failure in failures))
        self.assertTrue(any("marker 'paint.extra'" in failure for failure in failures))


if __name__ == "__main__":
    unittest.main()
