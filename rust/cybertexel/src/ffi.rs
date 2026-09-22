//! Audited unsafe boundary between safe Rust values and `cybertexel-sys`.

use std::ffi::{CStr, CString};
use std::ptr::{self, NonNull};

use cybertexel_sys as sys;

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
    if version.major == 0 {
        Ok(())
    } else {
        Err(Error::IncompatibleAbi {
            expected_major: 0,
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
