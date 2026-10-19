<script setup lang="ts">
import type { Candidate } from '@/types/engine'
import type { DisplayMode } from '@/types/settings'
defineProps<{
  candidate: Candidate
  index: number
  selected: boolean
  mode: DisplayMode
  compact: boolean
  fontSize: number
}>()
defineEmits<{ choose: [candidate: Candidate] }>()
</script>
<template>
  <button
    type="button"
    role="option"
    :aria-selected="selected"
    :aria-label="`${index + 1} ${candidate.text}${mode !== 'chinese' && candidate.translation ? ` ${candidate.translation}` : ''}`"
    class="flex w-full cursor-pointer items-start gap-3 rounded-md px-2.5 text-left transition-colors"
    :class="[
      selected
        ? 'bg-candidate-selected text-candidate-text'
        : 'text-ink hover:bg-subtle',
      compact ? 'py-1.5' : 'py-2',
    ]"
    @click="$emit('choose', candidate)"
  >
    <span class="w-3 shrink-0 pt-1 text-[11px] text-muted">
      {{ index + 1 }}
    </span>
    <span class="min-w-0 flex-1">
      <span class="flex min-w-0 items-center gap-4">
        <span
          class="max-w-[45%] shrink-0 truncate leading-6"
          :style="{ fontSize: `${fontSize}px` }"
        >
          {{ candidate.text }}
        </span>
        <span
          v-if="mode !== 'chinese' && candidate.translation"
          class="min-w-0 truncate text-[12px] leading-6 text-translation"
        >
          {{ candidate.translation }}
        </span>
      </span>
      <span
        v-if="mode === 'detailed'"
        class="mt-1 block truncate text-[10px] text-muted"
      >
        {{ candidate.reading }}
        <span v-if="selected">· {{ candidate.source }}</span>
      </span>
    </span>
  </button>
</template>
