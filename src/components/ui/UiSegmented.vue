<script setup lang="ts">
import type { SelectOption } from '@/types/settings'
const model = defineModel<string | number>({ required: true })
defineProps<{ label: string; options: SelectOption[] }>()
</script>
<template><div role="group" :aria-label="label" class="inline-flex rounded-lg border border-line bg-subtle p-1"><button v-for="option in options" :key="option.value" type="button" :aria-pressed="model === option.value" :disabled="option.disabled" class="min-h-8 cursor-pointer rounded-md px-3.5 text-xs font-medium transition-colors disabled:opacity-40" :class="model === option.value ? 'bg-surface text-ink shadow-sm ring-1 ring-line' : 'text-muted hover:text-ink'" @click="model = option.value">{{ option.label }}</button></div></template>
