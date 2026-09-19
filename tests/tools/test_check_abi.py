from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_abi.py"
SPEC = importlib.util.spec_from_file_location("check_abi", SCRIPT)
assert SPEC and SPEC.loader
CHECK_ABI = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK_ABI)


def surface(version: str = "0.1.0") -> dict:
    return {
        "abi_version": version,
        "symbols": {"ctex_old": "int ctex_old(void)"},
        "structures": {"ctex_item_descriptor": ["uint32_t size", "uint32_t value"]},
        "enums": {"ctex_result": [["CTEX_OK", 0], ["CTEX_ERROR", 1]]},
    }


class AbiGateTests(unittest.TestCase):
    def test_append_only_additions_are_compatible(self) -> None:
        baseline = surface()
        current = surface("0.2.0")
        current["symbols"]["ctex_new"] = "int ctex_new(void)"
        current["structures"]["ctex_item_descriptor"].append("uint32_t appended")
        current["enums"]["ctex_result"].append(["CTEX_NEW_ERROR", 2])
        self.assertEqual(CHECK_ABI.compatibility_failures(baseline, current), [])

    def test_same_major_breaks_are_named(self) -> None:
        baseline = surface()
        current = surface("0.2.0")
        current["symbols"]["ctex_old"] = "int ctex_old(uint32_t value)"
        current["structures"]["ctex_item_descriptor"].reverse()
        current["enums"]["ctex_result"][1][1] = 7
        failures = CHECK_ABI.compatibility_failures(baseline, current)
        self.assertTrue(any("signature changed" in failure for failure in failures))
        self.assertTrue(any("structure fields changed" in failure for failure in failures))
        self.assertTrue(any("enum values changed" in failure for failure in failures))

    def test_same_major_symbol_removal_is_named(self) -> None:
        baseline = surface()
        current = surface("0.2.0")
        del current["symbols"]["ctex_old"]
        self.assertEqual(
            CHECK_ABI.compatibility_failures(baseline, current),
            ["ABI symbol removed without a major bump: ctex_old"],
        )

    def test_major_bump_permits_breaks(self) -> None:
        baseline = surface()
        current = surface("1.0.0")
        current["symbols"] = {}
        current["structures"] = {}
        current["enums"] = {}
        self.assertEqual(CHECK_ABI.compatibility_failures(baseline, current), [])

    def test_descriptor_size_must_be_first(self) -> None:
        current = surface()
        current["structures"]["ctex_item_descriptor"] = ["uint32_t value", "uint32_t size"]
        self.assertEqual(
            CHECK_ABI.descriptor_failures(current),
            ["ABI descriptor does not begin with uint32_t size: ctex_item_descriptor"],
        )


if __name__ == "__main__":
    unittest.main()
