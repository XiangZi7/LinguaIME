export type DisplayMode = 'chinese' | 'bilingual' | 'detailed'
export type Theme = 'light' | 'dark' | 'system'
export interface Settings {
  displayMode: DisplayMode
  pageSize: number
  autoLearn: boolean
  collectStats: boolean
  theme: Theme
  accent: 'green' | 'blue'
  fontSize: number
  rowSpacing: 'compact' | 'comfortable'
  chinesePunctuation: boolean
  enterBehavior: 'candidate' | 'raw'
  previousPageKey: '-' | 'PageUp' | '['
  nextPageKey: '=' | 'PageDown' | ']'
}
export interface SelectOption {
  value: string | number
  label: string
  disabled?: boolean
}
