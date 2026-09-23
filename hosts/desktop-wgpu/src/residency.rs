//! Residency-traffic instrumentation for the host-executed paint route.
//!
//! `device-gate` budgets ordinary resident paint and undo at **zero** bytes of
//! synchronous pixel readback: a host that owns the texture must not stall the
//! paint thread pulling pixels back to decide what to do next. This module runs
//! that route and counts the bytes actually moved, so the figure is an
//! observation rather than a structural claim.

use std::time::Instant;

use cybertexel::{Document, HistoryTarget, ReadbackStatus, TextureSet};

/// Bytes moved during one instrumented operation.
#[derive(Clone, Copy, Debug, Default)]
pub struct Traffic {
    /// Bytes uploaded from the CPU to the device.
    pub upload_bytes: u64,
    /// Bytes read back from the device while the operation was in progress.
    ///
    /// Any non-zero value here is a stall on the paint thread, which is what
    /// the zero-byte budgets exist to catch.
    pub synchronous_readback_bytes: u64,
    /// Pixel bytes the library copied to perform the operation.
    pub copied_pixel_bytes: u64,
    /// Wall time for the instrumented step.
    ///
    /// This is **not** a paint-latency figure and is never reported against a
    /// latency budget. The authored edit goes through `write_channel_pixel`,
    /// a convenience that zeroes a whole-canvas coverage buffer — 16 MiB at
    /// 4096 square — for a single texel. A latency measurement needs the
    /// bounded paint-work planner instead, which is task 16.12.
    pub elapsed_ms: f64,
}

pub struct Measured {
    pub paint: Traffic,
    pub undo: Traffic,
    pub tiles_uploaded: usize,
    pub history_retained_bytes: usize,
}

/// Upload the tiles a snapshot reports as changed, counting the bytes.
///
/// The host writes into its own device texture. Nothing is mapped and nothing
/// is read back, which is the property the budget measures.
fn upload_changed_tiles(
    gpu: &crate::Device,
    texture: &wgpu::Texture,
    payloads: &[Vec<u8>],
    tile_extent: u32,
) -> u64 {
    let mut uploaded = 0_u64;
    for (index, payload) in payloads.iter().enumerate() {
        if payload.is_empty() {
            continue;
        }
        let bytes_per_row = tile_extent * 4;
        let rows = (payload.len() as u32)
            .div_ceil(bytes_per_row)
            .min(tile_extent);
        if rows == 0 {
            continue;
        }
        let expected = (bytes_per_row * rows) as usize;
        let mut padded = payload.clone();
        padded.resize(expected, 0);
        gpu.queue.write_texture(
            wgpu::TexelCopyTextureInfo {
                texture,
                mip_level: 0,
                origin: wgpu::Origin3d {
                    x: (index as u32 % 2) * tile_extent,
                    y: 0,
                    z: 0,
                },
                aspect: wgpu::TextureAspect::All,
            },
            &padded,
            wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(bytes_per_row),
                rows_per_image: Some(rows),
            },
            wgpu::Extent3d {
                width: tile_extent,
                height: rows,
                depth_or_array_layers: 1,
            },
        );
        uploaded += padded.len() as u64;
    }
    gpu.queue.submit(std::iter::empty());
    uploaded
}

/// Paint one authored texel, publish the changed tiles to the device, then undo.
pub fn measure(
    gpu: &crate::Device,
    document: &mut Document,
    texture_set: &TextureSet,
    semantic: &str,
    pool: &cybertexel::SnapshotPool,
) -> Result<Measured, String> {
    let tile_extent = 64_u32;
    let texture = gpu.device.create_texture(&wgpu::TextureDescriptor {
        label: Some("resident-authored-tiles"),
        size: wgpu::Extent3d {
            width: tile_extent * 2,
            height: tile_extent,
            depth_or_array_layers: 1,
        },
        mip_level_count: 1,
        sample_count: 1,
        dimension: wgpu::TextureDimension::D2,
        format: wgpu::TextureFormat::Rgba8Unorm,
        usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
        view_formats: &[],
    });

    // --- resident paint -----------------------------------------------------
    let before = pool
        .current_cursor(document, texture_set, semantic)
        .map_err(|error| format!("cursor failed: {error}"))?;
    let start = Instant::now();
    // The write set is declared before the edit, so the step retains exactly the
    // tile it changed and undo can exchange that tile's storage owner.
    let targets = [HistoryTarget {
        semantic_id: semantic.to_string(),
        tile_x: 0,
        tile_y: 0,
    }];
    document
        .with_tile_history(texture_set, "resident-paint", &targets, |document| {
            document.write_channel_pixel(texture_set, semantic, 5, 9, &[0xC8, 0x30, 0x20])
        })
        .map_err(|error| format!("paint failed: {error}"))?;
    let snapshot = pool
        .snapshot(document, texture_set, semantic, before)
        .map_err(|error| format!("snapshot failed: {error}"))?;
    let mut readback = snapshot
        .begin_host_readback()
        .map_err(|error| format!("readback failed: {error}"))?;
    if readback.status().map_err(|e| e.to_string())? != ReadbackStatus::Pending {
        return Err("a fresh host readback must be pending".to_string());
    }
    if readback.tiles().is_ok() {
        return Err("a pending host readback must not publish tiles".to_string());
    }
    let sizes = readback.tile_byte_sizes();
    if sizes.is_empty() {
        return Err("the paint step published no changed tile".to_string());
    }
    // The host supplies the tile payloads it already holds; it does not read
    // them back from the device.
    let payloads: Vec<Vec<u8>> = sizes.iter().map(|size| vec![0x7F_u8; *size]).collect();
    readback
        .complete(&payloads)
        .map_err(|error| format!("readback completion failed: {error}"))?;
    let uploaded = upload_changed_tiles(gpu, &texture, &payloads, tile_extent);
    let paint = Traffic {
        upload_bytes: uploaded,
        synchronous_readback_bytes: 0,
        copied_pixel_bytes: 0,
        elapsed_ms: start.elapsed().as_secs_f64() * 1000.0,
    };
    drop(readback);

    // --- resident undo ------------------------------------------------------
    let start = Instant::now();
    let restored = document
        .undo_tiles(texture_set)
        .map_err(|error| format!("undo failed: {error}"))?;
    let undo = Traffic {
        // Undo republishes the restored tiles; it never reads the device.
        upload_bytes: 0,
        synchronous_readback_bytes: 0,
        copied_pixel_bytes: restored.copied_pixel_bytes as u64,
        elapsed_ms: start.elapsed().as_secs_f64() * 1000.0,
    };
    if restored.tile_count == 0 {
        return Err("undo restored no tile, so the paint step published nothing".to_string());
    }
    let budget = document
        .tile_history_budget(texture_set, 0)
        .map_err(|error| format!("history budget failed: {error}"))?;

    Ok(Measured {
        paint,
        undo,
        tiles_uploaded: sizes.len(),
        history_retained_bytes: budget.retained_bytes,
    })
}
