// @env browser
import type { AppService, AppSnapshot } from '@/types/app'
import type { Candidate, LearningItem, UserPhrase } from '@/types/engine'
import { DEFAULT_SETTINGS } from '@/constants/settings'
import { queryPreview } from './preview-engine'
import { validatePhrase, validateSettings } from './validation'

interface PreviewData {
  snapshot: AppSnapshot
  frequencies: Record<string, number>
}
const STORAGE_KEY = 'linguaime-preview-v1'
let dictionary: Candidate[] | undefined

function read(): PreviewData {
  const raw = localStorage.getItem(STORAGE_KEY)
  if (raw) {
    const parsed = JSON.parse(raw) as PreviewData
    validateSettings(parsed.snapshot.settings)
    if (
      !Array.isArray(parsed.snapshot.phrases) ||
      !Array.isArray(parsed.snapshot.learning)
    )
      throw new Error('预览数据格式无效')
    return parsed
  }
  return {
    snapshot: {
      settings: { ...DEFAULT_SETTINGS },
      phrases: [],
      learning: [],
      statistics: { selections: 0, favorites: 0, mastered: 0, daily: [] },
      dictionaryCount: 0,
      nativeRegistered: false,
      dataPath: '此浏览器的本地存储 · 与桌面数据独立',
    },
    frequencies: {},
  }
}
function write(data: PreviewData): void {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(data))
}
async function loadDictionary(): Promise<Candidate[]> {
  if (dictionary) return dictionary
  const response = await fetch('/preview-dictionary.json')
  if (!response.ok) throw new Error('预览词典加载失败，请刷新重试')
  const entries = (await response.json()) as Candidate[]
  dictionary = entries.map(entry => ({ ...entry, source: 'CC-CEDICT' }))
  return dictionary
}
function mutate(action: (data: PreviewData) => void): void {
  const data = read()
  action(data)
  write(data)
}
export const browserService: AppService = {
  async snapshot() {
    const data = read()
    const entries = await loadDictionary()
    return {
      ...data.snapshot,
      dictionaryCount: entries.length,
      statistics: {
        ...data.snapshot.statistics,
        favorites: data.snapshot.learning.length,
        mastered: data.snapshot.learning.filter(
          item => item.status === 'mastered'
        ).length,
      },
    }
  },
  async query(request) {
    const data = read()
    return queryPreview(
      await loadDictionary(),
      data.snapshot.phrases,
      data.frequencies,
      request
    )
  },
  async saveSettings(settings) {
    validateSettings(settings)
    mutate(data => {
      data.snapshot.settings = settings
    })
  },
  async savePhrase(phrase) {
    validatePhrase(phrase)
    mutate(data => {
      if (
        data.snapshot.phrases.some(
          item =>
            item.id !== phrase.id &&
            item.text === phrase.text &&
            item.reading === phrase.reading
        )
      )
        throw new Error('词条已存在')
      data.snapshot.phrases = [
        phrase,
        ...data.snapshot.phrases.filter(item => item.id !== phrase.id),
      ]
    })
  },
  async deletePhrase(id) {
    mutate(data => {
      data.snapshot.phrases = data.snapshot.phrases.filter(
        item => item.id !== id
      )
    })
  },
  async importPhrases(phrases: UserPhrase[]) {
    phrases.forEach(validatePhrase)
    mutate(data => {
      const merged = new Map(
        data.snapshot.phrases.map(item => [
          `${item.text}|${item.reading}`,
          item,
        ])
      )
      phrases.forEach(item => {
        const key = `${item.text}|${item.reading}`
        merged.set(key, { ...item, id: merged.get(key)?.id ?? item.id })
      })
      data.snapshot.phrases = [...merged.values()]
    })
    return phrases.length
  },
  async recordSelection(id) {
    mutate(data => {
      if (data.snapshot.settings.autoLearn)
        data.frequencies[id] = (data.frequencies[id] ?? 0) + 1
      if (data.snapshot.settings.collectStats) {
        data.snapshot.statistics.selections++
        const date = new Intl.DateTimeFormat('sv-SE').format(new Date())
        const day = data.snapshot.statistics.daily.find(
          item => item.date === date
        )
        if (day) day.count++
        else data.snapshot.statistics.daily.push({ date, count: 1 })
      }
    })
  },
  async saveLearning(candidate: Candidate, status: LearningItem['status']) {
    mutate(data => {
      const existing = data.snapshot.learning.find(
        item => item.candidate.id === candidate.id
      )
      if (existing) {
        existing.status = status
        existing.updatedAt = Date.now() / 1000
      } else
        data.snapshot.learning.unshift({
          candidate,
          status,
          createdAt: Date.now() / 1000,
          updatedAt: Date.now() / 1000,
        })
    })
  },
  async deleteLearning(id) {
    mutate(data => {
      data.snapshot.learning = data.snapshot.learning.filter(
        item => item.candidate.id !== id
      )
    })
  },
  async exportData() {
    const { settings, phrases, learning } = read().snapshot
    return { schemaVersion: 1, settings, phrases, learning }
  },
  async clearData(scope) {
    mutate(data => {
      if (scope === 'frequency' || scope === 'all') data.frequencies = {}
      if (scope === 'learning' || scope === 'all') {
        data.snapshot.learning = []
        data.snapshot.statistics = {
          selections: 0,
          favorites: 0,
          mastered: 0,
          daily: [],
        }
      }
      if (scope === 'all') {
        data.snapshot.phrases = []
        data.snapshot.settings = { ...DEFAULT_SETTINGS }
      }
    })
  },
}
