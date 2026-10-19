<script setup lang="ts">
import { UiButton, UiIcon } from '@/components/ui'
import { desktop } from '@/services'
import { useAppStore } from '@/stores/app'
import { useNativeInstallationStore } from '@/stores/native-installation'
withDefaults(defineProps<{ compact?: boolean }>(), { compact: false })
const app = useAppStore()
const native = useNativeInstallationStore()
</script>
<template>
  <div
    v-if="native.busy"
    role="status"
    class="rounded-lg bg-accent-wash p-3.5 text-xs leading-6 text-muted"
  >
    <p class="font-medium text-accent">正在同步系统输入法…</p>
    <p class="mt-1">
      每次启动自动检查并安装当前版本；首次注册可能需要 Windows 管理员授权。
    </p>
  </div>
  <div
    v-else-if="native.status.phase === 'failed'"
    role="alert"
    class="rounded-lg border border-warning/15 bg-warning-soft p-3.5 text-xs leading-6 text-warning"
  >
    <p class="font-medium">输入法自动更新未完成</p>
    <p class="mt-1">{{ native.status.error?.message }}</p>
    <UiButton
      class="mt-3"
      size="sm"
      @click="native.install"
    >
      重试更新
    </UiButton>
  </div>
  <div
    v-else-if="!app.snapshot?.nativeRegistered"
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
          ? '安装后即可在 Windows 输入法列表中选择 LinguaIME，之后每次启动会自动检查更新。'
          : '浏览器仅用于界面预览。请使用 Windows 桌面管理端安装系统输入法。'
      }}
    </p>
    <UiButton
      class="mt-3"
      :class="compact ? 'w-full' : ''"
      variant="primary"
      size="sm"
      :disabled="!desktop"
      @click="native.install"
    >
      <UiIcon
        name="download"
        :size="14"
      />
      安装到 Windows 输入法
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
      LinguaIME。每次启动管理端自动同步当前版本，输入空闲时加载。首次迁移需完全退出旧应用后重开一次。
    </span>
    <UiButton
      v-if="desktop"
      variant="secondary"
      size="sm"
      @click="native.install"
    >
      更新输入法组件
    </UiButton>
  </div>
</template>
