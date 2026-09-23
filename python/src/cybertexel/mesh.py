from __future__ import annotations

import ctypes
from dataclasses import dataclass
from enum import IntEnum

import numpy as np
import numpy.typing as npt

from ._native import (
    LIB,
    MeshDescriptor,
    MeshMapGeneratorParameterClamp,
    MeshMapGeneratorResolvedParameter,
    MeshMapGeneratorResultInfo,
    MeshMapImportDescriptor,
    MeshMapImportInfo,
    MeshMapPixelBufferDescriptor,
    MeshMapStaleness,
    MeshPartitionDescriptor,
    UvSetDescriptor,
    check,
)
from .document import Document, TextureSet


class MeshMapKind(IntEnum):
    TANGENT_SPACE_NORMAL = 0
    OBJECT_SPACE_NORMAL = 1
    WORLD_SPACE_DIRECTION = 2
    AMBIENT_OCCLUSION = 3
    CURVATURE = 4
    THICKNESS = 5
    POSITION = 6
    HEIGHT = 7
    BENT_NORMAL = 8
    MATERIAL_ID = 9
    OBJECT_ID = 10
    UV_DENSITY = 11
    VERTEX_COLOR = 12


class MeshMapGeneratorKind(IntEnum):
    AMBIENT_OCCLUSION = 0
    CURVATURE = 1
    THICKNESS = 2
    POSITION_GRADIENT = 3
    WORLD_SPACE_DIRECTION = 4
    DIRT = 5
    EDGE_WEAR = 6
    SCRATCHES = 7


@dataclass(frozen=True)
class MeshMapImportReport:
    replaced_existing: bool
    resolution_mismatch: bool
    stale: bool
    map_width: int
    map_height: int


@dataclass(frozen=True)
class PickHit:
    position: tuple[float, float, float]
    normal: tuple[float, float, float]
    uv: tuple[float, float]
    barycentric: tuple[float, float, float]
    triangle_index: int
    material_id: int
    texture_set_id: str


def _array(
    value: npt.ArrayLike, dtype: np.dtype[np.generic], columns: int
) -> np.ndarray:
    result = np.asarray(value, dtype=dtype)
    if result.ndim != 2 or result.shape[1] != columns:
        raise ValueError(f"expected an N x {columns} array, got shape {result.shape}")
    return np.ascontiguousarray(result)


class Mesh:
    """Allocator-owned mesh copied from NumPy inputs during construction."""

    def __init__(
        self,
        positions: npt.ArrayLike,
        triangle_indices: npt.ArrayLike,
        *,
        normals: npt.ArrayLike,
        uv: npt.ArrayLike,
        uv_name: str = "uv0",
        partition_key: str = "mesh",
        partition_name: str = "Mesh",
    ) -> None:
        position_array = _array(positions, np.dtype(np.float32), 3)
        normal_array = _array(normals, np.dtype(np.float32), 3)
        uv_array = _array(uv, np.dtype(np.float32), 2)
        triangle_array = _array(triangle_indices, np.dtype(np.uint32), 3)
        if (
            normal_array.shape[0] != position_array.shape[0]
            or uv_array.shape[0] != position_array.shape[0]
        ):
            raise ValueError(
                "positions, normals and UV arrays must have the same row count"
            )
        face_count = triangle_array.shape[0]
        face_partitions = np.zeros(face_count, dtype=np.uint32)
        face_materials = np.zeros(face_count, dtype=np.uint32)
        uv_name_bytes = uv_name.encode("utf-8")
        uv_descriptor = UvSetDescriptor(
            ctypes.sizeof(UvSetDescriptor),
            uv_name_bytes,
            ctypes.c_void_p(uv_array.ctypes.data),
            uv_array.shape[0],
        )
        partition = MeshPartitionDescriptor(
            ctypes.sizeof(MeshPartitionDescriptor),
            0,
            partition_key.encode("utf-8"),
            partition_name.encode("utf-8"),
        )
        descriptor = MeshDescriptor(
            ctypes.sizeof(MeshDescriptor),
            ctypes.c_void_p(position_array.ctypes.data),
            position_array.shape[0],
            ctypes.c_void_p(normal_array.ctypes.data),
            normal_array.shape[0],
            None,
            0,
            ctypes.c_void_p(triangle_array.ctypes.data),
            triangle_array.size,
            ctypes.pointer(uv_descriptor),
            1,
            uv_name_bytes,
            ctypes.pointer(partition),
            1,
            ctypes.c_void_p(face_partitions.ctypes.data),
            face_count,
            ctypes.c_void_p(face_materials.ctypes.data),
            face_count,
        )
        handle = ctypes.c_void_p()
        check(LIB.ctex_mesh_create(ctypes.byref(descriptor), ctypes.byref(handle)))
        self._handle = handle
        self._uv_name = uv_name

    def __enter__(self) -> Mesh:  # noqa: PYI034
        self._require_open()
        return self

    def __exit__(self, _type: object, _value: object, _traceback: object) -> None:
        self.close()

    def __del__(self) -> None:
        self.close()

    def _require_open(self) -> ctypes.c_void_p:
        handle = getattr(self, "_handle", None)
        if not handle:
            raise RuntimeError("mesh is closed")
        return handle

    def close(self) -> None:
        handle = getattr(self, "_handle", None)
        if handle:
            LIB.ctex_mesh_destroy(handle)
            self._handle = None

    def pick_uv(self, u: float, v: float) -> PickHit | None:
        """Return the deterministic surface hit at a UV coordinate, or ``None``."""

        from . import capi

        mesh = ctypes.cast(self._require_open(), ctypes.POINTER(capi.ctex_mesh))
        index = ctypes.POINTER(capi.ctex_uv_pick_index)()
        check(capi.ctex_uv_pick_index_create(mesh, self._uv_name.encode(), ctypes.byref(index)))
        try:
            binding = capi.ctex_pick_texture_set_binding_descriptor(
                ctypes.sizeof(capi.ctex_pick_texture_set_binding_descriptor),
                0,
                capi.String(self._uv_name.encode()),
            )
            info = capi.ctex_pick_query_info()
            info.size = ctypes.sizeof(info)
            coordinate = capi.ctex_vec2f(u, v)
            check(
                capi.ctex_pick_uv_query(
                    index,
                    coordinate,
                    ctypes.byref(binding),
                    None,
                    None,
                    0,
                    ctypes.byref(info),
                )
            )
            if info.result_count == 0:
                return None
            hit = capi.ctex_pick_hit()
            identity = ctypes.create_string_buffer(info.required_texture_set_id_size)
            check(
                capi.ctex_pick_uv_query(
                    index,
                    coordinate,
                    ctypes.byref(binding),
                    ctypes.byref(hit),
                    identity,
                    len(identity),
                    ctypes.byref(info),
                )
            )
            return PickHit(
                (hit.position.x, hit.position.y, hit.position.z),
                (
                    hit.interpolated_normal.x,
                    hit.interpolated_normal.y,
                    hit.interpolated_normal.z,
                ),
                (hit.uv.x, hit.uv.y),
                (hit.barycentric.x, hit.barycentric.y, hit.barycentric.z),
                int(hit.triangle_index),
                int(hit.material_id),
                identity.value.decode("utf-8"),
            )
        finally:
            capi.ctex_uv_pick_index_destroy(index)


class MeshMapSet:
    """Mesh maps bound to a document texture set and mesh revision."""

    def __init__(self, document: Document, texture_set: TextureSet, mesh: Mesh) -> None:
        handle = ctypes.c_void_p()
        check(
            LIB.ctex_mesh_map_set_create(
                document._require_open(),
                texture_set.identifier.encode("utf-8"),
                mesh._require_open(),
                ctypes.byref(handle),
            )
        )
        self._handle = handle
        self._document = document
        self._mesh = mesh

    def __enter__(self) -> MeshMapSet:  # noqa: PYI034
        self._require_open()
        return self

    def __exit__(self, _type: object, _value: object, _traceback: object) -> None:
        self.close()

    def __del__(self) -> None:
        self.close()

    def _require_open(self) -> ctypes.c_void_p:
        handle = getattr(self, "_handle", None)
        if not handle:
            raise RuntimeError("mesh-map set is closed")
        return handle

    def close(self) -> None:
        handle = getattr(self, "_handle", None)
        if handle:
            LIB.ctex_mesh_map_set_destroy(handle)
            self._handle = None

    def import_map(
        self,
        kind: MeshMapKind,
        pixels: npt.ArrayLike,
        *,
        channel_meaning: int | None = None,
    ) -> MeshMapImportReport:
        array = np.asarray(pixels)
        if array.dtype not in (
            np.dtype(np.uint8),
            np.dtype(np.uint16),
            np.dtype(np.float32),
        ):
            raise TypeError("mesh maps require uint8, uint16 or float32 pixels")
        if array.ndim == 2:
            array = array[:, :, np.newaxis]
        if array.ndim != 3 or not 1 <= array.shape[2] <= 4:
            raise ValueError(
                "mesh maps require shape (height, width) or (height, width, 1..4)"
            )
        array = np.ascontiguousarray(array)
        component_type = {
            np.dtype(np.uint8): 0,
            np.dtype(np.uint16): 1,
            np.dtype(np.float32): 2,
        }[array.dtype]
        components = array.shape[2]
        meaning = (
            channel_meaning
            if channel_meaning is not None
            else (0 if components == 1 else 5)
        )
        buffer = MeshMapPixelBufferDescriptor(
            ctypes.sizeof(MeshMapPixelBufferDescriptor),
            array.shape[1],
            array.shape[0],
            component_type,
            components,
            array.strides[0],
            ctypes.c_void_p(array.ctypes.data),
            array.nbytes,
        )
        descriptor = MeshMapImportDescriptor(
            ctypes.sizeof(MeshMapImportDescriptor),
            int(kind),
            meaning,
            0,
            0,
            0,
            None,
            buffer,
        )
        info = MeshMapImportInfo()
        info.size = ctypes.sizeof(MeshMapImportInfo)
        check(
            LIB.ctex_mesh_map_set_import_external(
                self._require_open(), ctypes.byref(descriptor), ctypes.byref(info)
            )
        )
        return MeshMapImportReport(
            replaced_existing=bool(info.replaced_existing),
            resolution_mismatch=bool(info.resolution_mismatch),
            stale=bool(info.stale),
            map_width=info.map_width,
            map_height=info.map_height,
        )

    def generate_mask(
        self, kind: MeshMapGeneratorKind, width: int, height: int
    ) -> np.ndarray:
        """Generate a float32 mask with the generator's default parameters."""
        sizing = MeshMapGeneratorResultInfo()
        sizing.size = ctypes.sizeof(MeshMapGeneratorResultInfo)
        check(
            LIB.ctex_mesh_map_generator_generate(
                self._require_open(),
                int(kind),
                width,
                height,
                None,
                0,
                ctypes.byref(sizing),
                None,
                0,
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
        mask = np.empty((sizing.height, sizing.width), dtype=np.float32)
        resolved = (
            MeshMapGeneratorResolvedParameter * sizing.required_resolved_parameter_count
        )()
        clamps = (
            MeshMapGeneratorParameterClamp * sizing.required_parameter_clamp_count
        )()
        stale = (MeshMapStaleness * sizing.required_stale_map_count)()
        strings = ctypes.create_string_buffer(sizing.required_string_size)
        result = MeshMapGeneratorResultInfo()
        result.size = ctypes.sizeof(MeshMapGeneratorResultInfo)
        check(
            LIB.ctex_mesh_map_generator_generate(
                self._require_open(),
                int(kind),
                width,
                height,
                None,
                0,
                ctypes.byref(result),
                ctypes.c_void_p(mask.ctypes.data),
                mask.size,
                resolved,
                len(resolved),
                clamps,
                len(clamps),
                stale,
                len(stale),
                strings,
                len(strings),
            )
        )
        return mask
