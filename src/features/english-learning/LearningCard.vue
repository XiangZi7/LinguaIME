<script setup lang="ts">
import type { LearningItem } from '@/types/engine'
import { UiBadge, UiButton, UiIcon } from '@/components/ui'
defineProps<{ item: LearningItem; busy: boolean }>()
defineEmits<{
  detail: [item: LearningItem]
  status: [item: LearningItem]
  remove: [item: LearningItem]
}>()
</script>
<template>
  <article class="flex flex-col rounded-xl border border-line bg-surface p-5">
    <div class="flex items-center justify-between">
      <span class="text-[10px] text-faint">{{ item.candidate.source }}</span>
      <UiBadge :tone="item.status === 'mastered' ? 'green' : 'neutral'">
        {{ item.status === 'mastered' ? '已掌握' : '待复习' }}
      </UiBadge>
    </div>
    <button
      type="button"
      class="mt-5 cursor-pointer text-left"
      @click="$emit('detail', item)"
    >
      <h2 class="text-xl font-medium text-ink">
        {{ item.candidate.text }}
      </h2>
      <p class="mt-2 line-clamp-2 min-h-10 text-sm leading-6 text-translation">
        {{ item.candidate.translation || '暂无可靠英文释义' }}
      </p>
      <p class="mt-3 truncate text-[11px] text-muted">
        {{ item.candidate.reading }}
      </p>
    </button>
    <div
      class="mt-5 flex items-center justify-between gap-2 border-t border-line pt-4"
    >
      <UiButton
        size="sm"
        :disabled="busy"
        :variant="item.status === 'mastered' ? 'ghost' : 'secondary'"
        @click="$emit('status', item)"
      >
        <UiIcon
          :name="item.status === 'mastered' ? 'refresh' : 'check'"
          :size="13"
        />
        {{ item.status === 'mastered' ? '重新复习' : '标记掌握' }}
      </UiButton>
      <UiButton
        size="sm"
        variant="ghost"
        :disabled="busy"
        :aria-label="`移除收藏${item.candidate.text}`"
        @click="$emit('remove', item)"
      >
        <UiIcon
          name="trash"
          :size="14"
        />
      </UiButton>
    </div>
  </article>
</template>
