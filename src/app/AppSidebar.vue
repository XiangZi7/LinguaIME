<script setup lang="ts">
import { UiIcon } from '@/components/ui'
import { NAVIGATION } from './navigation'
const groups = [...new Set(NAVIGATION.map(item => item.group))].map(label => ({ label, items: NAVIGATION.filter(item => item.group === label) }))
</script>
<template>
  <aside class="sticky top-0 flex h-screen w-[216px] shrink-0 flex-col border-r border-line bg-sidebar max-md:w-[70px]">
    <RouterLink to="/" class="flex h-[84px] shrink-0 items-center gap-2.5 px-6 max-md:justify-center max-md:px-0" aria-label="LinguaIME 首页">
      <span class="relative grid size-9 place-items-center rounded-[11px] bg-accent text-surface"><UiIcon name="leaf" :size="22" /></span>
      <span class="max-md:hidden"><span class="block text-[19px] leading-tight font-semibold tracking-[-0.8px] text-ink">Lingua<span class="font-normal text-accent">IME</span></span><span class="mt-1 block text-[9px] tracking-[0.22em] text-faint">让语言，自然连接</span></span>
    </RouterLink>
    <nav aria-label="主导航" class="flex-1 overflow-y-auto px-3 pb-4">
      <div v-for="group in groups" :key="group.label" class="mb-4">
        <p class="px-3 pb-2 pt-2 text-[10px] tracking-wider text-faint max-md:sr-only">{{ group.label }}</p>
        <RouterLink v-for="item in group.items" :key="item.path" :to="item.path" :title="item.label" class="mb-1 flex min-h-10 items-center gap-3 rounded-lg px-3 text-[13px] transition-colors max-md:justify-center max-md:px-0" :class="$route.path === item.path ? 'bg-accent-soft font-medium text-accent' : 'text-muted hover:bg-subtle hover:text-ink'" :aria-current="$route.path === item.path ? 'page' : undefined"><UiIcon :name="item.icon" :size="17" /><span class="max-md:hidden">{{ item.label }}</span><span v-if="$route.path === item.path" class="ml-auto size-1.5 rounded-full bg-accent max-md:hidden" /></RouterLink>
      </div>
    </nav>
    <div class="mx-4 mb-4 rounded-lg border border-line bg-surface p-3 max-md:mx-2 max-md:px-0">
      <div class="flex items-center gap-2 text-[11px] font-medium text-accent max-md:justify-center"><UiIcon name="shield" :size="15" /><span class="max-md:hidden">本地优先，安心输入</span></div><p class="mt-1.5 text-[10px] leading-relaxed text-muted max-md:hidden">你的词库与输入习惯，留在你的设备。</p>
    </div>
    <div class="flex h-12 shrink-0 items-center justify-between border-t border-line px-6 text-[10px] text-faint max-md:justify-center max-md:px-0"><span>v0.1.0</span><span class="max-md:hidden">Windows Preview</span></div>
  </aside>
</template>
