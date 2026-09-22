from __future__ import annotations

import ctypes
import os
import platform
from pathlib import Path

from .errors import Result, error_type

ABI_MAJOR = 0


class Version(ctypes.Structure):
    _fields_ = [
        ("major", ctypes.c_uint32),
        ("minor", ctypes.c_uint32),
        ("patch", ctypes.c_uint32),
        ("string", ctypes.c_char_p),
    ]


class ImageDecodeLimits(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("maximum_width", ctypes.c_uint32),
        ("maximum_height", ctypes.c_uint32),
        ("maximum_decoded_bytes", ctypes.c_size_t),
    ]


class DecodedImageInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("channel_count", ctypes.c_uint32),
        ("scalar_representation", ctypes.c_uint32),
        ("bit_depth", ctypes.c_uint32),
        ("color_space", ctypes.c_uint32),
        ("detected_format", ctypes.c_uint32),
        ("extension_mismatch", ctypes.c_uint32),
        ("color_space_source", ctypes.c_uint32),
        ("uninterpretable_profile", ctypes.c_uint32),
    ]


class TextureSetDescriptor(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("display_name", ctypes.c_char_p),
        ("partition_kind", ctypes.c_uint32),
        ("partition_key", ctypes.c_char_p),
        ("uv_set", ctypes.c_char_p),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("default_bit_depth", ctypes.c_uint8),
        ("udim_tiling", ctypes.c_uint32),
    ]


class ChannelInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("component_count", ctypes.c_uint32),
        ("scalar_representation", ctypes.c_uint32),
        ("preferred_bit_depth", ctypes.c_uint32),
        ("default_value", ctypes.c_double * 4),
        ("default_value_count", ctypes.c_uint32),
        ("classification", ctypes.c_uint32),
        ("blending_policy", ctypes.c_uint32),
        ("evaluable", ctypes.c_uint32),
        ("enabled", ctypes.c_uint32),
        ("storage_bit_depth", ctypes.c_uint32),
    ]


class PaintPreviewInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("state", ctypes.c_uint32),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("component_count", ctypes.c_uint32),
        ("scalar_representation", ctypes.c_uint32),
        ("bit_depth", ctypes.c_uint32),
        ("pixel_byte_count", ctypes.c_size_t),
        ("resolved_dilation_radius", ctypes.c_uint32),
        ("dilation_radius_clamped", ctypes.c_uint32),
        ("dilated_texel_count", ctypes.c_size_t),
        ("zero_gradient_texel_count", ctypes.c_size_t),
        ("baseline_epoch", ctypes.c_uint64),
        ("baseline_revision", ctypes.c_uint64),
        ("preview_epoch", ctypes.c_uint64),
        ("preview_revision", ctypes.c_uint64),
        ("committed_epoch", ctypes.c_uint64),
        ("committed_revision", ctypes.c_uint64),
        ("changed_tile_count", ctypes.c_size_t),
        ("maximum_component_error", ctypes.c_double),
    ]


class UvSetDescriptor(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("name", ctypes.c_char_p),
        ("values", ctypes.c_void_p),
        ("value_count", ctypes.c_size_t),
    ]


class MeshPartitionDescriptor(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("kind", ctypes.c_uint32),
        ("stable_key", ctypes.c_char_p),
        ("display_name", ctypes.c_char_p),
    ]


class MeshDescriptor(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("positions", ctypes.c_void_p),
        ("position_count", ctypes.c_size_t),
        ("normals", ctypes.c_void_p),
        ("normal_count", ctypes.c_size_t),
        ("vertex_colors", ctypes.c_void_p),
        ("vertex_color_count", ctypes.c_size_t),
        ("triangle_indices", ctypes.c_void_p),
        ("triangle_index_count", ctypes.c_size_t),
        ("uv_sets", ctypes.POINTER(UvSetDescriptor)),
        ("uv_set_count", ctypes.c_size_t),
        ("default_uv_set", ctypes.c_char_p),
        ("partitions", ctypes.POINTER(MeshPartitionDescriptor)),
        ("partition_count", ctypes.c_size_t),
        ("face_partition_indices", ctypes.c_void_p),
        ("face_partition_index_count", ctypes.c_size_t),
        ("face_material_ids", ctypes.c_void_p),
        ("face_material_id_count", ctypes.c_size_t),
    ]


class MeshMapPixelBufferDescriptor(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("component_type", ctypes.c_uint32),
        ("component_count", ctypes.c_uint32),
        ("row_stride_bytes", ctypes.c_size_t),
        ("pixels", ctypes.c_void_p),
        ("pixel_bytes", ctypes.c_size_t),
    ]


class MeshMapImportDescriptor(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("kind", ctypes.c_uint32),
        ("channel_meaning", ctypes.c_uint32),
        ("color_space", ctypes.c_uint32),
        ("has_normal_convention", ctypes.c_uint32),
        ("normal_convention", ctypes.c_uint32),
        ("tangent_frame", ctypes.c_void_p),
        ("buffer", MeshMapPixelBufferDescriptor),
    ]


class MeshMapImportInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("replaced_existing", ctypes.c_uint32),
        ("resolution_mismatch", ctypes.c_uint32),
        ("stale", ctypes.c_uint32),
        ("converted_to_working_space", ctypes.c_uint32),
        ("storage_color_space", ctypes.c_uint32),
        ("channel_meaning", ctypes.c_uint32),
        ("map_width", ctypes.c_uint32),
        ("map_height", ctypes.c_uint32),
        ("texture_set_width", ctypes.c_uint32),
        ("texture_set_height", ctypes.c_uint32),
        ("produced_mesh_revision", ctypes.c_uint64),
        ("current_mesh_revision", ctypes.c_uint64),
    ]


class MeshMapGeneratorResolvedParameter(ctypes.Structure):
    _fields_ = [
        ("name_offset", ctypes.c_size_t),
        ("name_size", ctypes.c_size_t),
        ("value", ctypes.c_double),
    ]


class MeshMapGeneratorParameterClamp(ctypes.Structure):
    _fields_ = [
        ("name_offset", ctypes.c_size_t),
        ("name_size", ctypes.c_size_t),
        ("supplied", ctypes.c_double),
        ("resolved", ctypes.c_double),
    ]


class MeshMapStaleness(ctypes.Structure):
    _fields_ = [
        ("kind", ctypes.c_uint32),
        ("produced_mesh_revision", ctypes.c_uint64),
        ("current_mesh_revision", ctypes.c_uint64),
    ]


class MeshMapGeneratorResultInfo(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint32),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("row_stride_bytes", ctypes.c_size_t),
        ("required_mask_value_count", ctypes.c_size_t),
        ("required_resolved_parameter_count", ctypes.c_size_t),
        ("required_parameter_clamp_count", ctypes.c_size_t),
        ("required_stale_map_count", ctypes.c_size_t),
        ("message_offset", ctypes.c_size_t),
        ("message_size", ctypes.c_size_t),
        ("required_string_size", ctypes.c_size_t),
    ]


def _bundled_name() -> str:
    if os.name == "nt":
        return "cybertexel_c.dll"
    if platform.system() == "Darwin":
        return "libcybertexel_c.dylib"
    return "libcybertexel_c.so"


def _library_path() -> Path:
    configured = os.environ.get("CYBERTEXEL_LIBRARY")
    if configured:
        return Path(configured).expanduser().resolve()
    return Path(__file__).resolve().parent / "_native_lib" / _bundled_name()


def _signature(
    library: ctypes.CDLL, name: str, arguments: list[object], result: object
) -> None:
    function = getattr(library, name)
    function.argtypes = arguments
    function.restype = result


def _load() -> ctypes.CDLL:
    path = _library_path()
    if not path.is_file():
        raise ImportError(
            f"CyberTexel native library was not found at {path}; reinstall the platform wheel"
        )
    library = ctypes.CDLL(str(path))
    _signature(library, "ctex_get_version", [], Version)
    _signature(library, "ctex_get_abi_version", [], Version)
    _signature(library, "ctex_get_last_diagnostic", [], ctypes.c_char_p)
    _signature(library, "ctex_get_last_diagnostic_code", [], ctypes.c_uint32)
    _signature(
        library,
        "ctex_image_decode_memory",
        [
            ctypes.c_void_p,
            ctypes.c_size_t,
            ctypes.c_char_p,
            ctypes.c_uint32,
            ctypes.c_uint32,
            ctypes.POINTER(ImageDecodeLimits),
            ctypes.POINTER(DecodedImageInfo),
            ctypes.c_void_p,
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t),
        ],
        ctypes.c_uint32,
    )
    _signature(
        library,
        "ctex_document_create",
        [ctypes.POINTER(ctypes.c_void_p)],
        ctypes.c_uint32,
    )
    _signature(library, "ctex_document_destroy", [ctypes.c_void_p], None)
    _signature(
        library,
        "ctex_document_create_texture_set",
        [ctypes.c_void_p, ctypes.POINTER(TextureSetDescriptor)],
        ctypes.c_uint32,
    )
    _signature(
        library,
        "ctex_document_get_texture_set_ids",
        [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t),
            ctypes.POINTER(ctypes.c_size_t),
        ],
        ctypes.c_uint32,
    )
    _signature(
        library,
        "ctex_texture_set_set_channel_enabled",
        [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_uint32,
            ctypes.c_uint32,
        ],
        ctypes.c_uint32,
    )
    _signature(
        library,
        "ctex_texture_set_get_channel_info",
        [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.POINTER(ChannelInfo),
            ctypes.c_char_p,
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t),
        ],
        ctypes.c_uint32,
    )
    _signature(
        library,
        "ctex_paint_preview_session_create",
        [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_void_p),
        ],
        ctypes.c_uint32,
    )
    _signature(library, "ctex_paint_preview_session_destroy", [ctypes.c_void_p], None)
    _signature(
        library,
        "ctex_paint_preview_session_get_info",
        [ctypes.c_void_p, ctypes.POINTER(PaintPreviewInfo)],
        ctypes.c_uint32,
    )
    _signature(
        library,
        "ctex_paint_preview_session_get_pixels",
        [
            ctypes.c_void_p,
            ctypes.c_void_p,
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t),
        ],
        ctypes.c_uint32,
    )
    _signature(
        library,
        "ctex_mesh_create",
        [ctypes.POINTER(MeshDescriptor), ctypes.POINTER(ctypes.c_void_p)],
        ctypes.c_uint32,
    )
    _signature(library, "ctex_mesh_destroy", [ctypes.c_void_p], None)
    _signature(
        library,
        "ctex_mesh_map_set_create",
        [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_void_p),
        ],
        ctypes.c_uint32,
    )
    _signature(library, "ctex_mesh_map_set_destroy", [ctypes.c_void_p], None)
    _signature(
        library,
        "ctex_mesh_map_set_import_external",
        [
            ctypes.c_void_p,
            ctypes.POINTER(MeshMapImportDescriptor),
            ctypes.POINTER(MeshMapImportInfo),
        ],
        ctypes.c_uint32,
    )
    _signature(
        library,
        "ctex_mesh_map_generator_generate",
        [
            ctypes.c_void_p,
            ctypes.c_uint32,
            ctypes.c_uint32,
            ctypes.c_uint32,
            ctypes.c_void_p,
            ctypes.c_size_t,
            ctypes.POINTER(MeshMapGeneratorResultInfo),
            ctypes.c_void_p,
            ctypes.c_size_t,
            ctypes.POINTER(MeshMapGeneratorResolvedParameter),
            ctypes.c_size_t,
            ctypes.POINTER(MeshMapGeneratorParameterClamp),
            ctypes.c_size_t,
            ctypes.POINTER(MeshMapStaleness),
            ctypes.c_size_t,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ],
        ctypes.c_uint32,
    )
    abi = library.ctex_get_abi_version()
    if abi.major != ABI_MAJOR:
        native = (
            abi.string.decode("utf-8")
            if abi.string
            else f"{abi.major}.{abi.minor}.{abi.patch}"
        )
        raise ImportError(
            f"CyberTexel Python binding expects ABI major {ABI_MAJOR}, but loaded native {native}"
        )
    return library


LIB = _load()


def check(result_value: int) -> None:
    result = Result(result_value)
    if result == Result.SUCCESS:
        return
    raw = LIB.ctex_get_last_diagnostic()
    diagnostic = (
        raw.decode("utf-8", errors="replace") if raw else "CyberTexel operation failed"
    )
    diagnostic_code = int(LIB.ctex_get_last_diagnostic_code())
    raise error_type(result)(result, diagnostic_code, diagnostic)


def native_version() -> str:
    value = LIB.ctex_get_version()
    return (
        value.string.decode("utf-8")
        if value.string
        else f"{value.major}.{value.minor}.{value.patch}"
    )
