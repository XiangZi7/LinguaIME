# LinguaIME

本地优先的 Windows 中英双语输入法。C++ TSF/COM + Rust 输入核心 + Tauri 2 + Vue 3 / TypeScript + Tailwind CSS v4。

## 运行

需要 Node.js 22.18+、pnpm 10、Rust MSVC、Visual Studio C++ 桌面构建工具与 Windows SDK。

```powershell
pnpm install --frozen-lockfile
pnpm dev
# 浏览器交互预览：http://127.0.0.1:1430
pnpm tauri dev
# 原生构建验证后启动桌面管理应用
```

系统输入法需要另外安装：点击管理端的「安装到 Windows 输入法」，接受 Windows 管理员授权后，按 Win+Space 选择 LinguaIME。也可以运行：

```powershell
pnpm native:build
pnpm native:register
# 移除注册，保留个人数据
pnpm native:unregister
```

管理应用使用自定义 Windows 标题栏，支持拖动、最小化、最大化/还原、关闭与未保存设置保护。修改窗口配置后需要重启桌面应用。

## 已实现与边界

- 独立原生 TSF 宿主和 Win32 候选窗，不依赖管理窗口进行输入。
- 系统任务栏「中 / 英」模式按钮，单击切换拼音输入与英文直通；关闭管理窗口后仍可使用。
- 全拼检索、显式音节边界、连续拼音组合、词频排序、去重与分页。
- CC-CEDICT 121,202 条离线词条，jieba 静态词频；来源、许可与摘要随数据保存。
- 十个管理页面、Tailwind v4 语义令牌和可复用 UI 组件库；深浅色、实时候选预览、设置保存与取消。
- SQLite 设置、个人词条、词频、收藏与复习，导入校验/事务、导出与确认清除。
- 系统注册、当前用户启用、COM 与进程内真实 TSF 激活已验证。

当前是 x64 开发预览。记事本/浏览器/IDE 全链路、多 DPI、多显示器与发布安装签名仍需验收。在线翻译、发音、日语、32 位宿主与完整备份恢复尚未实现，不在界面冒充可用。

桌面数据在 `%LOCALAPPDATA%/LinguaIME/lingua.db`。浏览器预览使用 699 条同源子集和独立本地存储，不是 Windows 输入法。基础输入没有联网请求，不保存完整输入文本，统计默认关闭。

## 检查

```powershell
pnpm test
pnpm test:e2e
pnpm build
pnpm lint
pnpm format:check
cargo fmt --all --check
cargo test --workspace --locked --offline
cargo clippy --workspace --locked --offline -- -D warnings
pnpm native:build
```

端到端测试使用本机 Microsoft Edge 和独立端口 1438。原生测试隔离个人数据库，不更改系统默认输入法。生产构建：`pnpm tauri build`，会先构建原生宿主并打包安装资源。

- [实施计划](docs/implementation-plan.md)
- [架构与代码规范](docs/architecture.md)
- [UI 组件库](docs/ui-components.md)
- [原生安装与故障处理](docs/native.md)
- [验证记录与未验收项目](docs/verification.md)
- [词典来源与许可证](data/dictionaries/README.md)
- [原始需求](prompt/LinguaIME.md)
