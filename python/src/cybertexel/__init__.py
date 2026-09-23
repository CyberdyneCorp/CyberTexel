from typing import TYPE_CHECKING

from . import capi
from ._native import native_version
from .document import (
    Document,
    LayerChannel,
    LayerEntry,
    LayerKind,
    SourceDeletionPolicy,
    TextureChannel,
    TextureSet,
)
from .errors import (
    BufferTooSmallError,
    CancelledError,
    CyberTexelError,
    InvalidArgumentError,
    MissingResourceError,
    NoRedoError,
    NoUndoError,
    OutOfMemoryError,
    OverBudgetError,
    Result,
    StaleStateError,
    UnsupportedOperationError,
)
from .host import (
    CompletedResource,
    CompletionDisposition,
    CompletionResult,
    HostExecutionSession,
    HostMaterialProgram,
    HostReadback,
    HostResource,
    ReadbackStatus,
    Recovery,
    ReplaySemantics,
    ResourceOwner,
    ResourceState,
    RevisionCursor,
    ShaderTarget,
    Snapshot,
    SnapshotPool,
    emit_default_host_material,
)

if TYPE_CHECKING:
    from .image import (
        ChannelSemantic,
        ColorSpace,
        DecodedImage,
        ImageFileFormat,
        InputColorSpace,
        decode_image,
        encode_image,
    )
    from .mesh import (
        Mesh,
        MeshMapGeneratorKind,
        MeshMapImportReport,
        MeshMapKind,
        MeshMapSet,
        PickHit,
    )


_LAZY_EXPORTS = {
    "ChannelSemantic": (".image", "ChannelSemantic"),
    "ColorSpace": (".image", "ColorSpace"),
    "DecodedImage": (".image", "DecodedImage"),
    "ImageFileFormat": (".image", "ImageFileFormat"),
    "InputColorSpace": (".image", "InputColorSpace"),
    "decode_image": (".image", "decode_image"),
    "encode_image": (".image", "encode_image"),
    "Mesh": (".mesh", "Mesh"),
    "MeshMapGeneratorKind": (".mesh", "MeshMapGeneratorKind"),
    "MeshMapImportReport": (".mesh", "MeshMapImportReport"),
    "MeshMapKind": (".mesh", "MeshMapKind"),
    "MeshMapSet": (".mesh", "MeshMapSet"),
    "PickHit": (".mesh", "PickHit"),
}


def __getattr__(name: str) -> object:
    import importlib

    target = _LAZY_EXPORTS.get(name)
    if target is None:
        raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
    module_name, attribute = target
    value = getattr(importlib.import_module(module_name, __name__), attribute)
    globals()[name] = value
    return value


__all__ = [
    "BufferTooSmallError",
    "capi",
    "CancelledError",
    "ChannelSemantic",
    "ColorSpace",
    "CompletedResource",
    "CompletionDisposition",
    "CompletionResult",
    "CyberTexelError",
    "DecodedImage",
    "Document",
    "LayerChannel",
    "LayerEntry",
    "LayerKind",
    "SourceDeletionPolicy",
    "TextureChannel",
    "HostExecutionSession",
    "HostMaterialProgram",
    "HostReadback",
    "HostResource",
    "ImageFileFormat",
    "InputColorSpace",
    "InvalidArgumentError",
    "Mesh",
    "MeshMapGeneratorKind",
    "MeshMapImportReport",
    "MeshMapKind",
    "MeshMapSet",
    "PickHit",
    "MissingResourceError",
    "NoRedoError",
    "NoUndoError",
    "OutOfMemoryError",
    "OverBudgetError",
    "ReadbackStatus",
    "Recovery",
    "ReplaySemantics",
    "ResourceOwner",
    "ResourceState",
    "Result",
    "RevisionCursor",
    "ShaderTarget",
    "Snapshot",
    "SnapshotPool",
    "StaleStateError",
    "TextureSet",
    "UnsupportedOperationError",
    "decode_image",
    "encode_image",
    "emit_default_host_material",
    "native_version",
]

__version__ = "0.1.0"
