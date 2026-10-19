# AMV Studio

动漫、音乐与每一个灵感瞬间的本地创作工作台。Vue 3 + TypeScript + Tailwind CSS v4 + Vue Router + Pinia + Tauri 2 / Rust。

## 运行

需要 Node.js 22.18+、pnpm 10。桌面模式还需要 Rust 与 Windows 的 Tauri 开发环境。

```powershell
pnpm install
pnpm dev
# 启动原生桌面窗口
pnpm tauri dev
```

浏览器预览：[http://127.0.0.1:1420](http://127.0.0.1:1420)。新建项目、导入本地视频 / 图片 / 音频，点击素材加号或双击加入时间线。选择片段后在 Inspector 中调整；回车或移开焦点提交数值。支持同轨拖动、拆分与撤销重做。Storyboard 与 Edit 共用真实工程数据。

## 当前状态

已实现项目管理、本地素材与预览、基础多轨编辑、自动保存、撤销重做、离线规则分镜提案、三种可用技能、工程备份和六个带按需缓存的路由。

**尚未实现云端 AI、FFmpeg 成片渲染、字幕 / 关键帧与复杂转场，当前不能导出 MP4。** 界面明确区分已实现与待接入的能力，不使用虚假生成与渲染进度。完整计划见 [实施路线](docs/roadmap.md)。

浏览器工程与媒体缓存在 IndexedDB；桌面工程通过 Rust 保存到应用数据目录并保留上一版 .bak，桌面媒体仍由 WebView IndexedDB 缓存。`.amv.json` 备份不包含原视频，迁移时需保留媒体并重新关联。浏览器与桌面的工作空间相互独立。

## 维护与验证

```powershell
pnpm test
pnpm build
pnpm format:check
cd src-tauri
cargo fmt --check
cargo test --offline
cargo clippy --offline -- -D warnings
```

- [架构、组件边界、路由缓存与存储](docs/architecture.md)
- [通用 UI 组件库与使用约定](docs/ui-components.md)
- [实施进度与后续开发计划](docs/roadmap.md)
- [原创视觉资源与生成提示词](docs/assets.md)
- [完整产品需求](prompt/AMV-Studio.md)

样式 Token 统一位于 `src/assets/styles/main.css` 的 `@theme`，基础 UI 组件从 `@/components/ui` 导入。页面通过 `route.meta.keepAlive` 控制缓存，编辑器释放播放器实例并按项目保留轻量视图状态。
