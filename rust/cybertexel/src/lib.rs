//! Safe CyberTexel binding over the stable C ABI.

mod ffi;

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
}
