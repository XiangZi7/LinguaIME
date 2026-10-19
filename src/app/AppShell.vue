<script setup lang="ts">
import { storeToRefs } from 'pinia'
import { computed, onBeforeUnmount, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import { UiButton, UiEmptyState, UiIcon } from '@/components/ui'
import { useAppStore } from '@/stores/app'
import AppSidebar from './AppSidebar.vue'
import AppTitlebar from './AppTitlebar.vue'
import { NAVIGATION } from './navigation'
const store = useAppStore()
const { loading, error, dirty, saving, draft, notices } = storeToRefs(store)
const route = useRoute()
const currentPage = computed(() =>
  NAVIGATION.find(item => item.path === route.path)
)
const media = window.matchMedia('(prefers-color-scheme: dark)')
function updateTheme(): void {
  document.documentElement.classList.toggle(
    'dark',
    draft.value.theme === 'dark' ||
      (draft.value.theme === 'system' && media.matches)
  )
}
function beforeUnload(event: BeforeUnloadEvent): void {
  if (store.dirty) {
    event.preventDefault()
    event.returnValue = ''
  }
}
watch(() => draft.value.theme, updateTheme, { immediate: true })
onMounted(() => {
  void store.initialize()
  media.addEventListener('change', updateTheme)
  window.addEventListener('beforeunload', beforeUnload)
})
onBeforeUnmount(() => {
  media.removeEventListener('change', updateTheme)
  window.removeEventListener('beforeunload', beforeUnload)
})
</script>
<template>
  <div class="flex min-h-screen">
    <AppSidebar />
    <div class="min-w-0 flex-1">
      <AppTitlebar :page="currentPage?.label" />
      <main
        id="main-content"
        class="mx-auto max-w-[1360px] px-9 pt-8 pb-12 max-lg:px-6 max-sm:px-4"
        :class="dirty ? 'pb-28' : ''"
      >
        <div
          v-if="loading"
          role="status"
          class="space-y-5"
        >
          <div class="h-16 w-72 animate-pulse rounded-lg bg-subtle" />
          <div
            class="h-60 animate-pulse rounded-xl border border-line bg-surface"
          />
          <p class="text-sm text-muted">正在读取本地数据…</p>
        </div>
        <UiEmptyState
          v-else-if="error"
          title="暂时无法读取本地数据"
          :description="error"
          icon="alert"
        >
          <UiButton @click="store.initialize">重新加载</UiButton>
        </UiEmptyState>
        <RouterView v-else />
      </main>
      <div
        v-if="dirty"
        class="fixed right-0 bottom-0 left-[216px] z-30 flex min-h-[76px] items-center justify-between gap-3 border-t border-line bg-surface/95 px-9 backdrop-blur-md max-md:left-[70px] max-sm:px-4"
      >
        <div class="text-xs text-muted">
          <span class="mr-2 inline-block size-1.5 rounded-full bg-warning" />
          有未保存的更改
          <span class="ml-2 text-faint max-lg:hidden">切换页面会保留草稿</span>
        </div>
        <div class="flex gap-2">
          <UiButton
            :disabled="saving"
            @click="store.discard"
          >
            取消
          </UiButton>
          <UiButton
            variant="primary"
            :busy="saving"
            @click="store.save"
          >
            <UiIcon
              name="check"
              :size="15"
            />
            保存设置
          </UiButton>
        </div>
      </div>
    </div>
    <div
      aria-live="polite"
      aria-atomic="false"
      class="fixed right-6 bottom-24 z-50 flex max-w-[calc(100vw-32px)] flex-col gap-2"
    >
      <div
        v-for="notice in notices"
        :key="notice.id"
        role="status"
        class="flex items-center gap-3 rounded-xl border border-line bg-surface px-5 py-4 text-xs shadow-lg"
        :class="notice.kind === 'error' ? 'text-danger' : 'text-accent'"
      >
        <UiIcon :name="notice.kind === 'error' ? 'alert' : 'check'" />
        {{ notice.message }}
      </div>
    </div>
  </div>
</template>
