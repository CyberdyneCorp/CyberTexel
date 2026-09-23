//! Audited unsafe boundary between safe Rust values and `cybertexel-sys`.

use std::ffi::{CStr, CString};
use std::ptr::{self, NonNull};

use cybertexel_sys as sys;

use crate::host::{
    CompletedHostResource, CompletionDisposition, HostCompletionResult, HostMaterialProgram,
    HostRecovery, HostResource, ReadbackStatus, ReleasedResource, ReplaySemantics, RevisionCursor,
    ShaderTarget,
};
use crate::layers::LayerEntry;
use crate::{Error, ResultCode, TextureSet, Version};

pub(crate) struct DocumentHandle(NonNull<sys::ctex_document>);

// SAFETY: the handle has a single Rust owner and may be moved between threads.
// Document calls cannot overlap because the public wrapper is not Sync and
// mutations require exclusive access.
unsafe impl Send for DocumentHandle {}

impl DocumentHandle {
    pub(crate) fn create() -> Result<Self, Error> {
        check_abi()?;
        let mut handle = ptr::null_mut();
        unsafe { check(sys::ctex_document_create(&mut handle))? };
        NonNull::new(handle)
            .map(Self)
            .ok_or_else(|| Error::InvalidNativeState("document creation returned no handle".into()))
    }

    pub(crate) fn texture_set_ids(&self) -> Result<Vec<String>, Error> {
        let mut required = 0;
        let mut count = 0;
        unsafe {
            check(sys::ctex_document_get_texture_set_ids(
                self.0.as_ptr(),
                ptr::null_mut(),
                0,
                &mut required,
                &mut count,
            ))?;
        }
        let mut bytes = vec![0_i8; required];
        unsafe {
            check(sys::ctex_document_get_texture_set_ids(
                self.0.as_ptr(),
                bytes.as_mut_ptr(),
                bytes.len(),
                &mut required,
                &mut count,
            ))?;
        }
        let mut identifiers = Vec::with_capacity(count);
        for value in bytes
            .split(|byte| *byte == 0)
            .filter(|value| !value.is_empty())
        {
            let octets: Vec<u8> = value.iter().map(|byte| *byte as u8).collect();
            identifiers.push(
                String::from_utf8(octets).map_err(|_| {
                    Error::InvalidNativeState("native identifier is not UTF-8".into())
                })?,
            );
        }
        Ok(identifiers)
    }

    pub(crate) fn create_texture_set(
        &self,
        display_name: &str,
        partition_key: &str,
        uv_set: &str,
        width: u32,
        height: u32,
        default_bit_depth: u8,
    ) -> Result<TextureSet, Error> {
        let previous = self.texture_set_ids()?;
        let display_name = c_string(display_name)?;
        let partition_key = c_string(partition_key)?;
        let uv_set = c_string(uv_set)?;
        let descriptor = sys::ctex_texture_set_descriptor {
            size: std::mem::size_of::<sys::ctex_texture_set_descriptor>() as u32,
            display_name: display_name.as_ptr(),
            partition_kind: 0,
            partition_key: partition_key.as_ptr(),
            uv_set: uv_set.as_ptr(),
            width,
            height,
            default_bit_depth,
            udim_tiling: 0,
        };
        unsafe {
            check(sys::ctex_document_create_texture_set(
                self.0.as_ptr(),
                &descriptor,
            ))?
        };
        let identifier = self
            .texture_set_ids()?
            .into_iter()
            .find(|value| !previous.contains(value))
            .ok_or_else(|| {
                Error::InvalidNativeState("created texture set has no identifier".into())
            })?;
        Ok(TextureSet {
            identifier,
            width,
            height,
        })
    }

    pub(crate) fn set_channel_enabled(
        &self,
        texture_set_id: &str,
        semantic_id: &str,
        bit_depth: u32,
    ) -> Result<(), Error> {
        let texture_set_id = c_string(texture_set_id)?;
        let semantic_id = c_string(semantic_id)?;
        unsafe {
            check(sys::ctex_texture_set_set_channel_enabled(
                self.0.as_ptr(),
                texture_set_id.as_ptr(),
                semantic_id.as_ptr(),
                1,
                bit_depth,
            ))
        }
    }

    #[allow(clippy::too_many_arguments)]
    pub(crate) fn write_channel_pixel(
        &self,
        texture_set_id: &str,
        semantic_id: &str,
        width: u32,
        height: u32,
        x: u32,
        y: u32,
        pixel: &[u8],
    ) -> Result<(), Error> {
        let texture_set_id = c_string(texture_set_id)?;
        let semantic_id = c_string(semantic_id)?;
        let mut raw = ptr::null_mut();
        unsafe {
            check(sys::ctex_paint_preview_session_create(
                self.0.as_ptr(),
                texture_set_id.as_ptr(),
                semantic_id.as_ptr(),
                &mut raw,
            ))?;
        }
        let preview = PreviewHandle(NonNull::new(raw).ok_or_else(|| {
            Error::InvalidNativeState("preview creation returned no handle".into())
        })?);
        unsafe {
            check(sys::ctex_paint_preview_session_write_pixel(
                preview.0.as_ptr(),
                x,
                y,
                pixel.as_ptr().cast(),
                pixel.len(),
            ))?;
        }
        let mut coverage = vec![0_u8; width as usize * height as usize];
        coverage[y as usize * width as usize + x as usize] = 1;
        let mut info = sys::ctex_paint_preview_info {
            size: std::mem::size_of::<sys::ctex_paint_preview_info>() as u32,
            ..Default::default()
        };
        unsafe {
            check(sys::ctex_paint_preview_session_finalize(
                preview.0.as_ptr(),
                coverage.as_ptr(),
                coverage.len(),
                0,
                &mut info,
            ))?;
            check(sys::ctex_paint_preview_session_commit(
                preview.0.as_ptr(),
                &mut info,
            ))
        }
    }

    pub(crate) fn as_ptr(&self) -> *const sys::ctex_document {
        self.0.as_ptr()
    }
}

struct PreviewHandle(NonNull<sys::ctex_paint_preview_session>);

impl Drop for PreviewHandle {
    fn drop(&mut self) {
        unsafe { sys::ctex_paint_preview_session_destroy(self.0.as_ptr()) };
    }
}

pub(crate) struct HostExecutionHandle(NonNull<sys::ctex_host_execution_session>);

impl HostExecutionHandle {
    pub(crate) fn create(initial_revision: u64) -> Result<Self, Error> {
        let mut handle = ptr::null_mut();
        unsafe {
            check(sys::ctex_host_execution_session_create(
                initial_revision,
                &mut handle,
            ))?
        };
        NonNull::new(handle).map(Self).ok_or_else(|| {
            Error::InvalidNativeState("host session creation returned no handle".into())
        })
    }

    pub(crate) fn submit(
        &self,
        operation: &str,
        base_revision: u64,
        resources: &[HostResource],
        replay: ReplaySemantics,
    ) -> Result<u64, Error> {
        let operation = c_string(operation)?;
        let names = resources
            .iter()
            .map(|resource| Ok((c_string(&resource.logical_id)?, c_string(&resource.role)?)))
            .collect::<Result<Vec<_>, Error>>()?;
        let native = resources
            .iter()
            .zip(&names)
            .map(
                |(resource, (logical_id, role))| sys::ctex_host_resource_descriptor {
                    size: std::mem::size_of::<sys::ctex_host_resource_descriptor>() as u32,
                    logical_id: logical_id.as_ptr(),
                    generation: resource.generation,
                    role: role.as_ptr(),
                    format: resource.format,
                    width: resource.width,
                    height: resource.height,
                    layers: resource.layers,
                    mip_levels: resource.mip_levels,
                    tile_width: if resource.tile_width == 0 {
                        resource.width
                    } else {
                        resource.tile_width
                    },
                    tile_height: if resource.tile_height == 0 {
                        resource.height
                    } else {
                        resource.tile_height
                    },
                    externally_initialized: u32::from(resource.externally_initialized),
                    owner: resource.owner as u32,
                    required_state: resource.required_state as u32,
                    output: u32::from(resource.output),
                },
            )
            .collect::<Vec<_>>();
        let descriptor = sys::ctex_host_submission_descriptor {
            size: std::mem::size_of::<sys::ctex_host_submission_descriptor>() as u32,
            operation: operation.as_ptr(),
            base_revision,
            resources: native.as_ptr(),
            resource_count: native.len(),
            replay_semantics: replay as u32,
        };
        let mut info = sys::ctex_host_submission_info {
            size: std::mem::size_of::<sys::ctex_host_submission_info>() as u32,
            ..Default::default()
        };
        unsafe {
            check(sys::ctex_host_execution_session_submit(
                self.0.as_ptr(),
                &descriptor,
                &mut info,
            ))?;
        }
        Ok(info.completion_token)
    }

    pub(crate) fn complete(
        &self,
        token: u64,
        outputs: &[CompletedHostResource],
        recovery: Option<&HostRecovery>,
    ) -> Result<HostCompletionResult, Error> {
        let names = outputs
            .iter()
            .map(|output| c_string(&output.logical_id))
            .collect::<Result<Vec<_>, Error>>()?;
        let native_outputs = outputs
            .iter()
            .zip(&names)
            .map(
                |(output, logical_id)| sys::ctex_host_completed_resource_descriptor {
                    size: std::mem::size_of::<sys::ctex_host_completed_resource_descriptor>()
                        as u32,
                    logical_id: logical_id.as_ptr(),
                    generation: output.generation,
                    format: output.format,
                    width: output.width,
                    height: output.height,
                    layers: output.layers,
                },
            )
            .collect::<Vec<_>>();
        let recovery_version = recovery
            .map(|value| c_string(&value.operation_record_version))
            .transpose()?;
        let native_recovery = recovery.map(|value| sys::ctex_host_recovery_descriptor {
            size: std::mem::size_of::<sys::ctex_host_recovery_descriptor>() as u32,
            kind: 1,
            checkpoint_complete: 1,
            checkpoint_revision: value.checkpoint_revision,
            operation_record_version: recovery_version.as_ref().unwrap().as_ptr(),
            inputs_pinned: u32::from(value.inputs_pinned),
            retained_bytes: value.retained_bytes,
        });
        let descriptor = sys::ctex_host_completion_descriptor {
            size: std::mem::size_of::<sys::ctex_host_completion_descriptor>() as u32,
            completion_token: token,
            status: 0,
            outputs: native_outputs.as_ptr(),
            output_count: native_outputs.len(),
            recovery: native_recovery
                .as_ref()
                .map_or(ptr::null(), std::ptr::from_ref),
            detail: ptr::null(),
        };
        let mut result = ptr::null_mut();
        unsafe {
            check(sys::ctex_host_execution_session_complete(
                self.0.as_ptr(),
                &descriptor,
                &mut result,
            ))?;
        }
        let result = CompletionHandle(NonNull::new(result).ok_or_else(|| {
            Error::InvalidNativeState("host completion returned no result".into())
        })?);
        result.read()
    }

    pub(crate) fn committed_generation(&self, logical_id: &str) -> Result<Option<u64>, Error> {
        let logical_id = c_string(logical_id)?;
        let mut found = 0;
        let mut generation = 0;
        unsafe {
            check(sys::ctex_host_execution_session_get_committed_resource(
                self.0.as_ptr(),
                logical_id.as_ptr(),
                &mut found,
                &mut generation,
            ))?;
        }
        Ok((found != 0).then_some(generation))
    }

    pub(crate) fn resource_is_held(
        &self,
        logical_id: &str,
        generation: u64,
    ) -> Result<bool, Error> {
        let logical_id = c_string(logical_id)?;
        let mut held = 0;
        unsafe {
            check(sys::ctex_host_execution_session_resource_is_held(
                self.0.as_ptr(),
                logical_id.as_ptr(),
                generation,
                &mut held,
            ))?;
        }
        Ok(held != 0)
    }
}

impl Drop for HostExecutionHandle {
    fn drop(&mut self) {
        unsafe { sys::ctex_host_execution_session_destroy(self.0.as_ptr()) };
    }
}

struct CompletionHandle(NonNull<sys::ctex_host_completion_result>);

impl CompletionHandle {
    fn read(&self) -> Result<HostCompletionResult, Error> {
        let mut info = sys::ctex_host_completion_result_info {
            size: std::mem::size_of::<sys::ctex_host_completion_result_info>() as u32,
            ..Default::default()
        };
        unsafe {
            check(sys::ctex_host_completion_result_get_info(
                self.0.as_ptr(),
                &mut info,
                ptr::null_mut(),
                0,
                ptr::null_mut(),
                0,
                ptr::null_mut(),
                0,
            ))?;
        }
        let mut released =
            vec![sys::ctex_host_resource_version::default(); info.released_resource_count];
        let mut identities = vec![0_i8; info.required_released_identity_size];
        let mut message = vec![0_i8; info.required_message_size];
        unsafe {
            check(sys::ctex_host_completion_result_get_info(
                self.0.as_ptr(),
                &mut info,
                released.as_mut_ptr(),
                released.len(),
                identities.as_mut_ptr(),
                identities.len(),
                message.as_mut_ptr(),
                message.len(),
            ))?;
        }
        let released_resources = released
            .into_iter()
            .map(|resource| {
                Ok(ReleasedResource {
                    logical_id: packed_string(&identities, resource.logical_id_offset)?,
                    generation: resource.generation,
                })
            })
            .collect::<Result<Vec<_>, Error>>()?;
        Ok(HostCompletionResult {
            disposition: CompletionDisposition::from_raw(info.disposition)?,
            completion_token: info.completion_token,
            published_revision: (info.has_published_revision != 0)
                .then_some(info.published_revision),
            released_resources,
            message: packed_string(&message, 0)?,
        })
    }
}

impl Drop for CompletionHandle {
    fn drop(&mut self) {
        unsafe { sys::ctex_host_completion_result_destroy(self.0.as_ptr()) };
    }
}

pub(crate) struct SnapshotPoolHandle(NonNull<sys::ctex_transport_snapshot_pool>);

impl SnapshotPoolHandle {
    pub(crate) fn create(budget_bytes: usize) -> Result<Self, Error> {
        let mut handle = ptr::null_mut();
        unsafe {
            check(sys::ctex_transport_snapshot_pool_create(
                budget_bytes,
                &mut handle,
            ))?
        };
        NonNull::new(handle).map(Self).ok_or_else(|| {
            Error::InvalidNativeState("snapshot pool creation returned no handle".into())
        })
    }

    pub(crate) fn current_cursor(
        &self,
        document: &DocumentHandle,
        texture_set_id: &str,
        semantic_id: &str,
    ) -> Result<RevisionCursor, Error> {
        let texture_set_id = c_string(texture_set_id)?;
        let semantic_id = c_string(semantic_id)?;
        let mut info = sys::ctex_transport_delta_info {
            size: std::mem::size_of::<sys::ctex_transport_delta_info>() as u32,
            ..Default::default()
        };
        unsafe {
            check(sys::ctex_texture_set_query_channel_delta(
                document.as_ptr(),
                texture_set_id.as_ptr(),
                semantic_id.as_ptr(),
                sys::ctex_transport_revision_cursor::default(),
                ptr::null_mut(),
                0,
                &mut info,
            ))?;
        }
        Ok(RevisionCursor {
            epoch: info.current_cursor.epoch,
            revision: info.current_cursor.revision,
        })
    }

    pub(crate) fn snapshot(
        &self,
        document: &DocumentHandle,
        texture_set_id: &str,
        semantic_id: &str,
        since: RevisionCursor,
    ) -> Result<SnapshotHandle, Error> {
        let texture_set_id = c_string(texture_set_id)?;
        let semantic_id = c_string(semantic_id)?;
        let mut handle = ptr::null_mut();
        let mut info = sys::ctex_transport_snapshot_query_info {
            size: std::mem::size_of::<sys::ctex_transport_snapshot_query_info>() as u32,
            ..Default::default()
        };
        unsafe {
            check(sys::ctex_texture_set_query_channel_snapshot(
                self.0.as_ptr(),
                document.as_ptr(),
                texture_set_id.as_ptr(),
                semantic_id.as_ptr(),
                sys::ctex_transport_revision_cursor {
                    epoch: since.epoch,
                    revision: since.revision,
                },
                &mut handle,
                &mut info,
            ))?;
        }
        NonNull::new(handle)
            .map(SnapshotHandle)
            .ok_or_else(|| Error::InvalidNativeState("snapshot query returned no handle".into()))
    }
}

impl Drop for SnapshotPoolHandle {
    fn drop(&mut self) {
        unsafe { sys::ctex_transport_snapshot_pool_destroy(self.0.as_ptr()) };
    }
}

pub(crate) struct SnapshotHandle(NonNull<sys::ctex_transport_snapshot>);

impl SnapshotHandle {
    pub(crate) fn begin_host_readback(&self) -> Result<ReadbackHandle, Error> {
        let mut count = 0;
        unsafe {
            check(sys::ctex_transport_snapshot_get_tile_versions(
                self.0.as_ptr(),
                ptr::null_mut(),
                0,
                &mut count,
            ))?;
        }
        let mut versions = vec![sys::ctex_transport_tile_version::default(); count];
        unsafe {
            check(sys::ctex_transport_snapshot_get_tile_versions(
                self.0.as_ptr(),
                versions.as_mut_ptr(),
                versions.len(),
                &mut count,
            ))?;
        }
        let mut layouts = Vec::with_capacity(versions.len());
        let mut buffers = Vec::with_capacity(versions.len());
        for version in &mut versions {
            let mut layout = sys::ctex_transport_tile_memory_layout {
                size: std::mem::size_of::<sys::ctex_transport_tile_memory_layout>() as u32,
                ..Default::default()
            };
            unsafe {
                check(sys::ctex_transport_snapshot_get_tile_memory_layout(
                    self.0.as_ptr(),
                    *version,
                    ptr::null(),
                    &mut layout,
                ))?;
            }
            version.residency = 1;
            buffers.push(vec![0_u8; layout.byte_size].into_boxed_slice());
            layouts.push(layout);
        }
        let destinations = versions
            .iter()
            .zip(&layouts)
            .zip(&mut buffers)
            .map(
                |((version, layout), output)| sys::ctex_transport_tile_readback_destination {
                    size: std::mem::size_of::<sys::ctex_transport_tile_readback_destination>()
                        as u32,
                    version: *version,
                    layout: *layout,
                    output: output.as_mut_ptr().cast(),
                    output_size: output.len(),
                },
            )
            .collect::<Vec<_>>();
        let mut handle = ptr::null_mut();
        unsafe {
            check(sys::ctex_transport_snapshot_begin_host_readback(
                self.0.as_ptr(),
                ptr::null(),
                destinations.as_ptr(),
                destinations.len(),
                &mut handle,
            ))?;
        }
        Ok(ReadbackHandle {
            handle: NonNull::new(handle).ok_or_else(|| {
                Error::InvalidNativeState("readback creation returned no handle".into())
            })?,
            versions,
            layouts,
            buffers,
        })
    }
}

impl Drop for SnapshotHandle {
    fn drop(&mut self) {
        unsafe { sys::ctex_transport_snapshot_destroy(self.0.as_ptr()) };
    }
}

pub(crate) struct ReadbackHandle {
    handle: NonNull<sys::ctex_transport_readback>,
    versions: Vec<sys::ctex_transport_tile_version>,
    layouts: Vec<sys::ctex_transport_tile_memory_layout>,
    buffers: Vec<Box<[u8]>>,
}

impl ReadbackHandle {
    pub(crate) fn status(&self) -> Result<ReadbackStatus, Error> {
        let mut info = sys::ctex_transport_readback_info {
            size: std::mem::size_of::<sys::ctex_transport_readback_info>() as u32,
            ..Default::default()
        };
        unsafe {
            check(sys::ctex_transport_readback_get_info(
                self.handle.as_ptr(),
                &mut info,
                ptr::null_mut(),
                0,
            ))?;
        }
        ReadbackStatus::from_raw(info.status)
    }

    pub(crate) fn tile_byte_sizes(&self) -> Vec<usize> {
        self.layouts.iter().map(|layout| layout.byte_size).collect()
    }

    pub(crate) fn complete(&mut self, tiles: &[Vec<u8>]) -> Result<(), Error> {
        if tiles.len() != self.versions.len() {
            return Err(Error::InvalidNativeState(
                "completion must contain one payload per requested tile".into(),
            ));
        }
        let completions = self
            .versions
            .iter()
            .zip(&self.layouts)
            .zip(tiles)
            .map(
                |((version, layout), bytes)| sys::ctex_transport_host_tile_completion {
                    size: std::mem::size_of::<sys::ctex_transport_host_tile_completion>() as u32,
                    version: *version,
                    layout: *layout,
                    bytes: bytes.as_ptr().cast(),
                    byte_size: bytes.len(),
                },
            )
            .collect::<Vec<_>>();
        unsafe {
            check(sys::ctex_transport_readback_complete_host(
                self.handle.as_ptr(),
                completions.as_ptr(),
                completions.len(),
            ))
        }
    }

    pub(crate) fn tiles(&self) -> Vec<&[u8]> {
        self.buffers.iter().map(AsRef::as_ref).collect()
    }
}

impl Drop for ReadbackHandle {
    fn drop(&mut self) {
        unsafe { sys::ctex_transport_readback_destroy(self.handle.as_ptr()) };
    }
}

fn packed_string(buffer: &[i8], offset: usize) -> Result<String, Error> {
    let tail = buffer.get(offset..).ok_or_else(|| {
        Error::InvalidNativeState("native string offset is outside its buffer".into())
    })?;
    let end = tail.iter().position(|value| *value == 0).ok_or_else(|| {
        Error::InvalidNativeState("native string buffer is not terminated".into())
    })?;
    let bytes = tail[..end].iter().map(|value| *value as u8).collect();
    String::from_utf8(bytes)
        .map_err(|_| Error::InvalidNativeState("native string is not UTF-8".into()))
}

pub(crate) fn emit_default_host_material(
    stable_identity: &str,
    output_identity: &str,
    width: u32,
    height: u32,
    target: ShaderTarget,
) -> Result<HostMaterialProgram, Error> {
    let mut graph_info = sys::ctex_material_graph_info {
        size: std::mem::size_of::<sys::ctex_material_graph_info>() as u32,
        ..Default::default()
    };
    unsafe {
        check(sys::ctex_material_graph_create_default(
            &mut graph_info,
            ptr::null_mut(),
            0,
            ptr::null_mut(),
            0,
        ))?;
    }
    let mut graph = vec![0_u8; graph_info.canonical_size];
    unsafe {
        check(sys::ctex_material_graph_create_default(
            &mut graph_info,
            graph.as_mut_ptr().cast(),
            graph.len(),
            ptr::null_mut(),
            0,
        ))?;
    }
    let stable_identity = c_string(stable_identity)?;
    let output_identity = c_string(output_identity)?;
    let role = c_string("material output")?;
    let formats = [2_u32];
    let request = sys::ctex_shader_material_request {
        size: std::mem::size_of::<sys::ctex_shader_material_request>() as u32,
        stable_identity: stable_identity.as_ptr(),
        target: target as u32,
        features: sys::ctex_shader_device_features_descriptor {
            size: std::mem::size_of::<sys::ctex_shader_device_features_descriptor>() as u32,
            binding_budget: 16,
            maximum_texture_dimension: 8192,
            supported_texture_formats: formats.as_ptr(),
            supported_texture_format_count: formats.len(),
            floating_point_filtering: 1,
            compute_available: 0,
        },
        resources: ptr::null(),
        resource_count: 0,
        output: sys::ctex_shader_texture_descriptor {
            size: std::mem::size_of::<sys::ctex_shader_texture_descriptor>() as u32,
            logical_id: output_identity.as_ptr(),
            generation: 1,
            role: role.as_ptr(),
            format: 2,
            width,
            height,
            layers: 1,
            mip_levels: 1,
            tile_width: width,
            tile_height: height,
            externally_initialized: 0,
        },
        requested_filter: 1,
        vertex_count: 3,
    };
    let mut info = sys::ctex_shader_material_info {
        size: std::mem::size_of::<sys::ctex_shader_material_info>() as u32,
        ..Default::default()
    };
    unsafe {
        check(sys::ctex_shader_emit_material(
            ptr::null(),
            graph.as_ptr().cast(),
            graph.len(),
            &request,
            &mut info,
            ptr::null_mut(),
            0,
            ptr::null_mut(),
            0,
            ptr::null_mut(),
            0,
            ptr::null_mut(),
            0,
        ))?;
    }
    let mut vertex = vec![0_u8; info.vertex_artifact_size];
    let mut fragment = vec![0_u8; info.fragment_artifact_size];
    let mut plan = vec![0_i8; info.pass_plan_size];
    let mut workarounds = vec![0_i8; info.workaround_report_size];
    unsafe {
        check(sys::ctex_shader_emit_material(
            ptr::null(),
            graph.as_ptr().cast(),
            graph.len(),
            &request,
            &mut info,
            vertex.as_mut_ptr().cast(),
            vertex.len(),
            fragment.as_mut_ptr().cast(),
            fragment.len(),
            plan.as_mut_ptr(),
            plan.len(),
            workarounds.as_mut_ptr(),
            workarounds.len(),
        ))?;
    }
    Ok(HostMaterialProgram {
        target,
        vertex_artifact: vertex,
        fragment_artifact: fragment,
        pass_plan: packed_string(&plan, 0)?,
        workaround_report: packed_string(&workarounds, 0)?,
    })
}

impl Drop for DocumentHandle {
    fn drop(&mut self) {
        unsafe { sys::ctex_document_destroy(self.0.as_ptr()) };
    }
}

pub(crate) fn version() -> Version {
    unsafe { convert_version(sys::ctex_get_version()) }
}

fn check_abi() -> Result<(), Error> {
    let version = unsafe { convert_version(sys::ctex_get_abi_version()) };
    validate_abi(version, 0)
}

fn validate_abi(version: Version, expected_major: u32) -> Result<(), Error> {
    if version.major == expected_major {
        Ok(())
    } else {
        Err(Error::IncompatibleAbi {
            expected_major,
            native: version.string,
        })
    }
}

unsafe fn convert_version(value: sys::ctex_version) -> Version {
    let string = if value.string.is_null() {
        format!("{}.{}.{}", value.major, value.minor, value.patch)
    } else {
        CStr::from_ptr(value.string).to_string_lossy().into_owned()
    };
    Version {
        major: value.major,
        minor: value.minor,
        patch: value.patch,
        string,
    }
}

unsafe fn check(result: sys::ctex_result) -> Result<(), Error> {
    if result == sys::CTEX_RESULT_SUCCESS {
        return Ok(());
    }
    let diagnostic = sys::ctex_get_last_diagnostic();
    let diagnostic = if diagnostic.is_null() {
        "CyberTexel operation failed".to_owned()
    } else {
        CStr::from_ptr(diagnostic).to_string_lossy().into_owned()
    };
    Err(Error::Native {
        result: ResultCode::from_raw(result),
        diagnostic_code: sys::ctex_get_last_diagnostic_code(),
        diagnostic,
    })
}

fn c_string(value: &str) -> Result<CString, Error> {
    CString::new(value).map_err(|_| Error::InteriorNul)
}

impl DocumentHandle {
    /// Append a complete ordered batch, validated against the resulting stack.
    pub(crate) fn layer_append(
        &self,
        texture_set_id: &str,
        entries: &[LayerEntry],
    ) -> Result<(), Error> {
        if entries.is_empty() {
            return Err(Error::InvalidNativeState(
                "a layer batch carries at least one entry".into(),
            ));
        }
        let texture_set_id = c_string(texture_set_id)?;
        // Every CString and channel array must outlive the call.
        let mut retained: Vec<CString> = Vec::new();
        let mut channel_storage: Vec<Vec<sys::ctex_layer_channel_descriptor>> = Vec::new();
        for entry in entries {
            let mut channels = Vec::with_capacity(entry.channels.len());
            for channel in &entry.channels {
                let semantic = c_string(&channel.semantic_id)?;
                channels.push(sys::ctex_layer_channel_descriptor {
                    size: std::mem::size_of::<sys::ctex_layer_channel_descriptor>() as u32,
                    semantic_id: semantic.as_ptr(),
                    enabled: u32::from(channel.enabled),
                    opacity: channel.opacity,
                });
                retained.push(semantic);
            }
            channel_storage.push(channels);
        }
        let mut descriptors = Vec::with_capacity(entries.len());
        for (index, entry) in entries.iter().enumerate() {
            let identifier = c_string(&entry.identifier)?;
            let display_name = c_string(&entry.display_name)?;
            let blend_mode = c_string(&entry.blend_mode)?;
            let parent = entry
                .parent_identifier
                .as_deref()
                .map(c_string)
                .transpose()?;
            let target = entry
                .target_identifier
                .as_deref()
                .map(c_string)
                .transpose()?;
            let source = entry
                .source_identifier
                .as_deref()
                .map(c_string)
                .transpose()?;
            let channels = &channel_storage[index];
            descriptors.push(sys::ctex_layer_entry_descriptor {
                size: std::mem::size_of::<sys::ctex_layer_entry_descriptor>() as u32,
                identifier: identifier.as_ptr(),
                display_name: display_name.as_ptr(),
                kind: entry.kind as u32,
                parent_identifier: parent.as_ref().map_or(std::ptr::null(), |v| v.as_ptr()),
                target_identifier: target.as_ref().map_or(std::ptr::null(), |v| v.as_ptr()),
                source_identifier: source.as_ref().map_or(std::ptr::null(), |v| v.as_ptr()),
                enabled: u32::from(entry.enabled),
                opacity: entry.opacity,
                blend_mode: blend_mode.as_ptr(),
                channels: if channels.is_empty() {
                    std::ptr::null()
                } else {
                    channels.as_ptr()
                },
                channel_count: channels.len(),
            });
            retained.push(identifier);
            retained.push(display_name);
            retained.push(blend_mode);
            retained.extend(parent);
            retained.extend(target);
            retained.extend(source);
        }
        unsafe {
            check(sys::ctex_texture_set_layer_append(
                self.0.as_ptr(),
                texture_set_id.as_ptr(),
                descriptors.as_ptr(),
                descriptors.len(),
            ))?;
        }
        drop(retained);
        Ok(())
    }

    /// The canonical stack snapshot as JSON, including its revision.
    pub(crate) fn layer_inspect(&self, texture_set_id: &str) -> Result<String, Error> {
        let texture_set_id = c_string(texture_set_id)?;
        let mut required: usize = 0;
        unsafe {
            check(sys::ctex_texture_set_layer_inspect(
                self.0.as_ptr(),
                texture_set_id.as_ptr(),
                std::ptr::null_mut(),
                0,
                &mut required,
            ))?;
        }
        let mut buffer = vec![0u8; required];
        unsafe {
            check(sys::ctex_texture_set_layer_inspect(
                self.0.as_ptr(),
                texture_set_id.as_ptr(),
                buffer.as_mut_ptr().cast(),
                buffer.len(),
                &mut required,
            ))?;
        }
        let end = buffer
            .iter()
            .position(|byte| *byte == 0)
            .unwrap_or(buffer.len());
        String::from_utf8(buffer[..end].to_vec())
            .map_err(|_| Error::InvalidNativeState("layer snapshot is not UTF-8".into()))
    }
}

#[cfg(test)]
mod abi_tests {
    use super::*;

    #[test]
    fn incompatible_native_abi_names_both_versions() {
        let error = validate_abi(
            Version {
                major: 7,
                minor: 2,
                patch: 1,
                string: "7.2.1-test".into(),
            },
            0,
        )
        .unwrap_err();
        assert_eq!(
            error,
            Error::IncompatibleAbi {
                expected_major: 0,
                native: "7.2.1-test".into(),
            }
        );
    }
}
