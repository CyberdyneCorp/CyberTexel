from __future__ import annotations

import contextlib
import os
from pathlib import Path
import runpy
import sys
import traceback
from collections.abc import Callable

from .document import Document


def _script_main(path: Path) -> Callable[[Document], object]:
    namespace = runpy.run_path(str(path), run_name="__cybertexel_script__")
    callback = namespace.get("main")
    if not callable(callback):
        raise TypeError("script must define callable main(document)")
    return callback


def _write_staged(path: Path, content: bytes) -> None:
    with path.open("xb") as stream:
        stream.write(content)
        stream.flush()
        os.fsync(stream.fileno())


def _redirect_stdout_to_diagnostics() -> None:
    sys.stdout.flush()
    os.dup2(sys.stderr.fileno(), sys.stdout.fileno())


def run(document_path: Path, script_path: Path, output_path: Path) -> None:
    project = document_path.read_bytes()
    with Document.from_project(project) as document:
        with contextlib.redirect_stdout(sys.stderr):
            callback = _script_main(script_path)
            callback(document)
        _write_staged(output_path, document.to_project_bytes())


def main(arguments: list[str] | None = None) -> int:
    values = sys.argv[1:] if arguments is None else arguments
    if len(values) != 3:
        print(
            "usage: python -m cybertexel._script_runner DOCUMENT SCRIPT OUTPUT",
            file=sys.stderr,
        )
        return 2
    try:
        _redirect_stdout_to_diagnostics()
        run(*(Path(value) for value in values))
    except KeyboardInterrupt:
        print("script interrupted", file=sys.stderr)
        return 130
    except BaseException:
        traceback.print_exc(file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
