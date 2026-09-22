from __future__ import annotations

import ctypes
from collections.abc import Sequence
from dataclasses import dataclass
from enum import IntEnum
from typing import TYPE_CHECKING

from ._native import LIB, _signature, check

if TYPE_CHECKING:
    from .document import Document, TextureSet


class ResourceOwner(IntEnum):
    LIBRARY = 0
    HOST = 1


class ResourceState(IntEnum):
    SHADER_READ = 0
    STORAGE_READ = 1
    STORAGE_WRITE = 2
    RENDER_TARGET = 3
    DEPTH_TARGET = 4


class ReplaySemantics(IntEnum):
    DETERMINISTIC = 0
    CHECKPOINT_ONLY = 1


class CompletionDisposition(IntEnum):
    PUBLISHED = 0
    AWAITING_RECOVERY = 1
    STALE = 2
    CANCELLED = 3
    FAILED = 4
    REJECTED = 5
    DUPLICATE = 6
    UNKNOWN_TOKEN = 7


class ReadbackStatus(IntEnum):
    PENDING = 0
    COMPLETE = 1
    CANCELLED = 2
    FAILED = 3


class ShaderTarget(IntEnum):
    WGSL = 0
    MSL = 1
    SPIRV = 2
    HLSL = 3


@dataclass(frozen=True)
class RevisionCursor:
    epoch: int
    revision: int


@dataclass(frozen=True)
class HostResource:
    logical_id: str
    generation: int
    role: str
    format: int
    width: int
    height: int
    output: bool
    layers: int = 1
    mip_levels: int = 1
    tile_width: int = 0
    tile_height: int = 0
    externally_initialized: bool = True
    owner: ResourceOwner = ResourceOwner.HOST
    required_state: ResourceState = ResourceState.SHADER_READ


@dataclass(frozen=True)
class CompletedResource:
    logical_id: str
    generation: int
    format: int
    width: int
    height: int
    layers: int = 1


@dataclass(frozen=True)
class Recovery:
    deterministic_record_version: str
    checkpoint_revision: int
    retained_bytes: int = 0
    inputs_pinned: bool = True


@dataclass(frozen=True)
class ReleasedResource:
    logical_id: str
    generation: int


@dataclass(frozen=True)
class CompletionResult:
    disposition: CompletionDisposition
    completion_token: int
    published_revision: int | None
    released_resources: tuple[ReleasedResource, ...]
    message: str


@dataclass(frozen=True)
class HostMaterialProgram:
    target: ShaderTarget
    vertex_artifact: bytes
    fragment_artifact: bytes
    pass_plan: str
    workaround_report: str


class _RevisionCursor(ctypes.Structure):
    _fields_ = [("epoch", ctypes.c_uint64), ("revision", ctypes.c_uint64)]


class _TileVersion(ctypes.Structure):
    _fields_ = [
        ("x", ctypes.c_uint32),
        ("y", ctypes.c_uint32),
        ("revision", ctypes.c_uint64),
        ("generation", ctypes.c_uint64),
        ("residency", ctypes.c_uint32),
    ]


class _DeltaInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("disposition", ctypes.c_uint32),
        ("synchronized_cursor", _RevisionCursor),
        ("current_cursor", _RevisionCursor),
        ("changed_tile_count", ctypes.c_size_t),
        ("indexed_tiles_visited", ctypes.c_size_t),
    ]


class _SnapshotInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("disposition", ctypes.c_uint32),
        ("synchronized_cursor", _RevisionCursor),
        ("current_cursor", _RevisionCursor),
        ("changed_tile_count", ctypes.c_size_t),
        ("indexed_tiles_visited", ctypes.c_size_t),
        ("retained_bytes", ctypes.c_size_t),
        ("additional_pinned_bytes", ctypes.c_size_t),
    ]


class _TileLayout(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("row_pitch_bytes", ctypes.c_size_t),
        ("pixel_stride_bytes", ctypes.c_size_t),
        ("channel_order", ctypes.c_uint32),
        ("component_type", ctypes.c_uint32),
        ("component_byte_order", ctypes.c_uint32),
        ("tile_contiguity", ctypes.c_uint32),
        ("byte_size", ctypes.c_size_t),
    ]


class _ReadbackDestination(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("version", _TileVersion),
        ("layout", _TileLayout),
        ("output", ctypes.c_void_p),
        ("output_size", ctypes.c_size_t),
    ]


class _HostTileCompletion(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("version", _TileVersion),
        ("layout", _TileLayout),
        ("bytes", ctypes.c_void_p),
        ("byte_size", ctypes.c_size_t),
    ]


class _ReadbackInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("status", ctypes.c_uint32),
        ("output_readable", ctypes.c_uint32),
        ("tile_count", ctypes.c_size_t),
        ("required_detail_size", ctypes.c_size_t),
    ]


class _HostResource(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("logical_id", ctypes.c_char_p),
        ("generation", ctypes.c_uint64),
        ("role", ctypes.c_char_p),
        ("format", ctypes.c_uint32),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("layers", ctypes.c_uint32),
        ("mip_levels", ctypes.c_uint32),
        ("tile_width", ctypes.c_uint32),
        ("tile_height", ctypes.c_uint32),
        ("externally_initialized", ctypes.c_uint32),
        ("owner", ctypes.c_uint32),
        ("required_state", ctypes.c_uint32),
        ("output", ctypes.c_uint32),
    ]


class _Submission(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("operation", ctypes.c_char_p),
        ("base_revision", ctypes.c_uint64),
        ("resources", ctypes.POINTER(_HostResource)),
        ("resource_count", ctypes.c_size_t),
        ("replay_semantics", ctypes.c_uint32),
    ]


class _SubmissionInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("completion_token", ctypes.c_uint64),
        ("base_revision", ctypes.c_uint64),
        ("resource_count", ctypes.c_size_t),
    ]


class _CompletedResource(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("logical_id", ctypes.c_char_p),
        ("generation", ctypes.c_uint64),
        ("format", ctypes.c_uint32),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("layers", ctypes.c_uint32),
    ]


class _Recovery(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("kind", ctypes.c_uint32),
        ("checkpoint_complete", ctypes.c_uint32),
        ("checkpoint_revision", ctypes.c_uint64),
        ("operation_record_version", ctypes.c_char_p),
        ("inputs_pinned", ctypes.c_uint32),
        ("retained_bytes", ctypes.c_size_t),
    ]


class _Completion(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("completion_token", ctypes.c_uint64),
        ("status", ctypes.c_uint32),
        ("outputs", ctypes.POINTER(_CompletedResource)),
        ("output_count", ctypes.c_size_t),
        ("recovery", ctypes.POINTER(_Recovery)),
        ("detail", ctypes.c_char_p),
    ]


class _ResourceVersion(ctypes.Structure):
    _fields_ = [("logical_id_offset", ctypes.c_size_t), ("generation", ctypes.c_uint64)]


class _CompletionInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("disposition", ctypes.c_uint32),
        ("completion_token", ctypes.c_uint64),
        ("has_published_revision", ctypes.c_uint32),
        ("published_revision", ctypes.c_uint64),
        ("released_resource_count", ctypes.c_size_t),
        ("required_released_identity_size", ctypes.c_size_t),
        ("required_message_size", ctypes.c_size_t),
    ]


class _MaterialGraphInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("output_node_id", ctypes.c_uint64),
        ("node_count", ctypes.c_size_t),
        ("link_count", ctypes.c_size_t),
        ("output_channel_count", ctypes.c_size_t),
        ("canonical_size", ctypes.c_size_t),
        ("report_size", ctypes.c_size_t),
    ]


class _ShaderFeatures(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("binding_budget", ctypes.c_uint32),
        ("maximum_texture_dimension", ctypes.c_uint32),
        ("supported_texture_formats", ctypes.POINTER(ctypes.c_uint32)),
        ("supported_texture_format_count", ctypes.c_size_t),
        ("floating_point_filtering", ctypes.c_uint32),
        ("compute_available", ctypes.c_uint32),
    ]


class _ShaderTexture(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("logical_id", ctypes.c_char_p),
        ("generation", ctypes.c_uint64),
        ("role", ctypes.c_char_p),
        ("format", ctypes.c_uint32),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("layers", ctypes.c_uint32),
        ("mip_levels", ctypes.c_uint32),
        ("tile_width", ctypes.c_uint32),
        ("tile_height", ctypes.c_uint32),
        ("externally_initialized", ctypes.c_uint32),
    ]


class _ShaderRequest(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("stable_identity", ctypes.c_char_p),
        ("target", ctypes.c_uint32),
        ("features", _ShaderFeatures),
        ("resources", ctypes.c_void_p),
        ("resource_count", ctypes.c_size_t),
        ("output", _ShaderTexture),
        ("requested_filter", ctypes.c_uint32),
        ("vertex_count", ctypes.c_uint32),
    ]


class _ShaderInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("target", ctypes.c_uint32),
        ("vertex_artifact_size", ctypes.c_size_t),
        ("fragment_artifact_size", ctypes.c_size_t),
        ("pass_plan_size", ctypes.c_size_t),
        ("workaround_report_size", ctypes.c_size_t),
        ("pass_count", ctypes.c_size_t),
        ("logical_resource_count", ctypes.c_size_t),
        ("binding_count", ctypes.c_size_t),
        ("workaround_count", ctypes.c_size_t),
    ]


def _configure_signatures() -> None:
    u32 = ctypes.c_uint32
    ptr = ctypes.c_void_p
    _signature(
        LIB,
        "ctex_host_execution_session_create",
        [ctypes.c_uint64, ctypes.POINTER(ptr)],
        u32,
    )
    _signature(LIB, "ctex_host_execution_session_destroy", [ptr], None)
    _signature(
        LIB,
        "ctex_host_execution_session_submit",
        [ptr, ctypes.POINTER(_Submission), ctypes.POINTER(_SubmissionInfo)],
        u32,
    )
    _signature(
        LIB,
        "ctex_host_execution_session_complete",
        [ptr, ctypes.POINTER(_Completion), ctypes.POINTER(ptr)],
        u32,
    )
    _signature(
        LIB,
        "ctex_host_execution_session_get_committed_resource",
        [ptr, ctypes.c_char_p, ctypes.POINTER(u32), ctypes.POINTER(ctypes.c_uint64)],
        u32,
    )
    _signature(
        LIB,
        "ctex_host_execution_session_resource_is_held",
        [ptr, ctypes.c_char_p, ctypes.c_uint64, ctypes.POINTER(u32)],
        u32,
    )
    _signature(LIB, "ctex_host_completion_result_destroy", [ptr], None)
    _signature(
        LIB,
        "ctex_host_completion_result_get_info",
        [
            ptr,
            ctypes.POINTER(_CompletionInfo),
            ctypes.POINTER(_ResourceVersion),
            ctypes.c_size_t,
            ctypes.c_char_p,
            ctypes.c_size_t,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ],
        u32,
    )
    _signature(
        LIB,
        "ctex_texture_set_query_channel_delta",
        [
            ptr,
            ctypes.c_char_p,
            ctypes.c_char_p,
            _RevisionCursor,
            ctypes.POINTER(_TileVersion),
            ctypes.c_size_t,
            ctypes.POINTER(_DeltaInfo),
        ],
        u32,
    )
    _signature(
        LIB,
        "ctex_transport_snapshot_pool_create",
        [ctypes.c_size_t, ctypes.POINTER(ptr)],
        u32,
    )
    _signature(LIB, "ctex_transport_snapshot_pool_destroy", [ptr], None)
    _signature(
        LIB,
        "ctex_texture_set_query_channel_snapshot",
        [
            ptr,
            ptr,
            ctypes.c_char_p,
            ctypes.c_char_p,
            _RevisionCursor,
            ctypes.POINTER(ptr),
            ctypes.POINTER(_SnapshotInfo),
        ],
        u32,
    )
    _signature(LIB, "ctex_transport_snapshot_destroy", [ptr], None)
    _signature(
        LIB,
        "ctex_transport_snapshot_get_tile_versions",
        [
            ptr,
            ctypes.POINTER(_TileVersion),
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t),
        ],
        u32,
    )
    _signature(
        LIB,
        "ctex_transport_snapshot_get_tile_memory_layout",
        [ptr, _TileVersion, ptr, ctypes.POINTER(_TileLayout)],
        u32,
    )
    _signature(
        LIB,
        "ctex_transport_snapshot_begin_host_readback",
        [
            ptr,
            ptr,
            ctypes.POINTER(_ReadbackDestination),
            ctypes.c_size_t,
            ctypes.POINTER(ptr),
        ],
        u32,
    )
    _signature(LIB, "ctex_transport_readback_destroy", [ptr], None)
    _signature(
        LIB,
        "ctex_transport_readback_get_info",
        [ptr, ctypes.POINTER(_ReadbackInfo), ctypes.c_char_p, ctypes.c_size_t],
        u32,
    )
    _signature(
        LIB,
        "ctex_transport_readback_complete_host",
        [ptr, ctypes.POINTER(_HostTileCompletion), ctypes.c_size_t],
        u32,
    )
    _signature(
        LIB,
        "ctex_material_graph_create_default",
        [
            ctypes.POINTER(_MaterialGraphInfo),
            ptr,
            ctypes.c_size_t,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ],
        u32,
    )
    _signature(
        LIB,
        "ctex_shader_emit_material",
        [
            ptr,
            ptr,
            ctypes.c_size_t,
            ctypes.POINTER(_ShaderRequest),
            ctypes.POINTER(_ShaderInfo),
            ptr,
            ctypes.c_size_t,
            ptr,
            ctypes.c_size_t,
            ctypes.c_char_p,
            ctypes.c_size_t,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ],
        u32,
    )


_configure_signatures()


def emit_default_host_material(
    *,
    stable_identity: str,
    output_identity: str,
    width: int,
    height: int,
    target: ShaderTarget = ShaderTarget.WGSL,
) -> HostMaterialProgram:
    """Emit real shader bytes and the JSON pass plan consumed by a host."""

    graph_info = _MaterialGraphInfo()
    graph_info.size = ctypes.sizeof(_MaterialGraphInfo)
    check(
        LIB.ctex_material_graph_create_default(
            ctypes.byref(graph_info), None, 0, None, 0
        )
    )
    graph = ctypes.create_string_buffer(graph_info.canonical_size)
    check(
        LIB.ctex_material_graph_create_default(
            ctypes.byref(graph_info), graph, len(graph), None, 0
        )
    )
    formats = (ctypes.c_uint32 * 1)(2)
    request = _ShaderRequest(
        ctypes.sizeof(_ShaderRequest),
        stable_identity.encode(),
        int(target),
        _ShaderFeatures(ctypes.sizeof(_ShaderFeatures), 16, 8192, formats, 1, 1, 0),
        None,
        0,
        _ShaderTexture(
            ctypes.sizeof(_ShaderTexture),
            output_identity.encode(),
            1,
            b"material output",
            2,
            width,
            height,
            1,
            1,
            width,
            height,
            0,
        ),
        1,
        3,
    )
    info = _ShaderInfo()
    info.size = ctypes.sizeof(_ShaderInfo)
    check(
        LIB.ctex_shader_emit_material(
            None,
            graph,
            len(graph),
            ctypes.byref(request),
            ctypes.byref(info),
            None,
            0,
            None,
            0,
            None,
            0,
            None,
            0,
        )
    )
    vertex = ctypes.create_string_buffer(info.vertex_artifact_size)
    fragment = (
        ctypes.create_string_buffer(info.fragment_artifact_size)
        if info.fragment_artifact_size
        else None
    )
    plan = ctypes.create_string_buffer(info.pass_plan_size)
    workarounds = ctypes.create_string_buffer(info.workaround_report_size)
    check(
        LIB.ctex_shader_emit_material(
            None,
            graph,
            len(graph),
            ctypes.byref(request),
            ctypes.byref(info),
            vertex,
            len(vertex),
            fragment,
            len(fragment) if fragment else 0,
            plan,
            len(plan),
            workarounds,
            len(workarounds),
        )
    )
    return HostMaterialProgram(
        target,
        bytes(vertex.raw),
        bytes(fragment.raw) if fragment else b"",
        plan.value.decode("utf-8"),
        workarounds.value.decode("utf-8"),
    )


def _text(buffer: ctypes.Array[ctypes.c_char], offset: int = 0) -> str:
    return ctypes.string_at(ctypes.addressof(buffer) + offset).decode("utf-8")


class HostExecutionSession:
    def __init__(self, initial_revision: int = 0) -> None:
        handle = ctypes.c_void_p()
        check(
            LIB.ctex_host_execution_session_create(
                initial_revision, ctypes.byref(handle)
            )
        )
        self._handle: ctypes.c_void_p | None = handle

    def __enter__(self) -> HostExecutionSession:  # noqa: PYI034
        self._require_open()
        return self

    def __exit__(self, *_args: object) -> None:
        self.close()

    def __del__(self) -> None:
        self.close()

    def _require_open(self) -> ctypes.c_void_p:
        if not self._handle:
            raise RuntimeError("host execution session is closed")
        return self._handle

    def close(self) -> None:
        if getattr(self, "_handle", None):
            LIB.ctex_host_execution_session_destroy(self._handle)
            self._handle = None

    def submit(
        self,
        operation: str,
        base_revision: int,
        resources: Sequence[HostResource],
        replay: ReplaySemantics = ReplaySemantics.DETERMINISTIC,
    ) -> int:
        encoded = [(item.logical_id.encode(), item.role.encode()) for item in resources]
        native = (_HostResource * len(resources))(
            *[
                _HostResource(
                    ctypes.sizeof(_HostResource),
                    names[0],
                    item.generation,
                    names[1],
                    item.format,
                    item.width,
                    item.height,
                    item.layers,
                    item.mip_levels,
                    item.tile_width or item.width,
                    item.tile_height or item.height,
                    int(item.externally_initialized),
                    int(item.owner),
                    int(item.required_state),
                    int(item.output),
                )
                for item, names in zip(resources, encoded, strict=True)
            ]
        )
        descriptor = _Submission(
            ctypes.sizeof(_Submission),
            operation.encode(),
            base_revision,
            native,
            len(native),
            int(replay),
        )
        info = _SubmissionInfo()
        info.size = ctypes.sizeof(_SubmissionInfo)
        check(
            LIB.ctex_host_execution_session_submit(
                self._require_open(), ctypes.byref(descriptor), ctypes.byref(info)
            )
        )
        return int(info.completion_token)

    def complete(
        self,
        token: int,
        outputs: Sequence[CompletedResource],
        recovery: Recovery | None = None,
    ) -> CompletionResult:
        names = [item.logical_id.encode() for item in outputs]
        native_outputs = (_CompletedResource * len(outputs))(
            *[
                _CompletedResource(
                    ctypes.sizeof(_CompletedResource),
                    name,
                    item.generation,
                    item.format,
                    item.width,
                    item.height,
                    item.layers,
                )
                for item, name in zip(outputs, names, strict=True)
            ]
        )
        recovery_version = (
            recovery.deterministic_record_version.encode() if recovery else None
        )
        native_recovery = (
            _Recovery(
                ctypes.sizeof(_Recovery),
                1,
                1,
                recovery.checkpoint_revision,
                recovery_version,
                int(recovery.inputs_pinned),
                recovery.retained_bytes,
            )
            if recovery
            else None
        )
        descriptor = _Completion(
            ctypes.sizeof(_Completion),
            token,
            0,
            native_outputs,
            len(native_outputs),
            ctypes.pointer(native_recovery) if native_recovery else None,
            None,
        )
        result = ctypes.c_void_p()
        check(
            LIB.ctex_host_execution_session_complete(
                self._require_open(), ctypes.byref(descriptor), ctypes.byref(result)
            )
        )
        try:
            return _read_completion(result)
        finally:
            LIB.ctex_host_completion_result_destroy(result)

    def committed_generation(self, logical_id: str) -> int | None:
        found = ctypes.c_uint32()
        generation = ctypes.c_uint64()
        check(
            LIB.ctex_host_execution_session_get_committed_resource(
                self._require_open(),
                logical_id.encode(),
                ctypes.byref(found),
                ctypes.byref(generation),
            )
        )
        return int(generation.value) if found.value else None

    def resource_is_held(self, logical_id: str, generation: int) -> bool:
        held = ctypes.c_uint32()
        check(
            LIB.ctex_host_execution_session_resource_is_held(
                self._require_open(),
                logical_id.encode(),
                generation,
                ctypes.byref(held),
            )
        )
        return bool(held.value)


def _read_completion(handle: ctypes.c_void_p) -> CompletionResult:
    info = _CompletionInfo()
    info.size = ctypes.sizeof(_CompletionInfo)
    check(
        LIB.ctex_host_completion_result_get_info(
            handle, ctypes.byref(info), None, 0, None, 0, None, 0
        )
    )
    released = (_ResourceVersion * info.released_resource_count)()
    identities = ctypes.create_string_buffer(
        max(1, info.required_released_identity_size)
    )
    message = ctypes.create_string_buffer(max(1, info.required_message_size))
    check(
        LIB.ctex_host_completion_result_get_info(
            handle,
            ctypes.byref(info),
            released,
            len(released),
            identities,
            info.required_released_identity_size,
            message,
            info.required_message_size,
        )
    )
    return CompletionResult(
        CompletionDisposition(info.disposition),
        int(info.completion_token),
        int(info.published_revision) if info.has_published_revision else None,
        tuple(
            ReleasedResource(
                _text(identities, item.logical_id_offset), int(item.generation)
            )
            for item in released
        ),
        _text(message),
    )


class _SnapshotPoolStorage:
    def __init__(self, handle: ctypes.c_void_p) -> None:
        self.handle = handle

    def __del__(self) -> None:
        LIB.ctex_transport_snapshot_pool_destroy(self.handle)


class SnapshotPool:
    def __init__(self, budget_bytes: int) -> None:
        handle = ctypes.c_void_p()
        check(
            LIB.ctex_transport_snapshot_pool_create(budget_bytes, ctypes.byref(handle))
        )
        self._storage: _SnapshotPoolStorage | None = _SnapshotPoolStorage(handle)

    def __enter__(self) -> SnapshotPool:  # noqa: PYI034
        self._require_open()
        return self

    def __exit__(self, *_args: object) -> None:
        self.close()

    def __del__(self) -> None:
        self.close()

    def _require_open(self) -> ctypes.c_void_p:
        if not self._storage:
            raise RuntimeError("snapshot pool is closed")
        return self._storage.handle

    def close(self) -> None:
        self._storage = None

    def current_cursor(
        self, document: Document, texture_set: TextureSet, semantic_id: str
    ) -> RevisionCursor:
        info = _DeltaInfo()
        info.size = ctypes.sizeof(_DeltaInfo)
        check(
            LIB.ctex_texture_set_query_channel_delta(
                document._require_open(),
                texture_set.identifier.encode(),
                semantic_id.encode(),
                _RevisionCursor(0, 0),
                None,
                0,
                ctypes.byref(info),
            )
        )
        return RevisionCursor(info.current_cursor.epoch, info.current_cursor.revision)

    def snapshot(
        self,
        document: Document,
        texture_set: TextureSet,
        semantic_id: str,
        since: RevisionCursor,
    ) -> Snapshot:
        handle = ctypes.c_void_p()
        info = _SnapshotInfo()
        info.size = ctypes.sizeof(_SnapshotInfo)
        check(
            LIB.ctex_texture_set_query_channel_snapshot(
                self._require_open(),
                document._require_open(),
                texture_set.identifier.encode(),
                semantic_id.encode(),
                _RevisionCursor(since.epoch, since.revision),
                ctypes.byref(handle),
                ctypes.byref(info),
            )
        )
        assert self._storage is not None
        return Snapshot(
            handle,
            self._storage,
            RevisionCursor(info.current_cursor.epoch, info.current_cursor.revision),
        )


class Snapshot:
    def __init__(
        self,
        handle: ctypes.c_void_p,
        pool: _SnapshotPoolStorage,
        cursor: RevisionCursor,
    ) -> None:
        self._handle: ctypes.c_void_p | None = handle
        self._pool = pool
        self.cursor = cursor

    def __del__(self) -> None:
        self.close()

    def _require_open(self) -> ctypes.c_void_p:
        if not self._handle:
            raise RuntimeError("snapshot is closed")
        return self._handle

    def close(self) -> None:
        if getattr(self, "_handle", None):
            LIB.ctex_transport_snapshot_destroy(self._handle)
            self._handle = None

    def begin_host_readback(self) -> HostReadback:
        count = ctypes.c_size_t()
        handle = self._require_open()
        check(
            LIB.ctex_transport_snapshot_get_tile_versions(
                handle, None, 0, ctypes.byref(count)
            )
        )
        versions = (_TileVersion * count.value)()
        check(
            LIB.ctex_transport_snapshot_get_tile_versions(
                handle, versions, len(versions), ctypes.byref(count)
            )
        )
        layouts: list[_TileLayout] = []
        buffers: list[ctypes.Array[ctypes.c_char]] = []
        destinations = (_ReadbackDestination * len(versions))()
        for index, version in enumerate(versions):
            layout = _TileLayout()
            layout.size = ctypes.sizeof(_TileLayout)
            check(
                LIB.ctex_transport_snapshot_get_tile_memory_layout(
                    handle, version, None, ctypes.byref(layout)
                )
            )
            host_version = _TileVersion.from_buffer_copy(version)
            host_version.residency = 1
            output = ctypes.create_string_buffer(layout.byte_size)
            layouts.append(layout)
            buffers.append(output)
            destinations[index] = _ReadbackDestination(
                ctypes.sizeof(_ReadbackDestination),
                host_version,
                layout,
                ctypes.addressof(output),
                layout.byte_size,
            )
            versions[index] = host_version
        readback = ctypes.c_void_p()
        check(
            LIB.ctex_transport_snapshot_begin_host_readback(
                handle, None, destinations, len(destinations), ctypes.byref(readback)
            )
        )
        return HostReadback(readback, self, list(versions), layouts, buffers)


class HostReadback:
    def __init__(
        self,
        handle: ctypes.c_void_p,
        snapshot: Snapshot,
        versions: list[_TileVersion],
        layouts: list[_TileLayout],
        buffers: list[ctypes.Array[ctypes.c_char]],
    ) -> None:
        self._handle: ctypes.c_void_p | None = handle
        self._snapshot = snapshot
        self._versions = versions
        self._layouts = layouts
        self._buffers = buffers

    def __del__(self) -> None:
        self.close()

    def _require_open(self) -> ctypes.c_void_p:
        if not self._handle:
            raise RuntimeError("readback is closed")
        return self._handle

    @property
    def status(self) -> ReadbackStatus:
        info = _ReadbackInfo()
        info.size = ctypes.sizeof(_ReadbackInfo)
        check(
            LIB.ctex_transport_readback_get_info(
                self._require_open(), ctypes.byref(info), None, 0
            )
        )
        return ReadbackStatus(info.status)

    @property
    def tiles(self) -> tuple[bytes, ...]:
        if self.status is not ReadbackStatus.COMPLETE:
            raise RuntimeError("readback output is not readable")
        return tuple(bytes(buffer.raw) for buffer in self._buffers)

    @property
    def tile_byte_sizes(self) -> tuple[int, ...]:
        return tuple(int(layout.byte_size) for layout in self._layouts)

    def complete(self, tiles: Sequence[bytes]) -> None:
        if len(tiles) != len(self._versions):
            raise ValueError("completion must contain one payload per requested tile")
        inputs = [ctypes.create_string_buffer(value) for value in tiles]
        completions = (_HostTileCompletion * len(inputs))(
            *[
                _HostTileCompletion(
                    ctypes.sizeof(_HostTileCompletion),
                    version,
                    layout,
                    ctypes.addressof(value),
                    len(value.raw) - 1,
                )
                for version, layout, value in zip(
                    self._versions, self._layouts, inputs, strict=True
                )
            ]
        )
        check(
            LIB.ctex_transport_readback_complete_host(
                self._require_open(), completions, len(completions)
            )
        )

    def close(self) -> None:
        if getattr(self, "_handle", None):
            LIB.ctex_transport_readback_destroy(self._handle)
            self._handle = None
