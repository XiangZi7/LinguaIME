# LinguaIME 实施计划

依据：`prompt/LinguaIME.md` 与 `prompt/ui1.png`。开始日期：2026-10-19。

## 执行顺序与验收

| 阶段        | 交付                                                                        | 验收                                                |
| ----------- | --------------------------------------------------------------------------- | --------------------------------------------------- |
| M0 原生验证 | Rust workspace、版本化 C ABI、C++ TSF COM DLL、Win32 非激活候选窗、注册工具 | 构建成功，原生契约测试通过；记事本实际输入另行记录  |
| M1 输入核心 | 全拼归一化、词典索引、精确/前缀检索、去重、可解释排序、分页、词频           | Rust 单元测试，空串、越界、重复和符号路由测试       |
| M2 双语候选 | 中英字段、三种显示模式、主题、DPI/边缘定位                                  | 原生窗口与管理端预览共享字段契约；缺失数据不造假    |
| M3 管理应用 | 十个管理页面、Tailwind v4、基础 UI 库、设置保存/取消、SQLite 持久化         | TypeScript、生产构建、格式检查和浏览器交互验证      |
| M4 个人数据 | 个人词条 CRUD、导入预览/校验、导出、收藏/复习、隐私清除                     | 事务、迁移、重启持久化和无效导入测试                |
| 发布验证    | 构建/安装说明、故障与状态、兼容性矩阵                                       | Windows 10/11、记事本、浏览器、IDE、多 DPI 实机验收 |
| 后续        | 在线翻译/安全密钥、发音、日语独立引擎、签名安装包                           | 未接入前不显示为可用                                |

## 模块与组件边界

- `crates/ime-types`：序列化契约、设置、候选、错误；不依赖 UI。
- `crates/ime-core`：本地词典索引、拼音检索和排序。
- `crates/ime-storage`：SQLite 迁移、个人词条、词频、收藏和设置。
- `crates/ime-ffi`：C ABI；UTF-8 输入、固定布局输出、panic 隔离与调用方缓冲区。
- `native/tsf-host`：COM 生命周期、键盘路由、编辑会话、组合串、Win32 候选窗口；无 WebView。
- `src-tauri/src/commands`：薄命令层，通过核心与存储访问数据。
- `src/app`：布局、导航、启动和全局消息。
- `src/components/ui`：Button、Icon、Badge、Switch、Select、Dialog、EmptyState、PageHeader、SettingRow、Panel；仅 props/emits 与插槽。
- `src/features/candidate`：CandidateWindow 接收候选页与显示配置，发出选择/翻页事件；CandidateRow 仅呈现单个词条。
- `src/features/*`：页面按业务划分，查询与变更放 composable/service；路由页面仅组合功能。
- `src/services`：Tauri adapter 与明确标记的浏览器预览 adapter；页面不直接访问 invoke/存储。
- `src/stores`：跨页面设置草稿、保存状态、个人数据；组合输入状态不进入全局 store。
- `data/dictionaries`：离线数据和可追溯来源；不在 Vue 文件内硬编码词典。

## 工程约定

Vue 使用 Composition API、`script setup lang="ts"`、显式 props/emits，类型与常量独立。关联状态使用带中文注释的 reactive 对象，模板通过 toRefs 访问。Tailwind 使用 CSS-first `@theme` 与语义令牌。Rust 按领域拆 crate，snake_case、Result 与结构化错误、rustfmt、Clippy；FFI 禁止 panic/异常跨界。C++ 使用 RAII、ComPtr、明确 HRESULT 和所有权。源文件保持单一职责，避免空壳目录和过度拆分。

## 验证记录

实施完成后填写 `docs/verification.md`，严格区分自动化通过、人工实测与尚未验证。

## 本轮交付与接续计划

已落地 Rust 核心/存储/FFI、可编译与可注册的 TSF 宿主、原生 context 测试、十页管理端、基础 UI 库与自定义 Windows 标题栏。已在当前用户完成系统安装，Windows API 确认 profile 启用、COM 加载与隔离进程内 TSF 激活成功。

下一轮按顺序推进：

1. **P0 实机输入验收**：记事本、浏览器、VS Code 连续输入、取消、焦点切换和管理端关闭场景，记录 Windows 版本、DPI 与复现结果。
2. **P0 发布可靠性**：候选窗口 UI Automation、32 位宿主、安装签名、升级回滚、数据库恢复与完整备份恢复。
3. **P1 输入质量**：扩充排序基准、增加多路径分词与前缀完整搜索、常用短语的人工校验双语映射。
4. **P2 主动学习扩展**：安全密钥与提供商抽象后接入用户主动翻译、例句与发音；完成数据授权后再接独立日语引擎。

不把上述未完成验收或后续能力标记为已发布。
