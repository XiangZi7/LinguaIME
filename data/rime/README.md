# Rime 输入资源

Windows 系统输入与桌面候选预览使用 librime 1.17.0；现有 CC-CEDICT 继续提供英文释义。浏览器预览仍使用独立的精简词典，不加载原生 Rime。

- 引擎：<https://github.com/rime/librime/releases/tag/1.17.0>，BSD-3-Clause，见 `LIBRIME-LICENSE.txt`。官方 MSVC x64 发布包及 DLL 的 SHA-256 固定在 `scripts/prepare-rime.ps1`。
- 字词库：<https://github.com/rime/rime-pinyin-simp/tree/0c6861ef7420ee780270ca6d993d18d4101049d0>，Apache-2.0，见 `LICENSE` 和 `AUTHORS`。`pinyin_simp.dict.yaml` 保持原样，SHA-256 为 `E341598343A0F0F2035BB1AAFC34A7F3BB7887DEEECB3F60796262AAA2983E6B`。
- `lingua_pinyin.schema.yaml` 与 `default.yaml` 是 LinguaIME 配置：简体、全拼/首拼/混输、连续组句、常见拼写纠错；不使用 Lua 或联网服务。
- 首次运行 `powershell -File scripts/prepare-rime.ps1 -Download` 下载官方运行库，需要 7-Zip。之后 `pnpm native:build` 在本地编译方案，打字与后续构建均不联网。
- 部署器只在构建期间运行；安装包带上预编译词典、运行库、摘要清单与许可证。资源摘要嵌入 Rust 核心，变更资源后必须重新构建核心，再发布整个运行模块。

选词频率、个人短语与隐私开关继续使用 LinguaIME SQLite。每个应用进程只读共享的 Rime 基础词典，关闭 Rime LevelDB 用户词库，避免跨进程数据库锁。首拼个人短语的读音应以空格分隔音节，例如 `zhong guo`。当前候选上屏以整个组合串为单位，只展示能完整覆盖本次输入的 Rime 候选；逐段锁词和大型语法模型尚未接入。
