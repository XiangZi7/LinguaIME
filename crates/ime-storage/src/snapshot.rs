use crate::{Storage, db_error};
use ime_types::{AppResult, AppSnapshot, Backup, DailyCount, SCHEMA_VERSION, Statistics};

impl Storage {
    pub fn statistics(&self) -> AppResult<Statistics> {
        let count = |sql: &str| self.connection.query_row(sql, [], |r| Ok(r.get::<_,i64>(0)?.max(0) as u64)).map_err(db_error);
        let mut query = self.connection.prepare("SELECT date(created_at,'unixepoch','localtime'),count(*) FROM learning_events WHERE event_type='selection' AND created_at >= strftime('%s','now','-6 days','start of day') GROUP BY 1 ORDER BY 1").map_err(db_error)?;
        let daily = query.query_map([], |r| Ok(DailyCount { date:r.get(0)?,count:r.get::<_,i64>(1)?.max(0) as u64 })).map_err(db_error)?.collect::<Result<Vec<_>,_>>().map_err(db_error)?;
        Ok(Statistics { selections:count("SELECT count(*) FROM learning_events WHERE event_type='selection'")?,
            favorites:count("SELECT count(*) FROM learning_items")?, mastered:count("SELECT count(*) FROM learning_items WHERE status='mastered'")?, daily })
    }

    pub fn snapshot(&self, dictionary_count: usize, native_registered: bool, data_path: String) -> AppResult<AppSnapshot> {
        Ok(AppSnapshot { settings:self.settings()?, phrases:self.phrases()?, learning:self.learning()?,
            statistics:self.statistics()?,dictionary_count,native_registered,data_path })
    }

    pub fn backup(&self) -> AppResult<Backup> {
        Ok(Backup { schema_version:SCHEMA_VERSION,settings:self.settings()?,phrases:self.phrases()?,learning:self.learning()? })
    }
}
