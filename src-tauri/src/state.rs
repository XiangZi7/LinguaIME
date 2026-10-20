use ime_core::PinyinEngine;
use ime_rime::{InputEngine, default_runtime_directory};
use ime_storage::{Storage, default_data_path};
use ime_types::{AppError, AppResult};
use std::{
    path::Path,
    sync::{Arc, Mutex, MutexGuard},
};

pub struct AppState {
    pub engine: InputEngine,
    pub storage: Mutex<Storage>,
    pub data_path: String,
}

impl AppState {
    pub fn new(runtime_directory: Option<&Path>) -> AppResult<Self> {
        let path = default_data_path();
        Ok(Self {
            engine: InputEngine::new(
                Arc::new(PinyinEngine::default()),
                &runtime_directory
                    .map(Path::to_path_buf)
                    .unwrap_or_else(default_runtime_directory),
                path.parent()
                    .ok_or_else(|| AppError::new("DATA_DIRECTORY", "数据路径无效"))?,
            )?,
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
