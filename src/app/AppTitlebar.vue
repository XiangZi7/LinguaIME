<script setup lang="ts">
import { UiBadge, UiButton, UiDialog, UiIcon } from '@/components/ui'
import { desktop } from '@/services'
import { useWindowControls } from './useWindowControls'
defineProps<{ page?: string }>()
const {
  maximized,
  confirmClose,
  minimize,
  toggleMaximize,
  close,
  saveAndClose,
  discardAndClose,
} = useWindowControls()
</script>
<template>
  <header
    data-tauri-drag-region
    class="sticky top-0 z-20 flex h-[62px] shrink-0 items-center justify-between border-b border-line bg-surface/95 pl-9 backdrop-blur-sm select-none max-lg:pl-6 max-sm:pl-4"
    aria-label="应用标题栏"
  >
    <div
      data-tauri-drag-region
      class="flex h-full min-w-0 flex-1 items-center gap-3 text-[11px]"
    >
      <span
        data-tauri-drag-region
        class="text-faint max-sm:hidden"
      >
        个人工作台
      </span>
      <span
        data-tauri-drag-region
        class="text-faint max-sm:hidden"
      >
        /
      </span>
      <span
        data-tauri-drag-region
        class="truncate text-ink"
      >
        {{ page }}
      </span>
    </div>
    <div class="flex h-full shrink-0 items-center gap-4">
      <UiBadge
        :tone="desktop ? 'green' : 'amber'"
        dot
      >
        {{ desktop ? '桌面管理端' : '浏览器预览' }}
      </UiBadge>
      <RouterLink
        to="/about"
        class="text-muted hover:text-accent"
        aria-label="帮助与关于"
      >
        <UiIcon
          name="info"
          :size="17"
        />
      </RouterLink>
      <div
        v-if="desktop"
        class="ml-2 flex h-full items-stretch border-l border-line"
        aria-label="窗口控制"
      >
        <button
          type="button"
          aria-label="最小化窗口"
          title="最小化"
          class="grid w-11 cursor-pointer place-items-center text-muted hover:bg-subtle hover:text-ink"
          @click="minimize"
        >
          <svg
            width="12"
            height="12"
            viewBox="0 0 12 12"
            aria-hidden="true"
          >
            <path
              d="M1 6.5h10"
              fill="none"
              stroke="currentColor"
            />
          </svg>
        </button>
        <button
          type="button"
          :aria-label="maximized ? '还原窗口' : '最大化窗口'"
          :title="maximized ? '还原' : '最大化'"
          class="grid w-11 cursor-pointer place-items-center text-muted hover:bg-subtle hover:text-ink"
          @click="toggleMaximize"
        >
          <svg
            width="12"
            height="12"
            viewBox="0 0 12 12"
            aria-hidden="true"
          >
            <path
              v-if="maximized"
              d="M3.5 3.5V1.5h7v7h-2 M1.5 3.5h7v7h-7z"
              fill="none"
              stroke="currentColor"
            />
            <rect
              v-else
              x="1.5"
              y="1.5"
              width="9"
              height="9"
              fill="none"
              stroke="currentColor"
            />
          </svg>
        </button>
        <button
          type="button"
          aria-label="关闭窗口"
          title="关闭"
          class="grid w-11 cursor-pointer place-items-center text-muted hover:bg-[#c42b1c] hover:text-white"
          @click="close"
        >
          <UiIcon
            name="close"
            :size="16"
          />
        </button>
      </div>
      <span
        v-else
        class="w-5 max-sm:w-1"
      />
    </div>
  </header>
  <UiDialog
    v-model="confirmClose"
    title="保存更改后再离开？"
    description="还有未保存的设置。关闭管理应用不会退出系统输入法。"
  >
    <p class="text-xs leading-6 text-muted">
      可以保存设置后关闭，或放弃这次更改。
    </p>
    <template #footer>
      <UiButton @click="confirmClose = false">继续编辑</UiButton>
      <UiButton @click="discardAndClose">放弃并关闭</UiButton>
      <UiButton
        variant="primary"
        @click="saveAndClose"
      >
        保存并关闭
      </UiButton>
    </template>
  </UiDialog>
</template>
