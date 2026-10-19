import type { CandidatePage, UserPhrase } from '@/types/engine'
import { computed, onBeforeUnmount, reactive, toRefs, watch } from 'vue'
import { EMPTY_PAGE } from '@/constants/settings'
import { errorMessage, service } from '@/services'
import { parsePhraseImport } from '@/services/validation'
import { useAppStore } from '@/stores/app'

export function useDictionary() {
  const app = useAppStore()
  // 响应式状态
  const state = reactive({
    // 当前词库来源
    tab: 'system',
    // 拼音或个人词条筛选词
    search: 'xue',
    // 系统词库分页结果
    page: { ...EMPTY_PAGE } as CandidatePage,
    // 系统检索状态
    loading: false,
    // 操作与检索错误
    error: '',
    // 个人词条编辑窗口
    editorOpen: false,
    // 当前编辑对象；null 代表新增
    editing: null as UserPhrase | null,
    // 导入确认窗口
    importOpen: false,
    // 经格式校验后的导入预览
    imported: [] as UserPhrase[],
    // 写入期间禁用重复提交
    busy: false,
    // 待删除词条的明确确认对象
    deleting: null as UserPhrase | null,
  })
  let requestId = 0
  let timer: ReturnType<typeof setTimeout> | undefined
  const personal = computed(() =>
    (app.snapshot?.phrases ?? []).filter(item =>
      `${item.text} ${item.reading} ${item.translation ?? ''}`
        .toLowerCase()
        .includes(state.search.toLowerCase())
    )
  )
  async function query(page = 0): Promise<void> {
    const id = ++requestId
    state.loading = true
    state.error = ''
    try {
      const result = await service.query({
        composition: state.search,
        page,
        pageSize: 9,
      })
      if (id === requestId) state.page = result
    } catch (error) {
      if (id === requestId) {
        state.error = errorMessage(error)
        state.page = { ...EMPTY_PAGE }
      }
    } finally {
      if (id === requestId) state.loading = false
    }
  }
  watch(
    [() => state.search, () => state.tab],
    () => {
      ++requestId
      clearTimeout(timer)
      state.error = ''
      if (state.tab === 'system')
        timer = setTimeout(() => {
          void query()
        }, 100)
      else state.loading = false
    },
    { immediate: true }
  )
  onBeforeUnmount(() => {
    ++requestId
    clearTimeout(timer)
  })
  async function run(
    action: () => Promise<unknown>,
    success: string
  ): Promise<boolean> {
    if (state.busy) return false
    state.busy = true
    state.error = ''
    try {
      await action()
      await app.refresh()
      app.notify(success)
      return true
    } catch (error) {
      state.error = errorMessage(error)
      app.notify(state.error, 'error')
      return false
    } finally {
      state.busy = false
    }
  }
  function edit(phrase: UserPhrase | null): void {
    state.editing = phrase
    state.editorOpen = true
  }
  async function save(phrase: UserPhrase): Promise<void> {
    if (await run(() => service.savePhrase(phrase), '个人词条已保存')) {
      state.editorOpen = false
      if (state.tab === 'system') await query()
    }
  }
  async function remove(): Promise<void> {
    const id = state.deleting?.id
    if (id && (await run(() => service.deletePhrase(id), '个人词条已删除')))
      state.deleting = null
  }
  async function readImport(file: File): Promise<void> {
    state.error = ''
    try {
      if (file.size > 5_000_000) throw new Error('文件不能超过 5 MB')
      state.imported = parsePhraseImport(await file.text())
      state.importOpen = true
    } catch (error) {
      state.error = errorMessage(error)
    }
  }
  async function confirmImport(): Promise<void> {
    if (
      await run(
        () => service.importPhrases(state.imported),
        `已导入 ${state.imported.length} 条个人词条`
      )
    )
      state.importOpen = false
  }
  return {
    ...toRefs(state),
    personal,
    query,
    edit,
    save,
    remove,
    readImport,
    confirmImport,
    run,
  }
}
