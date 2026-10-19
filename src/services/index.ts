import type { AppService } from '@/types/app'
import { invoke, isTauri } from '@tauri-apps/api/core'
import { browserService } from './browser'

export const desktop = isTauri()
const tauriService: AppService = {
  snapshot: () => invoke('get_snapshot'),
  query: request => invoke('query_candidates', { request }),
  saveSettings: settings => invoke('save_settings', { settings }),
  savePhrase: phrase => invoke('save_phrase', { phrase }),
  deletePhrase: id => invoke('delete_phrase', { id }),
  importPhrases: phrases => invoke('import_phrases', { phrases }),
  recordSelection: id => invoke('record_selection', { id }),
  saveLearning: (candidate, status) =>
    invoke('save_learning', { candidate, status }),
  deleteLearning: id => invoke('delete_learning', { id }),
  exportData: () => invoke('export_data'),
  clearData: scope => invoke('clear_data', { scope }),
}
export const service = desktop ? tauriService : browserService
export function errorMessage(error: unknown): string {
  if (error instanceof Error) return error.message
  if (error && typeof error === 'object' && 'message' in error)
    return String(error.message)
  return '操作失败，请重试'
}
