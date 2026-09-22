from __future__ import annotations

import ctypes
from dataclasses import dataclass

import numpy as np
import numpy.typing as npt

from ._native import LIB, ChannelInfo, PaintPreviewInfo, TextureSetDescriptor, check


def _dtype(scalar_representation: int, bit_depth: int) -> np.dtype[np.generic]:
    if scalar_representation == 1 and bit_depth == 32:
        return np.dtype(np.float32)
    if scalar_representation == 0 and bit_depth == 8:
        return np.dtype(np.uint8)
    if scalar_representation == 0 and bit_depth == 16:
        return np.dtype(np.uint16)
    raise RuntimeError(
        f"unsupported channel representation={scalar_representation} bit_depth={bit_depth}"
    )


@dataclass(frozen=True)
class TextureSet:
    identifier: str
    width: int
    height: int


class Document:
    """Owns one native document handle; use as a context manager or call close()."""

    def __init__(self) -> None:
        handle = ctypes.c_void_p()
        check(LIB.ctex_document_create(ctypes.byref(handle)))
        self._handle = handle
        self._texture_sets: dict[str, TextureSet] = {}

    def __enter__(self) -> Document:  # noqa: PYI034
        self._require_open()
        return self

    def __exit__(self, _type: object, _value: object, _traceback: object) -> None:
        self.close()

    def __del__(self) -> None:
        self.close()

    def _require_open(self) -> ctypes.c_void_p:
        handle = getattr(self, "_handle", None)
        if not handle:
            raise RuntimeError("document is closed")
        return handle

    def close(self) -> None:
        handle = getattr(self, "_handle", None)
        if handle:
            LIB.ctex_document_destroy(handle)
            self._handle = None

    def create_texture_set(
        self,
        display_name: str,
        *,
        partition_key: str,
        uv_set: str = "uv0",
        width: int,
        height: int,
        default_bit_depth: int = 8,
    ) -> TextureSet:
        descriptor = TextureSetDescriptor(
            ctypes.sizeof(TextureSetDescriptor),
            display_name.encode("utf-8"),
            0,
            partition_key.encode("utf-8"),
            uv_set.encode("utf-8"),
            width,
            height,
            default_bit_depth,
            0,
        )
        check(
            LIB.ctex_document_create_texture_set(
                self._require_open(), ctypes.byref(descriptor)
            )
        )
        identifiers = self.texture_set_ids()
        identifier = next(
            value for value in identifiers if value not in self._texture_sets
        )
        result = TextureSet(identifier, width, height)
        self._texture_sets[identifier] = result
        return result

    def texture_set_ids(self) -> list[str]:
        required = ctypes.c_size_t()
        count = ctypes.c_size_t()
        handle = self._require_open()
        check(
            LIB.ctex_document_get_texture_set_ids(
                handle, None, 0, ctypes.byref(required), ctypes.byref(count)
            )
        )
        buffer = ctypes.create_string_buffer(required.value)
        check(
            LIB.ctex_document_get_texture_set_ids(
                handle, buffer, len(buffer), ctypes.byref(required), ctypes.byref(count)
            )
        )
        packed = bytes(buffer.raw[: required.value])
        return [part.decode("utf-8") for part in packed.split(b"\0") if part]

    def set_channel_enabled(
        self,
        texture_set: TextureSet | str,
        semantic_id: str,
        enabled: bool = True,
        *,
        bit_depth: int = 0,
    ) -> None:
        identifier = (
            texture_set.identifier
            if isinstance(texture_set, TextureSet)
            else texture_set
        )
        check(
            LIB.ctex_texture_set_set_channel_enabled(
                self._require_open(),
                identifier.encode("utf-8"),
                semantic_id.encode("utf-8"),
                int(enabled),
                bit_depth,
            )
        )

    def read_channel(
        self, texture_set: TextureSet, semantic_id: str
    ) -> npt.NDArray[np.generic]:
        """Return an independent caller-owned NumPy snapshot of a flat channel.

        CyberTexel writes directly into the array's contiguous storage. No
        intermediate Python pixel copy is made. The array remains valid after
        the document changes or closes because it is a snapshot, not a view.
        """

        handle = self._require_open()
        identifier = texture_set.identifier.encode("utf-8")
        semantic = semantic_id.encode("utf-8")
        channel = ChannelInfo()
        channel.size = ctypes.sizeof(ChannelInfo)
        mapping_size = ctypes.c_size_t()
        check(
            LIB.ctex_texture_set_get_channel_info(
                handle,
                identifier,
                semantic,
                ctypes.byref(channel),
                None,
                0,
                ctypes.byref(mapping_size),
            )
        )
        if not channel.enabled:
            raise ValueError(f"channel is disabled: {semantic_id}")
        session = ctypes.c_void_p()
        check(
            LIB.ctex_paint_preview_session_create(
                handle, identifier, semantic, ctypes.byref(session)
            )
        )
        try:
            preview = PaintPreviewInfo()
            preview.size = ctypes.sizeof(PaintPreviewInfo)
            check(
                LIB.ctex_paint_preview_session_get_info(session, ctypes.byref(preview))
            )
            dtype = _dtype(preview.scalar_representation, preview.bit_depth)
            pixels = np.empty(
                (preview.height, preview.width, preview.component_count),
                dtype=dtype,
                order="C",
            )
            required = ctypes.c_size_t()
            check(
                LIB.ctex_paint_preview_session_get_pixels(
                    session,
                    ctypes.c_void_p(pixels.ctypes.data),
                    pixels.nbytes,
                    ctypes.byref(required),
                )
            )
            if required.value != pixels.nbytes:
                raise RuntimeError(
                    "native channel snapshot size does not match its metadata"
                )
            return pixels
        finally:
            LIB.ctex_paint_preview_session_destroy(session)
