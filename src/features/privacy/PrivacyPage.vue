<script setup lang="ts">
import { storeToRefs } from 'pinia'
import { computed, reactive, toRefs } from 'vue'
import {
  UiBadge,
  UiButton,
  UiDialog,
  UiIcon,
  UiPageHeader,
  UiPanel,
  UiSettingRow,
  UiSwitch,
} from '@/components/ui'
import { errorMessage, service } from '@/services'
import { downloadJson } from '@/services/download'
import { useAppStore } from '@/stores/app'
const app = useAppStore()
const { draft } = storeToRefs(app)
// 响应式状态
const state = reactive({
  // 需要二次确认的清除范围
  scope: '',
  // 防止重复导出或清除
  busy: false,
})
const confirmation = computed(
  () =>
    ({
      frequency: '清空本地词频',
      learning: '清空学习数据',
      all: '重置全部个人数据',
    })[state.scope] ?? ''
)
async function exportData(): Promise<void> {
  state.busy = true
  try {
    downloadJson('linguaime-personal-data.json', await service.exportData())
    app.notify('个人数据已导出')
  } catch (error) {
    app.notify(errorMessage(error), 'error')
  } finally {
    state.busy = false
  }
}
async function clear(): Promise<void> {
  state.busy = true
  try {
    await service.clearData(state.scope)
    await app.refresh(state.scope === 'all')
    app.notify('所选本地数据已清除')
    state.scope = ''
  } catch (error) {
    app.notify(errorMessage(error), 'error')
  } finally {
    state.busy = false
  }
}
const { scope, busy } = toRefs(state)
</script>
<template>
  <UiPageHeader
    title="隐私与数据"
    description="你的表达，始终由你掌握。"
    eyebrow="LOCAL FIRST. ALWAYS YOURS."
  />
  <div class="max-w-4xl space-y-5">
    <div
      class="flex gap-4 rounded-xl border border-accent/15 bg-accent-wash p-5"
    >
      <span
        class="grid size-10 shrink-0 place-items-center rounded-xl bg-accent-soft text-accent"
      >
        <UiIcon
          name="shield"
          :size="22"
        />
      </span>
      <div>
        <h2 class="text-sm font-semibold">输入留在本地，安心不需要开关。</h2>
        <p class="mt-2 text-xs leading-6 text-muted">
          基础输入、候选排序与词典释义均在设备上完成。不默认采集完整输入，不上传密码或敏感文本，当前版本没有在线翻译请求。
        </p>
      </div>
    </div>
    <UiPanel title="本地学习与统计">
      <UiSettingRow
        title="学习常用词"
        description="保存词条 ID、选择次数与最近选择时间，用于个性化排序。"
      >
        <UiSwitch
          v-model="draft.autoLearn"
          label="学习常用词"
        />
      </UiSettingRow>
      <UiSettingRow
        title="本地输入统计"
        description="默认关闭。启用后仅记录候选选择的事件类型与时间，不记录完整输入内容。"
      >
        <UiSwitch
          v-model="draft.collectStats"
          label="本地输入统计"
        />
      </UiSettingRow>
      <UiSettingRow
        title="在线服务"
        description="在线翻译、发音和 API Key 管理尚未接入。未来只会在你主动触发并知情后联网。"
      >
        <UiBadge>尚未接入</UiBadge>
      </UiSettingRow>
    </UiPanel>
    <UiPanel title="你的数据">
      <UiSettingRow
        title="导出个人数据"
        description="导出设置、个人词条和收藏复习记录为 JSON。系统词典不包含在导出中。"
      >
        <UiButton
          :busy="busy"
          @click="exportData"
        >
          <UiIcon
            name="download"
            :size="14"
          />
          导出数据
        </UiButton>
      </UiSettingRow>
      <div class="border-t border-line bg-canvas/50 px-5 py-4">
        <p class="mb-2 text-[10px] text-faint">数据位置</p>
        <code class="text-[11px] break-all text-muted">
          {{ app.snapshot?.dataPath }}
        </code>
      </div>
    </UiPanel>
    <UiPanel
      title="清除与重置"
      description="所有清除操作都需要再次确认。系统词典始终保留。"
    >
      <UiSettingRow
        title="清空本地词频"
        description="重置个性化排序，不会删除个人词条或生词本。"
      >
        <UiButton
          size="sm"
          :disabled="busy"
          @click="scope = 'frequency'"
        >
          清空词频
        </UiButton>
      </UiSettingRow>
      <UiSettingRow
        title="清空学习数据"
        description="删除收藏、复习状态和输入统计。个人词条不受影响。"
      >
        <UiButton
          size="sm"
          :disabled="busy"
          @click="scope = 'learning'"
        >
          清空学习数据
        </UiButton>
      </UiSettingRow>
      <UiSettingRow
        title="重置全部个人数据"
        description="删除个人词条、词频、收藏、统计，并恢复默认设置。建议先导出备份。"
      >
        <UiButton
          variant="danger"
          size="sm"
          :disabled="busy"
          @click="scope = 'all'"
        >
          重置数据
        </UiButton>
      </UiSettingRow>
    </UiPanel>
  </div>
  <UiDialog
    :model-value="!!scope"
    :title="`${confirmation}？`"
    description="此操作无法撤销。请确认已导出需要保留的个人数据。"
    @update:model-value="
      value => {
        if (!value) scope = ''
      }
    "
  >
    <p class="text-sm leading-7 text-muted">
      {{
        scope === 'all'
          ? '所有个人词条、词频、收藏和统计将被删除，设置将恢复默认。'
          : scope === 'frequency'
            ? '仅删除本地词频。个人词条、生词本和系统词典保留。'
            : '删除全部收藏、复习状态与输入统计，保留个人词条和系统词典。'
      }}
    </p>
    <template #footer>
      <UiButton
        :disabled="busy"
        @click="scope = ''"
      >
        取消
      </UiButton>
      <UiButton
        variant="danger"
        :busy="busy"
        @click="clear"
      >
        确认{{ confirmation }}
      </UiButton>
    </template>
  </UiDialog>
</template>
