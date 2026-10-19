<script setup lang="ts">
import { shallowRef } from 'vue'
import { UiButton, UiIcon } from '@/components/ui'
import { desktop, errorMessage, service } from '@/services'
import { useAppStore } from '@/stores/app'
withDefaults(defineProps<{ compact?: boolean }>(), { compact: false })
const app = useAppStore()
const busy = shallowRef(false)
async function install(): Promise<void> {
  if (busy.value) return
  busy.value = true
  try {
    await service.installNative(false)
    await app.refresh()
    app.notify(
      '输入法组件已发布，支持自动更新的应用会在拼音提交或取消后加载；旧加载方式的应用需重开一次'
    )
  } catch (error) {
    app.notify(errorMessage(error), 'error')
  } finally {
    busy.value = false
  }
}
</script>
<template>
  <div
    v-if="!app.snapshot?.nativeRegistered"
    class="rounded-lg border border-warning/15 bg-warning-soft p-3.5"
  >
    <p class="flex items-center gap-2 text-xs font-medium text-warning">
      <UiIcon
        name="info"
        :size="14"
      />
      系统输入法尚未安装
    </p>
    <p class="mt-2 text-[11px] leading-6 text-muted">
      {{
        desktop
          ? '管理应用与系统输入法独立。安装后才能在 Windows 输入法列表中选择 LinguaIME。'
          : '浏览器仅用于界面预览。请使用 Windows 桌面管理端安装系统输入法。'
      }}
    </p>
    <UiButton
      class="mt-3"
      :class="compact ? 'w-full' : ''"
      variant="primary"
      size="sm"
      :disabled="!desktop"
      :busy="busy"
      @click="install"
    >
      <UiIcon
        name="download"
        :size="14"
      />
      {{ busy ? '正在安装与注册…' : '安装到 Windows 输入法' }}
    </UiButton>
  </div>
  <div
    v-else
    class="flex flex-wrap items-start gap-2 rounded-lg bg-accent-wash p-3 text-[11px] leading-6 text-muted"
  >
    <UiIcon
      name="check"
      :size="14"
      class="mt-1 text-accent"
    />
    <span class="min-w-0 flex-1 basis-40">
      已加入 Windows 输入法列表。按
      <kbd class="keycap">Win</kbd>
      +
      <kbd class="keycap">Space</kbd>
      选择
      LinguaIME。更新组件后会在输入空闲时加载；首次迁移到自动更新版本需重开目标应用一次。
    </span>
    <UiButton
      v-if="desktop"
      variant="secondary"
      size="sm"
      :busy="busy"
      @click="install"
    >
      {{ busy ? '正在发布…' : '更新输入法组件' }}
    </UiButton>
  </div>
</template>
