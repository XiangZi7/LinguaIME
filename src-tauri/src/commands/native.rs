use ime_types::{AppError, AppResult};
use tauri::{AppHandle, Manager};

#[tauri::command]
pub async fn install_native(app: AppHandle, uninstall: bool) -> AppResult<()> {
    #[cfg(windows)]
    {
        let root = if cfg!(debug_assertions) {
            std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"))
                .parent()
                .map(std::path::Path::to_path_buf)
        } else {
            app.path().resource_dir().ok()
        };
        let root = root.ok_or_else(|| AppError::new("INSTALLER_PATH", "无法定位输入法安装文件"))?;
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
        tauri::async_runtime::spawn_blocking(move || {
            use std::os::windows::process::CommandExt;
            let mut command=std::process::Command::new("powershell.exe");
            command.args(["-NoProfile","-NonInteractive","-ExecutionPolicy","Bypass","-File"])
                .arg(script).arg("-NativeDirectory").arg(native).creation_flags(0x08000000);
            if uninstall { command.arg("-Unregister"); }
            let output=command.output().map_err(|_| AppError::new("INSTALL_START", "无法启动系统输入法安装程序"))?;
            if !output.status.success() {
                return Err(AppError::new("INSTALL_FAILED", "Windows 未完成输入法注册。请运行 pnpm native:register 查看系统错误；如提示拒绝访问，请使用管理员终端。"));
            }
            Ok(())
        }).await.map_err(|_| AppError::new("INSTALL_TASK", "输入法安装任务异常结束"))?
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
