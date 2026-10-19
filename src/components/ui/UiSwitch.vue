<script setup lang="ts">
const model = defineModel<boolean>({ required: true })
defineProps<{ label: string; disabled?: boolean }>()
</script>
<template>
  <button type="button" role="switch" :aria-checked="model" :aria-label="label" :disabled="disabled" class="relative inline-flex min-h-11 min-w-11 cursor-pointer items-center justify-center disabled:cursor-not-allowed disabled:opacity-40" @click="model = !model">
    <span class="flex h-[22px] w-10 items-center rounded-full p-[3px] transition-colors duration-150" :class="model ? 'bg-accent' : 'bg-switch-off'"><span class="size-4 rounded-full bg-white shadow-sm transition-transform duration-150" :class="model ? 'translate-x-[18px]' : 'translate-x-0'" /></span>
  </button>
</template>
