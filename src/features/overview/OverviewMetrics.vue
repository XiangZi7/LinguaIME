<script setup lang="ts">
import { computed } from 'vue'
import { useAppStore } from '@/stores/app'
import { UiIcon } from '@/components/ui'
import type { IconName } from '@/components/ui/icons'
const app = useAppStore()
const metrics = computed((): { label: string; value: string; detail: string; icon: IconName }[] => [
  { label: '可用语言', value: '01', detail: '中文拼音 · 英语辅助', icon: 'globe' },
  { label: '本地词条', value: (app.snapshot?.dictionaryCount ?? 0).toLocaleString(), detail: 'CC-CEDICT · 离线可用', icon: 'book' },
  { label: '我的生词本', value: String(app.snapshot?.statistics.favorites ?? 0).padStart(2, '0'), detail: '每一个新词，都是进步', icon: 'star' },
  { label: '已掌握词汇', value: String(app.snapshot?.statistics.mastered ?? 0).padStart(2, '0'), detail: '把遇见的词，变成熟悉', icon: 'chart' },
])
</script>
<template><div class="grid grid-cols-2 gap-4 xl:grid-cols-4"><div v-for="metric in metrics" :key="metric.label" class="rounded-xl border border-line bg-surface p-4"><div class="flex items-center justify-between text-[11px] text-muted"><span>{{ metric.label }}</span><UiIcon :name="metric.icon" :size="16" class="text-faint" /></div><p class="mt-4 text-[27px] leading-none font-semibold tracking-tight text-ink tabular-nums">{{ metric.value }}</p><p class="mt-3 text-[10px] text-faint">{{ metric.detail }}</p></div></div></template>
