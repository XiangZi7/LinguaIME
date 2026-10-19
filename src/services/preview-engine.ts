import type {
  Candidate,
  CandidatePage,
  EngineRequest,
  UserPhrase,
} from '@/types/engine'
import { normalize } from './validation.ts'

export function queryPreview(
  dictionary: Candidate[],
  phrases: UserPhrase[],
  frequencies: Record<string, number>,
  request: EngineRequest
): CandidatePage {
  const key = normalize(request.composition)
  const size = Math.max(1, Math.min(9, request.pageSize))
  const personal = phrases.map(phrase => ({
    ...phrase,
    key: normalize(phrase.reading),
    source: '个人词库',
    frequency: 1,
  }))
  const candidates = key
    ? [...personal, ...dictionary].filter(entry => entry.key.startsWith(key))
    : []
  candidates.sort(
    (a, b) =>
      Number(b.key === key) - Number(a.key === key) ||
      (frequencies[b.id] ?? 0) - (frequencies[a.id] ?? 0) ||
      Number(b.source === '个人词库') - Number(a.source === '个人词库') ||
      a.key.length - b.key.length ||
      b.frequency - a.frequency ||
      a.id.localeCompare(b.id)
  )
  const seen = new Set<string>()
  const unique = candidates.filter(item => {
    if (seen.has(item.text)) return false
    seen.add(item.text)
    return true
  })
  const pageCount = Math.ceil(unique.length / size)
  const pageIndex = Math.max(0, Math.min(request.page, pageCount - 1))
  return {
    schemaVersion: 1,
    composition: request.composition,
    items: unique.slice(pageIndex * size, (pageIndex + 1) * size),
    pageIndex,
    pageCount,
    total: unique.length,
  }
}
