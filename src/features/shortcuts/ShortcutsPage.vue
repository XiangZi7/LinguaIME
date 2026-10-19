<script setup lang="ts">
import { storeToRefs } from 'pinia'
import { useAppStore } from '@/stores/app'
import { DEFAULT_SETTINGS } from '@/constants/settings'
import { UiPageHeader, UiPanel, UiSettingRow, UiSelect, UiButton, UiIcon, UiBadge } from '@/components/ui'
const { draft } = storeToRefs(useAppStore())
const shortcuts = [
  { title: '选择候选词', description: '仅在组合串或候选列表激活时生效', keys: ['1–9'] },
  { title: '选择上一项 / 下一项', description: '在当前页内循环选择', keys: ['↑', '↓'] },
  { title: '提交当前候选', description: 'Enter 行为可在输入设置中更改', keys: ['Space', 'Enter'] },
  { title: '删除组合串字符', description: '没有组合串时交还目标应用', keys: ['Backspace'] },
  { title: '取消组合', description: '关闭候选，不提交文字', keys: ['Esc'] },
]
function reset(): void { draft.value.previousPageKey = DEFAULT_SETTINGS.previousPageKey; draft.value.nextPageKey = DEFAULT_SETTINGS.nextPageKey }
</script>
<template><UiPageHeader title="快捷键" description="少一点操作，多一点流畅。" eyebrow="KEEP YOUR FLOW"><UiButton @click="reset"><UiIcon name="refresh" :size="14" />恢复默认翻页键</UiButton></UiPageHeader><div class="max-w-4xl space-y-5"><div class="flex items-start gap-3 rounded-xl border border-accent/15 bg-accent-wash px-5 py-4 text-xs leading-6 text-muted"><UiIcon name="info" class="mt-0.5 text-accent" /><span>翻页键只在组合态生效。输入 URL、代码和普通文本时，<kbd class="keycap mx-1">-</kbd>与<kbd class="keycap mx-1">=</kbd>会原样传递。自定义后会替换默认绑定。</span></div><UiPanel title="候选翻页" description="选项按互斥按键集合约束，避免与字母、数字和提交键冲突。"><UiSettingRow title="上一页"><UiSelect v-model="draft.previousPageKey" label="上一页快捷键" :options="[{ value: '-', label: '-（减号）' }, { value: 'PageUp', label: 'PageUp' }, { value: '[', label: '[（左方括号）' }]" /></UiSettingRow><UiSettingRow title="下一页"><UiSelect v-model="draft.nextPageKey" label="下一页快捷键" :options="[{ value: '=', label: '=（等号）' }, { value: 'PageDown', label: 'PageDown' }, { value: ']', label: ']（右方括号）' }]" /></UiSettingRow></UiPanel><UiPanel title="组合输入"><UiSettingRow v-for="shortcut in shortcuts" :key="shortcut.title" :title="shortcut.title" :description="shortcut.description"><div class="flex gap-1.5"><kbd v-for="key in shortcut.keys" :key="key" class="keycap min-h-7 min-w-8">{{ key }}</kbd></div></UiSettingRow></UiPanel><UiPanel title="系统快捷键"><UiSettingRow title="切换 Windows 输入法" description="由 Windows 管理，不在此处修改。"><div class="flex items-center gap-2"><kbd class="keycap">Win</kbd><span class="text-xs text-faint">+</span><kbd class="keycap">Space</kbd><UiBadge>系统级</UiBadge></div></UiSettingRow></UiPanel></div></template>
