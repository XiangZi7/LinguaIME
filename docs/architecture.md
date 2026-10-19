# 架构与代码规范

## 运行边界

Windows 应用 → C++ TSF/COM 加载层 → 可替换原生运行模块 → Rust C ABI → 拼音核心与 SQLite。候选窗口采用 Win32 非激活分层窗口、Direct2D 圆角/阴影与 DirectWrite 文本，`WS_EX_NOACTIVATE` 防止夺焦。Tauri 管理应用不在按键路径上，关闭它不会卸载输入法。

`bootstrap_service.cpp` 负责 TSF 激活、消息线程计时检查、空闲切换与失败回退；`runtime_loader.cpp` 负责严格解析版本指针、ABI 检查和 DLL/COM 生命周期。运行模块持有组合串、编辑会话、语言栏与候选窗口。模块只在无组合串、无待完成编辑、无回调栈时替换；不在调用自己的 DLL 内卸载自己。旧 COM 引用未释放时保留旧模块。运行模块结束后释放 Rust 词典缓存，再解除延迟加载引用，避免每次更新累积完整词典。

`language_bar.cpp` 封装系统任务栏的「中 / 英」按钮、单色图标、点击回调和语言栏通知；`input_mode.cpp` 管理模式与 TSF compartment 同步。按钮停用时先断开回调，再移除语言栏项与监听，避免 Windows 持有的 COM 引用回调已销毁的服务。英文模式在按键路由入口放行，不依赖管理窗口转发按键。

保留现有仓库的 `src` / `src-tauri` 根目录，新增 Rust workspace 与 `native/tsf-host`，避免为了目录形式迁移现有工具链。`Cargo.lock`（根目录）和 `pnpm-lock.yaml` 是版本基准。

| 目录                     | 职责                                                   |
| ------------------------ | ------------------------------------------------------ |
| `crates/ime-types`       | 候选、设置、个人数据、错误与 schema 契约               |
| `crates/ime-core`        | CC-CEDICT 索引、拼音边界、词频排序、动态规划组合与分页 |
| `crates/ime-storage`     | SQLite WAL、版本化迁移、设置、个人词条、词频与生词本   |
| `crates/ime-ffi`         | ABI v1，UTF-8、调用方输出缓冲区、panic 隔离            |
| `native/tsf-host`        | COM 生命周期、键盘策略、编辑会话、焦点与 Win32 候选窗  |
| `src-tauri/src/commands` | 参数与错误边界、核心调用、系统安装命令                 |
| `src/app`                | 应用壳、自定义 Windows 标题栏、导航与窗口控制          |
| `src/components/ui`      | 无业务基础 UI 组件库                                   |
| `src/features`           | 按功能划分的页面、组件与 composable                    |
| `src/services`           | 桌面 invoke 与浏览器预览 adapter、校验与导出           |
| `src/stores`             | 设置草稿、保存状态和管理数据快照                       |
| `data/dictionaries`      | 可追溯词典、摘要和数据许可证                           |
| `tests`                  | 引擎/导入契约与 Playwright 端到端测试                  |

## 输入与数据

词典初始化在宿主激活阶段完成；按键查询使用内存中的 BTree 前缀索引，不访问网络或逐键扫描整个词典。查询结果有界（最多约 512 个前缀候选），这是交互延迟与完整检索之间的 MVP 取舍；词库管理当前也复用该有界查询。

用户词频、精确匹配、个人词条与静态 jieba 权重用于可解释排序。无完整词条的连续拼音使用 unigram 动态规划分词，不为组合句子拼接英文释义。单引号/空格保留显式音节边界。未实现神经语言模型、完整上下文预测或双拼。

设置在新组合开始前读取，词条与词频同步到宿主缓存；核心查询不等待 SQLite。数据库忙等待上限 5 ms，学习失败不能阻止文字提交。跨进程通过同一 `%LOCALAPPDATA%/LinguaIME/lingua.db` 交换持久化数据。

SQLite 重要批量写入使用事务，迁移采用 `user_version`，拒绝打开更新版本数据库。错误仅返回结构化错误码和可操作说明，不将数据库底层文本写进日志。日志只含事件代码与 HRESULT，不记录用户组合串、密钥或完整输入。

## FFI 约定

每个线程持有独立 opaque context，必须由创建线程调用与销毁。ABI v1 的所有字符串是 NUL 终止 UTF-8；输出采用固定长度、调用方所有的 C 结构体，复制时保持 UTF-8 边界。跨界不传 Rust Vec/String，不转移字符串所有权。返回 0 成功，-1 指针为空，-2 输入无效，-3 panic，-4 学习存储不可用。调用方必须保证非空指针指向有效内存。

原生运行模块通过延迟加载钩子从自身版本目录加载 `ime_ffi.dll`。安装使用三个 DLL 的独立内容摘要目录，全部复制校验成功后原子替换 `active-runtime.txt`，保留上一版指针。只有加载层变化或修复注册时才重复 COM 注册。管理员授权只用于 TSF 注册；`InstallLayoutOrTip` 只启用本输入法，不替换已有语言列表，不设置默认输入法。

`src-tauri/src/native_installation.rs` 统一桌面启动与手动更新：每个管理进程在后台同步一次，互斥保护同进程的安装任务，完成后验证真实 TSF 启用与 COM 加载。状态留在 Rust 中供前端读取，不依赖 webview 监听时机；前端只在同步期间轮询，失败持久显示且可重试。同版启动不改写版本指针与上一版记录，也不强行卸载目标应用中的 DLL。

## 前端与 Rust 规范

- Vue 3、`script setup lang="ts"`；props 向下、事件向上，通用组件不依赖业务 store。
- 复杂类型放 `src/types`，常量放 `src/constants`；页面不直接访问数据库。
- 关联局部状态使用 reactive 与中文用途注释，模板通过 toRefs 访问；共享草稿使用 Pinia。
- 候选组合、选中项和已提交文本分别维护；组合状态只在预览 composable 内存在。浏览器预览不承担系统输入职责。
- 异步查询使用请求序号丢弃旧响应，卸载时清理计时器和监听器。
- `@antfu/eslint-config` 负责正确性，Prettier + Tailwind 插件作为唯一前端格式化器。显式锁定直接依赖版本。
- Rust snake_case、Result、领域 crate、rustfmt 与零 Clippy warning；FFI catch_unwind，C++ 入口捕获异常。
- 新功能优先扩展已有组件和服务接口；避免按键业务进入 Tauri store、页面内词典、单一巨型文件。

## 测试隔离

原生构建的契约与 TSF context 测试使用 `build/native/test-data`。`LINGUAIME_DATA_DIR` 可显式覆盖诊断数据目录，不应在日常运行时设置。浏览器测试使用独立端口 1438 与独立 context，本地预览数据和桌面数据完全独立。
