import type { Page } from '@playwright/test'
import type { AppSnapshot } from '../../src/types/app'
import { DEFAULT_SETTINGS } from '../../src/constants/settings'

export async function mockDesktop(page: Page): Promise<void> {
  const snapshot: AppSnapshot = {
    settings: { ...DEFAULT_SETTINGS },
    phrases: [],
    learning: [],
    statistics: { selections: 0, favorites: 0, mastered: 0, daily: [] },
    dictionaryCount: 121202,
    nativeRegistered: false,
    dataPath: 'mock-local-data',
  }
  await page.addInitScript(initial => {
    let maximized = false
    let callbackId = 0
    const commands: string[] = []
    Object.defineProperty(window, 'isTauri', { value: true })
    Object.defineProperty(window, '__TAURI_EVENT_PLUGIN_INTERNALS__', {
      value: { unregisterListener: () => {} },
    })
    Object.defineProperty(window, '__TAURI_INTERNALS__', {
      value: {
        metadata: {
          currentWindow: { label: 'main' },
          currentWebview: { label: 'main' },
        },
        transformCallback: () => ++callbackId,
        unregisterCallback: () => {},
        invoke: async (command: string) => {
          commands.push(command)
          sessionStorage.setItem('window-commands', JSON.stringify(commands))
          if (command === 'get_snapshot') return initial
          if (command === 'query_candidates')
            return {
              schemaVersion: 1,
              composition: 'xuexi',
              items: [],
              pageIndex: 0,
              pageCount: 0,
              total: 0,
            }
          if (command === 'install_native') {
            initial.nativeRegistered = true
            return
          }
          if (command === 'plugin:window|is_maximized') return maximized
          if (command === 'plugin:window|toggle_maximize')
            maximized = !maximized
          if (command === 'plugin:event|listen') return ++callbackId
        },
      },
    })
  }, snapshot)
}
