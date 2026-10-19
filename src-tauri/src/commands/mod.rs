use crate::state::{AppState, native_registered};
use ime_types::{AppResult, AppSnapshot, Backup, Candidate, CandidatePage, EngineRequest, Settings, UserPhrase};
use tauri::State;

#[tauri::command]
pub fn get_snapshot(state: State<'_, AppState>) -> AppResult<AppSnapshot> {
    state.storage()?.snapshot(state.engine.count(),native_registered(),state.data_path.clone())
}

#[tauri::command]
pub fn query_candidates(request: EngineRequest, state: State<'_, AppState>) -> AppResult<CandidatePage> {
    let storage=state.storage()?;
    state.engine.query(&request,&storage.phrases()?,&storage.frequencies()?)
}

#[tauri::command]
pub fn save_settings(settings: Settings, state: State<'_, AppState>) -> AppResult<()> { state.storage()?.save_settings(&settings) }

#[tauri::command]
pub fn save_phrase(phrase: UserPhrase, state: State<'_, AppState>) -> AppResult<()> { state.storage()?.save_phrase(&phrase) }

#[tauri::command]
pub fn delete_phrase(id: String, state: State<'_, AppState>) -> AppResult<()> { state.storage()?.delete_phrase(&id) }

#[tauri::command]
pub fn import_phrases(phrases: Vec<UserPhrase>, state: State<'_, AppState>) -> AppResult<usize> { state.storage()?.import_phrases(&phrases) }

#[tauri::command]
pub fn record_selection(id: String, state: State<'_, AppState>) -> AppResult<()> { state.storage()?.record_selection(&id) }

#[tauri::command]
pub fn save_learning(candidate: Candidate, status: String, state: State<'_, AppState>) -> AppResult<()> { state.storage()?.save_learning(&candidate,&status) }

#[tauri::command]
pub fn delete_learning(id: String, state: State<'_, AppState>) -> AppResult<()> { state.storage()?.delete_learning(&id) }

#[tauri::command]
pub fn export_data(state: State<'_, AppState>) -> AppResult<Backup> { state.storage()?.backup() }

#[tauri::command]
pub fn clear_data(scope: String, state: State<'_, AppState>) -> AppResult<()> { state.storage()?.clear_data(&scope) }
