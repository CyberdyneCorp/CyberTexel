from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "check_no_sibling_links", ROOT / "tools" / "check_no_sibling_links.py"
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def graph(nodes: list[tuple[str, str]], edges: list[tuple[str, str]]) -> str:
    lines = ['digraph "fixture" {']
    lines.extend(f'  "{node}" [ label = "{label}" ];' for node, label in nodes)
    lines.extend(f'  "{source}" -> "{target}";' for source, target in edges)
    lines.append("}")
    return "\n".join(lines)


class SiblingLinkAuditTests(unittest.TestCase):
    def test_internal_and_platform_dependencies_pass(self) -> None:
        fixture = graph(
            [("n0", "cybertexel"), ("n1", r"ctex_maps\n(CyberTexel::maps)"), ("n2", "Threads::Threads")],
            [("n0", "n1"), ("n0", "n2")],
        )
        self.assertEqual(MODULE.dependency_failures(fixture), [])

    def test_indirect_sibling_dependency_names_root_and_target(self) -> None:
        fixture = graph(
            [
                ("n0", "cybertexel"),
                ("n1", r"ctex_maps\n(CyberTexel::maps)"),
                ("n2", r"cyber::capi\n(CyberRemesher C API)"),
            ],
            [("n0", "n1"), ("n1", "n2")],
        )
        self.assertEqual(
            MODULE.dependency_failures(fixture),
            [
                "cybertexel reaches sibling link target: cyber::capi",
                "ctex_maps reaches sibling link target: cyber::capi",
            ],
        )

    def test_example_only_sibling_dependency_is_outside_library_graph(self) -> None:
        fixture = graph(
            [
                ("n0", "cybertexel"),
                ("n1", r"ctex_maps\n(CyberTexel::maps)"),
                ("n2", "ctex_cyber_remesher_provider_example"),
                ("n3", "cyber::capi"),
            ],
            [("n0", "n1"), ("n2", "n0"), ("n2", "n3")],
        )
        self.assertEqual(MODULE.dependency_failures(fixture), [])

    def test_missing_required_roots_fail_closed(self) -> None:
        self.assertEqual(
            MODULE.dependency_failures(graph([], [])),
            [
                "target graph is missing required library target: cybertexel",
                "target graph is missing required library target: ctex_maps",
            ],
        )


if __name__ == "__main__":
    unittest.main()
