# Tailwind UI 组件库

组件统一从 `@/components/ui` 导入。CSS-first Tailwind v4 令牌位于 `src/assets/styles/main.css`，使用 `@theme inline`，没有 v3 配置混用。

| 组件         | 契约与职责                                                                               |
| ------------ | ---------------------------------------------------------------------------------------- |
| UiButton     | variant: primary / secondary / ghost / danger；busy 禁止重复提交；原生 button attrs 透传 |
| UiIcon       | 类型安全的本地 SVG 图标名；默认 18 px、固定 stroke；无外网图标请求                       |
| UiBadge      | tone 和 dot；只表达状态，不伪造交互                                                      |
| UiSwitch     | boolean v-model、label、disabled；role=switch 与 aria-checked                            |
| UiSelect     | string/number v-model、options、label；保留原生键盘操作                                  |
| UiSegmented  | string/number v-model、options、label；aria-pressed 表达选择                             |
| UiPanel      | title、description、actions 插槽与内容插槽；统一分组边界                                 |
| UiSettingRow | title、description 与控件插槽；统一行高、留白和分隔线                                    |
| UiPageHeader | title、description、eyebrow；默认插槽放页面动作                                          |
| UiEmptyState | title、description、icon 与可选动作插槽                                                  |
| UiDialog     | boolean v-model、title、description、footer；原生 dialog 提供焦点约束、Esc 与模态背景    |

```vue
<script setup lang="ts">
import { UiPanel, UiSettingRow, UiSwitch } from '@/components/ui'
import { useAppStore } from '@/stores/app'
const app = useAppStore()
</script>

<template>
  <UiPanel title="本地学习">
    <UiSettingRow
      title="学习常用词"
      description="只记录词条 ID 与选择次数。"
    >
      <UiSwitch
        v-model="app.draft.autoLearn"
        label="学习常用词"
      />
    </UiSettingRow>
  </UiPanel>
</template>
```

候选职责拆分为 CandidatePlayground（管理端交互容器）、useCandidatePreview（查询与键盘状态）、CandidateWindow（组合头/列表/页脚）和 CandidateRow（词条呈现）。候选组件通过 props 接收分页与配置，通过 choose/page 事件输出操作，不访问 SQLite。

自定义窗口栏独立为 AppTitlebar 与 useWindowControls。只有桌面环境展示最小化、最大化/还原和关闭按钮；拖动区采用 `data-tauri-drag-region`，按钮不进入拖动区。关闭窗口前通过对话框保护未保存的设置，Alt+F4 也受保护。

颜色统一使用 `surface`、`canvas`、`line`、`ink`、`muted`、`accent`、`translation` 等语义令牌；浅色/深色由根节点 dark 类切换。候选高亮单独采用 `data-accent`，不为缺失释义、词性或例句添加假占位。遵守可见焦点、键盘导航、原生表单标签和 reduced-motion。

候选默认高亮按参考图调整为浅青色，同时保留浅绿和浅蓝选项。原生端采用 Direct2D/DirectWrite 绘制透明圆角、短阴影、图标/组合串/模式头部与数字/翻页页脚；详细模式只展开选中项。`tsf_context_test.exe --preview` 从同一渲染管线导出实际像素供视觉比对，避免只验证 HTML 预览。
