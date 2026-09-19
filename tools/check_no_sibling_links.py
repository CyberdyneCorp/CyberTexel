#!/usr/bin/env python3
"""Prove the shipped CyberTexel library graph has no sibling-engine links."""

from __future__ import annotations

import re
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT_TARGETS = ("cybertexel", "ctex_maps")
FORBIDDEN_NAMES = (
    "cyberremesher",
    "cyberremesheranduv",
    "cybercapi",
    "claycore",
    "claycapi",
)
NODE_PATTERN = re.compile(r'^\s*"([^"]+)" \[ label = "((?:[^"\\]|\\.)*)"', re.MULTILINE)
EDGE_PATTERN = re.compile(r'^\s*"([^"]+)" -> "([^"]+)"', re.MULTILINE)


def normalized(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.lower())


def parse_graph(text: str) -> tuple[dict[str, str], dict[str, set[str]]]:
    labels = {
        node: label.replace(r"\n", "\n")
        for node, label in NODE_PATTERN.findall(text)
        if not node.startswith("legendNode")
    }
    edges: dict[str, set[str]] = {node: set() for node in labels}
    for source, target in EDGE_PATTERN.findall(text):
        if source in labels and target in labels:
            edges[source].add(target)
    return labels, edges


def target_name(label: str) -> str:
    return label.splitlines()[0]


def reachable_nodes(root: str, edges: dict[str, set[str]]) -> set[str]:
    pending = [root]
    visited: set[str] = set()
    while pending:
        node = pending.pop()
        if node in visited:
            continue
        visited.add(node)
        pending.extend(edges.get(node, ()))
    return visited


def sibling_targets(nodes: set[str], labels: dict[str, str]) -> list[str]:
    matches = []
    for node in sorted(nodes):
        normalized_name = normalized(labels[node])
        if any(forbidden in normalized_name for forbidden in FORBIDDEN_NAMES):
            matches.append(target_name(labels[node]))
    return matches


def dependency_failures(text: str) -> list[str]:
    labels, edges = parse_graph(text)
    nodes_by_name = {target_name(label): node for node, label in labels.items()}
    failures: list[str] = []
    for root_name in ROOT_TARGETS:
        root = nodes_by_name.get(root_name)
        if root is None:
            failures.append(f"target graph is missing required library target: {root_name}")
            continue
        failures.extend(
            f"{root_name} reaches sibling link target: {name}"
            for name in sibling_targets(reachable_nodes(root, edges), labels)
        )
    return failures


def generate_graph(build_directory: Path) -> str:
    if not (build_directory / "CMakeCache.txt").is_file():
        raise RuntimeError(f"not a configured CMake build directory: {build_directory}")
    with tempfile.TemporaryDirectory(prefix="ctex-link-audit-") as temporary_directory:
        graph = Path(temporary_directory) / "targets.dot"
        result = subprocess.run(
            ["cmake", f"--graphviz={graph}", str(build_directory)],
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            detail = result.stderr.strip() or result.stdout.strip()
            raise RuntimeError(f"CMake target graph generation failed: {detail}")
        return graph.read_text(encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_no_sibling_links.py BUILD_DIRECTORY", file=sys.stderr)
        return 2
    try:
        failures = dependency_failures(generate_graph(Path(sys.argv[1]).resolve()))
    except (OSError, RuntimeError) as error:
        print(f"sibling-link audit failed: {error}", file=sys.stderr)
        return 1
    if failures:
        print("sibling-link audit failed:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    print("ok: CyberTexel library and mesh-map targets have no sibling-engine links")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
