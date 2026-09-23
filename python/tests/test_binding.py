from __future__ import annotations

import gc
import unittest

import cybertexel
import numpy as np
from cybertexel import _native

GRAY8_PNG = bytes.fromhex(
    "89504e470d0a1a0a0000000d4948445200000002000000010800000000d1492056"
    "0000000b49444154789c63107c0600010b00f81014c2310000000049454e44ae426082"
)


class BindingTest(unittest.TestCase):
    def test_incompatible_native_abi_names_both_versions(self) -> None:
        incompatible = _native.Version(7, 2, 1, b"7.2.1-test")
        with self.assertRaisesRegex(
            ImportError, "expects ABI major 0, but loaded native 7.2.1-test"
        ):
            _native._validate_abi(incompatible)

    def test_raw_capi_exposes_the_complete_generated_surface(self) -> None:
        version = cybertexel.capi.ctex_get_version()
        required = cybertexel.capi.c_size_t()
        operations = {
            name
            for name, value in vars(cybertexel.capi).items()
            if name.startswith("ctex_")
            and hasattr(value, "argtypes")
            and not isinstance(value, type)
        }
        result = cybertexel.capi.ctex_project_container_create_empty(
            None, 0, cybertexel.capi.byref(required)
        )
        self.assertEqual(
            (version.major, version.minor, version.patch),
            tuple(map(int, cybertexel.native_version().split("."))),
        )
        self.assertEqual(result, cybertexel.capi.CTEX_RESULT_SUCCESS)
        self.assertGreater(required.value, 40)
        self.assertEqual(len(operations), 353)
        self.assertIn("ctex_image_decode_layered_memory", operations)

    def test_version_and_numpy_decode(self) -> None:
        decoded = cybertexel.decode_image(
            GRAY8_PNG,
            source_name="brush.jpg",
            intended_channel=cybertexel.ChannelSemantic.ROUGHNESS,
        )
        self.assertEqual(cybertexel.native_version(), cybertexel.__version__)
        self.assertEqual(decoded.pixels.shape, (1, 2, 1))
        self.assertEqual(decoded.pixels.dtype, np.uint8)
        self.assertTrue(decoded.pixels.flags.c_contiguous)
        np.testing.assert_array_equal(decoded.pixels[:, :, 0], [[17, 230]])
        self.assertTrue(decoded.extension_mismatch)

    def test_numpy_encode_round_trip_owns_its_result(self) -> None:
        pixels = np.array(
            [[[255, 0, 0], [0, 255, 0]], [[0, 0, 255], [255, 255, 255]]],
            dtype=np.uint8,
        )
        encoded = cybertexel.encode_image(pixels)
        pixels.fill(0)
        decoded = cybertexel.decode_image(encoded, source_name="result.png")
        self.assertTrue(encoded.startswith(b"\x89PNG\r\n\x1a\n"))
        np.testing.assert_array_equal(
            decoded.pixels,
            [[[255, 0, 0], [0, 255, 0]], [[0, 0, 255], [255, 255, 255]]],
        )

    def test_numpy_encode_rejects_unsupported_shapes_and_types(self) -> None:
        with self.assertRaisesRegex(ValueError, "between one and four channels"):
            cybertexel.encode_image(np.zeros((2, 2, 5), dtype=np.uint8))
        with self.assertRaisesRegex(TypeError, "uint8, uint16 or float32"):
            cybertexel.encode_image(np.zeros((2, 2, 3), dtype=np.float64))

    def test_document_channel_snapshot_is_numpy_owned(self) -> None:
        document = cybertexel.Document()
        texture_set = document.create_texture_set(
            "Body", partition_key="body", width=8, height=4
        )
        document.set_channel_enabled(texture_set, "pbr.base_color")
        pixels = document.read_channel(texture_set, "pbr.base_color")
        self.assertEqual(pixels.shape, (4, 8, 3))
        self.assertEqual(pixels.dtype, np.uint8)
        self.assertTrue(pixels.flags.owndata)
        document.close()
        document.close()
        gc.collect()
        self.assertEqual(pixels.shape, (4, 8, 3))

    def test_document_round_trips_through_project_bytes(self) -> None:
        with cybertexel.Document() as document:
            texture_set = document.create_texture_set(
                "Body", partition_key="body", width=8, height=4
            )
            document.set_channel_enabled(texture_set, "pbr.base_color")
            project = document.to_project_bytes(asset_identifier="documents/body")

        with cybertexel.Document.from_project(project) as restored:
            self.assertEqual(restored.asset_identifier, "documents/body")
            self.assertEqual(restored.texture_set_ids(), [texture_set.identifier])
            restored.set_channel_enabled(texture_set.identifier, "pbr.roughness")
            updated = restored.to_project_bytes()

        with cybertexel.Document.from_project(updated) as reopened:
            self.assertEqual(reopened.texture_set_ids(), [texture_set.identifier])

        with cybertexel.Document.from_project(project) as first:
            multiple = first.to_project_bytes(
                project=project, asset_identifier="documents/copy"
            )
        with self.assertRaisesRegex(ValueError, "exactly one texture document"):
            cybertexel.Document.from_project(multiple)
        with cybertexel.Document.from_project(
            multiple, asset_identifier="documents/copy"
        ) as selected:
            self.assertEqual(selected.asset_identifier, "documents/copy")

    def test_native_failures_raise_typed_exceptions(self) -> None:
        with (
            cybertexel.Document() as document,
            self.assertRaises(cybertexel.MissingResourceError) as raised,
        ):
            document.set_channel_enabled("missing-set", "pbr.base_color")
        self.assertEqual(raised.exception.result, cybertexel.Result.MISSING_RESOURCE)
        self.assertIn("missing-set", raised.exception.diagnostic)
        self.assertGreater(raised.exception.diagnostic_code, 0)

    def test_mesh_and_map_numpy_inputs_cross_the_c_boundary(self) -> None:
        positions = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=np.float32)
        normals = np.array([[0, 0, 1], [0, 0, 1], [0, 0, 1]], dtype=np.float32)
        uv = np.array([[0, 0], [1, 0], [0, 1]], dtype=np.float32)
        triangles = np.array([[0, 1, 2]], dtype=np.uint32)
        ambient_occlusion = np.array([[1, 2], [3, 4]], dtype=np.uint8)
        with (
            cybertexel.Document() as document,
            cybertexel.Mesh(positions, triangles, normals=normals, uv=uv) as mesh,
        ):
            texture_set = document.create_texture_set(
                "Body", partition_key="body", width=4, height=4
            )
            with cybertexel.MeshMapSet(document, texture_set, mesh) as maps:
                report = maps.import_map(
                    cybertexel.MeshMapKind.AMBIENT_OCCLUSION, ambient_occlusion
                )
                mask = maps.generate_mask(
                    cybertexel.MeshMapGeneratorKind.AMBIENT_OCCLUSION, 4, 4
                )
        self.assertEqual((report.map_width, report.map_height), (2, 2))
        self.assertTrue(report.resolution_mismatch)
        self.assertEqual(mask.shape, (4, 4))
        self.assertEqual(mask.dtype, np.float32)
        self.assertTrue(mask.flags.owndata)
        np.testing.assert_array_equal(ambient_occlusion, [[1, 2], [3, 4]])

    def test_missing_generator_map_raises_typed_exception(self) -> None:
        positions = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=np.float32)
        normals = np.array([[0, 0, 1], [0, 0, 1], [0, 0, 1]], dtype=np.float32)
        uv = np.array([[0, 0], [1, 0], [0, 1]], dtype=np.float32)
        triangles = np.array([[0, 1, 2]], dtype=np.uint32)
        with (
            cybertexel.Document() as document,
            cybertexel.Mesh(positions, triangles, normals=normals, uv=uv) as mesh,
        ):
            texture_set = document.create_texture_set(
                "Body", partition_key="body", width=4, height=4
            )
            with (
                cybertexel.MeshMapSet(document, texture_set, mesh) as maps,
                self.assertRaises(cybertexel.MissingResourceError) as raised,
            ):
                maps.generate_mask(cybertexel.MeshMapGeneratorKind.SCRATCHES, 4, 4)
        self.assertEqual(raised.exception.result, cybertexel.Result.MISSING_RESOURCE)
        self.assertIn("requires missing map", raised.exception.diagnostic)
        self.assertIn("position", raised.exception.diagnostic)

    def test_uv_picking_returns_typed_hit_and_distinct_miss(self) -> None:
        positions = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=np.float32)
        normals = np.array([[0, 0, 1], [0, 0, 1], [0, 0, 1]], dtype=np.float32)
        uv = np.array([[0, 0], [1, 0], [0, 1]], dtype=np.float32)
        triangles = np.array([[0, 1, 2]], dtype=np.uint32)
        with cybertexel.Mesh(
            positions,
            triangles,
            normals=normals,
            uv=uv,
            partition_key="body",
        ) as mesh:
            hit = mesh.pick_uv(0.25, 0.25)
            miss = mesh.pick_uv(2.0, 2.0)
        self.assertIsInstance(hit, cybertexel.PickHit)
        assert hit is not None
        np.testing.assert_allclose(hit.position, [0.25, 0.25, 0.0])
        np.testing.assert_allclose(hit.barycentric, [0.5, 0.25, 0.25])
        self.assertEqual(hit.triangle_index, 0)
        self.assertEqual(hit.texture_set_id, "material/4:body/uv/3:uv0")
        self.assertIsNone(miss)

    def test_host_execution_retains_resident_result_without_readback(self) -> None:
        program = cybertexel.emit_default_host_material(
            stable_identity="binding/paint",
            output_identity="paint",
            width=64,
            height=32,
        )
        self.assertIn(b"@vertex", program.vertex_artifact)
        self.assertIn('"logical_id":"paint"', program.pass_plan)
        source = cybertexel.HostResource(
            "source",
            1,
            "input",
            2,
            64,
            32,
            False,
        )
        output = cybertexel.HostResource(
            "paint",
            1,
            "output",
            2,
            64,
            32,
            True,
            externally_initialized=False,
            required_state=cybertexel.ResourceState.RENDER_TARGET,
        )
        with cybertexel.HostExecutionSession() as session:
            token = session.submit("paint", 0, [source, output])
            result = session.complete(
                token,
                [cybertexel.CompletedResource("paint", 1, 2, 64, 32)],
                cybertexel.Recovery("paint-v1", 0, 96),
            )
            self.assertEqual(
                result.disposition, cybertexel.CompletionDisposition.PUBLISHED
            )
            self.assertEqual(result.published_revision, 1)
            self.assertEqual(session.committed_generation("paint"), 1)
            self.assertTrue(session.resource_is_held("paint", 1))

    def test_explicit_host_readback_publishes_only_after_completion(self) -> None:
        with cybertexel.Document() as document:
            texture_set = document.create_texture_set(
                "Readback", partition_key="readback", width=8, height=4
            )
            document.set_channel_enabled(texture_set, "pbr.base_color")
            with cybertexel.SnapshotPool(64 * 64 * 3) as pool:
                cursor = pool.current_cursor(document, texture_set, "pbr.base_color")
                document.write_channel_pixel(
                    texture_set, "pbr.base_color", 1, 2, bytes([7, 11, 13])
                )
                snapshot = pool.snapshot(
                    document, texture_set, "pbr.base_color", cursor
                )
                readback = snapshot.begin_host_readback()
                self.assertEqual(readback.status, cybertexel.ReadbackStatus.PENDING)
                with self.assertRaises(RuntimeError):
                    _ = readback.tiles
                payloads = tuple(
                    bytes([42]) * size for size in readback.tile_byte_sizes
                )
                readback.complete(payloads)
                self.assertEqual(readback.status, cybertexel.ReadbackStatus.COMPLETE)
                self.assertEqual(readback.tiles, payloads)
                snapshot.close()
                readback.close()


if __name__ == "__main__":
    unittest.main()
