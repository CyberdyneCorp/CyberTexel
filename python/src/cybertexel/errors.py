from __future__ import annotations

from enum import IntEnum


class Result(IntEnum):
    SUCCESS = 0
    INVALID_ARGUMENT = 1
    MISSING_RESOURCE = 2
    UNSUPPORTED_OPERATION = 3
    OUT_OF_MEMORY = 4
    OVER_BUDGET = 5
    CANCELLED = 6
    INTERNAL_ERROR = 7
    BUFFER_TOO_SMALL = 8
    NO_UNDO = 9
    NO_REDO = 10
    STALE_STATE = 11


class CyberTexelError(RuntimeError):
    """Base error raised for a failed CyberTexel C operation."""

    def __init__(self, result: Result, diagnostic_code: int, diagnostic: str) -> None:
        super().__init__(diagnostic)
        self.result = result
        self.diagnostic_code = diagnostic_code
        self.diagnostic = diagnostic


class InvalidArgumentError(CyberTexelError):
    pass


class MissingResourceError(CyberTexelError):
    pass


class UnsupportedOperationError(CyberTexelError):
    pass


class OutOfMemoryError(CyberTexelError):
    pass


class OverBudgetError(CyberTexelError):
    pass


class CancelledError(CyberTexelError):
    pass


class BufferTooSmallError(CyberTexelError):
    pass


class NoUndoError(CyberTexelError):
    pass


class NoRedoError(CyberTexelError):
    pass


class StaleStateError(CyberTexelError):
    pass


_ERROR_TYPES: dict[Result, type[CyberTexelError]] = {
    Result.INVALID_ARGUMENT: InvalidArgumentError,
    Result.MISSING_RESOURCE: MissingResourceError,
    Result.UNSUPPORTED_OPERATION: UnsupportedOperationError,
    Result.OUT_OF_MEMORY: OutOfMemoryError,
    Result.OVER_BUDGET: OverBudgetError,
    Result.CANCELLED: CancelledError,
    Result.BUFFER_TOO_SMALL: BufferTooSmallError,
    Result.NO_UNDO: NoUndoError,
    Result.NO_REDO: NoRedoError,
    Result.STALE_STATE: StaleStateError,
}


def error_type(result: Result) -> type[CyberTexelError]:
    return _ERROR_TYPES.get(result, CyberTexelError)
