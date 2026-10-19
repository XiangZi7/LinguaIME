use crate::{Storage, db_error, timestamp};
use ime_types::{AppError, AppResult, Candidate, LearningItem};
use rusqlite::params;
use std::collections::HashMap;

impl Storage {
    pub fn frequencies(&self) -> AppResult<HashMap<String,u64>> {
        let mut query = self.connection.prepare("SELECT entry_id,selection_count FROM user_word_frequency").map_err(db_error)?;
        query.query_map([], |r| Ok((r.get(0)?, r.get::<_,i64>(1)?.max(0) as u64))).map_err(db_error)?
            .collect::<Result<HashMap<_,_>,_>>().map_err(db_error)
    }

    pub fn record_selection(&mut self, id: &str) -> AppResult<()> {
        if id.len() > 256 || id.is_empty() { return Err(AppError::new("INVALID_ID", "词条标识无效")); }
        let settings = self.settings()?;
        let transaction = self.connection.transaction().map_err(db_error)?;
        if settings.auto_learn {
            transaction.execute("INSERT INTO user_word_frequency VALUES (?1,1,?2) ON CONFLICT(entry_id) DO UPDATE SET selection_count=selection_count+1,last_selected_at=excluded.last_selected_at", params![id,timestamp()]).map_err(db_error)?;
        }
        if settings.collect_stats {
            transaction.execute("INSERT INTO learning_events (event_type,created_at) VALUES ('selection',?1)", [timestamp()]).map_err(db_error)?;
        }
        transaction.commit().map_err(db_error)?;
        Ok(())
    }

    pub fn learning(&self) -> AppResult<Vec<LearningItem>> {
        let mut query = self.connection.prepare("SELECT candidate_json,status,created_at,updated_at FROM learning_items ORDER BY created_at DESC").map_err(db_error)?;
        let records = query.query_map([], |r| Ok((r.get::<_,String>(0)?,r.get::<_,String>(1)?,r.get(2)?,r.get(3)?))).map_err(db_error)?;
        records.map(|record| {
            let (json,status,created_at,updated_at) = record.map_err(db_error)?;
            let candidate = serde_json::from_str(&json).map_err(|_| AppError::new("INVALID_LEARNING", "学习数据损坏"))?;
            Ok(LearningItem { candidate,status,created_at,updated_at })
        }).collect()
    }

    pub fn save_learning(&self, candidate: &Candidate, status: &str) -> AppResult<()> {
        if !["review", "mastered"].contains(&status) || candidate.id.is_empty() || candidate.id.len() > 256 || candidate.text.len() > 512 || candidate.translation.as_ref().is_some_and(|s| s.len() > 4096) {
            return Err(AppError::new("INVALID_LEARNING", "词条或学习状态无效"));
        }
        let json = serde_json::to_string(candidate).map_err(|_| AppError::new("SERIALIZE", "无法编码词条"))?;
        self.connection.execute("INSERT INTO learning_items VALUES (?1,?2,?3,?4,?4) ON CONFLICT(id) DO UPDATE SET status=excluded.status,updated_at=excluded.updated_at", params![candidate.id,json,status,timestamp()]).map_err(db_error)?;
        Ok(())
    }

    pub fn delete_learning(&self, id: &str) -> AppResult<()> {
        self.connection.execute("DELETE FROM learning_items WHERE id=?1", [id]).map_err(db_error)?;
        Ok(())
    }

    pub fn clear_data(&mut self, scope: &str) -> AppResult<()> {
        let transaction = self.connection.transaction().map_err(db_error)?;
        match scope {
            "frequency" => { transaction.execute("DELETE FROM user_word_frequency", []).map_err(db_error)?; }
            "learning" => { transaction.execute("DELETE FROM learning_items", []).map_err(db_error)?; transaction.execute("DELETE FROM learning_events", []).map_err(db_error)?; }
            "all" => { transaction.execute_batch("DELETE FROM user_word_frequency; DELETE FROM learning_items; DELETE FROM learning_events; DELETE FROM user_phrases; DELETE FROM app_settings;").map_err(db_error)?; }
            _ => return Err(AppError::new("INVALID_SCOPE", "未知清除范围")),
        }
        transaction.commit().map_err(db_error)?;
        Ok(())
    }
}
