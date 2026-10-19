# Windows 原生输入法

## 开发与安装

要求 x64 Windows 10/11、Rust MSVC toolchain、Visual Studio C++ 桌面工具和 Windows SDK。无需 CMake 也可通过项目脚本调用 MSVC；CMakeLists 同时保留供原生 IDE 使用。

```powershell
pnpm native:build
pnpm native:register
```

桌面管理端每次启动都在后台自动安装或更新当前构建，开发版与打包版共用安装器。无需再手动点击发布；「概览」与「语言与引擎」显示同步状态、保留失败原因并提供重试或手动更新。任务由 Rust 启动一次，切换路由与前端 HMR 不会重新安装；失败不阻止管理窗口使用。

首次注册会触发 Windows 管理员授权。脚本分开安装 TSF 加载层与可替换输入模块，再调用 Windows `InstallLayoutOrTip` 启用本 profile。同版跳过 COM 注册且不改写版本指针或回滚记录，加载层变化时才重新注册。它不替换微软拼音、不更改默认输入法，保留个人词库和设置。

注册后按 Win+Space 选择 `LinguaIME · 中英双语`。从原来的整体 DLL 迁移时，已打开的应用需要重开一次以加载新加载层；之后兼容的输入模块更新会在输入空闲时自动应用。输入法独立于 Tauri 主窗口运行。

## 运行中的自动更新

安装目录位于 `%LOCALAPPDATA%/LinguaIME/native/`：

- `host-v1-<摘要>/lingua_tsf.dll`：Windows 注册的薄加载层。
- `runtime/<运行模块摘要>-<Rust 摘要>/lingua_runtime.dll` 与 `ime_ffi.dll`：同一版输入行为、候选绘制和 Rust 核心。
- `active-runtime.txt`：当前版本指针；首行是加载协议版本 `1`，第二行是两个完整 SHA-256。
- `previous-runtime.txt`：上一版指针，供启动失败回退与手动回滚。

更新会先复制并校验全部文件，再原子发布版本指针，不覆盖运行中的 DLL。加载层每秒在所属 TSF 线程的消息循环中检查一次；检测不进入按键检索路径。当前没有组合串、编辑会话和原生回调执行时，加载层验证新版 ABI，停用旧服务并激活新版，恢复「中 / 英」模式。正在输入的拼音会等到提交或取消后再更新；暂停消息循环的应用会等到恢复运行后检查。

新版文件缺失、ABI 不兼容或激活失败时保留或重新激活旧版，不持续重试同一个失败版本。新进程首次加载失败会尝试上一版。Windows 若仍持有旧 COM 对象，旧模块会保留到引用释放，避免其回调失效；不会强制结束用户应用。

已安装加载层后，更新候选窗、输入行为、词典或 Rust 引擎无需逐个重开目标应用。**加载层自身的修复、加载协议升级，以及第一次从旧结构迁移，仍可能需要重开应用。** 管理端启动自动发布本机当前构建，不进行联网下载。只重新注册或卸载重装，不能替换旧进程已经加载且正在使用的整体 DLL。

```powershell
# 构建并发布新版；加载层未变时无需重复 COM 提权注册
pnpm native:build
pnpm native:register
# 发布上一版，已运行的兼容加载层也会在空闲时切换
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/register-ime.ps1 -Rollback
```

加载层、运行模块和 Rust 桥接保持独立责任。模块 ABI v1 通过 `ILinguaRuntimeControl` 报告空闲/中英状态，不能跨不兼容接口热换。DLL 引用计数与卸载规则依据 [Windows FreeLibrary 文档](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-freelibrary)。

## 任务栏中英文切换

选择 LinguaIME 后，Windows 输入指示器显示「中 / 英」模式按钮，单击切换：

- **中**：拼音组合与候选，按设置处理中文标点。
- **英**：字母、空格和标点直接交给目标应用，不产生中文候选。
- 切换时若仍有组合串，先提交当前候选；没有候选时提交原始拼音。编辑锁失败时保留原模式和组合串。

每次激活默认中文；当前模式属于目标应用的 TSF 线程，不写入个人词库或管理端外观设置。按钮使用真正的 TSF `GUID_LBI_INPUTMODE`，通过 `ITfLangBarItemButton` 接收点击，并同步键盘开关与转换模式 compartment。图标为单色「中 / 英」，由 Windows 按任务栏主题着色；不创建独立托盘进程。

宿主注册系统托盘支持与输入模式 compartment 能力。该按钮随运行模块更新；管理窗口关闭后仍可使用。

接口依据：[Windows 语言栏项](https://learn.microsoft.com/en-us/windows/win32/api/ctfutb/ns-ctfutb-tf_langbariteminfo)、[TSF 能力注册](https://learn.microsoft.com/en-us/windows/win32/tsf/predefined-category-values)。

## 诊断命令

```powershell
# 查看 Windows TSF 启用状态与 COM 加载结果
.\build\native\profile_tool.exe status
# 仅在测试进程激活 profile，并检查系统模式按钮与中英切换，不改变其他应用或默认输入法
.\build\native\profile_tool.exe probe
# 保持真实 TSF 测试进程运行，等待另一个终端发布新版（最多 180 秒）
.\build\native\profile_tool.exe watch-update
# 移除系统注册；保留词典与个人数据
pnpm native:unregister
```

当前构建只含 x64 宿主；32 位目标应用需要后续 x86 宿主。管理应用安装包包含三个原生 DLL 与注册/更新脚本。桌面开发和生产构建会先构建/验证原生组件；仅构建不会发布到系统，启动桌面管理端会自动发布。

## 已测试边界

原生构建自动执行：C ABI 布局与词典桥接、空闲符号路由、真实 TSF 内存 context 中的拼音组合、中文提交和取消；另验证中英按钮、切换时提交组合串、英文按键直通、系统 compartment 同步与停用清理。该 context harness 直接注入应用 client ID，隔离 Windows 的 OS profile 分配；它不冒充记事本或浏览器实测。

context 与模拟更新测试通过 `ITfThreadMgrEx::ActivateEx` 的 `TF_TMAE_NOACTIVATETIP | TF_TMAE_NOACTIVATEKEYBOARDLAYOUT` 建立测试线程，不自动加载当前选中的系统输入法。否则当 LinguaIME 已被选中时，已安装实例和测试实例会重复注册同一语言栏按钮，导致 `E_FAIL`。真实安装/激活验证保持在单独的 `profile_tool` 中。[接口说明](https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-itfthreadmgrex-activateex)

```powershell
# 回归：先仅在本测试进程选择已安装的 LinguaIME，再验证测试线程隔离
# 该选项需要已完成系统安装；普通构建测试不要求安装输入法
.\build\native\tsf_context_test.exe --installed-profile
```

另通过 Windows API 验证已注册 profile enabled=true、COM 激活成功、隔离测试进程中的真实 TSF profile 激活成功；已安装 DLL 注册的模式按钮可被系统取得，执行中文 → 英文 → 中文往返后，文字和两个模式 compartment 均一致。记录见 `docs/verification.md`。

更新测试包括模拟模块的计时器切换、忙时等待、失败回退、ABI 拒绝、旧 COM 引用保护；PowerShell 安装器的不可变文件校验、带读句柄的原子替换与回滚。SHA-256 使用 .NET API，避免 Windows PowerShell 子进程继承 PowerShell 7 模块路径后找不到 `Get-FileHash`；测试同时覆盖标准摘要向量和该命令不可用的场景。真实已安装 TSF profile 另完成同一进程跨版本升级，确认新运行模块、新 Rust DLL 与英文模式保留。

仍需在记事本、浏览器、IDE 中人工测试：连续输入、焦点变化、高 DPI、多显示器、屏幕边缘、辅助技术和升级/回滚。Win32 候选窗口当前未实现完整 UI Automation provider；不声明已完成可访问性发布验收。

## 故障处理

- **启动管理应用后列表没有 LinguaIME**：等待自动同步完成，首次注册允许 Windows 管理员授权；失败时点击「重试更新」。浏览器预览不能注册 TSF。
- **注册阶段 code 5 / HRESULT 0x80004005**：Windows 可能拒绝未提权的 TSF machine 注册；允许系统 UAC 提示后重试。
- **启动前测试提示 test engine initialization / 0x80004005**：旧测试把语言栏重复注册也统一报成引擎失败。测试已改为隔离 TSF 激活并细分日志，更新代码后重新运行 `pnpm tauri dev`；无需卸载输入法或删除数据库。
- **尚未显示新标题栏**：重启 `pnpm tauri dev`；`decorations` 不是普通 CSS 热更新配置。
- **COM 加载失败**：核对两个 DLL 同目录、x64 架构与 MSVC runtime；用 profile_tool status 查看返回码。
- **首次迁移后仍有旧行为**：旧整体 DLL 无法自行获得新加载器；完全退出目标应用一次，再选 LinguaIME。浏览器关闭标签页或全部窗口后，后台进程仍可能存活，需使用浏览器的退出功能确保旧进程结束。
- **之后更新仍未生效**：先提交或取消拼音；确认已通过更新入口发布。若新版加载失败会继续旧版，可用 `watch-update` 检查真实进程升级。不要手动覆盖正在加载的 DLL。
- **数据库错误**：输入路径保持词典查询降级，学习写入可失败；先导出个人词条，离线检查数据库或恢复备份。

宿主只输出事件码与 HRESULT 到调试输出，不记录输入文本。
