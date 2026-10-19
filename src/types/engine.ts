export interface Candidate {
  id: string
  text: string
  reading: string
  key: string
  translation: string | null
  source: string
  frequency: number
}
export interface CandidatePage {
  schemaVersion: number
  composition: string
  items: Candidate[]
  pageIndex: number
  pageCount: number
  total: number
}
export interface EngineRequest {
  composition: string
  page: number
  pageSize: number
}
export interface UserPhrase {
  id: string
  text: string
  reading: string
  translation: string | null
}
export interface LearningItem {
  candidate: Candidate
  status: 'review' | 'mastered'
  createdAt: number
  updatedAt: number
}
