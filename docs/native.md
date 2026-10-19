# Windows 原生输入法

## 开发与安装

要求 x64 Windows 10/11、Rust MSVC toolchain、Visual Studio C++ 桌面工具和 Windows SDK。无需 CMake 也可通过项目脚本调用 MSVC；CMakeLists 同时保留供原生 IDE 使用。

```powershell
pnpm native:build
pnpm native:register
```

桌面应用的「概览」与「语言与引擎」也提供安装按钮。首次注册会触发 Windows 管理员授权。脚本将两个 DLL 复制到 `%LOCALAPPDATA%/LinguaIME/native/<摘要>/`，注册 COM/TSF，再调用 Windows `InstallLayoutOrTip` 启用该 profile。它不替换微软拼音、不更改默认输入法。

注册后按 Win+Space 选择 `LinguaIME · 中英双语`。已打开的应用可能缓存旧 profile/DLL，必要时重新打开目标应用。输入法独立于 Tauri 主窗口运行。

## 任务栏中英文切换

选择 LinguaIME 后，Windows 输入指示器显示「中 / 英」模式按钮，单击切换：

- **中**：拼音组合与候选，按设置处理中文标点。
- **英**：字母、空格和标点直接交给目标应用，不产生中文候选。
- 切换时若仍有组合串，先提交当前候选；没有候选时提交原始拼音。编辑锁失败时保留原模式和组合串。

每次激活默认中文；当前模式属于目标应用的 TSF 线程，不写入个人词库或管理端外观设置。按钮使用真正的 TSF `GUID_LBI_INPUTMODE`，通过 `ITfLangBarItemButton` 接收点击，并同步键盘开关与转换模式 compartment。图标为单色「中 / 英」，由 Windows 按任务栏主题着色；不创建独立托盘进程。

宿主注册系统托盘支持与输入模式 compartment 能力。安装新版后，已加载旧 DLL 的应用需要重开，再用 Win+Space 选择 LinguaIME；不需要重启管理窗口来维持该按钮。

接口依据：[Windows 语言栏项](https://learn.microsoft.com/en-us/windows/win32/api/ctfutb/ns-ctfutb-tf_langbariteminfo)、[TSF 能力注册](https://learn.microsoft.com/en-us/windows/win32/tsf/predefined-category-values)。

## 诊断命令

```powershell
# 查看 Windows TSF 启用状态与 COM 加载结果
.\build\native\profile_tool.exe status
# 仅在测试进程激活 profile，并检查系统模式按钮与中英切换，不改变其他应用或默认输入法
.\build\native\profile_tool.exe probe
# 移除系统注册；保留词典与个人数据
pnpm native:unregister
```

当前构建只含 x64 宿主；32 位目标应用需要后续 x86 宿主。管理应用安装包打包两个原生 DLL 和安装脚本，但不会未经操作自动修改系统输入法。启动桌面开发和生产构建会先构建/验证原生组件。

## 已测试边界

原生构建自动执行：C ABI 布局与词典桥接、空闲符号路由、真实 TSF 内存 context 中的拼音组合、中文提交和取消；另验证中英按钮、切换时提交组合串、英文按键直通、系统 compartment 同步与停用清理。该 context harness 直接注入应用 client ID，隔离 Windows 的 OS profile 分配；它不冒充记事本或浏览器实测。

另通过 Windows API 验证已注册 profile enabled=true、COM 激活成功、隔离测试进程中的真实 TSF profile 激活成功；已安装 DLL 注册的模式按钮可被系统取得，执行中文 → 英文 → 中文往返后，文字和两个模式 compartment 均一致。记录见 `docs/verification.md`。

仍需在记事本、浏览器、IDE 中人工测试：连续输入、焦点变化、高 DPI、多显示器、屏幕边缘、辅助技术和升级/回滚。Win32 候选窗口当前未实现完整 UI Automation provider；不声明已完成可访问性发布验收。

## 故障处理

- **只打开管理应用、列表没有 LinguaIME**：点击安装按钮。运行 Tauri 应用本身不会代替 TSF 注册。
- **注册码 5 / HRESULT 0x80004005**：Windows 拒绝未提权的 TSF machine 注册；允许系统 UAC 提示后重试。
- **尚未显示新标题栏**：重启 `pnpm tauri dev`；`decorations` 不是普通 CSS 热更新配置。
- **COM 加载失败**：核对两个 DLL 同目录、x64 架构与 MSVC runtime；用 profile_tool status 查看返回码。
- **更新后仍有旧行为**：关闭正在使用输入法的目标应用，再重新打开。不要手动删除正在加载的 DLL。
- **数据库错误**：输入路径保持词典查询降级，学习写入可失败；先导出个人词条，离线检查数据库或恢复备份。

宿主只输出事件码与 HRESULT 到调试输出，不记录输入文本。
