<script setup lang="ts">
import { storeToRefs } from 'pinia'
import {
  UiBadge,
  UiPageHeader,
  UiPanel,
  UiSegmented,
  UiSelect,
  UiSettingRow,
  UiSwitch,
} from '@/components/ui'
import { DISPLAY_OPTIONS } from '@/constants/settings'
import { useAppStore } from '@/stores/app'
const { draft } = storeToRefs(useAppStore())
</script>
<template>
  <UiPageHeader
    title="输入设置"
    description="让每一个按键，都符合你的习惯。"
    eyebrow="MAKE IT YOURS"
  />
  <div class="max-w-4xl space-y-5">
    <UiPanel
      title="输入与候选"
      description="基础输入在本地完成，不等待网络。"
    >
      <UiSettingRow
        title="输入方案"
        description="当前支持简体中文全拼。双拼、模糊音与繁体转换将在后续提供。"
      >
        <UiBadge tone="green">简体中文 · 全拼</UiBadge>
      </UiSettingRow>
      <UiSettingRow
        title="候选显示模式"
        description="简洁模式直显英文释义；详细模式补充读音与来源。"
      >
        <UiSegmented
          v-model="draft.displayMode"
          label="候选显示模式"
          :options="DISPLAY_OPTIONS"
        />
      </UiSettingRow>
      <UiSettingRow
        title="每页候选数量"
        description="可使用数字键 1–9 选择对应候选。"
      >
        <UiSelect
          v-model="draft.pageSize"
          label="每页候选数量"
          :options="
            [3, 4, 5, 6, 7, 8, 9].map(value => ({
              value,
              label: `${value} 个候选`,
            }))
          "
        />
      </UiSettingRow>
      <UiSettingRow
        title="候选排序"
        description="先匹配完整拼音，再结合本地选择次数和个人词条排序。不使用在线上下文。"
      >
        <UiBadge>精确匹配 + 本地词频</UiBadge>
      </UiSettingRow>
    </UiPanel>
    <UiPanel title="按键行为">
      <UiSettingRow
        title="Enter 确认方式"
        description="空格始终选择当前候选；Esc 取消组合，不提交文字。"
      >
        <UiSelect
          v-model="draft.enterBehavior"
          label="Enter 确认方式"
          :options="[
            { value: 'candidate', label: '提交当前候选' },
            { value: 'raw', label: '提交原始拼音' },
          ]"
        />
      </UiSettingRow>
      <UiSettingRow
        title="中文标点"
        description="组合结束后输入常用中文标点。仅在原生中文输入模式下生效。"
      >
        <UiSwitch
          v-model="draft.chinesePunctuation"
          label="使用中文标点"
        />
      </UiSettingRow>
      <UiSettingRow
        title="退格与取消"
        description="退格删除组合串最后一个字符；焦点变化时取消未提交组合。"
      >
        <UiBadge>保留明确的组合边界</UiBadge>
      </UiSettingRow>
    </UiPanel>
    <UiPanel title="本地学习">
      <UiSettingRow
        title="学习常用词"
        description="只记录选择过的词条 ID 与次数，帮助常用词排在前面。不会记录完整输入内容。"
      >
        <UiSwitch
          v-model="draft.autoLearn"
          label="学习常用词"
        />
      </UiSettingRow>
    </UiPanel>
    <p class="px-1 text-xs leading-6 text-muted">
      更改会保留为草稿，点击底部「保存设置」后生效。原生宿主会在下一次组合开始时读取设置。
    </p>
  </div>
</template>
