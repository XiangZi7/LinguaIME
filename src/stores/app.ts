import type { AppSnapshot, Notice } from '@/types/app'
import { acceptHMRUpdate, defineStore } from 'pinia'
import { computed, reactive, toRefs } from 'vue'
import { DEFAULT_SETTINGS } from '@/constants/settings'
import { errorMessage, service } from '@/services'

export const useAppStore = defineStore('app', () => {
  // 响应式状态
  const state = reactive({
    // 当前持久化数据快照
    snapshot: null as AppSnapshot | null,
    // 跨页面保留的设置草稿
    draft: { ...DEFAULT_SETTINGS },
    // 首次读取状态
    loading: false,
    // 保存中的互斥状态
    saving: false,
    // 可重试的启动错误
    error: '',
    // 全局操作反馈
    notices: [] as Notice[],
  })
  const dirty = computed(
    () =>
      !!state.snapshot &&
      JSON.stringify(state.draft) !== JSON.stringify(state.snapshot.settings)
  )
  function notify(message: string, kind: Notice['kind'] = 'success'): void {
    const id = Date.now() + Math.random()
    state.notices.push({ id, message, kind })
    setTimeout(() => {
      state.notices = state.notices.filter(item => item.id !== id)
    }, 5000)
  }
  async function refresh(resetDraft = false): Promise<void> {
    const snapshot = await service.snapshot()
    if (!state.snapshot || resetDraft)
      Object.assign(state.draft, snapshot.settings)
    state.snapshot = snapshot
  }
  async function initialize(): Promise<void> {
    state.loading = true
    state.error = ''
    try {
      await refresh(true)
    } catch (error) {
      state.error = errorMessage(error)
    } finally {
      state.loading = false
    }
  }
  async function save(): Promise<void> {
    if (state.saving) return
    state.saving = true
    try {
      await service.saveSettings({ ...state.draft })
      await refresh()
      notify('设置已保存，下次开始组合输入时生效')
    } catch (error) {
      notify(errorMessage(error), 'error')
    } finally {
      state.saving = false
    }
  }
  function discard(): void {
    if (state.snapshot) Object.assign(state.draft, state.snapshot.settings)
  }
  return { ...toRefs(state), dirty, initialize, refresh, save, discard, notify }
})
if (import.meta.hot)
  import.meta.hot.accept(acceptHMRUpdate(useAppStore, import.meta.hot))
