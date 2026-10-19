<script setup lang="ts">
import type { Candidate, CandidatePage } from '@/types/engine'
import type { Settings } from '@/types/settings'
import { UiIcon } from '@/components/ui'
import CandidateRow from './CandidateRow.vue'
defineProps<{
  page: CandidatePage
  selected: number
  settings: Settings
  loading?: boolean
}>()
defineEmits<{ choose: [candidate: Candidate]; page: [direction: number] }>()
</script>
<template>
  <div
    class="w-full max-w-[360px] overflow-hidden rounded-xl border border-line bg-surface shadow-[0_8px_25px_-10px_#243c3133]"
    :data-accent="settings.accent"
    :aria-busy="loading || undefined"
  >
    <header
      class="flex min-h-10 items-center justify-between gap-3 border-b border-line px-3.5 text-[11px]"
    >
      <div class="flex min-w-0 items-center gap-2 text-ink">
        <UiIcon
          :name="settings.displayMode === 'chinese' ? 'keyboard' : 'globe'"
          :size="13"
          class="text-accent"
        />
        <span class="truncate">{{ page.composition || '等待拼音输入' }}</span>
      </div>
      <span class="shrink-0 text-[10px] text-faint">
        {{
          settings.displayMode === 'chinese'
            ? '纯中文'
            : settings.displayMode === 'detailed'
              ? '双语详细'
              : '双语简洁'
        }}
      </span>
    </header>
    <div
      role="listbox"
      aria-label="拼音候选词"
      class="space-y-0.5 p-1.5"
      :class="loading ? 'opacity-50' : ''"
    >
      <CandidateRow
        v-for="(candidate, index) in page.items"
        :key="candidate.id"
        :candidate="candidate"
        :index="index"
        :selected="index === selected"
        :mode="settings.displayMode"
        :font-size="settings.fontSize"
        :compact="settings.rowSpacing === 'compact'"
        @choose="$emit('choose', $event)"
      />
      <p
        v-if="!page.items.length"
        class="px-3 py-7 text-center text-xs text-muted"
      >
        {{
          loading
            ? '正在检索本地词典…'
            : page.composition
              ? '未找到匹配词条'
              : '输入拼音，开始一次表达'
        }}
      </p>
    </div>
    <footer
      class="flex h-8 items-center justify-between border-t border-line px-3 text-[10px] text-faint"
    >
      <span>↑↓ 选择 · Enter 确认</span>
      <span class="flex items-center gap-1.5">
        <button
          type="button"
          aria-label="上一页候选"
          :disabled="loading || page.pageIndex === 0"
          class="grid size-6 cursor-pointer place-items-center rounded hover:bg-subtle disabled:cursor-default disabled:opacity-30"
          @click="$emit('page', -1)"
        >
          <UiIcon
            name="left"
            :size="12"
          />
        </button>
        <span>
          {{ page.pageCount ? page.pageIndex + 1 : 0 }} / {{ page.pageCount }}
        </span>
        <button
          type="button"
          aria-label="下一页候选"
          :disabled="loading || page.pageIndex + 1 >= page.pageCount"
          class="grid size-6 cursor-pointer place-items-center rounded hover:bg-subtle disabled:cursor-default disabled:opacity-30"
          @click="$emit('page', 1)"
        >
          <UiIcon
            name="chevron"
            :size="12"
          />
        </button>
      </span>
    </footer>
  </div>
</template>
