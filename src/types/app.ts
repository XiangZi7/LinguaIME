import type {
  Candidate,
  CandidatePage,
  EngineRequest,
  LearningItem,
  UserPhrase,
} from './engine'
import type { Settings } from './settings'

export interface Statistics {
  selections: number
  favorites: number
  mastered: number
  daily: { date: string; count: number }[]
}
export interface AppSnapshot {
  settings: Settings
  phrases: UserPhrase[]
  learning: LearningItem[]
  statistics: Statistics
  dictionaryCount: number
  nativeRegistered: boolean
  dataPath: string
}
export interface Backup {
  schemaVersion: number
  settings: Settings
  phrases: UserPhrase[]
  learning: LearningItem[]
}
export interface AppService {
  nativeInstallation: () => Promise<NativeInstallationStatus>
  installNative: (uninstall: boolean) => Promise<void>
  snapshot: () => Promise<AppSnapshot>
  query: (request: EngineRequest) => Promise<CandidatePage>
  saveSettings: (settings: Settings) => Promise<void>
  savePhrase: (phrase: UserPhrase) => Promise<void>
  deletePhrase: (id: string) => Promise<void>
  importPhrases: (phrases: UserPhrase[]) => Promise<number>
  recordSelection: (id: string) => Promise<void>
  saveLearning: (
    candidate: Candidate,
    status: LearningItem['status']
  ) => Promise<void>
  deleteLearning: (id: string) => Promise<void>
  exportData: () => Promise<Backup>
  clearData: (scope: string) => Promise<void>
}
export interface NativeInstallationStatus {
  phase: 'checking' | 'ready' | 'failed' | 'removed' | 'unsupported'
  error: { code: string; message: string } | null
}
export interface Notice {
  id: number
  message: string
  kind: 'success' | 'error'
}
