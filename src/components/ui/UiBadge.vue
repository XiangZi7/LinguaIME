<script setup lang="ts">
withDefaults(defineProps<{ tone?: 'green' | 'neutral' | 'amber' | 'blue'; dot?: boolean }>(), { tone: 'neutral' })
</script>
<template><span class="inline-flex items-center gap-1.5 rounded-md px-2 py-1 text-[11px] leading-none font-medium" :class="{ 'bg-accent-soft text-accent': tone === 'green', 'bg-subtle text-muted': tone === 'neutral', 'bg-warning-soft text-warning': tone === 'amber', 'bg-blue-soft text-blue-ink': tone === 'blue' }"><span v-if="dot" class="size-1.5 rounded-full bg-current" /><slot /></span></template>
