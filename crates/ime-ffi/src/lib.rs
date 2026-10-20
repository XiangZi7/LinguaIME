//! ABI v1: UTF-8, caller-owned fixed-size output, opaque thread-affine context.
//! All pointer arguments must be valid for the documented lifetime; null is checked.
mod types;
pub use types::{FfiCandidate, FfiConfig, FfiPage};

use ime_core::PinyinEngine;
use ime_rime::{InputEngine, default_runtime_directory};
use ime_storage::{Storage, default_data_path};
use ime_types::{EngineRequest, Settings, UserPhrase};
use std::{
    collections::HashMap,
    ffi::{CStr, c_char},
    panic::{AssertUnwindSafe, catch_unwind},
    sync::{Arc, Mutex, OnceLock},
};
use types::copy_utf8;

static ENGINE_CACHE: OnceLock<Mutex<Option<Arc<PinyinEngine>>>> = OnceLock::new();

fn shared_engine() -> Arc<PinyinEngine> {
    let mut cache = ENGINE_CACHE
        .get_or_init(|| Mutex::new(None))
        .lock()
        .unwrap_or_else(std::sync::PoisonError::into_inner);
    cache
        .get_or_insert_with(|| Arc::new(PinyinEngine::default()))
        .clone()
}

pub struct EngineContext {
    engine: InputEngine,
    storage: Option<Storage>,
    settings: Settings,
    phrases: Vec<UserPhrase>,
    frequencies: HashMap<String, u64>,
}

fn guard(action: impl FnOnce() -> i32) -> i32 {
    catch_unwind(AssertUnwindSafe(action)).unwrap_or(-3)
}

#[unsafe(no_mangle)]
pub extern "C" fn lingua_abi_version() -> u32 {
    1
}

/// Release the optional shared cache before a runtime generation is unloaded.
/// Existing contexts retain their own Arc; calls are serialized with cache creation.
#[unsafe(no_mangle)]
pub extern "C" fn lingua_release_cache() {
    let _ = catch_unwind(|| {
        if let Some(cache) = ENGINE_CACHE.get() {
            cache
                .lock()
                .unwrap_or_else(std::sync::PoisonError::into_inner)
                .take();
        }
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn lingua_create() -> *mut EngineContext {
    catch_unwind(|| {
        let data_path = default_data_path();
        let storage = Storage::open(&data_path).ok();
        let engine = InputEngine::new(
            shared_engine(),
            &default_runtime_directory(),
            data_path.parent()?,
        )
        .ok()?;
        let mut context = Box::new(EngineContext {
            engine,
            storage,
            settings: Settings::default(),
            phrases: Vec::new(),
            frequencies: HashMap::new(),
        });
        context.refresh();
        Some(Box::into_raw(context))
    })
    .ok()
    .flatten()
    .unwrap_or(std::ptr::null_mut())
}

impl EngineContext {
    fn refresh(&mut self) {
        if let Some(storage) = &self.storage {
            if let Ok(settings) = storage.settings() {
                self.settings = settings;
            }
            if let Ok(phrases) = storage.phrases() {
                self.phrases = phrases;
            }
            if let Ok(frequencies) = storage.frequencies() {
                self.frequencies = frequencies;
            }
        }
    }
}

/// # Safety
/// `context` must be a live handle returned by lingua_create, used on its creating thread.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn lingua_destroy(context: *mut EngineContext) {
    let _ = catch_unwind(AssertUnwindSafe(|| {
        if !context.is_null() {
            drop(unsafe { Box::from_raw(context) });
        }
    }));
}

/// # Safety
/// Valid live context and a writable, aligned FfiConfig output are required.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn lingua_refresh(
    context: *mut EngineContext,
    output: *mut FfiConfig,
) -> i32 {
    guard(|| {
        if context.is_null() || output.is_null() {
            return -1;
        }
        let context = unsafe { &mut *context };
        context.refresh();
        let s = &context.settings;
        let previous = match s.previous_page_key.as_str() {
            "PageUp" => 0x21,
            "[" => 0xDB,
            _ => 0xBD,
        };
        let next = match s.next_page_key.as_str() {
            "PageDown" => 0x22,
            "]" => 0xDD,
            _ => 0xBB,
        };
        unsafe {
            *output = FfiConfig {
                page_size: s.page_size as u32,
                display_mode: match s.display_mode.as_str() {
                    "chinese" => 0,
                    "detailed" => 2,
                    _ => 1,
                },
                theme: match s.theme.as_str() {
                    "dark" => 2,
                    "light" => 1,
                    _ => 0,
                },
                accent: match s.accent.as_str() {
                    "green" => 0,
                    "blue" => 1,
                    _ => 2,
                },
                font_size: s.font_size,
                compact: u32::from(s.row_spacing == "compact"),
                chinese_punctuation: u32::from(s.chinese_punctuation),
                enter_raw: u32::from(s.enter_behavior == "raw"),
                previous_key: previous,
                next_key: next,
            };
        }
        0
    })
}

/// # Safety
/// Context must be live; composition must be NUL-terminated UTF-8, output writable/aligned.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn lingua_query(
    context: *mut EngineContext,
    composition: *const c_char,
    page: u32,
    output: *mut FfiPage,
) -> i32 {
    guard(|| {
        if context.is_null() || composition.is_null() || output.is_null() {
            return -1;
        }
        let context = unsafe { &mut *context };
        let Ok(composition) = unsafe { CStr::from_ptr(composition) }.to_str() else {
            return -2;
        };
        let request = EngineRequest {
            composition: composition.into(),
            page: page as usize,
            page_size: context.settings.page_size,
        };
        let Ok(result) = context
            .engine
            .query(&request, &context.phrases, &context.frequencies)
        else {
            return -2;
        };
        let empty = FfiCandidate {
            id: [0; 256],
            text: [0; 256],
            reading: [0; 256],
            translation: [0; 768],
        };
        let mut output_page = FfiPage {
            schema_version: 1,
            item_count: result.items.len() as u32,
            page_index: result.page_index as u32,
            page_count: result.page_count as u32,
            items: [empty; 9],
        };
        for (index, item) in result.items.iter().enumerate() {
            output_page.items[index] = FfiCandidate {
                id: copy_utf8(&item.id),
                text: copy_utf8(&item.text),
                reading: copy_utf8(&item.reading),
                translation: copy_utf8(item.translation.as_deref().unwrap_or("")),
            };
        }
        unsafe {
            *output = output_page;
        }
        0
    })
}

/// # Safety
/// Context must be live; ID must be a NUL-terminated UTF-8 candidate identifier.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn lingua_record(context: *mut EngineContext, id: *const c_char) -> i32 {
    guard(|| {
        if context.is_null() || id.is_null() {
            return -1;
        }
        let context = unsafe { &mut *context };
        let Ok(id) = unsafe { CStr::from_ptr(id) }.to_str() else {
            return -2;
        };
        match &mut context.storage {
            Some(storage) => match storage.record_selection(id) {
                Ok(()) => {
                    if context.settings.auto_learn {
                        *context.frequencies.entry(id.to_owned()).or_default() += 1;
                    }
                    0
                }
                Err(_) => -4,
            },
            None => -4,
        }
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn cache_release_preserves_live_engines() {
        let first = shared_engine();
        let second = shared_engine();
        assert!(Arc::ptr_eq(&first, &second));
        lingua_release_cache();
        let replacement = shared_engine();
        assert!(!Arc::ptr_eq(&first, &replacement));
        assert_eq!(first.count(), replacement.count());
        assert!(first.count() > 100_000);
    }

    #[test]
    fn abi_layout_and_utf8_are_stable() {
        assert_eq!(std::mem::size_of::<FfiCandidate>(), 1536);
        assert_eq!(std::mem::size_of::<FfiPage>(), 13840);
        assert_eq!(std::mem::size_of::<FfiConfig>(), 40);
        let value = copy_utf8::<5>("学习");
        assert_eq!(std::str::from_utf8(&value[..3]).expect("utf8"), "学");
        assert_eq!(
            unsafe {
                lingua_query(
                    std::ptr::null_mut(),
                    std::ptr::null(),
                    0,
                    std::ptr::null_mut(),
                )
            },
            -1
        );
        assert_eq!(guard(|| panic!("test isolation")), -3);
    }
}
