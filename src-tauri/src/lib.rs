mod commands;
mod native_installation;
mod native_status;
mod state;

use state::AppState;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    let result = tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .setup(|app| {
            use tauri::Manager;
            let runtime_directory = if cfg!(debug_assertions) {
                None
            } else {
                Some(app.path().resource_dir()?.join("native/rime"))
            };
            let state = AppState::new(runtime_directory.as_deref())
                .map_err(Box::<dyn std::error::Error>::from)?;
            app.manage(state);
            app.manage(native_installation::NativeInstallation::default());
            native_installation::start(app.handle().clone());
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            commands::native::install_native,
            commands::native::get_native_installation,
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
