<script setup lang="ts">
import { computed } from 'vue'
import { UiBadge, UiIcon, UiPageHeader, UiPanel } from '@/components/ui'
import { useAppStore } from '@/stores/app'
const app = useAppStore()
const days = computed(() =>
  Array.from({ length: 7 }, (_, index) => {
    const day = new Date()
    day.setDate(day.getDate() - 6 + index)
    const date = new Intl.DateTimeFormat('sv-SE').format(day)
    return {
      date,
      label: new Intl.DateTimeFormat('zh-CN', { weekday: 'short' }).format(day),
      count:
        app.snapshot?.statistics.daily.find(item => item.date === date)
          ?.count ?? 0,
    }
  })
)
const maxCount = computed(() =>
  Math.max(1, ...days.value.map(day => day.count))
)
const metrics = computed(() => [
  {
    label: '已收藏',
    value: app.snapshot?.statistics.favorites ?? 0,
    unit: '个词条',
  },
  {
    label: '已掌握',
    value: app.snapshot?.statistics.mastered ?? 0,
    unit: '个词条',
  },
  {
    label: '待复习',
    value:
      (app.snapshot?.statistics.favorites ?? 0) -
      (app.snapshot?.statistics.mastered ?? 0),
    unit: '个词条',
  },
  {
    label: '记录的候选选择',
    value: app.snapshot?.statistics.selections ?? 0,
    unit: '次选择',
  },
])
</script>
<template>
  <UiPageHeader
    title="学习统计"
    description="不用着急，每一次积累，都在悄悄发生。"
    eyebrow="SMALL STEPS, REAL PROGRESS"
  />
  <div class="space-y-5">
    <div class="grid grid-cols-2 gap-4 xl:grid-cols-4">
      <div
        v-for="metric in metrics"
        :key="metric.label"
        class="rounded-xl border border-line bg-surface p-5"
      >
        <p class="text-xs text-muted">
          {{ metric.label }}
        </p>
        <p class="mt-4 text-3xl font-semibold tracking-tight tabular-nums">
          {{ metric.value.toLocaleString() }}
        </p>
        <p class="mt-2 text-[10px] text-faint">
          {{ metric.unit }}
        </p>
      </div>
    </div>
    <UiPanel
      title="最近七天的输入积累"
      description="只统计启用后主动选择候选的次数，不保存输入内容。"
    >
      <template #actions>
        <UiBadge
          :tone="app.snapshot?.settings.collectStats ? 'green' : 'neutral'"
          dot
        >
          {{
            app.snapshot?.settings.collectStats
              ? '本地统计已开启'
              : '本地统计已关闭'
          }}
        </UiBadge>
      </template>
      <div class="px-6 pt-8 pb-5">
        <div
          class="flex h-52 items-end gap-3 sm:gap-7"
          role="img"
          :aria-label="
            days.map(day => `${day.date}：${day.count}次`).join('；')
          "
        >
          <div
            v-for="day in days"
            :key="day.date"
            class="flex h-full min-w-0 flex-1 flex-col justify-end"
          >
            <span class="mb-2 text-center text-[11px] text-muted">
              {{ day.count }}
            </span>
            <div
              class="mx-auto w-full max-w-16 rounded-t-md bg-accent/70 transition-[height] duration-200"
              :style="{
                height: `${day.count ? Math.max(6, (day.count / maxCount) * 155) : 2}px`,
              }"
              :title="`${day.date} · ${day.count} 次选择`"
            />
            <p class="mt-4 text-center text-[10px] text-muted">
              {{ day.label }}
            </p>
            <p class="mt-1 text-center text-[9px] text-faint">
              {{ day.date.slice(5).replace('-', '/') }}
            </p>
          </div>
        </div>
      </div>
      <div
        class="flex flex-wrap items-center justify-between gap-3 border-t border-line bg-canvas/50 px-5 py-4 text-[11px] text-muted"
      >
        <span class="flex items-center gap-2">
          <UiIcon
            name="shield"
            :size="14"
          />
          统计默认关闭，数据仅保存在本地
        </span>
        <RouterLink
          to="/privacy"
          class="text-link"
        >
          管理统计与隐私
          <UiIcon
            name="arrow"
            :size="13"
          />
        </RouterLink>
      </div>
    </UiPanel>
    <UiPanel title="最近的学习记录">
      <div
        v-if="app.snapshot?.learning.length"
        class="divide-y divide-line"
      >
        <div
          v-for="item in app.snapshot.learning.slice(0, 8)"
          :key="item.candidate.id"
          class="flex items-center justify-between gap-4 px-5 py-4"
        >
          <div class="flex items-center gap-3">
            <UiIcon
              :name="item.status === 'mastered' ? 'check' : 'star'"
              :size="16"
              class="text-accent"
            />
            <span class="text-sm">{{ item.candidate.text }}</span>
            <span class="text-[11px] text-muted">
              {{ item.status === 'mastered' ? '已标记掌握' : '加入生词本' }}
            </span>
          </div>
          <time class="text-[10px] text-faint">
            {{ new Date(item.updatedAt * 1000).toLocaleDateString('zh-CN') }}
          </time>
        </div>
      </div>
      <p
        v-else
        class="px-5 py-9 text-center text-xs text-muted"
      >
        还没有学习记录。从收藏一个词开始吧。
      </p>
    </UiPanel>
  </div>
</template>
