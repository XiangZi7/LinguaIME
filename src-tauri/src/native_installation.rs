use ime_types::{AppError, AppResult};
use serde::Serialize;
use std::sync::Mutex;
use tauri::{AppHandle, Manager};

#[derive(Clone, Copy, Debug, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum InstallationPhase {
    Checking,
    Ready,
    Failed,
    Removed,
    Unsupported,
}

#[derive(Clone, Serialize)]
pub struct InstallationStatus {
    phase: InstallationPhase,
    error: Option<AppError>,
}

struct InstallationState {
    running: bool,
    status: InstallationStatus,
}

pub struct NativeInstallation(Mutex<InstallationState>);

impl Default for NativeInstallation {
    fn default() -> Self {
        Self(Mutex::new(InstallationState {
            running: false,
            status: InstallationStatus {
                phase: if cfg!(windows) {
                    InstallationPhase::Checking
                } else {
                    InstallationPhase::Unsupported
                },
                error: None,
            },
        }))
    }
}

impl NativeInstallation {
    pub fn status(&self) -> AppResult<InstallationStatus> {
        self.0
            .lock()
            .map(|state| state.status.clone())
            .map_err(|_| installation_lock_error())
    }

    fn begin(&self) -> AppResult<()> {
        let mut state = self.0.lock().map_err(|_| installation_lock_error())?;
        if state.running {
            return Err(AppError::new("INSTALL_BUSY", "输入法正在更新，请稍候"));
        }
        state.running = true;
        state.status = InstallationStatus {
            phase: InstallationPhase::Checking,
            error: None,
        };
        Ok(())
    }

    fn finish(&self, result: &AppResult<()>, uninstall: bool) -> AppResult<()> {
        let mut state = self.0.lock().map_err(|_| installation_lock_error())?;
        state.running = false;
        state.status = InstallationStatus {
            phase: match result {
                Err(_) => InstallationPhase::Failed,
                Ok(()) if uninstall => InstallationPhase::Removed,
                Ok(()) => InstallationPhase::Ready,
            },
            error: result.as_ref().err().cloned(),
        };
        Ok(())
    }
}

fn installation_lock_error() -> AppError {
    AppError::new("INSTALL_STATE", "无法读取输入法更新状态，请重启管理应用")
}

/// Run once per desktop process, independently of webview loading, routes and HMR.
pub fn start(app: AppHandle) {
    #[cfg(windows)]
    tauri::async_runtime::spawn(async move {
        match install(app, false).await {
            Ok(()) => eprintln!("LinguaIME: NATIVE_STARTUP_UPDATE_READY"),
            Err(_) => eprintln!("LinguaIME: NATIVE_STARTUP_UPDATE_FAILED"),
        }
    });
    #[cfg(not(windows))]
    let _ = app;
}

pub async fn install(app: AppHandle, uninstall: bool) -> AppResult<()> {
    let state = app.state::<NativeInstallation>();
    state.begin()?;
    let installer_app = app.clone();
    let result =
        tauri::async_runtime::spawn_blocking(move || run_installer(&installer_app, uninstall))
            .await
            .map_err(|_| AppError::new("INSTALL_TASK", "输入法安装任务异常结束"))
            .and_then(|result| result);
    state.finish(&result, uninstall)?;
    result
}

fn run_installer(app: &AppHandle, uninstall: bool) -> AppResult<()> {
    #[cfg(windows)]
    {
        use std::{os::windows::process::CommandExt, path::PathBuf, process::Command};
        let root = if cfg!(debug_assertions) {
            PathBuf::from(env!("CARGO_MANIFEST_DIR"))
                .parent()
                .map(std::path::Path::to_path_buf)
        } else {
            app.path().resource_dir().ok()
        }
        .ok_or_else(|| AppError::new("INSTALLER_PATH", "无法定位输入法安装文件"))?;
        let (script, native) = if cfg!(debug_assertions) {
            (
                root.join("scripts/register-ime.ps1"),
                root.join("build/native"),
            )
        } else {
            (root.join("native/register-ime.ps1"), root.join("native"))
        };
        if !script.is_file() {
            return Err(AppError::new(
                "INSTALLER_MISSING",
                "安装脚本缺失，请重新构建桌面应用",
            ));
        }
        let mut command = Command::new("powershell.exe");
        command
            .args([
                "-NoProfile",
                "-NonInteractive",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
            ])
            .arg(script)
            .arg("-NativeDirectory")
            .arg(native)
            .creation_flags(0x08000000);
        if uninstall {
            command.arg("-Unregister");
        }
        let output = command.output().map_err(|_| {
            AppError::new(
                "INSTALL_START",
                "无法启动系统输入法安装程序，请点击重试更新",
            )
        })?;
        if !output.status.success() {
            return Err(AppError::new(
                "INSTALL_FAILED",
                "Windows 未完成输入法更新。若出现管理员授权提示，请允许后重试；仍失败时可运行 pnpm native:register 查看系统错误。",
            ));
        }
        if !uninstall && !crate::native_status::native_registered() {
            return Err(AppError::new(
                "INSTALL_VERIFY",
                "组件已发布，但 Windows 尚未启用输入法，请点击重试更新",
            ));
        }
        Ok(())
    }
    #[cfg(not(windows))]
    {
        let _ = (app, uninstall);
        Err(AppError::new(
            "UNSUPPORTED_PLATFORM",
            "系统输入法仅支持 Windows",
        ))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn startup_and_manual_updates_do_not_overlap() -> AppResult<()> {
        let state = NativeInstallation::default();
        state.begin()?;
        assert_eq!(state.status()?.phase, InstallationPhase::Checking);
        assert_eq!(
            state.begin().err().map(|error| error.code),
            Some("INSTALL_BUSY")
        );
        state.finish(&Ok(()), false)?;
        assert_eq!(state.status()?.phase, InstallationPhase::Ready);
        state.begin()?;
        state.finish(&Ok(()), true)?;
        assert_eq!(state.status()?.phase, InstallationPhase::Removed);
        Ok(())
    }

    #[test]
    fn failed_startup_remains_visible_and_can_be_retried() -> AppResult<()> {
        let state = NativeInstallation::default();
        state.begin()?;
        state.finish(&Err(AppError::new("INSTALL_FAILED", "更新失败")), false)?;
        let failed = state.status()?;
        assert_eq!(failed.phase, InstallationPhase::Failed);
        assert_eq!(failed.error.map(|error| error.code), Some("INSTALL_FAILED"));
        state.begin()?;
        assert!(state.status()?.error.is_none());
        state.finish(&Ok(()), false)?;
        assert_eq!(state.status()?.phase, InstallationPhase::Ready);
        Ok(())
    }
}
