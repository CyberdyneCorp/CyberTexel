//! Read-only mesh values.
//!
//! Plain owned Rust data. The audited FFI boundary copies it into the native
//! mesh during construction; the library never aliases these buffers.

/// A validated read-only mesh with one named UV set and one partition.
#[derive(Clone, Debug, PartialEq)]
pub struct MeshData {
    pub positions: Vec<[f32; 3]>,
    pub normals: Vec<[f32; 3]>,
    pub uv: Vec<[f32; 2]>,
    pub triangle_indices: Vec<u32>,
    pub uv_set_name: String,
    pub partition_key: String,
    pub partition_name: String,
}

impl MeshData {
    /// A UV-unwrapped grid carrying at least `triangles` triangles.
    ///
    /// `device-gate` configurations name a mesh by triangle count, so a host can
    /// build the declared configuration without shipping a fixture asset.
    pub fn grid(triangles: usize) -> Self {
        let side = ((triangles as f64 / 2.0).sqrt().ceil() as usize).max(1);
        let stride = side + 1;
        let mut positions = Vec::with_capacity(stride * stride);
        let mut normals = Vec::with_capacity(stride * stride);
        let mut uv = Vec::with_capacity(stride * stride);
        for row in 0..stride {
            for column in 0..stride {
                let u = column as f32 / side as f32;
                let v = row as f32 / side as f32;
                positions.push([u, v, 0.0]);
                normals.push([0.0, 0.0, 1.0]);
                uv.push([u, v]);
            }
        }
        let mut triangle_indices = Vec::with_capacity(side * side * 6);
        for row in 0..side {
            for column in 0..side {
                let a = (row * stride + column) as u32;
                let b = a + 1;
                let c = ((row + 1) * stride + column) as u32 + 1;
                let d = ((row + 1) * stride + column) as u32;
                triangle_indices.extend_from_slice(&[a, b, c, a, c, d]);
            }
        }
        Self {
            positions,
            normals,
            uv,
            triangle_indices,
            uv_set_name: "uv0".to_string(),
            partition_key: "mesh".to_string(),
            partition_name: "Mesh".to_string(),
        }
    }

    #[must_use]
    pub fn triangle_count(&self) -> usize {
        self.triangle_indices.len() / 3
    }
}
