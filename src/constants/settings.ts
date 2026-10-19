import type { SelectOption, Settings } from '@/types/settings'

export const DEFAULT_SETTINGS: Settings = {
  displayMode: 'bilingual',
  pageSize: 7,
  autoLearn: true,
  collectStats: false,
  theme: 'system',
  accent: 'green',
  fontSize: 17,
  rowSpacing: 'comfortable',
  chinesePunctuation: true,
  enterBehavior: 'candidate',
  previousPageKey: '-',
  nextPageKey: '=',
}
export const DISPLAY_OPTIONS: SelectOption[] = [
  { value: 'chinese', label: '纯中文' },
  { value: 'bilingual', label: '双语简洁' },
  { value: 'detailed', label: '双语详细' },
]
export const THEME_OPTIONS: SelectOption[] = [
  { value: 'light', label: '浅色' },
  { value: 'dark', label: '深色' },
  { value: 'system', label: '跟随系统' },
]
export const EMPTY_PAGE = {
  schemaVersion: 1,
  composition: '',
  items: [],
  pageIndex: 0,
  pageCount: 0,
  total: 0,
}
