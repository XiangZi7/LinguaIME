import type { UserPhrase } from '@/types/engine'
import type { Settings } from '@/types/settings'

export function normalize(input: string): string {
  if (input.length > 128) throw new Error('组合串不能超过 128 个字符')
  const key = input
    .trim()
    .toLowerCase()
    .replace(/u:|ü/g, 'v')
    .replace(/[' 1-5]/g, '')
  if (!/^[a-z]*$/.test(key)) throw new Error('请使用拼音字母、空格或单引号')
  return key
}
export function validatePhrase(phrase: UserPhrase): void {
  if (
    !phrase.id ||
    phrase.id.length > 96 ||
    !phrase.text.trim() ||
    [...phrase.text].length > 32 ||
    [...phrase.text].some(
      char => char.charCodeAt(0) < 32 || char.charCodeAt(0) === 127
    ) ||
    !normalize(phrase.reading) ||
    (phrase.translation &&
      new TextEncoder().encode(phrase.translation).length > 512)
  ) {
    throw new Error('词条须为 1–32 个字符，填写有效拼音，释义不超过 512 字节')
  }
}
export function validateSettings(settings: Settings): void {
  if (
    !['chinese', 'bilingual', 'detailed'].includes(settings.displayMode) ||
    !Number.isInteger(settings.pageSize) ||
    settings.pageSize < 3 ||
    settings.pageSize > 9 ||
    !['light', 'dark', 'system'].includes(settings.theme) ||
    !['green', 'blue'].includes(settings.accent) ||
    settings.fontSize < 14 ||
    settings.fontSize > 22 ||
    !['compact', 'comfortable'].includes(settings.rowSpacing) ||
    !['candidate', 'raw'].includes(settings.enterBehavior) ||
    !['-', 'PageUp', '['].includes(settings.previousPageKey) ||
    !['=', 'PageDown', ']'].includes(settings.nextPageKey)
  ) {
    throw new Error('设置值不在允许范围内')
  }
}
export function parsePhraseImport(raw: string): UserPhrase[] {
  if (new TextEncoder().encode(raw).length > 5_000_000)
    throw new Error('文件不能超过 5 MB')
  const parsed: unknown = JSON.parse(raw)
  if (!Array.isArray(parsed) || parsed.length > 10000)
    throw new Error('请选择最多含 10000 条词条的 JSON 数组')
  const seen = new Set<string>()
  return parsed.map((value: unknown, index: number) => {
    if (
      !value ||
      typeof value !== 'object' ||
      !('text' in value) ||
      !('reading' in value) ||
      typeof value.text !== 'string' ||
      typeof value.reading !== 'string'
    )
      throw new Error(`第 ${index + 1} 条缺少 text 或 reading`)
    const translation = 'translation' in value ? value.translation : null
    if (translation !== null && typeof translation !== 'string')
      throw new Error(`第 ${index + 1} 条释义格式无效`)
    const phrase = {
      id: crypto.randomUUID(),
      text: value.text.trim(),
      reading: value.reading.trim(),
      translation,
    }
    validatePhrase(phrase)
    const key = `${phrase.text}|${normalize(phrase.reading)}`
    if (seen.has(key)) throw new Error(`第 ${index + 1} 条与文件内的词条重复`)
    seen.add(key)
    return phrase
  })
}
