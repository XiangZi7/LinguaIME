import type { NativeInstallationStatus } from '@/types/app'
import { acceptHMRUpdate, defineStore } from 'pinia'
import { computed, onScopeDispose, reactive, toRefs } from 'vue'
import { desktop, errorMessage, service } from '@/services'
import { useAppStore } from './app'

export const useNativeInstallationStore = defineStore(
  'native-installation',
  () => {
    // 响应式状态
    const state = reactive({
      // Rust 启动任务的持久状态，跨路由显示失败与重试入口
      status: {
        phase: desktop ? 'checking' : 'unsupported',
        error: null,
      } as NativeInstallationStatus,
    })
    let timer: ReturnType<typeof setTimeout> | undefined
    let checkId = 0
    const busy = computed(() => state.status.phase === 'checking')
    onScopeDispose(() => {
      clearTimeout(timer)
      checkId++
    })

    async function initialize(): Promise<void> {
      clearTimeout(timer)
      if (!desktop) return
      const requestId = ++checkId
      try {
        const status = await service.nativeInstallation()
        if (requestId !== checkId) return
        state.status = status
      } catch (error) {
        if (requestId !== checkId) return
        state.status = {
          phase: 'failed',
          error: { code: 'INSTALL_STATUS', message: errorMessage(error) },
        }
        return
      }
      if (state.status.phase === 'checking') {
        timer = setTimeout(() => void initialize(), 1000)
      } else {
        const app = useAppStore()
        try {
          await app.refresh()
        } catch (error) {
          app.notify(errorMessage(error), 'error')
        }
      }
    }

    async function install(): Promise<void> {
      if (busy.value || !desktop) return
      clearTimeout(timer)
      checkId++
      state.status = { phase: 'checking', error: null }
      try {
        await service.installNative(false)
        await initialize()
        if (state.status.phase === 'ready')
          useAppStore().notify(
            '输入法组件已同步，支持自动更新的应用会在输入空闲时加载'
          )
      } catch (error) {
        state.status = {
          phase: 'failed',
          error: { code: 'INSTALL_FAILED', message: errorMessage(error) },
        }
      }
    }

    return { ...toRefs(state), busy, initialize, install }
  }
)

if (import.meta.hot)
  import.meta.hot.accept(
    acceptHMRUpdate(useNativeInstallationStore, import.meta.hot)
  )
