//! Safe CyberTexel binding over the stable C ABI.

mod ffi;
mod host;

pub use host::*;

use std::cell::Cell;
use std::fmt;
use std::marker::PhantomData;

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Version {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
    pub string: String,
}

impl Version {
    #[must_use]
    pub fn current() -> Self {
        ffi::version()
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ResultCode {
    InvalidArgument,
    MissingResource,
    UnsupportedOperation,
    OutOfMemory,
    OverBudget,
    Cancelled,
    InternalError,
    BufferTooSmall,
    NoUndo,
    NoRedo,
    StaleState,
    Unknown(u32),
}

impl ResultCode {
    fn from_raw(value: u32) -> Self {
        match value {
            1 => Self::InvalidArgument,
            2 => Self::MissingResource,
            3 => Self::UnsupportedOperation,
            4 => Self::OutOfMemory,
            5 => Self::OverBudget,
            6 => Self::Cancelled,
            7 => Self::InternalError,
            8 => Self::BufferTooSmall,
            9 => Self::NoUndo,
            10 => Self::NoRedo,
            11 => Self::StaleState,
            other => Self::Unknown(other),
        }
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Error {
    Native {
        result: ResultCode,
        diagnostic_code: u32,
        diagnostic: String,
    },
    IncompatibleAbi {
        expected_major: u32,
        native: String,
    },
    InteriorNul,
    InvalidNativeState(String),
}

impl fmt::Display for Error {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Native { diagnostic, .. } => formatter.write_str(diagnostic),
            Self::IncompatibleAbi {
                expected_major,
                native,
            } => {
                write!(
                    formatter,
                    "expected ABI major {expected_major}, loaded {native}"
                )
            }
            Self::InteriorNul => formatter.write_str("text input contains an interior NUL byte"),
            Self::InvalidNativeState(message) => formatter.write_str(message),
        }
    }
}

impl std::error::Error for Error {}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct TextureSet {
    pub identifier: String,
    pub width: u32,
    pub height: u32,
}

/// An owned document handle.
///
/// A document is `Send` but deliberately not `Sync`: it may move between
/// threads, while the C ABI requires serialized calls for one document.
///
/// ```compile_fail
/// fn assert_sync<T: Sync>() {}
/// assert_sync::<cybertexel::Document>();
/// ```
pub struct Document {
    handle: ffi::DocumentHandle,
    not_sync: PhantomData<Cell<()>>,
}

impl Document {
    pub fn new() -> Result<Self, Error> {
        Ok(Self {
            handle: ffi::DocumentHandle::create()?,
            not_sync: PhantomData,
        })
    }

    pub fn texture_set_identifiers(&self) -> Result<Vec<String>, Error> {
        self.handle.texture_set_ids()
    }

    pub fn create_texture_set(
        &mut self,
        display_name: &str,
        partition_key: &str,
        width: u32,
        height: u32,
    ) -> Result<TextureSet, Error> {
        self.handle
            .create_texture_set(display_name, partition_key, "uv0", width, height, 8)
    }

    pub fn set_channel_enabled(
        &mut self,
        texture_set: &TextureSet,
        semantic_id: &str,
        bit_depth: u32,
    ) -> Result<(), Error> {
        self.handle
            .set_channel_enabled(&texture_set.identifier, semantic_id, bit_depth)
    }

    pub fn write_channel_pixel(
        &mut self,
        texture_set: &TextureSet,
        semantic_id: &str,
        x: u32,
        y: u32,
        pixel: &[u8],
    ) -> Result<(), Error> {
        if x >= texture_set.width || y >= texture_set.height {
            return Err(Error::InvalidNativeState(
                "pixel coordinate is outside the texture set".into(),
            ));
        }
        self.handle.write_channel_pixel(
            &texture_set.identifier,
            semantic_id,
            texture_set.width,
            texture_set.height,
            x,
            y,
            pixel,
        )
    }

    pub(crate) fn native_handle(&self) -> &ffi::DocumentHandle {
        &self.handle
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn document_can_move_between_threads() {
        fn assert_send<T: Send>() {}
        assert_send::<Document>();
    }

    #[test]
    fn version_and_document_round_trip() {
        assert_eq!(Version::current().major, 0);
        let mut document = Document::new().unwrap();
        let texture_set = document.create_texture_set("Body", "body", 8, 4).unwrap();
        assert_eq!(
            document.texture_set_identifiers().unwrap(),
            [texture_set.identifier]
        );
    }

    #[test]
    fn native_failure_is_typed() {
        let mut document = Document::new().unwrap();
        let error = document.create_texture_set("", "body", 8, 4).unwrap_err();
        match error {
            Error::Native {
                result,
                diagnostic_code,
                diagnostic,
            } => {
                assert_eq!(result, ResultCode::InvalidArgument);
                assert!(diagnostic_code > 0);
                assert!(diagnostic.contains("display_name"));
            }
            unexpected => panic!("unexpected error: {unexpected}"),
        }
    }

    #[test]
    fn host_execution_retains_resident_result_without_readback() {
        let program =
            emit_default_host_material("binding/paint", "paint", 64, 32, ShaderTarget::Wgsl)
                .unwrap();
        assert!(String::from_utf8_lossy(&program.vertex_artifact).contains("@vertex"));
        assert!(program.pass_plan.contains("\"logical_id\":\"paint\""));
        let mut session = HostExecutionSession::new(0).unwrap();
        let source = HostResource {
            logical_id: "source".into(),
            generation: 1,
            role: "input".into(),
            format: 2,
            width: 64,
            height: 32,
            layers: 1,
            mip_levels: 1,
            tile_width: 64,
            tile_height: 32,
            externally_initialized: true,
            owner: ResourceOwner::Host,
            required_state: ResourceState::ShaderRead,
            output: false,
        };
        let output = HostResource {
            logical_id: "paint".into(),
            generation: 1,
            role: "output".into(),
            externally_initialized: false,
            required_state: ResourceState::RenderTarget,
            output: true,
            ..source.clone()
        };
        let token = session
            .submit(
                "paint",
                0,
                &[source, output],
                ReplaySemantics::Deterministic,
            )
            .unwrap();
        let result = session
            .complete(
                token,
                &[CompletedHostResource {
                    logical_id: "paint".into(),
                    generation: 1,
                    format: 2,
                    width: 64,
                    height: 32,
                    layers: 1,
                }],
                Some(&HostRecovery {
                    operation_record_version: "paint-v1".into(),
                    checkpoint_revision: 0,
                    retained_bytes: 96,
                    inputs_pinned: true,
                }),
            )
            .unwrap();
        assert_eq!(result.disposition, CompletionDisposition::Published);
        assert_eq!(result.published_revision, Some(1));
        assert_eq!(session.committed_generation("paint").unwrap(), Some(1));
        assert!(session.resource_is_held("paint", 1).unwrap());
    }

    #[test]
    fn explicit_host_readback_publishes_only_after_completion() {
        let mut document = Document::new().unwrap();
        let texture_set = document
            .create_texture_set("Readback", "readback", 8, 4)
            .unwrap();
        document
            .set_channel_enabled(&texture_set, "pbr.base_color", 0)
            .unwrap();
        let pool = SnapshotPool::new(64 * 64 * 3).unwrap();
        let cursor = pool
            .current_cursor(&document, &texture_set, "pbr.base_color")
            .unwrap();
        document
            .write_channel_pixel(&texture_set, "pbr.base_color", 1, 2, &[7, 11, 13])
            .unwrap();
        let snapshot = pool
            .snapshot(&document, &texture_set, "pbr.base_color", cursor)
            .unwrap();
        let mut readback = snapshot.begin_host_readback().unwrap();
        assert_eq!(readback.status().unwrap(), ReadbackStatus::Pending);
        assert!(readback.tiles().is_err());
        let payloads = readback
            .tile_byte_sizes()
            .into_iter()
            .map(|size| vec![42_u8; size])
            .collect::<Vec<_>>();
        readback.complete(&payloads).unwrap();
        assert_eq!(readback.status().unwrap(), ReadbackStatus::Complete);
        assert_eq!(readback.tiles().unwrap(), payloads);
    }
}
