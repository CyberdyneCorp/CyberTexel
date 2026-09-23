"""Record which C ABI symbols an example actually calls.

Placed on ``PYTHONPATH`` by ``examples/run_all.py`` when ``CTEX_SYMBOL_TRACE``
names a directory. Python imports ``sitecustomize`` before any example code, so
the wrapper is installed before ``cybertexel`` loads the shared library.

The trace records a symbol when it is *called*, not when its ctypes signature is
declared, so declaring a prototype cannot stand in for exercising the feature.
"""

from __future__ import annotations

import atexit
import ctypes
import json
import os
import sys
import threading
from pathlib import Path


_PREFIX = "ctex_"
_called: set[str] = set()
_lock = threading.Lock()


class _TracedFunction:
    """Proxy that records a call and forwards everything else to the real one."""

    def __init__(self, function: object, name: str) -> None:
        object.__setattr__(self, "_ctex_function", function)
        object.__setattr__(self, "_ctex_name", name)

    def __call__(self, *arguments: object, **keywords: object) -> object:
        with _lock:
            _called.add(object.__getattribute__(self, "_ctex_name"))
        return object.__getattribute__(self, "_ctex_function")(*arguments, **keywords)

    def __getattr__(self, attribute: str) -> object:
        return getattr(object.__getattribute__(self, "_ctex_function"), attribute)

    def __setattr__(self, attribute: str, value: object) -> None:
        setattr(object.__getattribute__(self, "_ctex_function"), attribute, value)

    def __repr__(self) -> str:
        return f"<traced {object.__getattribute__(self, '_ctex_name')}>"


def _install() -> None:
    original = ctypes.CDLL.__getattr__

    def traced(self: ctypes.CDLL, name: str) -> object:
        function = original(self, name)
        if not name.startswith(_PREFIX):
            return function
        wrapper = _TracedFunction(function, name)
        # ``CDLL.__getattr__`` caches the raw function on the instance; replace
        # that cache so every later call goes through the proxy.
        self.__dict__[name] = wrapper
        return wrapper

    ctypes.CDLL.__getattr__ = traced  # type: ignore[method-assign]


def _write(destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    name = os.environ.get("CTEX_SYMBOL_TRACE_NAME") or Path(sys.argv[0]).stem or "trace"
    with _lock:
        symbols = sorted(_called)
    payload = {"schema": 1, "example": name, "symbols": symbols}
    (destination / f"{name}.json").write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


_destination = os.environ.get("CTEX_SYMBOL_TRACE")
if _destination:
    _install()
    atexit.register(_write, Path(_destination))
