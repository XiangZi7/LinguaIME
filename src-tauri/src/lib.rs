mod commands;
mod state;

use state::AppState;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    let result = tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .setup(|app| {
            use tauri::Manager;
            let state = AppState::new().map_err(Box::<dyn std::error::Error>::from)?;
            app.manage(state);
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            commands::get_snapshot,
            commands::query_candidates,
            commands::save_settings,
            commands::save_phrase,
            commands::delete_phrase,
            commands::import_phrases,
            commands::record_selection,
            commands::save_learning,
            commands::delete_learning,
            commands::export_data,
            commands::clear_data,
        ])
        .run(tauri::generate_context!());
    if result.is_err() {
        eprintln!("LinguaIME: DESKTOP_START_FAILED");
    }
}
