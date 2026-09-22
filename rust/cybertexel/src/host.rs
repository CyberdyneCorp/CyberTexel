use std::rc::Rc;

use crate::{ffi, Document, Error, TextureSet};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ShaderTarget {
    Wgsl = 0,
    Msl = 1,
    SpirV = 2,
    Hlsl = 3,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct HostMaterialProgram {
    pub target: ShaderTarget,
    pub vertex_artifact: Vec<u8>,
    pub fragment_artifact: Vec<u8>,
    pub pass_plan: String,
    pub workaround_report: String,
}

pub fn emit_default_host_material(
    stable_identity: &str,
    output_identity: &str,
    width: u32,
    height: u32,
    target: ShaderTarget,
) -> Result<HostMaterialProgram, Error> {
    ffi::emit_default_host_material(stable_identity, output_identity, width, height, target)
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ResourceOwner {
    Library = 0,
    Host = 1,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ResourceState {
    ShaderRead = 0,
    StorageRead = 1,
    StorageWrite = 2,
    RenderTarget = 3,
    DepthTarget = 4,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ReplaySemantics {
    Deterministic = 0,
    CheckpointOnly = 1,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum CompletionDisposition {
    Published,
    AwaitingRecovery,
    Stale,
    Cancelled,
    Failed,
    Rejected,
    Duplicate,
    UnknownToken,
}

impl CompletionDisposition {
    pub(crate) fn from_raw(value: u32) -> Result<Self, Error> {
        match value {
            0 => Ok(Self::Published),
            1 => Ok(Self::AwaitingRecovery),
            2 => Ok(Self::Stale),
            3 => Ok(Self::Cancelled),
            4 => Ok(Self::Failed),
            5 => Ok(Self::Rejected),
            6 => Ok(Self::Duplicate),
            7 => Ok(Self::UnknownToken),
            _ => Err(Error::InvalidNativeState(
                "native returned an unknown completion disposition".into(),
            )),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ReadbackStatus {
    Pending,
    Complete,
    Cancelled,
    Failed,
}

impl ReadbackStatus {
    pub(crate) fn from_raw(value: u32) -> Result<Self, Error> {
        match value {
            0 => Ok(Self::Pending),
            1 => Ok(Self::Complete),
            2 => Ok(Self::Cancelled),
            3 => Ok(Self::Failed),
            _ => Err(Error::InvalidNativeState(
                "native returned an unknown readback status".into(),
            )),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct RevisionCursor {
    pub epoch: u64,
    pub revision: u64,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct HostResource {
    pub logical_id: String,
    pub generation: u64,
    pub role: String,
    pub format: u32,
    pub width: u32,
    pub height: u32,
    pub layers: u32,
    pub mip_levels: u32,
    pub tile_width: u32,
    pub tile_height: u32,
    pub externally_initialized: bool,
    pub owner: ResourceOwner,
    pub required_state: ResourceState,
    pub output: bool,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct CompletedHostResource {
    pub logical_id: String,
    pub generation: u64,
    pub format: u32,
    pub width: u32,
    pub height: u32,
    pub layers: u32,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct HostRecovery {
    pub operation_record_version: String,
    pub checkpoint_revision: u64,
    pub retained_bytes: usize,
    pub inputs_pinned: bool,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ReleasedResource {
    pub logical_id: String,
    pub generation: u64,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct HostCompletionResult {
    pub disposition: CompletionDisposition,
    pub completion_token: u64,
    pub published_revision: Option<u64>,
    pub released_resources: Vec<ReleasedResource>,
    pub message: String,
}

pub struct HostExecutionSession {
    handle: ffi::HostExecutionHandle,
}

impl HostExecutionSession {
    pub fn new(initial_revision: u64) -> Result<Self, Error> {
        Ok(Self {
            handle: ffi::HostExecutionHandle::create(initial_revision)?,
        })
    }

    pub fn submit(
        &mut self,
        operation: &str,
        base_revision: u64,
        resources: &[HostResource],
        replay: ReplaySemantics,
    ) -> Result<u64, Error> {
        self.handle
            .submit(operation, base_revision, resources, replay)
    }

    pub fn complete(
        &mut self,
        token: u64,
        outputs: &[CompletedHostResource],
        recovery: Option<&HostRecovery>,
    ) -> Result<HostCompletionResult, Error> {
        self.handle.complete(token, outputs, recovery)
    }

    pub fn committed_generation(&self, logical_id: &str) -> Result<Option<u64>, Error> {
        self.handle.committed_generation(logical_id)
    }

    pub fn resource_is_held(&self, logical_id: &str, generation: u64) -> Result<bool, Error> {
        self.handle.resource_is_held(logical_id, generation)
    }
}

pub struct SnapshotPool {
    handle: Rc<ffi::SnapshotPoolHandle>,
}

impl SnapshotPool {
    pub fn new(budget_bytes: usize) -> Result<Self, Error> {
        Ok(Self {
            handle: Rc::new(ffi::SnapshotPoolHandle::create(budget_bytes)?),
        })
    }

    pub fn current_cursor(
        &self,
        document: &Document,
        texture_set: &TextureSet,
        semantic_id: &str,
    ) -> Result<RevisionCursor, Error> {
        self.handle.current_cursor(
            document.native_handle(),
            &texture_set.identifier,
            semantic_id,
        )
    }

    pub fn snapshot(
        &self,
        document: &Document,
        texture_set: &TextureSet,
        semantic_id: &str,
        since: RevisionCursor,
    ) -> Result<TransportSnapshot, Error> {
        Ok(TransportSnapshot {
            handle: self.handle.snapshot(
                document.native_handle(),
                &texture_set.identifier,
                semantic_id,
                since,
            )?,
            _pool: Rc::clone(&self.handle),
        })
    }
}

pub struct TransportSnapshot {
    handle: ffi::SnapshotHandle,
    _pool: Rc<ffi::SnapshotPoolHandle>,
}

impl TransportSnapshot {
    pub fn begin_host_readback(self) -> Result<HostReadback, Error> {
        let handle = self.handle.begin_host_readback()?;
        Ok(HostReadback {
            handle,
            _snapshot: self,
        })
    }
}

pub struct HostReadback {
    handle: ffi::ReadbackHandle,
    _snapshot: TransportSnapshot,
}

impl HostReadback {
    pub fn status(&self) -> Result<ReadbackStatus, Error> {
        self.handle.status()
    }

    #[must_use]
    pub fn tile_byte_sizes(&self) -> Vec<usize> {
        self.handle.tile_byte_sizes()
    }

    pub fn complete(&mut self, tiles: &[Vec<u8>]) -> Result<(), Error> {
        self.handle.complete(tiles)
    }

    pub fn tiles(&self) -> Result<Vec<&[u8]>, Error> {
        if self.status()? != ReadbackStatus::Complete {
            return Err(Error::InvalidNativeState(
                "readback output is not readable".into(),
            ));
        }
        Ok(self.handle.tiles())
    }
}
