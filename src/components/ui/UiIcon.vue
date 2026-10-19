<script setup lang="ts">
import { ICON_PATHS } from './icons'
import type { IconName } from './icons'
withDefaults(defineProps<{ name: IconName; size?: number }>(), { size: 18 })
</script>
<template>
  <svg :width="size" :height="size" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" class="shrink-0">
    <path :d="ICON_PATHS[name]" />
  </svg>
</template>
