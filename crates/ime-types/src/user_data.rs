use crate::{Candidate, Settings};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct UserPhrase {
    pub id: String,
    pub text: String,
    pub reading: String,
    pub translation: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LearningItem {
    pub candidate: Candidate,
    pub status: String,
    pub created_at: i64,
    pub updated_at: i64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DailyCount {
    pub date: String,
    pub count: u64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Statistics {
    pub selections: u64,
    pub favorites: u64,
    pub mastered: u64,
    pub daily: Vec<DailyCount>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct Backup {
    pub schema_version: u32,
    pub settings: Settings,
    pub phrases: Vec<UserPhrase>,
    pub learning: Vec<LearningItem>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AppSnapshot {
    pub settings: Settings,
    pub phrases: Vec<UserPhrase>,
    pub learning: Vec<LearningItem>,
    pub statistics: Statistics,
    pub dictionary_count: usize,
    pub native_registered: bool,
    pub data_path: String,
}
