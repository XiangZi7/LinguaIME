import { getCurrentWindow } from '@tauri-apps/api/window'
import { onBeforeUnmount, onMounted, reactive, toRefs } from 'vue'
import { desktop, errorMessage } from '@/services'
import { useAppStore } from '@/stores/app'

export function useWindowControls() {
  // 响应式状态
  const state = reactive({
    // 最大化状态决定还原按钮的语义
    maximized: false,
    // 关闭前确认未保存的设置
    confirmClose: false,
  })
  const app = useAppStore()
  const appWindow = desktop ? getCurrentWindow() : null
  let unlisten: (() => void) | undefined
  let unlistenClose: (() => void) | undefined
  let disposed = false
  async function syncMaximized(): Promise<void> {
    if (appWindow) state.maximized = await appWindow.isMaximized()
  }
  onMounted(async () => {
    if (!appWindow) return
    try {
      await syncMaximized()
      const stop = await appWindow.onResized(() => {
        void syncMaximized().catch(() => {})
      })
      if (disposed) stop()
      else unlisten = stop
      const stopClose = await appWindow.onCloseRequested(event => {
        if (app.dirty) {
          event.preventDefault()
          state.confirmClose = true
        }
      })
      if (disposed) stopClose()
      else unlistenClose = stopClose
    } catch (error) {
      app.notify(errorMessage(error), 'error')
    }
  })
  onBeforeUnmount(() => {
    disposed = true
    unlisten?.()
    unlistenClose?.()
  })
  async function run(
    action: 'minimize' | 'toggleMaximize' | 'close'
  ): Promise<void> {
    if (!appWindow) return
    if (action === 'close' && app.dirty) {
      state.confirmClose = true
      return
    }
    try {
      await appWindow[action]()
      if (action === 'toggleMaximize') await syncMaximized()
    } catch (error) {
      app.notify(errorMessage(error), 'error')
    }
  }
  async function saveAndClose(): Promise<void> {
    await app.save()
    if (!app.dirty) {
      state.confirmClose = false
      await run('close')
    }
  }
  async function discardAndClose(): Promise<void> {
    app.discard()
    state.confirmClose = false
    await run('close')
  }
  return {
    ...toRefs(state),
    saveAndClose,
    discardAndClose,
    minimize: () => run('minimize'),
    toggleMaximize: () => run('toggleMaximize'),
    close: () => run('close'),
  }
}
