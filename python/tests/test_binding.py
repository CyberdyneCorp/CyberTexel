from __future__ import annotations

import gc
import unittest

import cybertexel
import numpy as np

GRAY8_PNG = bytes.fromhex(
    "89504e470d0a1a0a0000000d4948445200000002000000010800000000d1492056"
    "0000000b49444154789c63107c0600010b00f81014c2310000000049454e44ae426082"
)


class BindingTest(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
