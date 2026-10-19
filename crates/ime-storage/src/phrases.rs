use crate::{Storage, db_error};
use ime_types::{AppError, AppResult, UserPhrase};
use rusqlite::params;
use std::collections::HashSet;

pub fn validate_phrase(phrase: &UserPhrase) -> AppResult<()> {
    let key = ime_core::normalize(&phrase.reading)?;
    if phrase.id.is_empty()
        || phrase.id.len() > 96
        || phrase.text.trim().is_empty()
        || phrase.text.chars().count() > 32
        || key.is_empty()
        || phrase.translation.as_ref().is_some_and(|s| s.len() > 512)
        || phrase.text.chars().any(char::is_control)
    {
        return Err(AppError::new(
            "INVALID_PHRASE",
            "词条须为 1–32 个字符，拼音和 ID 不能为空，释义不超过 512 字节",
        ));
    }
    Ok(())
}

impl Storage {
    pub fn phrases(&self) -> AppResult<Vec<UserPhrase>> {
        let mut query = self
            .connection
            .prepare("SELECT id,text,reading,translation FROM user_phrases ORDER BY rowid DESC")
            .map_err(db_error)?;
        query
            .query_map([], |r| {
                Ok(UserPhrase {
                    id: r.get(0)?,
                    text: r.get(1)?,
                    reading: r.get(2)?,
                    translation: r.get(3)?,
                })
            })
            .map_err(db_error)?
            .collect::<Result<Vec<_>, _>>()
            .map_err(db_error)
    }

    pub fn save_phrase(&self, phrase: &UserPhrase) -> AppResult<()> {
        validate_phrase(phrase)?;
        self.connection.execute("INSERT INTO user_phrases VALUES (?1,?2,?3,?4) ON CONFLICT(id) DO UPDATE SET text=excluded.text,reading=excluded.reading,translation=excluded.translation",
            params![phrase.id, phrase.text.trim(), phrase.reading.trim(), phrase.translation]).map_err(db_error)?;
        Ok(())
    }

    pub fn delete_phrase(&self, id: &str) -> AppResult<()> {
        self.connection
            .execute("DELETE FROM user_phrases WHERE id=?1", [id])
            .map_err(db_error)?;
        Ok(())
    }

    pub fn import_phrases(&mut self, phrases: &[UserPhrase]) -> AppResult<usize> {
        if phrases.len() > 10_000 {
            return Err(AppError::new("IMPORT_LIMIT", "单次最多导入 10000 条"));
        }
        let mut seen = HashSet::new();
        for phrase in phrases {
            validate_phrase(phrase)?;
            if !seen.insert((&phrase.text, &phrase.reading)) {
                return Err(AppError::new("DUPLICATE_PHRASE", "导入文件存在重复词条"));
            }
        }
        let transaction = self.connection.transaction().map_err(db_error)?;
        for phrase in phrases {
            transaction.execute("INSERT INTO user_phrases VALUES (?1,?2,?3,?4) ON CONFLICT(text,reading) DO UPDATE SET translation=excluded.translation",
                params![phrase.id,phrase.text,phrase.reading,phrase.translation]).map_err(db_error)?;
        }
        transaction.commit().map_err(db_error)?;
        Ok(phrases.len())
    }
}
