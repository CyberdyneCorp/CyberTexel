from __future__ import annotations

import ctypes
from dataclasses import dataclass
from enum import IntEnum

import numpy as np
import numpy.typing as npt

from ._native import LIB, DecodedImageInfo, ImageDecodeLimits, check


class ChannelSemantic(IntEnum):
    BASE_COLOR = 0
    OPACITY = 1
    ROUGHNESS = 2
    METALLIC = 3
    NORMAL = 4
    HEIGHT = 5
    OCCLUSION = 6
    EMISSION = 7
    SUBSURFACE = 8


class InputColorSpace(IntEnum):
    AUTOMATIC = 0
    LINEAR_REC709 = 1
    SRGB_REC709 = 2


@dataclass(frozen=True)
class DecodedImage:
    pixels: npt.NDArray[np.generic]
    color_space: int
    detected_format: int
    extension_mismatch: bool
    color_space_source: int
    uninterpretable_profile: bool


def _dtype(info: DecodedImageInfo) -> np.dtype[np.generic]:
    if info.scalar_representation == 1 and info.bit_depth == 32:
        return np.dtype(np.float32)
    if info.scalar_representation == 0 and info.bit_depth == 8:
        return np.dtype(np.uint8)
    if info.scalar_representation == 0 and info.bit_depth == 16:
        return np.dtype(np.uint16)
    raise RuntimeError(
        f"native decoder returned unsupported representation={info.scalar_representation} "
        f"bit_depth={info.bit_depth}"
    )


def decode_image(
    encoded: bytes | bytearray | memoryview,
    *,
    source_name: str = "image.bin",
    intended_channel: ChannelSemantic = ChannelSemantic.BASE_COLOR,
    color_space: InputColorSpace = InputColorSpace.AUTOMATIC,
    maximum_width: int = 16_384,
    maximum_height: int = 16_384,
    maximum_decoded_bytes: int = 1 << 30,
) -> DecodedImage:
    """Decode to a caller-owned C-contiguous NumPy array.

    The C decoder writes directly into the returned array. The sizing call
    decodes once before allocation because the stable C ABI uses a two-call
    output contract; no library-owned pixel view escapes the call.
    """

    view = memoryview(encoded).cast("B")
    storage = (ctypes.c_ubyte * len(view)).from_buffer_copy(view)
    pointer = ctypes.cast(storage, ctypes.c_void_p) if len(view) else None
    limits = ImageDecodeLimits(
        ctypes.sizeof(ImageDecodeLimits),
        maximum_width,
        maximum_height,
        maximum_decoded_bytes,
    )
    info = DecodedImageInfo()
    info.size = ctypes.sizeof(DecodedImageInfo)
    required = ctypes.c_size_t()
    arguments = (
        pointer,
        len(view),
        source_name.encode("utf-8"),
        int(intended_channel),
        int(color_space),
        ctypes.byref(limits),
        ctypes.byref(info),
    )
    check(LIB.ctex_image_decode_memory(*arguments, None, 0, ctypes.byref(required)))
    dtype = _dtype(info)
    shape = (info.height, info.width, info.channel_count)
    pixels = np.empty(shape, dtype=dtype, order="C")
    check(
        LIB.ctex_image_decode_memory(
            *arguments,
            ctypes.c_void_p(pixels.ctypes.data),
            pixels.nbytes,
            ctypes.byref(required),
        )
    )
    if required.value != pixels.nbytes:
        raise RuntimeError(
            "native decoder changed its required output size between calls"
        )
    return DecodedImage(
        pixels=pixels,
        color_space=info.color_space,
        detected_format=info.detected_format,
        extension_mismatch=bool(info.extension_mismatch),
        color_space_source=info.color_space_source,
        uninterpretable_profile=bool(info.uninterpretable_profile),
    )
