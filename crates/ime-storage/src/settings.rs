use crate::{Storage, db_error};
use ime_types::{AppError, AppResult, Settings};
use rusqlite::{OptionalExtension, params};

impl Storage {
    pub fn settings(&self) -> AppResult<Settings> {
        let value: Option<String> = self
            .connection
            .query_row(
                "SELECT value_json FROM app_settings WHERE key='settings'",
                [],
                |r| r.get(0),
            )
            .optional()
            .map_err(db_error)?;
        let settings: Settings = match value {
            Some(json) => serde_json::from_str(&json)
                .map_err(|_| AppError::new("INVALID_CONFIG", "设置数据已损坏，请恢复备份"))?,
            None => Settings::default(),
        };
        settings.validate()?;
        Ok(settings)
    }

    pub fn save_settings(&self, settings: &Settings) -> AppResult<()> {
        settings.validate()?;
        let json = serde_json::to_string(settings)
            .map_err(|_| AppError::new("SERIALIZE", "无法编码设置"))?;
        self.connection.execute("INSERT INTO app_settings VALUES ('settings', ?1) ON CONFLICT(key) DO UPDATE SET value_json=excluded.value_json", params![json]).map_err(db_error)?;
        Ok(())
    }
}
