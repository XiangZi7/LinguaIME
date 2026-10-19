<script setup lang="ts">
import { computed } from 'vue'
import { useAppStore } from '@/stores/app'
import { UiBadge, UiButton, UiIcon } from '@/components/ui'
import { service, errorMessage } from '@/services'
import { useCandidatePreview } from './useCandidatePreview'
import CandidateWindow from './CandidateWindow.vue'
withDefaults(defineProps<{ compact?: boolean }>(), { compact: false })
const app = useAppStore()
const { composition, page, selected, committed, loading, error, choose, turnPage, onKeydown } = useCandidatePreview()
const selectedItem = computed(() => page.value.items[selected.value])
const isFavorite = computed(() => app.snapshot?.learning.some(item => item.candidate.id === selectedItem.value?.id))
async function favorite(): Promise<void> {
  if (!selectedItem.value) return
  try { await service.saveLearning(selectedItem.value, 'review'); await app.refresh(); app.notify('已加入生词本') }
  catch (error) { app.notify(errorMessage(error), 'error') }
}
</script>
<template>
  <section class="overflow-hidden rounded-xl border border-line bg-surface">
    <header class="flex items-center justify-between border-b border-line px-5 py-4"><div class="flex items-center gap-2 text-sm font-semibold text-ink"><UiIcon name="keyboard" :size="17" class="text-accent" />输入体验</div><UiBadge tone="neutral">交互预览</UiBadge></header>
    <div class="grid gap-7 p-6 max-sm:p-4" :class="compact ? 'grid-cols-1' : 'xl:grid-cols-[1fr_360px]'">
      <div class="flex min-w-0 flex-col"><div class="mb-4 flex items-center justify-between text-[10px] text-muted"><span class="tracking-wide">在这里，试着输入拼音</span><span>中文 · 全拼</span></div><label class="sr-only" for="pinyin-preview">输入预览拼音</label><div class="min-h-36 flex-1 rounded-lg border border-dashed border-line bg-canvas p-4"><p class="text-sm leading-7 text-ink">{{ committed || '每一次表达，都是新的连接。' }}</p><input id="pinyin-preview" v-model="composition" aria-label="输入预览拼音" autocomplete="off" spellcheck="false" maxlength="128" class="mt-4 w-full border-0 border-b border-accent/40 bg-transparent pb-1 text-sm text-accent outline-none placeholder:text-faint focus:border-accent" placeholder="试试 xuexi、nihao 或 zhongwen" @keydown="onKeydown" /><p v-if="error" role="alert" class="mt-2 text-xs text-danger">{{ error }}</p></div><p class="mt-4 text-[11px] leading-6 text-muted">中文是表达，英文是连接。<br />选择一个候选词，在熟悉的输入中积累新的词汇。</p><div class="mt-4 flex flex-wrap gap-2"><button v-for="example in ['xuexi', 'nihao', 'zhongwen']" :key="example" type="button" class="cursor-pointer rounded-md border border-line px-2 py-1 text-[10px] text-muted hover:border-accent hover:text-accent" @click="composition = example">{{ example }}</button><button v-if="committed" type="button" class="ml-auto cursor-pointer text-[10px] text-muted hover:text-ink" @click="committed = ''">清空预览文本</button></div></div>
      <div class="flex flex-col items-center justify-center rounded-xl bg-canvas p-4"><CandidateWindow :page="page" :selected="selected" :settings="app.draft" :loading="loading" @choose="choose" @page="turnPage" /><div class="mt-3 flex min-h-7 w-full items-center justify-between gap-2 text-[10px] text-faint"><span>仅查询本地词典</span><UiButton size="sm" variant="ghost" :disabled="!selectedItem || !!isFavorite || loading" @click="favorite"><UiIcon name="star" :size="13" />{{ isFavorite ? '已收藏' : '收藏选中词' }}</UiButton></div></div>
    </div>
  </section>
</template>
