#!/usr/bin/env python3
"""Check that numbered examples stay readable and show the API directly.

`examples` requires an example to explain itself in a header comment, name the
capability it covers and avoid helper indirection: the calls a consumer must
make have to be visible in the file rather than hidden behind repository-local
modules.
"""

from __future__ import annotations

import ast
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAXIMUM_LINES = 400


def import_roots(tree: ast.Module) -> list[tuple[str, int]]:
    roots: list[tuple[str, int]] = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            roots.extend((alias.name.split(".")[0], node.lineno) for alias in node.names)
        elif isinstance(node, ast.ImportFrom):
            if node.level:
                roots.append((".", node.lineno))
            elif node.module:
                roots.append((node.module.split(".")[0], node.lineno))
    return roots


def repository_local(root: str) -> bool:
    """True when the name resolves to a module or package inside this repository."""
    for directory in (ROOT / "examples", ROOT):
        if (directory / f"{root}.py").is_file() or (directory / root / "__init__.py").is_file():
            return True
    return False


def example_failures(script: Path) -> list[str]:
    source = script.read_text(encoding="utf-8")
    problems: list[str] = []
    lines = source.splitlines()
    if len(lines) > MAXIMUM_LINES:
        problems.append(f"is {len(lines)} lines; the readable ceiling is {MAXIMUM_LINES}")
    tree = ast.parse(source, filename=str(script))
    docstring = ast.get_docstring(tree)
    if not docstring:
        problems.append("has no header docstring explaining what it demonstrates")
    elif "Capabilities:" not in docstring:
        problems.append("header docstring does not name its capabilities")
    for root, line in import_roots(tree):
        if root == ".":
            problems.append(f"line {line} uses a relative import; an example is self-contained")
        elif repository_local(root):
            problems.append(
                f"line {line} imports repository-local {root!r}; the calls a consumer "
                "makes must be visible in the example"
            )
    if "sys.path" in source:
        problems.append("manipulates sys.path instead of importing the installed wheel")
    return problems


def main() -> int:
    failures: list[str] = []
    scripts = sorted((ROOT / "examples").glob("[0-9][0-9]_*.py"))
    if not scripts:
        print("example readability gate failed: no numbered examples", file=sys.stderr)
        return 1
    for script in scripts:
        for problem in example_failures(script):
            failures.append(f"{script.name}: {problem}")
    if failures:
        print("example readability gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1
    print(f"example readability gate passed: {len(scripts)} examples")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
