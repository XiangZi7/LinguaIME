<script setup lang="ts">
import type { IconName } from './icons'
import UiIcon from './UiIcon.vue'
withDefaults(
  defineProps<{ title: string; description: string; icon?: IconName }>(),
  { icon: 'book' }
)
</script>
<template>
  <div class="flex flex-col items-center px-5 py-12 text-center">
    <span
      class="mb-4 grid size-11 place-items-center rounded-xl border border-line bg-subtle text-muted"
    >
      <UiIcon
        :name="icon"
        :size="22"
      />
    </span>
    <h3 class="text-sm font-medium text-ink">
      {{ title }}
    </h3>
    <p class="mt-2 max-w-sm text-xs leading-6 text-muted">
      {{ description }}
    </p>
    <div
      v-if="$slots.default"
      class="mt-5"
    >
      <slot />
    </div>
  </div>
</template>
