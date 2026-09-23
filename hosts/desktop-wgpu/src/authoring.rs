//! A bounded authored edit, built on the stable C ABI's paint-work surface.
//!
//! The latency route needs an authored edit whose cost is representative of
//! painting. `Document::write_channel_pixel` is not one: it is a convenience
//! that allocates a whole-canvas coverage buffer and finalizes the preview by
//! decoding, seam-dilating and re-encoding every texel on the canvas. At the
//! interactive-4k configuration that costs about 360 ms for a single texel —
//! a thousand times the frame it would have to fit inside.
//!
//! The paint path's real shape is bounded, and the C ABI already exposes it:
//!
//!   1. `ctex_paint_plan_work` reports exactly the storage tiles a stamp
//!      footprint can reach, dilation included, without visiting the canvas.
//!   2. `ctex_texture_set_begin_transaction` declares that tile set *before*
//!      the edit, so tile history retains exactly those tiles and a write
//!      outside them is refused rather than silently widening the step.
//!   3. `ctex_texture_set_transaction_commit` publishes the step.
//!
//! The safe crate does not wrap that surface yet, so this module holds the
//! unsafe boundary. Every call is a plain C call with borrowed pointers that
//! outlive it; no pointer returned by the library is retained beyond the
//! handle types below, and both handles are destroyed in `Drop`.

use std::collections::{BTreeMap, BTreeSet};
use std::ffi::{CStr, CString};
use std::ptr;

use cybertexel::MeshData;
use cybertexel_sys as sys;

/// The storage-tile edge the library plans and retains history in.
pub const TILE_EXTENT: u32 = 64;
/// Bytes per texel in the device texture the host uploads into.
pub const DEVICE_TEXEL_BYTES: usize = 4;
/// Bytes per texel in the channel's authored 8-bit storage.
const AUTHORED_TEXEL_BYTES: usize = 3;
/// The canonical seam-dilation radius `ctex_paint_work_init` declares.
const DILATION_RADIUS: u32 = 2;

/// What one authored stamp published.
#[derive(Clone, Debug)]
pub struct Stamp {
    /// The storage tiles the declared write set covered.
    pub declared_tiles: Vec<(u32, u32)>,
    /// The storage tiles the stamp actually wrote texels into.
    pub changed_tiles: Vec<(u32, u32)>,
    /// Texels the stamp actually authored.
    pub texels: usize,
    /// Tiles the commit retained in history.
    pub committed_tiles: usize,
    /// Bytes tile history holds after the commit.
    pub retained_bytes: usize,
}

/// A document carrying one texture set at the measured configuration.
pub struct Canvas {
    document: *mut sys::ctex_document,
    snapshot: *mut sys::ctex_layer_snapshot,
    texture_set: CString,
    semantic: CString,
    extent: u32,
    /// The host's own copy of the tiles it has authored, in the device
    /// texture's format. A host owns its device texture, so it uploads the
    /// texels it wrote rather than reading them back from anywhere.
    mirror: BTreeMap<(u32, u32), Vec<u8>>,
}

/// Read the thread-local diagnostic the failing call left behind.
fn diagnostic(action: &str) -> String {
    // SAFETY: the pointer is owned by the library and valid until the next
    // fallible call on this thread; it is copied before any other call.
    let message = unsafe {
        let text = sys::ctex_get_last_diagnostic();
        if text.is_null() {
            String::new()
        } else {
            CStr::from_ptr(text).to_string_lossy().into_owned()
        }
    };
    if message.is_empty() {
        format!("{action} failed")
    } else {
        format!("{action} failed: {message}")
    }
}

fn check(result: sys::ctex_result, action: &str) -> Result<(), String> {
    if result == sys::CTEX_RESULT_SUCCESS {
        Ok(())
    } else {
        Err(diagnostic(action))
    }
}

fn text(value: &str) -> Result<CString, String> {
    CString::new(value).map_err(|_| format!("{value} carries an interior NUL"))
}

/// The native mesh, owned for exactly as long as the texture sets need it.
struct Mesh(*mut sys::ctex_mesh);

impl Mesh {
    fn create(data: &MeshData) -> Result<Self, String> {
        let uv_name = text(&data.uv_set_name)?;
        let partition_key = text(&data.partition_key)?;
        let partition_name = text(&data.partition_name)?;
        let faces = data.triangle_indices.len() / 3;
        let face_partitions = vec![0_u32; faces];
        let face_materials = vec![0_u32; faces];
        let uv_set = sys::ctex_uv_set_descriptor {
            size: size_of::<sys::ctex_uv_set_descriptor>() as u32,
            name: uv_name.as_ptr(),
            values: data.uv.as_ptr().cast(),
            value_count: data.uv.len(),
        };
        let partition = sys::ctex_mesh_partition_descriptor {
            size: size_of::<sys::ctex_mesh_partition_descriptor>() as u32,
            kind: 0,
            stable_key: partition_key.as_ptr(),
            display_name: partition_name.as_ptr(),
        };
        let descriptor = sys::ctex_mesh_descriptor {
            size: size_of::<sys::ctex_mesh_descriptor>() as u32,
            positions: data.positions.as_ptr().cast(),
            position_count: data.positions.len(),
            normals: data.normals.as_ptr().cast(),
            normal_count: data.normals.len(),
            vertex_colors: ptr::null(),
            vertex_color_count: 0,
            triangle_indices: data.triangle_indices.as_ptr(),
            triangle_index_count: data.triangle_indices.len(),
            uv_sets: &uv_set,
            uv_set_count: 1,
            default_uv_set: uv_name.as_ptr(),
            partitions: &partition,
            partition_count: 1,
            face_partition_indices: face_partitions.as_ptr(),
            face_partition_index_count: face_partitions.len(),
            face_material_ids: face_materials.as_ptr(),
            face_material_id_count: face_materials.len(),
        };
        let mut handle = ptr::null_mut();
        // SAFETY: every borrowed buffer above outlives this call, and the
        // library copies the mesh during it.
        check(
            unsafe { sys::ctex_mesh_create(&descriptor, &mut handle) },
            "mesh creation",
        )?;
        if handle.is_null() {
            return Err("mesh creation returned no handle".to_string());
        }
        Ok(Self(handle))
    }
}

impl Drop for Mesh {
    fn drop(&mut self) {
        // SAFETY: the handle came from `ctex_mesh_create` and is destroyed once.
        unsafe { sys::ctex_mesh_destroy(self.0) };
    }
}

/// A staged transaction, cancelled if it is dropped before it commits.
struct Transaction(*mut sys::ctex_texture_set_transaction);

impl Transaction {
    fn commit(self) -> Result<sys::ctex_tile_history_commit_info, String> {
        let mut info = sys::ctex_tile_history_commit_info {
            size: size_of::<sys::ctex_tile_history_commit_info>() as u32,
            ..Default::default()
        };
        // SAFETY: the handle is live and `info` is a fully initialized output.
        let result = unsafe { sys::ctex_texture_set_transaction_commit(self.0, &mut info) };
        check(result, "transaction commit")?;
        Ok(info)
    }
}

impl Drop for Transaction {
    fn drop(&mut self) {
        // SAFETY: commit consumes the staged transaction but not the handle,
        // and destroy tolerates a consumed one.
        unsafe { sys::ctex_texture_set_transaction_destroy(self.0) };
    }
}

impl Canvas {
    /// Build the declared configuration: a square texture set derived from the
    /// mesh, the named channels enabled, that many visible paint layers, and a
    /// tile-history ceiling.
    pub fn build(
        mesh: &MeshData,
        extent: u32,
        channels: &[&str],
        layers: usize,
        history_bytes: usize,
    ) -> Result<Self, String> {
        let semantic = text(channels.first().copied().unwrap_or_default())?;
        let mut document = ptr::null_mut();
        // SAFETY: the output pointer is a live local.
        check(
            unsafe { sys::ctex_document_create(&mut document) },
            "document creation",
        )?;
        let mut canvas = Self {
            document,
            snapshot: ptr::null_mut(),
            texture_set: CString::default(),
            semantic,
            extent,
            mirror: BTreeMap::new(),
        };
        canvas.texture_set = canvas.derive_texture_set(mesh, extent)?;
        canvas.enable_channels(channels)?;
        canvas.append_layers(layers)?;
        canvas.configure_history(history_bytes)?;
        canvas.snapshot = create_layer_snapshot(extent)?;
        Ok(canvas)
    }

    fn derive_texture_set(&mut self, mesh: &MeshData, extent: u32) -> Result<CString, String> {
        let native = Mesh::create(mesh)?;
        let uv_set = text(&mesh.uv_set_name)?;
        // SAFETY: the document, mesh and name all outlive the call.
        check(
            unsafe {
                sys::ctex_document_create_texture_sets_from_mesh(
                    self.document,
                    native.0,
                    uv_set.as_ptr(),
                    extent,
                    extent,
                    8,
                )
            },
            "texture-set derivation",
        )?;
        self.first_texture_set()
    }

    fn first_texture_set(&self) -> Result<CString, String> {
        let mut required = 0_usize;
        let mut count = 0_usize;
        // SAFETY: a null buffer with zero capacity is the documented query
        // form; both outputs are live locals.
        check(
            unsafe {
                sys::ctex_document_get_texture_set_ids(
                    self.document,
                    ptr::null_mut(),
                    0,
                    &mut required,
                    &mut count,
                )
            },
            "texture-set listing",
        )?;
        let mut buffer = vec![0_i8; required];
        // SAFETY: the buffer holds exactly the bytes the query required.
        check(
            unsafe {
                sys::ctex_document_get_texture_set_ids(
                    self.document,
                    buffer.as_mut_ptr(),
                    buffer.len(),
                    &mut required,
                    &mut count,
                )
            },
            "texture-set listing",
        )?;
        if count == 0 {
            return Err("the mesh derived no texture set".to_string());
        }
        let octets: Vec<u8> = buffer
            .iter()
            .take_while(|byte| **byte != 0)
            .map(|byte| *byte as u8)
            .collect();
        CString::new(octets).map_err(|_| "texture-set identifier is not text".to_string())
    }

    fn enable_channels(&mut self, channels: &[&str]) -> Result<(), String> {
        for channel in channels {
            let semantic = text(channel)?;
            // SAFETY: the document and both names outlive the call.
            check(
                unsafe {
                    sys::ctex_texture_set_set_channel_enabled(
                        self.document,
                        self.texture_set.as_ptr(),
                        semantic.as_ptr(),
                        1,
                        0,
                    )
                },
                "channel enable",
            )?;
        }
        Ok(())
    }

    fn append_layers(&mut self, layers: usize) -> Result<(), String> {
        let blend = text("normal")?;
        let names: Vec<(CString, CString)> = (0..layers)
            .map(|index| {
                Ok((
                    text(&format!("paint.{index}"))?,
                    text(&format!("Layer {index}"))?,
                ))
            })
            .collect::<Result<_, String>>()?;
        let entries: Vec<sys::ctex_layer_entry_descriptor> = names
            .iter()
            .map(
                |(identifier, display_name)| sys::ctex_layer_entry_descriptor {
                    size: size_of::<sys::ctex_layer_entry_descriptor>() as u32,
                    identifier: identifier.as_ptr(),
                    display_name: display_name.as_ptr(),
                    kind: 0,
                    parent_identifier: ptr::null(),
                    target_identifier: ptr::null(),
                    source_identifier: ptr::null(),
                    enabled: 1,
                    opacity: 1.0,
                    blend_mode: blend.as_ptr(),
                    channels: ptr::null(),
                    channel_count: 0,
                },
            )
            .collect();
        // SAFETY: `names` and `blend` own every string the descriptors point
        // at and outlive the call, which copies what it keeps.
        check(
            unsafe {
                sys::ctex_texture_set_layer_append(
                    self.document,
                    self.texture_set.as_ptr(),
                    entries.as_ptr(),
                    entries.len(),
                )
            },
            "layer append",
        )
    }

    fn configure_history(&mut self, budget_bytes: usize) -> Result<(), String> {
        // SAFETY: the document and identifier outlive the call.
        check(
            unsafe {
                sys::ctex_texture_set_configure_tile_history(
                    self.document,
                    self.texture_set.as_ptr(),
                    budget_bytes,
                )
            },
            "tile-history budget",
        )
    }

    /// The storage tiles a stamp footprint can reach, dilation included.
    ///
    /// This is `ctex_paint_plan_work`: the planner reports the tiles without
    /// touching texels, which is why it is bounded by the footprint rather
    /// than by the canvas.
    fn plan(&self, footprint: Footprint, ordinal: u64) -> Result<Vec<(u32, u32)>, String> {
        let mut work = sys::ctex_paint_work_descriptor::default();
        // SAFETY: the output is a live local.
        check(
            unsafe { sys::ctex_paint_work_init(&mut work) },
            "paint-work initialization",
        )?;
        let stamp = sys::ctex_paint_stamp_footprint {
            stamp_ordinal: ordinal,
            minimum_x: footprint.minimum_x,
            minimum_y: footprint.minimum_y,
            maximum_x: footprint.maximum_x,
            maximum_y: footprint.maximum_y,
        };
        work.canvas_width = self.extent;
        work.canvas_height = self.extent;
        work.tile_size = TILE_EXTENT;
        work.dilation_radius = DILATION_RADIUS;
        work.stamp_footprints = &stamp;
        work.stamp_footprint_count = 1;
        let mut info = sys::ctex_paint_work_info {
            size: size_of::<sys::ctex_paint_work_info>() as u32,
            ..Default::default()
        };
        let mut count = 0_usize;
        // SAFETY: a null tile buffer with zero capacity is the documented
        // query form; `stamp` outlives both calls.
        check(
            unsafe { sys::ctex_paint_plan_work(&work, &mut info, ptr::null_mut(), 0, &mut count) },
            "paint-work planning",
        )?;
        let mut tiles = vec![sys::ctex_paint_tile_coordinate { x: 0, y: 0 }; count];
        // SAFETY: the buffer holds exactly the count the query reported.
        check(
            unsafe {
                sys::ctex_paint_plan_work(
                    &work,
                    &mut info,
                    tiles.as_mut_ptr(),
                    tiles.len(),
                    &mut count,
                )
            },
            "paint-work planning",
        )?;
        Ok(tiles.iter().map(|tile| (tile.x, tile.y)).collect())
    }

    fn begin(&mut self, tiles: &[(u32, u32)], step: &str) -> Result<Transaction, String> {
        let step = text(step)?;
        let targets: Vec<sys::ctex_tile_history_target_descriptor> = tiles
            .iter()
            .map(|(x, y)| sys::ctex_tile_history_target_descriptor {
                size: size_of::<sys::ctex_tile_history_target_descriptor>() as u32,
                semantic_id: self.semantic.as_ptr(),
                tile_x: *x,
                tile_y: *y,
            })
            .collect();
        let mut handle = ptr::null_mut();
        // SAFETY: the document, snapshot, semantic and targets all outlive the
        // call; the transaction handle it returns is owned by `Transaction`.
        check(
            unsafe {
                sys::ctex_texture_set_begin_transaction(
                    self.document,
                    self.texture_set.as_ptr(),
                    step.as_ptr(),
                    targets.as_ptr(),
                    targets.len(),
                    self.snapshot,
                    &mut handle,
                )
            },
            "transaction start",
        )?;
        if handle.is_null() {
            return Err("transaction start returned no handle".to_string());
        }
        Ok(Transaction(handle))
    }

    /// Author one round stamp and publish it as one undoable step.
    ///
    /// The write set is declared from the planner's tiles before anything is
    /// written, so the library refuses any texel the plan did not reach.
    pub fn stamp(&mut self, sample: Sample) -> Result<Stamp, String> {
        let footprint = Footprint::around(sample.x, sample.y, sample.radius, self.extent);
        let tiles = self.plan(footprint, sample.ordinal)?;
        let transaction = self.begin(&tiles, "benchmark-stamp")?;
        let (texels, changed) = self.write_stamp(&transaction, sample, footprint)?;
        let info = transaction.commit()?;
        if info.committed == 0 {
            return Err(format!(
                "the authored stamp published nothing (ordinal {} at {},{} colour {:?} texels {} tiles {:?})",
                sample.ordinal, sample.x, sample.y, sample.colour, texels, tiles
            ));
        }
        Ok(Stamp {
            declared_tiles: tiles,
            changed_tiles: changed.into_iter().collect(),
            texels,
            committed_tiles: info.tile_count,
            retained_bytes: info.retained_bytes,
        })
    }

    fn write_stamp(
        &mut self,
        transaction: &Transaction,
        sample: Sample,
        footprint: Footprint,
    ) -> Result<(usize, BTreeSet<(u32, u32)>), String> {
        let radius = f64::from(sample.radius);
        let mut changed = BTreeSet::new();
        let mut texels = 0;
        for y in footprint.minimum_y..=footprint.maximum_y {
            for x in footprint.minimum_x..=footprint.maximum_x {
                let dx = f64::from(x) - f64::from(sample.x);
                let dy = f64::from(y) - f64::from(sample.y);
                if dx * dx + dy * dy > radius * radius {
                    continue;
                }
                changed.insert(self.write_texel(transaction, x, y, sample.colour)?);
                texels += 1;
            }
        }
        Ok((texels, changed))
    }

    fn write_texel(
        &mut self,
        transaction: &Transaction,
        x: u32,
        y: u32,
        colour: [u8; 3],
    ) -> Result<(u32, u32), String> {
        // SAFETY: the transaction, semantic and pixel bytes outlive the call,
        // whose declared length matches the channel's 8-bit storage.
        check(
            unsafe {
                sys::ctex_texture_set_transaction_write_pixel(
                    transaction.0,
                    self.semantic.as_ptr(),
                    x,
                    y,
                    colour.as_ptr().cast(),
                    AUTHORED_TEXEL_BYTES,
                )
            },
            "authored texel",
        )?;
        Ok(self.mirror_texel(x, y, colour))
    }

    /// Record the authored texel in the host's device-facing tile copy.
    fn mirror_texel(&mut self, x: u32, y: u32, colour: [u8; 3]) -> (u32, u32) {
        let tile = (x / TILE_EXTENT, y / TILE_EXTENT);
        let payload = self.mirror.entry(tile).or_insert_with(|| {
            vec![0_u8; (TILE_EXTENT * TILE_EXTENT) as usize * DEVICE_TEXEL_BYTES]
        });
        let offset =
            ((y % TILE_EXTENT) * TILE_EXTENT + (x % TILE_EXTENT)) as usize * DEVICE_TEXEL_BYTES;
        payload[offset] = colour[0];
        payload[offset + 1] = colour[1];
        payload[offset + 2] = colour[2];
        payload[offset + 3] = u8::MAX;
        tile
    }

    /// The host's copy of one authored tile, in the device texture's format.
    pub fn tile_payload(&self, tile: (u32, u32)) -> Option<&[u8]> {
        self.mirror.get(&tile).map(Vec::as_slice)
    }

    pub fn texture_set_identifier(&self) -> String {
        self.texture_set.to_string_lossy().into_owned()
    }
}

impl Drop for Canvas {
    fn drop(&mut self) {
        // SAFETY: both handles came from the library, are destroyed once, and
        // the snapshot outlives no transaction because `stamp` consumes each
        // transaction before it returns.
        unsafe {
            if !self.snapshot.is_null() {
                sys::ctex_layer_snapshot_destroy(self.snapshot);
            }
            sys::ctex_document_destroy(self.document);
        }
    }
}

fn create_layer_snapshot(extent: u32) -> Result<*mut sys::ctex_layer_snapshot, String> {
    let mut snapshot = ptr::null_mut();
    // SAFETY: a transaction that edits texels declares no composite content,
    // so both descriptor arrays are the documented empty form.
    check(
        unsafe {
            sys::ctex_layer_snapshot_create(
                extent,
                extent,
                ptr::null(),
                0,
                ptr::null(),
                0,
                &mut snapshot,
            )
        },
        "layer snapshot",
    )?;
    if snapshot.is_null() {
        return Err("layer snapshot creation returned no handle".to_string());
    }
    Ok(snapshot)
}

/// One synthetic pointer sample resolved into a brush stamp.
#[derive(Clone, Copy, Debug)]
pub struct Sample {
    pub ordinal: u64,
    pub x: u32,
    pub y: u32,
    pub radius: u32,
    pub colour: [u8; 3],
}

/// The inclusive canvas rectangle a stamp can touch.
#[derive(Clone, Copy, Debug)]
pub struct Footprint {
    pub minimum_x: u32,
    pub minimum_y: u32,
    pub maximum_x: u32,
    pub maximum_y: u32,
}

impl Footprint {
    fn around(x: u32, y: u32, radius: u32, extent: u32) -> Self {
        let last = extent.saturating_sub(1);
        Self {
            minimum_x: x.saturating_sub(radius),
            minimum_y: y.saturating_sub(radius),
            maximum_x: x.saturating_add(radius).min(last),
            maximum_y: y.saturating_add(radius).min(last),
        }
    }
}
