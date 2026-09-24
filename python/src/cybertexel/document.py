from __future__ import annotations

import ctypes
import json
from dataclasses import dataclass
from enum import IntEnum
from typing import TYPE_CHECKING, Sequence

if TYPE_CHECKING:
    import numpy as np
    import numpy.typing as npt

from ._native import (
    LIB,
    AtlasDescriptor,
    AtlasInfo,
    AtlasRegion,
    AtlasRegionDescriptor,
    ChannelDescriptor,
    ChannelInfo,
    ChannelRegionDescriptor,
    LayerChannelDescriptor,
    LayerEntryDescriptor,
    PaintPreviewInfo,
    ProjectContainerInfo,
    TextureSetDescriptor,
    TileHistoryCommitInfo,
    TileHistoryTargetDescriptor,
    UdimPixelWriteDescriptor,
    UdimWriteInfo,
    check,
)


@dataclass(frozen=True)
class UdimWriteReport:
    """What one absolute-UV write batch changed.

    Logical occupancy and physical memory are reported separately: a tile can be
    declared without being allocated until something is written into it.
    """

    changed_tile_count: int
    allocated_tile_count: int
    changed_pixel_count: int


@dataclass(frozen=True)
class AtlasRegionPlacement:
    """One texture set's non-overlapping place in an atlas."""

    texture_set_id: str
    x: int
    y: int
    width: int
    height: int


@dataclass(frozen=True)
class Atlas:
    """A validated grouping of texture sets into one deterministic output."""

    identifier: str
    display_name: str
    width: int
    height: int
    regions: tuple[AtlasRegionPlacement, ...]


class LayerKind(IntEnum):
    """The explicit kind a layer-stack entry declares."""

    PAINT = 0
    FILL = 1
    GROUP = 2
    MASK = 3
    FILTER = 4
    INSTANCE = 5
    EDITABLE_DECAL = 6
    EDITABLE_TEXT = 7
    SURFACE_PATH = 8


class SourceDeletionPolicy(IntEnum):
    """What removing an entry other entries reference does.

    `texture-document` makes this the caller's choice rather than a rule.
    """

    REFUSE = 0
    CONVERT_INSTANCES_TO_COPIES = 1


@dataclass(frozen=True)
class LayerChannel:
    """One channel's participation in a layer entry."""

    semantic_id: str
    enabled: bool = True
    opacity: float = 1.0


@dataclass(frozen=True)
class LayerEntry:
    """An ordered layer-stack entry.

    `parent_identifier` nests the entry in a group, `target_identifier` attaches
    a mask or filter to exactly one entry, and `source_identifier` is the entry
    an instance references.
    """

    identifier: str
    display_name: str
    kind: LayerKind = LayerKind.PAINT
    parent_identifier: str | None = None
    target_identifier: str | None = None
    source_identifier: str | None = None
    enabled: bool = True
    opacity: float = 1.0
    blend_mode: str = "normal"
    channels: tuple[LayerChannel, ...] = ()


@dataclass(frozen=True)
class TextureChannel:
    """An extensible channel descriptor.

    The nine-channel PBR preset is expressed through exactly these fields, so a
    custom semantic is not a second-class citizen.
    """

    semantic_id: str
    component_count: int
    scalar_representation: int = 0
    preferred_bit_depth: int = 8
    default_value: tuple[float, ...] = (0.0,)
    classification: int = 0
    blending_policy: int = 0
    export_mapping: str = ""
    evaluable: bool = True


def _input_buffer(value: bytes) -> ctypes.Array[ctypes.c_char]:
    return ctypes.create_string_buffer(value, len(value))


def _empty_project() -> bytes:
    required = ctypes.c_size_t()
    check(LIB.ctex_project_container_create_empty(None, 0, ctypes.byref(required)))
    output = (ctypes.c_ubyte * required.value)()
    check(
        LIB.ctex_project_container_create_empty(
            output, len(output), ctypes.byref(required)
        )
    )
    return bytes(output)


def _texture_document_ids(project: bytes) -> list[str]:
    source = _input_buffer(project)
    required = ctypes.c_size_t()
    count = ctypes.c_size_t()
    check(
        LIB.ctex_project_container_get_texture_document_ids(
            source,
            len(project),
            None,
            None,
            0,
            ctypes.byref(required),
            ctypes.byref(count),
        )
    )
    if required.value == 0:
        return []
    output = ctypes.create_string_buffer(required.value)
    check(
        LIB.ctex_project_container_get_texture_document_ids(
            source,
            len(project),
            None,
            output,
            len(output),
            ctypes.byref(required),
            ctypes.byref(count),
        )
    )
    packed = bytes(output.raw[: required.value])
    identifiers = [part.decode("utf-8") for part in packed.split(b"\0") if part]
    if len(identifiers) != count.value:
        raise RuntimeError("native texture-document inventory count is inconsistent")
    return identifiers


def _dtype(scalar_representation: int, bit_depth: int) -> np.dtype[np.generic]:
    import numpy as np

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
        self._source_project: bytes | None = None
        self._asset_identifier: str | None = None

    @classmethod
    def from_project(
        cls, project: bytes, *, asset_identifier: str | None = None
    ) -> Document:
        """Restore a live document from canonical project-container bytes."""

        canonical = bytes(project)
        identifiers = _texture_document_ids(canonical)
        if asset_identifier is None:
            if len(identifiers) != 1:
                raise ValueError(
                    "project must contain exactly one texture document when "
                    "asset_identifier is omitted"
                )
            asset_identifier = identifiers[0]
        elif asset_identifier not in identifiers:
            raise ValueError(f"texture document not found: {asset_identifier}")

        result = cls()
        source = _input_buffer(canonical)
        try:
            check(
                LIB.ctex_project_container_restore_texture_document(
                    source,
                    len(canonical),
                    None,
                    asset_identifier.encode("utf-8"),
                    result._require_open(),
                )
            )
        except Exception:
            result.close()
            raise
        result._source_project = canonical
        result._asset_identifier = asset_identifier
        return result

    @property
    def asset_identifier(self) -> str | None:
        return self._asset_identifier

    def to_project_bytes(
        self,
        *,
        project: bytes | None = None,
        asset_identifier: str | None = None,
    ) -> bytes:
        """Return canonical project bytes containing the current live state."""

        if project is not None:
            source_project = bytes(project)
        elif self._source_project is not None:
            source_project = self._source_project
        else:
            source_project = _empty_project()
        identifier = (
            asset_identifier
            if asset_identifier is not None
            else self._asset_identifier or "document/main"
        )
        source = _input_buffer(source_project)
        info = ProjectContainerInfo()
        info.size = ctypes.sizeof(ProjectContainerInfo)
        arguments = (
            source,
            len(source_project),
            None,
            self._require_open(),
            identifier.encode("utf-8"),
            ctypes.byref(info),
        )
        check(
            LIB.ctex_project_container_upsert_texture_document(
                *arguments, None, 0, None, 0
            )
        )
        output = (ctypes.c_ubyte * info.canonical_size)()
        report = ctypes.create_string_buffer(info.report_size)
        check(
            LIB.ctex_project_container_upsert_texture_document(
                *arguments, output, len(output), report, len(report)
            )
        )
        return bytes(output)

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
        udim: bool = False,
    ) -> TextureSet:
        """Create a texture set bound to a mesh partition and named UV set.

        `udim` opts into sparse UDIM storage, where tiles are addressed by the
        standard 1001-based numbering and allocated on demand.
        """

        descriptor = TextureSetDescriptor(
            ctypes.sizeof(TextureSetDescriptor),
            display_name.encode("utf-8"),
            0,
            partition_key.encode("utf-8"),
            uv_set.encode("utf-8"),
            width,
            height,
            default_bit_depth,
            1 if udim else 0,
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

        import numpy as np

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

    def configure_tile_history(self, texture_set: TextureSet, budget_bytes: int) -> None:
        """Set the texture set's undo history byte ceiling before pixel edits."""
        if budget_bytes < 0:
            raise ValueError("history budget must be nonnegative")
        check(LIB.ctex_texture_set_configure_tile_history(
            self._require_open(), texture_set.identifier.encode("utf-8"), budget_bytes))

    def write_channel(
        self,
        texture_set: TextureSet,
        semantic_id: str,
        pixels: npt.NDArray[np.generic],
        *,
        x: int = 0,
        y: int = 0,
        step_identifier: str = "write-channel",
    ) -> None:
        """Commit a contiguous NumPy channel rectangle as one undoable step."""

        import numpy as np

        if not isinstance(pixels, np.ndarray) or pixels.ndim != 3 or not pixels.flags.c_contiguous:
            raise ValueError("pixels must be a contiguous (height, width, components) NumPy array")
        height, width, components = pixels.shape
        if width == 0 or height == 0 or x < 0 or y < 0 or x + width > texture_set.width or y + height > texture_set.height:
            raise ValueError("channel region is outside the texture set")
        semantic = semantic_id.encode("utf-8")
        identifier = texture_set.identifier.encode("utf-8")
        channel = ChannelInfo()
        channel.size = ctypes.sizeof(ChannelInfo)
        required = ctypes.c_size_t()
        check(LIB.ctex_texture_set_get_channel_info(
            self._require_open(), identifier, semantic, ctypes.byref(channel),
            None, 0, ctypes.byref(required)))
        if not channel.enabled or components != channel.component_count or pixels.dtype != _dtype(channel.scalar_representation, channel.storage_bit_depth):
            raise ValueError("array shape or dtype does not match the enabled channel format")
        tile_size = 64
        coordinates = [(tx, ty)
                       for ty in range(y // tile_size, (y + height - 1) // tile_size + 1)
                       for tx in range(x // tile_size, (x + width - 1) // tile_size + 1)]
        targets = (TileHistoryTargetDescriptor * len(coordinates))(*[
            TileHistoryTargetDescriptor(ctypes.sizeof(TileHistoryTargetDescriptor), semantic, tx, ty)
            for tx, ty in coordinates
        ])
        region = ChannelRegionDescriptor(
            ctypes.sizeof(ChannelRegionDescriptor), x, y, width, height, pixels.strides[0])
        snapshot = ctypes.c_void_p()
        transaction = ctypes.c_void_p()
        check(LIB.ctex_layer_snapshot_create(texture_set.width, texture_set.height,
                                             None, 0, None, 0, ctypes.byref(snapshot)))
        try:
            check(LIB.ctex_texture_set_begin_transaction(
                self._require_open(), identifier, step_identifier.encode("utf-8"),
                targets, len(targets), snapshot, ctypes.byref(transaction)))
            try:
                check(LIB.ctex_texture_set_transaction_write_region(
                    transaction, semantic, ctypes.byref(region),
                    ctypes.c_void_p(pixels.ctypes.data), pixels.nbytes))
                commit = TileHistoryCommitInfo()
                commit.size = ctypes.sizeof(TileHistoryCommitInfo)
                check(LIB.ctex_texture_set_transaction_commit(transaction, ctypes.byref(commit)))
            finally:
                LIB.ctex_texture_set_transaction_destroy(transaction)
        finally:
            LIB.ctex_layer_snapshot_destroy(snapshot)

    def write_channel_pixel(
        self,
        texture_set: TextureSet,
        semantic_id: str,
        x: int,
        y: int,
        pixel: bytes,
    ) -> None:
        """Commit one authored pixel, primarily for examples and host synchronization."""

        if not (0 <= x < texture_set.width and 0 <= y < texture_set.height):
            raise ValueError("pixel coordinate is outside the texture set")
        session = ctypes.c_void_p()
        check(
            LIB.ctex_paint_preview_session_create(
                self._require_open(),
                texture_set.identifier.encode("utf-8"),
                semantic_id.encode("utf-8"),
                ctypes.byref(session),
            )
        )
        try:
            value = (ctypes.c_uint8 * len(pixel)).from_buffer_copy(pixel)
            check(
                LIB.ctex_paint_preview_session_write_pixel(
                    session, x, y, value, len(pixel)
                )
            )
            coverage = (ctypes.c_uint8 * (texture_set.width * texture_set.height))()
            coverage[(y * texture_set.width) + x] = 1
            info = PaintPreviewInfo()
            info.size = ctypes.sizeof(PaintPreviewInfo)
            check(
                LIB.ctex_paint_preview_session_finalize(
                    session, coverage, len(coverage), 0, ctypes.byref(info)
                )
            )
            check(LIB.ctex_paint_preview_session_commit(session, ctypes.byref(info)))
        finally:
            LIB.ctex_paint_preview_session_destroy(session)

    def register_channel(self, texture_set: TextureSet, channel: TextureChannel) -> None:
        """Register an extensible channel semantic on a texture set.

        The built-in PBR preset uses the same descriptor, so a custom semantic
        carries the same precision, classification and export policy.
        """

        if not 1 <= channel.component_count <= 4:
            raise ValueError("a channel carries one to four components")
        if len(channel.default_value) > 4:
            raise ValueError("a channel default carries at most four values")
        descriptor = ChannelDescriptor()
        descriptor.size = ctypes.sizeof(ChannelDescriptor)
        descriptor.semantic_id = channel.semantic_id.encode("utf-8")
        descriptor.component_count = channel.component_count
        descriptor.scalar_representation = channel.scalar_representation
        descriptor.preferred_bit_depth = channel.preferred_bit_depth
        for index, value in enumerate(channel.default_value):
            descriptor.default_value[index] = value
        descriptor.default_value_count = len(channel.default_value)
        descriptor.classification = channel.classification
        descriptor.blending_policy = channel.blending_policy
        descriptor.export_mapping = channel.export_mapping.encode("utf-8")
        descriptor.evaluable = 1 if channel.evaluable else 0
        check(
            LIB.ctex_texture_set_register_channel(
                self._require_open(),
                texture_set.identifier.encode("utf-8"),
                ctypes.byref(descriptor),
            )
        )

    def channel_ids(self, texture_set: TextureSet) -> list[str]:
        """Every channel semantic the texture set carries, in canonical order."""

        handle = self._require_open()
        identifier = texture_set.identifier.encode("utf-8")
        required = ctypes.c_size_t()
        count = ctypes.c_size_t()
        check(
            LIB.ctex_texture_set_get_channel_ids(
                handle, identifier, None, 0, ctypes.byref(required), ctypes.byref(count)
            )
        )
        if required.value == 0:
            return []
        buffer = ctypes.create_string_buffer(required.value)
        check(
            LIB.ctex_texture_set_get_channel_ids(
                handle,
                identifier,
                buffer,
                required.value,
                ctypes.byref(required),
                ctypes.byref(count),
            )
        )
        names = buffer.raw[: required.value].split(b"\0")
        return [name.decode("utf-8") for name in names if name][: count.value]

    def append_layers(
        self, texture_set: TextureSet, entries: Sequence[LayerEntry]
    ) -> None:
        """Append a complete ordered batch, validated atomically.

        The whole batch is validated against the resulting stack before any of
        it is published, so a batch that would produce an invalid stack changes
        nothing.
        """

        if not entries:
            raise ValueError("a layer batch carries at least one entry")
        # Keep every encoded string and channel array alive until the call ends.
        retained: list[object] = []
        descriptors = (LayerEntryDescriptor * len(entries))()

        def text(value: str | None) -> bytes | None:
            if value is None:
                return None
            encoded = value.encode("utf-8")
            retained.append(encoded)
            return encoded

        for index, entry in enumerate(entries):
            channels = (LayerChannelDescriptor * len(entry.channels))()
            for slot, channel in enumerate(entry.channels):
                channels[slot].size = ctypes.sizeof(LayerChannelDescriptor)
                channels[slot].semantic_id = text(channel.semantic_id)
                channels[slot].enabled = 1 if channel.enabled else 0
                channels[slot].opacity = channel.opacity
            retained.append(channels)
            descriptor = descriptors[index]
            descriptor.size = ctypes.sizeof(LayerEntryDescriptor)
            descriptor.identifier = text(entry.identifier)
            descriptor.display_name = text(entry.display_name)
            descriptor.kind = int(entry.kind)
            descriptor.parent_identifier = text(entry.parent_identifier)
            descriptor.target_identifier = text(entry.target_identifier)
            descriptor.source_identifier = text(entry.source_identifier)
            descriptor.enabled = 1 if entry.enabled else 0
            descriptor.opacity = entry.opacity
            descriptor.blend_mode = text(entry.blend_mode)
            descriptor.channels = channels if entry.channels else None
            descriptor.channel_count = len(entry.channels)
        check(
            LIB.ctex_texture_set_layer_append(
                self._require_open(),
                texture_set.identifier.encode("utf-8"),
                descriptors,
                len(entries),
            )
        )

    def inspect_layers(self, texture_set: TextureSet) -> dict[str, object]:
        """The canonical stack snapshot, including its revision and entry kinds."""

        handle = self._require_open()
        identifier = texture_set.identifier.encode("utf-8")
        required = ctypes.c_size_t()
        check(
            LIB.ctex_texture_set_layer_inspect(
                handle, identifier, None, 0, ctypes.byref(required)
            )
        )
        buffer = ctypes.create_string_buffer(required.value)
        check(
            LIB.ctex_texture_set_layer_inspect(
                handle, identifier, buffer, required.value, ctypes.byref(required)
            )
        )
        return json.loads(buffer.value.decode("utf-8"))

    def set_layer_state(
        self,
        texture_set: TextureSet,
        entry_identifier: str,
        *,
        display_name: str,
        enabled: bool,
        opacity: float,
        blend_mode: str,
    ) -> None:
        """Edit one entry's display name, visibility, opacity and blend mode."""

        check(
            LIB.ctex_texture_set_layer_set_state(
                self._require_open(),
                texture_set.identifier.encode("utf-8"),
                entry_identifier.encode("utf-8"),
                display_name.encode("utf-8"),
                1 if enabled else 0,
                opacity,
                blend_mode.encode("utf-8"),
            )
        )

    def remove_layers(
        self,
        texture_set: TextureSet,
        entry_identifiers: Sequence[str],
        *,
        source_deletion_policy: SourceDeletionPolicy = SourceDeletionPolicy.REFUSE,
    ) -> None:
        """Remove entries, choosing what happens to instances that reference them."""

        if not entry_identifiers:
            raise ValueError("removal names at least one entry")
        encoded = [value.encode("utf-8") for value in entry_identifiers]
        array = (ctypes.c_char_p * len(encoded))(*encoded)
        check(
            LIB.ctex_texture_set_layer_remove(
                self._require_open(),
                texture_set.identifier.encode("utf-8"),
                array,
                len(encoded),
                int(source_deletion_policy),
            )
        )

    def ensure_udim_tiles(
        self, texture_set: TextureSet, tile_numbers: Sequence[int]
    ) -> int:
        """Declare standard UDIM tiles and return how many were newly allocated.

        Declaring a tile makes it logically occupied; physical storage follows
        on demand, so the returned count is allocation, not occupancy.
        """

        if not tile_numbers:
            raise ValueError("declare at least one UDIM tile")
        numbers = (ctypes.c_uint32 * len(tile_numbers))(*tile_numbers)
        allocated = ctypes.c_size_t()
        check(
            LIB.ctex_texture_set_ensure_udim_tiles(
                self._require_open(),
                texture_set.identifier.encode("utf-8"),
                numbers,
                len(tile_numbers),
                ctypes.byref(allocated),
            )
        )
        return allocated.value

    def udim_tiles(self, texture_set: TextureSet) -> list[int]:
        """Every occupied UDIM tile number, in ascending order."""

        handle = self._require_open()
        identifier = texture_set.identifier.encode("utf-8")
        count = ctypes.c_size_t()
        check(
            LIB.ctex_texture_set_get_udim_tiles(
                handle, identifier, None, 0, ctypes.byref(count)
            )
        )
        if count.value == 0:
            return []
        numbers = (ctypes.c_uint32 * count.value)()
        check(
            LIB.ctex_texture_set_get_udim_tiles(
                handle, identifier, numbers, count.value, ctypes.byref(count)
            )
        )
        return list(numbers[: count.value])

    def write_udim_pixels(
        self,
        texture_set: TextureSet,
        semantic_id: str,
        writes: Sequence[tuple[float, float, bytes]],
    ) -> UdimWriteReport:
        """Write pixels addressed in absolute UV, which may cross tile borders.

        Each write is `(u, v, pixel)`. The batch is one operation, so writes
        that land in several tiles are still reported together.
        """

        if not writes:
            raise ValueError("a UDIM write batch carries at least one pixel")
        descriptors = (UdimPixelWriteDescriptor * len(writes))()
        retained: list[object] = []
        for index, (u, v, pixel) in enumerate(writes):
            buffer = ctypes.create_string_buffer(pixel, len(pixel))
            retained.append(buffer)
            descriptors[index].size = ctypes.sizeof(UdimPixelWriteDescriptor)
            descriptors[index].u = u
            descriptors[index].v = v
            descriptors[index].pixel = ctypes.cast(buffer, ctypes.c_void_p)
            descriptors[index].pixel_size = len(pixel)
        info = UdimWriteInfo()
        info.size = ctypes.sizeof(UdimWriteInfo)
        check(
            LIB.ctex_texture_set_write_udim_pixels(
                self._require_open(),
                texture_set.identifier.encode("utf-8"),
                semantic_id.encode("utf-8"),
                descriptors,
                len(writes),
                ctypes.byref(info),
            )
        )
        return UdimWriteReport(
            changed_tile_count=info.changed_tile_count,
            allocated_tile_count=info.allocated_tile_count,
            changed_pixel_count=info.changed_pixel_count,
        )

    def read_udim_pixel(
        self,
        texture_set: TextureSet,
        semantic_id: str,
        tile_number: int,
        x: int,
        y: int,
    ) -> bytes:
        """Read one texel from an occupied UDIM tile by checked tile number."""

        handle = self._require_open()
        identifier = texture_set.identifier.encode("utf-8")
        semantic = semantic_id.encode("utf-8")
        required = ctypes.c_size_t()
        check(
            LIB.ctex_texture_set_read_udim_pixel(
                handle, identifier, semantic, tile_number, x, y, None, 0,
                ctypes.byref(required),
            )
        )
        pixel = ctypes.create_string_buffer(required.value)
        check(
            LIB.ctex_texture_set_read_udim_pixel(
                handle, identifier, semantic, tile_number, x, y, pixel,
                required.value, ctypes.byref(required),
            )
        )
        return pixel.raw[: required.value]

    def create_atlas(
        self,
        identifier: str,
        display_name: str,
        *,
        width: int,
        height: int,
        regions: Sequence[AtlasRegionPlacement],
    ) -> None:
        """Group existing texture sets into validated, non-overlapping regions."""

        if not regions:
            raise ValueError("an atlas carries at least one region")
        retained: list[object] = []
        native = (AtlasRegionDescriptor * len(regions))()
        for index, region in enumerate(regions):
            encoded = region.texture_set_id.encode("utf-8")
            retained.append(encoded)
            native[index].size = ctypes.sizeof(AtlasRegionDescriptor)
            native[index].texture_set_id = encoded
            native[index].x = region.x
            native[index].y = region.y
            native[index].width = region.width
            native[index].height = region.height
        descriptor = AtlasDescriptor()
        descriptor.size = ctypes.sizeof(AtlasDescriptor)
        descriptor.identifier = identifier.encode("utf-8")
        descriptor.display_name = display_name.encode("utf-8")
        descriptor.width = width
        descriptor.height = height
        descriptor.regions = native
        descriptor.region_count = len(regions)
        check(
            LIB.ctex_document_create_atlas(self._require_open(), ctypes.byref(descriptor))
        )

    def atlas_ids(self) -> list[str]:
        """Every atlas identifier the document carries."""

        handle = self._require_open()
        required = ctypes.c_size_t()
        count = ctypes.c_size_t()
        check(
            LIB.ctex_document_get_atlas_ids(
                handle, None, 0, ctypes.byref(required), ctypes.byref(count)
            )
        )
        if required.value == 0:
            return []
        buffer = ctypes.create_string_buffer(required.value)
        check(
            LIB.ctex_document_get_atlas_ids(
                handle, buffer, required.value, ctypes.byref(required),
                ctypes.byref(count),
            )
        )
        names = buffer.raw[: required.value].split(b"\0")
        return [name.decode("utf-8") for name in names if name][: count.value]

    def atlas(self, identifier: str) -> Atlas:
        """Read one atlas, its extent and its ordered non-overlapping regions."""

        handle = self._require_open()
        atlas_id = identifier.encode("utf-8")
        info = AtlasInfo()
        info.size = ctypes.sizeof(AtlasInfo)
        check(LIB.ctex_document_get_atlas(handle, atlas_id, ctypes.byref(info), None, 0, None, 0, None, 0))
        regions = (AtlasRegion * info.region_count)()
        display = ctypes.create_string_buffer(info.required_display_name_size)
        identifiers = ctypes.create_string_buffer(info.required_texture_set_id_size)
        check(
            LIB.ctex_document_get_atlas(
                handle,
                atlas_id,
                ctypes.byref(info),
                regions,
                info.region_count,
                display,
                len(display),
                identifiers,
                len(identifiers),
            )
        )
        raw = identifiers.raw
        return Atlas(
            identifier=identifier,
            display_name=display.value.decode("utf-8"),
            width=info.width,
            height=info.height,
            regions=tuple(
                AtlasRegionPlacement(
                    texture_set_id=raw[
                        region.texture_set_id_offset : region.texture_set_id_offset
                        + region.texture_set_id_size
                    ]
                    .split(b"\0")[0]
                    .decode("utf-8"),
                    x=region.x,
                    y=region.y,
                    width=region.width,
                    height=region.height,
                )
                for region in regions
            ),
        )
