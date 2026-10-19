# Windows 原生输入法

## 开发与安装

要求 x64 Windows 10/11、Rust MSVC toolchain、Visual Studio C++ 桌面工具和 Windows SDK。无需 CMake 也可通过项目脚本调用 MSVC；CMakeLists 同时保留供原生 IDE 使用。

```powershell
pnpm native:build
pnpm native:register
```

桌面应用的「概览」与「语言与引擎」也提供安装按钮。首次注册会触发 Windows 管理员授权。脚本将两个 DLL 复制到 `%LOCALAPPDATA%/LinguaIME/native/<摘要>/`，注册 COM/TSF，再调用 Windows `InstallLayoutOrTip` 启用该 profile。它不替换微软拼音、不更改默认输入法。

注册后按 Win+Space 选择 `LinguaIME · 中英双语`。已打开的应用可能缓存旧 profile/DLL，必要时重新打开目标应用。输入法独立于 Tauri 主窗口运行。

```powershell
# 查看 Windows TSF 启用状态与 COM 加载结果
.\build\native\profile_tool.exe status
# 仅在测试进程激活 profile，不改变其他应用或默认输入法
.\build\native\profile_tool.exe probe
# 移除系统注册；保留词典与个人数据
pnpm native:unregister
```

当前构建只含 x64 宿主；32 位目标应用需要后续 x86 宿主。管理应用安装包打包两个原生 DLL 和安装脚本，但不会未经操作自动修改系统输入法。启动桌面开发和生产构建会先构建/验证原生组件。

## 已测试边界

原生构建自动执行：C ABI 布局与词典桥接、空闲符号路由、真实 TSF 内存 context 中的拼音组合、中文提交和取消。该 context harness 直接注入应用 client ID，隔离 Windows 的 OS profile 分配；它不冒充记事本或浏览器实测。

另通过 Windows API 验证已注册 profile enabled=true、COM 激活成功、隔离测试进程中的真实 TSF profile 激活成功。记录见 `docs/verification.md`。

仍需在记事本、浏览器、IDE 中人工测试：连续输入、焦点变化、高 DPI、多显示器、屏幕边缘、辅助技术和升级/回滚。Win32 候选窗口当前未实现完整 UI Automation provider；不声明已完成可访问性发布验收。

## 故障处理

- **只打开管理应用、列表没有 LinguaIME**：点击安装按钮。运行 Tauri 应用本身不会代替 TSF 注册。
- **注册码 5 / HRESULT 0x80004005**：Windows 拒绝未提权的 TSF machine 注册；允许系统 UAC 提示后重试。
- **尚未显示新标题栏**：重启 `pnpm tauri dev`；`decorations` 不是普通 CSS 热更新配置。
- **COM 加载失败**：核对两个 DLL 同目录、x64 架构与 MSVC runtime；用 profile_tool status 查看返回码。
- **更新后仍有旧行为**：关闭正在使用输入法的目标应用，再重新打开。不要手动删除正在加载的 DLL。
- **数据库错误**：输入路径保持词典查询降级，学习写入可失败；先导出个人词条，离线检查数据库或恢复备份。

宿主只输出事件码与 HRESULT 到调试输出，不记录输入文本。
