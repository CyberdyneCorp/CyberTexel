from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_capi_coverage.py"
SPEC = importlib.util.spec_from_file_location("check_capi_coverage", SCRIPT)
assert SPEC and SPEC.loader
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class CApiCoverageTests(unittest.TestCase):
    def fixture(self) -> tuple[tempfile.TemporaryDirectory[str], Path, Path]:
        temporary = tempfile.TemporaryDirectory()
        root = Path(temporary.name)
        for capability in ("runtime-one", "non-runtime-one"):
            spec = root / "openspec" / "changes" / "bootstrap-v1-cybertexel" / "specs" / capability
            spec.mkdir(parents=True, exist_ok=True)
            content = (
                "# fixture\n\n### Requirement: Runtime operation\n"
                if capability == "runtime-one"
                else "# fixture\n\n### Requirement: Repository policy\n"
            )
            (spec / "spec.md").write_text(content, encoding="utf-8")
        header = root / "include" / "ctex"
        header.mkdir(parents=True)
        (header / "capi.h").write_text(
            "CTEX_API int ctex_one(void);\nCTEX_API void ctex_two(int value);\n",
            encoding="utf-8",
        )
        manifest = root / "manifest.json"
        return temporary, root, manifest

    @staticmethod
    def valid_manifest() -> dict:
        return {
            "schema": 1,
            "capabilities": [
                {
                    "name": "runtime-one",
                    "kind": "runtime",
                    "requirements": [
                        {
                            "requirement": "Runtime operation",
                            "symbols": ["ctex_one", "ctex_two"],
                        },
                    ],
                },
                {
                    "name": "non-runtime-one",
                    "kind": "non-runtime",
                    "rationale": "fixture policy",
                    "evidence": ["fixture gate"],
                },
            ],
        }

    def run_check(self, mutate=None) -> list[str]:
        temporary, root, manifest = self.fixture()
        self.addCleanup(temporary.cleanup)
        value = self.valid_manifest()
        if mutate is not None:
            mutate(value)
        manifest.write_text(json.dumps(value), encoding="utf-8")
        return CHECK.check(root, manifest)

    def test_complete_manifest_passes(self) -> None:
        self.assertEqual(self.run_check(), [])

    def test_missing_capability_is_named(self) -> None:
        failures = self.run_check(lambda value: value["capabilities"].pop())
        self.assertIn(
            "OpenSpec capability is missing from C ABI manifest: non-runtime-one", failures
        )

    def test_runtime_capability_requires_every_requirement(self) -> None:
        failures = self.run_check(
            lambda value: value["capabilities"][0].update({"requirements": []})
        )
        self.assertIn(
            "runtime-one: requirement lacks C ABI evidence: Runtime operation", failures
        )

    def test_unknown_and_unmapped_symbols_are_named(self) -> None:
        def mutate(value):
            value["capabilities"][0]["requirements"] = [
                {"requirement": "Runtime operation", "symbols": ["ctex_unknown"]}
            ]

        failures = self.run_check(mutate)
        self.assertTrue(any("unknown symbol: ctex_unknown" in failure for failure in failures))
        self.assertTrue(any("not mapped" in failure and "ctex_one" in failure for failure in failures))

    def test_non_runtime_capability_requires_rationale_and_evidence(self) -> None:
        def mutate(value):
            value["capabilities"][1].pop("rationale")
            value["capabilities"][1]["evidence"] = []

        failures = self.run_check(mutate)
        self.assertIn("non-runtime capability has no rationale: non-runtime-one", failures)
        self.assertIn("non-runtime capability has no evidence: non-runtime-one", failures)

    def test_repository_manifest_has_only_the_known_coverage_gap(self) -> None:
        failures = CHECK.check(CHECK.ROOT, CHECK.MANIFEST)
        self.assertEqual(len(failures), 96)
        self.assertTrue(
            all("requirement lacks C ABI evidence" in failure for failure in failures)
        )


if __name__ == "__main__":
    unittest.main()
