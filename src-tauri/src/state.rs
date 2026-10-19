use ime_core::PinyinEngine;
use ime_storage::{Storage, default_data_path};
use ime_types::{AppError, AppResult};
use std::sync::{Mutex, MutexGuard};

pub struct AppState {
    pub engine: PinyinEngine,
    pub storage: Mutex<Storage>,
    pub data_path: String,
}

impl AppState {
    pub fn new() -> AppResult<Self> {
        let path = default_data_path();
        Ok(Self {
            engine: PinyinEngine::default(),
            storage: Mutex::new(Storage::open(&path)?),
            data_path: path.to_string_lossy().into(),
        })
    }
    pub fn storage(&self) -> AppResult<MutexGuard<'_, Storage>> {
        self.storage
            .lock()
            .map_err(|_| AppError::new("STORAGE_LOCK", "存储服务不可用，请重新启动管理应用"))
    }
}

pub use crate::native_status::native_registered;
