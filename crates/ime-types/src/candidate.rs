use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Candidate {
    pub id: String,
    pub text: String,
    pub reading: String,
    pub key: String,
    pub translation: Option<String>,
    pub source: String,
    pub frequency: u64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CandidatePage {
    pub schema_version: u32,
    pub composition: String,
    pub items: Vec<Candidate>,
    pub page_index: usize,
    pub page_count: usize,
    pub total: usize,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EngineRequest {
    pub composition: String,
    pub page: usize,
    pub page_size: usize,
}
