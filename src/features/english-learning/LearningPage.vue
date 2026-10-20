<script setup lang="ts">
import type { LearningItem } from '@/types/engine'
import { computed, reactive, toRefs } from 'vue'
import {
  UiBadge,
  UiButton,
  UiDialog,
  UiEmptyState,
  UiIcon,
  UiPageHeader,
  UiSegmented,
} from '@/components/ui'
import { errorMessage, service } from '@/services'
import { useAppStore } from '@/stores/app'
import LearningCard from './LearningCard.vue'
const app = useAppStore()
// 响应式状态
const state = reactive({
  // 中文或英文检索词
  search: '',
  // 当前学习状态筛选
  filter: 'all',
  // 正在查看的完整词条
  detail: null as LearningItem | null,
  // 明确请求移除的收藏
  removing: null as LearningItem | null,
  // 操作互斥状态
  busy: false,
})
const items = computed(() =>
  (app.snapshot?.learning ?? []).filter(
    item =>
      (state.filter === 'all' || item.status === state.filter) &&
      `${item.candidate.text} ${item.candidate.translation ?? ''} ${item.candidate.reading}`
        .toLowerCase()
        .includes(state.search.toLowerCase())
  )
)
const dataLicense = computed(() => {
  const source = state.detail?.candidate.source ?? ''
  if (source.includes('CC-CEDICT'))
    return 'CC BY-SA 4.0 · CC-CEDICT contributors'
  if (source === 'Rime')
    return 'Rime pinyin-simp · Apache-2.0；组合句不含推测释义'
  return '个人提供的词条与释义'
})
const options = computed(() => [
  { value: 'all', label: `全部 · ${app.snapshot?.learning.length ?? 0}` },
  { value: 'review', label: '待复习' },
  { value: 'mastered', label: '已掌握' },
])
async function run(
  action: () => Promise<void>,
  message: string
): Promise<void> {
  state.busy = true
  try {
    await action()
    await app.refresh()
    app.notify(message)
  } catch (error) {
    app.notify(errorMessage(error), 'error')
  } finally {
    state.busy = false
  }
}
async function toggle(item: LearningItem): Promise<void> {
  await run(
    () =>
      service.saveLearning(
        item.candidate,
        item.status === 'mastered' ? 'review' : 'mastered'
      ),
    item.status === 'mastered' ? '已设为待复习' : '又掌握了一个词，继续保持'
  )
  state.detail = null
}
async function remove(): Promise<void> {
  const id = state.removing?.candidate.id
  if (id) await run(() => service.deleteLearning(id), '已移除收藏')
  state.removing = null
}
const { search, filter, detail, removing, busy } = toRefs(state)
</script>
<template>
  <UiPageHeader
    title="我的生词本"
    description="在输入中遇见，在复习中熟悉。每一个词，都是新的可能。"
    eyebrow="A WORD A DAY"
  />
  <div class="mb-6 flex flex-wrap items-center justify-between gap-4">
    <UiSegmented
      v-model="filter"
      label="学习状态"
      :options="options"
    />
    <div class="relative">
      <UiIcon
        name="search"
        :size="15"
        class="absolute top-3 left-3 text-faint"
      />
      <input
        v-model="search"
        aria-label="搜索生词本"
        placeholder="搜索中文、英文或拼音"
        class="field w-64 pl-9"
      />
    </div>
  </div>
  <div
    v-if="items.length"
    class="grid gap-4 sm:grid-cols-2 2xl:grid-cols-3"
  >
    <LearningCard
      v-for="item in items"
      :key="item.candidate.id"
      :item="item"
      :busy="busy"
      @detail="detail = $event"
      @status="toggle"
      @remove="removing = $event"
    />
  </div>
  <div
    v-else
    class="rounded-xl border border-line bg-surface"
  >
    <UiEmptyState
      :title="
        search || filter !== 'all'
          ? '没有找到匹配的收藏'
          : '让第一个词，成为新的开始'
      "
      description="从输入预览或系统词库中收藏词条。它们只保存在本地，随时可以回来复习。"
      icon="star"
    >
      <RouterLink
        to="/dictionary"
        class="text-link"
      >
        去词库发现新词
        <UiIcon
          name="arrow"
          :size="14"
        />
      </RouterLink>
    </UiEmptyState>
  </div>
  <UiDialog
    :model-value="!!detail"
    title="词条详情"
    description="仅展示词典提供的信息，不推测词性、音标或例句。"
    @update:model-value="
      value => {
        if (!value) detail = null
      }
    "
  >
    <template v-if="detail">
      <div class="flex items-center gap-3">
        <h2 class="text-3xl font-medium">
          {{ detail.candidate.text }}
        </h2>
        <UiBadge tone="green">
          {{ detail.candidate.source }}
        </UiBadge>
      </div>
      <p class="mt-4 text-base leading-8 text-translation">
        {{ detail.candidate.translation || '暂无可靠英文释义' }}
      </p>
      <dl class="mt-6 space-y-4 border-t border-line pt-5 text-xs">
        <div class="flex gap-5">
          <dt class="w-14 text-faint">拼音</dt>
          <dd>{{ detail.candidate.reading }}</dd>
        </div>
        <div class="flex gap-5">
          <dt class="w-14 text-faint">收藏于</dt>
          <dd>
            {{ new Date(detail.createdAt * 1000).toLocaleDateString('zh-CN') }}
          </dd>
        </div>
        <div class="flex gap-5">
          <dt class="w-14 text-faint">数据许可</dt>
          <dd>
            {{ dataLicense }}
          </dd>
        </div>
      </dl>
    </template>
    <template #footer>
      <UiButton @click="detail = null">关闭</UiButton>
      <UiButton
        v-if="detail"
        variant="primary"
        :busy="busy"
        @click="toggle(detail)"
      >
        {{ detail.status === 'mastered' ? '设为待复习' : '标记已掌握' }}
      </UiButton>
    </template>
  </UiDialog>
  <UiDialog
    :model-value="!!removing"
    title="移除这个收藏？"
    :description="`「${removing?.candidate.text ?? ''}」将从生词本移除，原词典不受影响。`"
    @update:model-value="
      value => {
        if (!value) removing = null
      }
    "
  >
    <p class="text-xs leading-6 text-muted">需要时可以重新从词库收藏。</p>
    <template #footer>
      <UiButton @click="removing = null">取消</UiButton>
      <UiButton
        variant="danger"
        :busy="busy"
        @click="remove"
      >
        移除收藏
      </UiButton>
    </template>
  </UiDialog>
</template>
