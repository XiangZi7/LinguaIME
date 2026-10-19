<script setup lang="ts">
import type { SelectOption } from '@/types/settings'
import UiIcon from './UiIcon.vue'
defineProps<{ label: string; options: SelectOption[]; disabled?: boolean }>()
const model = defineModel<string | number>({ required: true })
</script>
<template>
  <div class="relative inline-flex min-w-36">
    <select
      v-model="model"
      :aria-label="label"
      :disabled="disabled"
      class="field w-full appearance-none pr-9 disabled:opacity-50"
    >
      <option
        v-for="option in options"
        :key="option.value"
        :value="option.value"
        :disabled="option.disabled"
      >
        {{ option.label }}
      </option>
    </select>
    <UiIcon
      name="down"
      :size="14"
      class="pointer-events-none absolute top-1/2 right-3 -translate-y-1/2 text-muted"
    />
  </div>
</template>
