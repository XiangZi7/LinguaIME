<script setup lang="ts">
import { UiPageHeader, UiIcon } from '@/components/ui'
import CandidatePlayground from '@/features/candidate/CandidatePlayground.vue'
import OverviewMetrics from './OverviewMetrics.vue'
import EngineSummary from './EngineSummary.vue'
import RecentLearning from './RecentLearning.vue'
</script>
<template>
  <UiPageHeader title="让表达，自然发生。" description="熟悉的中文输入，多一点与世界的连接。" eyebrow="YOUR WORDS, YOUR WORLD"><span class="flex items-center gap-1.5 pb-1 text-[11px] text-muted"><UiIcon name="shield" :size="13" />本地优先 · 无需联网</span></UiPageHeader>
  <div class="space-y-5"><OverviewMetrics /><div class="grid items-start gap-5 2xl:grid-cols-[minmax(0,1fr)_280px] xl:grid-cols-[minmax(0,1fr)_258px]"><div class="space-y-5"><CandidatePlayground /><RecentLearning /></div><EngineSummary /></div><footer class="flex flex-wrap items-center justify-between gap-2 px-1 text-[10px] text-faint"><span>LinguaIME · 在输入中遇见另一种语言</span><span class="flex items-center gap-1.5"><UiIcon name="cloudOff" :size="13" />基础输入与释义查询均在本地完成</span></footer></div>
</template>
