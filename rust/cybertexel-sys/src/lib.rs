//! Raw declarations for the stable CyberTexel C ABI.

#![allow(non_camel_case_types)]

use std::ffi::{c_char, c_void};

pub type ctex_result = u32;
pub const CTEX_RESULT_SUCCESS: ctex_result = 0;
pub const CTEX_RESULT_INVALID_ARGUMENT: ctex_result = 1;
pub const CTEX_RESULT_MISSING_RESOURCE: ctex_result = 2;
pub const CTEX_RESULT_UNSUPPORTED_OPERATION: ctex_result = 3;
pub const CTEX_RESULT_OUT_OF_MEMORY: ctex_result = 4;
pub const CTEX_RESULT_OVER_BUDGET: ctex_result = 5;
pub const CTEX_RESULT_CANCELLED: ctex_result = 6;
pub const CTEX_RESULT_INTERNAL_ERROR: ctex_result = 7;
pub const CTEX_RESULT_BUFFER_TOO_SMALL: ctex_result = 8;
pub const CTEX_RESULT_NO_UNDO: ctex_result = 9;
pub const CTEX_RESULT_NO_REDO: ctex_result = 10;
pub const CTEX_RESULT_STALE_STATE: ctex_result = 11;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ctex_version {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
    pub string: *const c_char,
}

#[repr(C)]
pub struct ctex_document {
    _private: [u8; 0],
}

#[repr(C)]
pub struct ctex_texture_set_descriptor {
    pub size: u32,
    pub display_name: *const c_char,
    pub partition_kind: u32,
    pub partition_key: *const c_char,
    pub uv_set: *const c_char,
    pub width: u32,
    pub height: u32,
    pub default_bit_depth: u8,
    pub udim_tiling: u32,
}

extern "C" {
    pub fn ctex_get_version() -> ctex_version;
    pub fn ctex_get_abi_version() -> ctex_version;
    pub fn ctex_get_last_diagnostic() -> *const c_char;
    pub fn ctex_get_last_diagnostic_code() -> u32;
    pub fn ctex_document_create(out_document: *mut *mut ctex_document) -> ctex_result;
    pub fn ctex_document_destroy(document: *mut ctex_document);
    pub fn ctex_document_create_texture_set(
        document: *mut ctex_document,
        descriptor: *const ctex_texture_set_descriptor,
    ) -> ctex_result;
    pub fn ctex_document_get_texture_set_ids(
        document: *const ctex_document,
        buffer: *mut c_char,
        buffer_size: usize,
        out_required_size: *mut usize,
        out_count: *mut usize,
    ) -> ctex_result;
}

pub type ctex_user_data = *mut c_void;
