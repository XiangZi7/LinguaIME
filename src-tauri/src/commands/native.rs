use crate::native_installation::{self, InstallationStatus, NativeInstallation};
use ime_types::AppResult;
use tauri::{AppHandle, State};

#[tauri::command]
pub fn get_native_installation(
    state: State<'_, NativeInstallation>,
) -> AppResult<InstallationStatus> {
    state.status()
}

#[tauri::command]
pub async fn install_native(app: AppHandle, uninstall: bool) -> AppResult<()> {
    native_installation::install(app, uninstall).await
}
