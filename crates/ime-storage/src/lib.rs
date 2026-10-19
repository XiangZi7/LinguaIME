mod learning;
mod phrases;
mod settings;
mod snapshot;

use ime_types::{AppError, AppResult};
use rusqlite::Connection;
use std::{path::{Path, PathBuf}, time::Duration};

pub struct Storage { connection: Connection }

impl Storage {
    pub fn open(path: &Path) -> AppResult<Self> {
        if let Some(parent) = path.parent() { std::fs::create_dir_all(parent).map_err(|_| AppError::new("DATA_DIRECTORY", "无法创建本地数据目录"))?; }
        Self::initialize(Connection::open(path).map_err(db_error)?)
    }

    fn initialize(connection: Connection) -> AppResult<Self> {
        connection.busy_timeout(Duration::from_millis(5)).map_err(db_error)?;
        connection.pragma_update(None, "journal_mode", "WAL").map_err(db_error)?;
        connection.pragma_update(None, "foreign_keys", "ON").map_err(db_error)?;
        let version: u32 = connection.pragma_query_value(None, "user_version", |row| row.get(0)).map_err(db_error)?;
        if version > 1 { return Err(AppError::new("NEWER_DATABASE", "数据由更新版本创建，请升级应用")); }
        if version == 0 {
            connection.execute_batch(include_str!("../migrations/001_initial.sql")).map_err(db_error)?;
        }
        Ok(Self { connection })
    }
}

pub fn default_data_path() -> PathBuf {
    let base = std::env::var_os("LOCALAPPDATA").map(PathBuf::from)
        .or_else(|| std::env::var_os("XDG_DATA_HOME").map(PathBuf::from))
        .unwrap_or_else(std::env::temp_dir);
    base.join("LinguaIME").join("lingua.db")
}

fn db_error(error: rusqlite::Error) -> AppError {
    // Only classify the failure; database messages may contain personal text or paths.
    let code = match error {
        rusqlite::Error::SqliteFailure(ref e, _) if e.code == rusqlite::ErrorCode::DatabaseBusy => "DATABASE_BUSY",
        _ => "DATABASE_ERROR",
    };
    AppError::new(code, "本地数据库暂不可用，请重试或从备份恢复")
}

pub fn timestamp() -> i64 {
    std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|v| v.as_secs() as i64).unwrap_or(0)
}

#[cfg(test)]
mod tests;
