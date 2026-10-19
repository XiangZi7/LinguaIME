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
    app.notify('已安装并加入 Windows 输入法列表，按 Win + Space 切换')
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
    class="flex items-start gap-2 rounded-lg bg-accent-wash p-3 text-[11px] leading-6 text-muted"
  >
    <UiIcon
      name="check"
      :size="14"
      class="mt-1 text-accent"
    />
    <span>
      已加入 Windows 输入法列表。按
      <kbd class="keycap">Win</kbd>
      +
      <kbd class="keycap">Space</kbd>
      选择 LinguaIME。关闭管理应用后仍可输入。
    </span>
  </div>
</template>
