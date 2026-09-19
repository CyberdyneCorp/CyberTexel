#!/usr/bin/env python3
"""Enforce CyberTexel's module dependency and backend isolation rules."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MODULE_FILE = ROOT / "CMakeLists.txt"
EXPECTED_DEPENDENCIES = {
    "image": set(),
    "mesh": set(),
    "pick": {"mesh"},
    "graph": {"image"},
    "emit": {"graph", "image"},
    "doc": {"graph", "image", "mesh"},
    "paint": {"doc", "emit", "graph", "pick"},
    "maps": {"doc", "image", "mesh"},
    "xport": {"doc"},
    "io": {"doc", "image"},
    "exec": {"emit", "image"},
    "capi": {"capi", "doc", "emit", "exec", "graph", "image", "io", "maps", "mesh", "paint", "pick", "xport"}
    - {"capi"},
}
MODULE_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]ctex/([^/]+)/')
BACKEND_INCLUDE = re.compile(
    r'^\s*#\s*include\s*[<"](?:Metal/|metal/|vulkan/|webgpu/|wgpu/|d3d\d*\.h|dxgi)',
    re.IGNORECASE,
)


def parse_modules(path: Path) -> dict[str, set[str]]:
    text = path.read_text(encoding="utf-8")
    graph: dict[str, set[str]] = {}
    for match in re.finditer(r"ctex_add_module\(\s*(\w+)(.*?)\)", text, re.DOTALL):
        name, arguments = match.groups()
        dependency_match = re.search(
            r"\bDEPENDS\b(.*?)(?:\bSOURCES\b|$)", arguments, re.DOTALL
        )
        dependencies = set(dependency_match.group(1).split()) if dependency_match else set()
        graph[name] = dependencies
    return graph


def find_cycle(graph: dict[str, set[str]]) -> list[str] | None:
    visited: set[str] = set()
    active: list[str] = []

    def visit(module: str) -> list[str] | None:
        if module in active:
            start = active.index(module)
            return [*active[start:], module]
        if module in visited:
            return None
        active.append(module)
        for dependency in sorted(graph.get(module, set())):
            cycle = visit(dependency)
            if cycle:
                return cycle
        active.pop()
        visited.add(module)
        return None

    for module in sorted(graph):
        cycle = visit(module)
        if cycle:
            return cycle
    return None


def graph_failures(graph: dict[str, set[str]]) -> list[str]:
    failures: list[str] = []
    cycle = find_cycle(graph)
    if cycle:
        failures.append(f"dependency cycle: {' -> '.join(cycle)}")

    for module in sorted(EXPECTED_DEPENDENCIES.keys() - graph.keys()):
        failures.append(f"module missing from CMake graph: {module}")
    for module in sorted(graph.keys() - EXPECTED_DEPENDENCIES.keys()):
        failures.append(f"unknown module in CMake graph: {module}")
    for module in sorted(EXPECTED_DEPENDENCIES.keys() & graph.keys()):
        if graph[module] != EXPECTED_DEPENDENCIES[module]:
            expected = ", ".join(sorted(EXPECTED_DEPENDENCIES[module])) or "nothing"
            actual = ", ".join(sorted(graph[module])) or "nothing"
            failures.append(f"{module} dependencies differ: expected {expected}; found {actual}")

    for module, dependencies in graph.items():
        if module != "capi" and "exec" in dependencies:
            failures.append(f"{module} must not depend on exec")
    return failures


def source_file_failures(
    root: Path,
    path: Path,
    module: str,
    dependencies: set[str],
) -> list[str]:
    failures: list[str] = []
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        backend = BACKEND_INCLUDE.match(line)
        if backend and module != "exec":
            failures.append(f"backend include outside exec: {path.relative_to(root)}:{line_number}")
        included = MODULE_INCLUDE.match(line)
        if not included:
            continue
        dependency = included.group(1)
        if dependency != module and dependency not in dependencies:
            failures.append(
                f"undeclared module dependency {module} -> {dependency}: "
                f"{path.relative_to(root)}:{line_number}"
            )
    return failures


def source_failures(root: Path, graph: dict[str, set[str]]) -> list[str]:
    failures: list[str] = []
    for module in sorted(graph):
        source_directory = root / "src" / module
        if not source_directory.is_dir():
            failures.append(f"module source directory missing: {source_directory.relative_to(root)}")
            continue
        paths = [*source_directory.rglob("*.cpp"), *source_directory.rglob("*.hpp")]
        for path in paths:
            failures.extend(source_file_failures(root, path, module, graph[module]))
    return failures


def main() -> int:
    graph = parse_modules(MODULE_FILE)
    failures = [*graph_failures(graph), *source_failures(ROOT, graph)]
    if failures:
        print("module layering check failed:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    print(f"ok: {len(graph)} modules, dependency graph is acyclic, backend includes isolated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
