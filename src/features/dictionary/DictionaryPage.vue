<script setup lang="ts">
import type { Candidate } from '@/types/engine'
import { computed, useTemplateRef } from 'vue'
import {
  UiButton,
  UiDialog,
  UiIcon,
  UiPageHeader,
  UiPanel,
  UiSegmented,
} from '@/components/ui'
import { service } from '@/services'
import { downloadJson } from '@/services/download'
import { useAppStore } from '@/stores/app'
import DictionaryTable from './DictionaryTable.vue'
import PhraseForm from './PhraseForm.vue'
import { useDictionary } from './useDictionary'
const app = useAppStore()
const {
  tab,
  search,
  page,
  loading,
  error,
  editorOpen,
  editing,
  importOpen,
  imported,
  busy,
  deleting,
  personal,
  query,
  edit,
  save,
  remove,
  readImport,
  confirmImport,
  run,
} = useDictionary()
const fileInput = useTemplateRef<HTMLInputElement>('fileInput')
const tabs = computed(() => [
  {
    value: 'system',
    label: `系统词库 · ${app.snapshot?.dictionaryCount.toLocaleString() ?? 0}`,
  },
  {
    value: 'personal',
    label: `个人词库 · ${app.snapshot?.phrases.length ?? 0}`,
  },
])
async function onFile(event: Event): Promise<void> {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0]
  if (file) await readImport(file)
  input.value = ''
}
function switchTab(value: string | number): void {
  tab.value = String(value)
  search.value = value === 'system' ? 'xue' : ''
}
async function favorite(candidate: Candidate): Promise<void> {
  await run(() => service.saveLearning(candidate, 'review'), '已加入生词本')
}
</script>
<template>
  <UiPageHeader
    title="词库管理"
    description="常用的词，值得被好好记住。"
    eyebrow="WORDS THAT MATTER"
  >
    <div class="flex gap-2">
      <UiButton @click="fileInput?.click()">
        <UiIcon
          name="upload"
          :size="14"
        />
        导入
      </UiButton>
      <UiButton
        variant="primary"
        @click="edit(null)"
      >
        <UiIcon
          name="plus"
          :size="15"
        />
        添加词条
      </UiButton>
    </div>
  </UiPageHeader>
  <input
    ref="fileInput"
    type="file"
    accept=".json,application/json"
    class="hidden"
    aria-label="导入个人词库 JSON"
    @change="onFile"
  />
  <UiPanel>
    <div
      class="flex flex-wrap items-center justify-between gap-4 border-b border-line p-5"
    >
      <UiSegmented
        :model-value="tab"
        label="词库来源"
        :options="tabs"
        @update:model-value="switchTab"
      />
      <div class="relative">
        <UiIcon
          name="search"
          :size="15"
          class="absolute top-3 left-3 text-faint"
        />
        <input
          v-model="search"
          :aria-label="tab === 'system' ? '搜索系统词库拼音' : '搜索个人词库'"
          :placeholder="
            tab === 'system' ? '输入拼音，检索系统词库' : '搜索词条、拼音或释义'
          "
          class="field w-64 pl-9 max-sm:w-full"
        />
      </div>
    </div>
    <p
      v-if="error"
      role="alert"
      class="border-b border-line bg-danger-soft px-5 py-3 text-xs text-danger"
    >
      {{ error }}
    </p>
    <DictionaryTable
      :entries="tab === 'system' ? page.items : personal"
      :personal="tab === 'personal'"
      :loading="tab === 'system' && loading"
      @edit="edit"
      @remove="deleting = $event"
      @favorite="favorite"
    />
    <footer
      class="flex flex-wrap items-center justify-between gap-3 border-t border-line px-5 py-4 text-[11px] text-muted"
    >
      <span>
        {{
          tab === 'system'
            ? `匹配 ${page.total} 条 · 查询范围为当前离线词典`
            : `共 ${personal.length} 条个人词条`
        }}
      </span>
      <div
        v-if="tab === 'system'"
        class="flex items-center gap-3"
      >
        <UiButton
          size="sm"
          :disabled="loading || page.pageIndex === 0"
          @click="query(page.pageIndex - 1)"
        >
          上一页
        </UiButton>
        <span>
          {{ page.pageCount ? page.pageIndex + 1 : 0 }} / {{ page.pageCount }}
        </span>
        <UiButton
          size="sm"
          :disabled="loading || page.pageIndex + 1 >= page.pageCount"
          @click="query(page.pageIndex + 1)"
        >
          下一页
        </UiButton>
      </div>
      <UiButton
        v-else
        size="sm"
        :disabled="!app.snapshot?.phrases.length"
        @click="downloadJson('linguaime-phrases.json', app.snapshot?.phrases)"
      >
        <UiIcon
          name="download"
          :size="13"
        />
        导出个人词库
      </UiButton>
    </footer>
  </UiPanel>
  <p class="mt-4 text-[11px] leading-6 text-muted">
    桌面输入词库来自 Rime pinyin-simp（Apache-2.0），英文释义来自 CC-CEDICT（CC
    BY-SA 4.0）。未收录的释义与词性不作推测，个人词条独立保存。
  </p>
  <UiDialog
    v-model="editorOpen"
    :title="editing ? '编辑个人词条' : '添加个人词条'"
    description="自己的词，自己的表达。释义可留空。"
  >
    <PhraseForm
      v-if="editorOpen"
      :phrase="editing"
      :busy="busy"
      @save="save"
      @cancel="editorOpen = false"
    />
  </UiDialog>
  <UiDialog
    v-model="importOpen"
    title="确认导入个人词库"
    :description="`已校验 ${imported.length} 条。相同中文和拼音的现有词条将更新释义。`"
  >
    <div class="max-h-64 overflow-auto rounded-lg border border-line">
      <div
        v-for="phrase in imported.slice(0, 100)"
        :key="phrase.id"
        class="flex justify-between gap-4 border-b border-line px-3 py-2 text-xs last:border-0"
      >
        <span>{{ phrase.text }}</span>
        <span class="text-muted">{{ phrase.reading }}</span>
        <span class="max-w-48 truncate text-translation">
          {{ phrase.translation }}
        </span>
      </div>
    </div>
    <p class="mt-3 text-[11px] text-muted">
      预览最多展示 100 条。导入以事务写入，格式错误不会部分保存。
    </p>
    <template #footer>
      <UiButton
        :disabled="busy"
        @click="importOpen = false"
      >
        取消
      </UiButton>
      <UiButton
        variant="primary"
        :busy="busy"
        :disabled="!imported.length"
        @click="confirmImport"
      >
        导入 {{ imported.length }} 条
      </UiButton>
    </template>
  </UiDialog>
  <UiDialog
    :model-value="!!deleting"
    title="删除这个个人词条？"
    :description="`将删除「${deleting?.text ?? ''}」。系统词典不受影响。`"
    @update:model-value="
      value => {
        if (!value) deleting = null
      }
    "
  >
    <p class="text-sm text-muted">此操作无法撤销，可先导出个人词库作为备份。</p>
    <template #footer>
      <UiButton @click="deleting = null">取消</UiButton>
      <UiButton
        variant="danger"
        :busy="busy"
        @click="remove"
      >
        删除词条
      </UiButton>
    </template>
  </UiDialog>
</template>
