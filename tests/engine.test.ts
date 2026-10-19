import type { Candidate } from '../src/types/engine.ts'
import assert from 'node:assert/strict'
import { describe, it } from 'vitest'
import { DEFAULT_SETTINGS } from '../src/constants/settings.ts'
import { queryPreview } from '../src/services/preview-engine.ts'
import {
  normalize,
  parsePhraseImport,
  validateSettings,
} from '../src/services/validation.ts'

const entries: Candidate[] = [
  {
    id: 'a',
    text: '学',
    reading: 'xue2',
    key: 'xue',
    translation: 'to learn',
    source: 'fixture',
    frequency: 1,
  },
  {
    id: 'b',
    text: '学习',
    reading: 'xue2 xi2',
    key: 'xuexi',
    translation: 'to study',
    source: 'fixture',
    frequency: 1,
  },
  {
    id: 'c',
    text: '学习',
    reading: 'xue2 xi2',
    key: 'xuexi',
    translation: 'to learn',
    source: 'fixture',
    frequency: 1,
  },
]
describe('preview query contract', () => {
  it('deduplicates, prefers exact matches and clamps pages', () => {
    const page = queryPreview(
      entries,
      [],
      {},
      { composition: 'xue', page: 99, pageSize: 1 }
    )
    assert.equal(page.total, 2)
    assert.equal(page.pageIndex, 1)
    assert.equal(page.items[0]?.text, '学习')
    const exact = queryPreview(
      entries,
      [],
      {},
      { composition: 'xue', page: 0, pageSize: 9 }
    )
    assert.equal(exact.items[0]?.text, '学')
  })
  it('does not turn empty input or punctuation into candidates', () => {
    assert.equal(
      queryPreview(entries, [], {}, { composition: '', page: 1, pageSize: 7 })
        .pageCount,
      0
    )
    assert.throws(() => normalize('xue='))
    assert.equal(normalize("NÜ3'peng2you3"), 'nvpengyou')
  })
})
describe('import validation', () => {
  it('rejects an invalid row before returning a partial import', () => {
    assert.throws(() =>
      parsePhraseImport(
        '[{"text":"你好","reading":"nihao"},{"text":"坏","reading":"bad="}]'
      )
    )
    assert.throws(() =>
      parsePhraseImport(
        '[{"text":"你好","reading":"ni hao"},{"text":"你好","reading":"nihao"}]'
      )
    )
    assert.equal(
      parsePhraseImport('[{"text":"你好","reading":"nihao"}]')[0]?.translation,
      null
    )
  })
  it('rejects unsupported keyboard and page configurations', () => {
    validateSettings(DEFAULT_SETTINGS)
    assert.throws(() => validateSettings({ ...DEFAULT_SETTINGS, pageSize: 0 }))
    assert.throws(() =>
      validateSettings({ ...DEFAULT_SETTINGS, nextPageKey: 'a' as '=' })
    )
  })
})
