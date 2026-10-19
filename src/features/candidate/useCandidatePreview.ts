import { onBeforeUnmount, reactive, toRefs, watch } from 'vue'
import { useAppStore } from '@/stores/app'
import { EMPTY_PAGE } from '@/constants/settings'
import type { Candidate, CandidatePage } from '@/types/engine'
import { service, errorMessage } from '@/services'

export function useCandidatePreview() {
  const app = useAppStore()
  // 响应式状态
  const state = reactive({
    // 当前预览组合串，不进入全局输入状态
    composition: 'xuexi',
    // 本次查询对应的候选分页
    page: { ...EMPTY_PAGE } as CandidatePage,
    // 键盘选择索引
    selected: 0,
    // 仅在本预览展示的已提交文本
    committed: '',
    // 本地检索反馈
    loading: false,
    // 可重试的查询错误
    error: '',
  })
  let requestId = 0
  let timer: ReturnType<typeof setTimeout> | undefined
  async function query(page = 0): Promise<void> {
    const id = ++requestId
    state.loading = true; state.error = ''
    try {
      const result = await service.query({ composition: state.composition, page, pageSize: app.draft.pageSize })
      if (id !== requestId) return
      state.page = result; state.selected = 0
    } catch (error) { if (id === requestId) { state.error = errorMessage(error); state.page = { ...EMPTY_PAGE } } }
    finally { if (id === requestId) state.loading = false }
  }
  watch([() => state.composition, () => app.draft.pageSize], () => {
    ++requestId
    clearTimeout(timer)
    state.loading = true
    timer = setTimeout(() => { void query() }, 70)
  }, { immediate: true })
  onBeforeUnmount(() => { ++requestId; clearTimeout(timer) })
  async function choose(candidate: Candidate): Promise<void> {
    if (state.loading) return
    state.committed += candidate.text
    state.composition = ''
    state.page = { ...EMPTY_PAGE }
    try { await service.recordSelection(candidate.id); await app.refresh() }
    catch (error) { app.notify(errorMessage(error), 'error') }
  }
  function turnPage(direction: number): void {
    const index = state.page.pageIndex + direction
    if (!state.loading && index >= 0 && index < state.page.pageCount) void query(index)
  }
  function onKeydown(event: KeyboardEvent): void {
    if (event.isComposing || event.ctrlKey || event.altKey || event.metaKey || !state.composition) return
    if (event.key === 'Escape') { event.preventDefault(); state.composition = ''; return }
    if (state.loading) return
    if (event.key === app.draft.previousPageKey || event.key === app.draft.nextPageKey) { event.preventDefault(); turnPage(event.key === app.draft.previousPageKey ? -1 : 1); return }
    const count = state.page.items.length
    if (count && ['ArrowUp', 'ArrowDown'].includes(event.key)) { event.preventDefault(); state.selected = (state.selected + count + (event.key === 'ArrowUp' ? -1 : 1)) % count }
    if (event.key === 'Enter' && app.draft.enterBehavior === 'raw') { event.preventDefault(); state.committed += state.composition; state.composition = ''; return }
    const index = /^[1-9]$/.test(event.key) ? Number(event.key) - 1 : ['Enter', ' '].includes(event.key) ? state.selected : -1
    const candidate = state.page.items[index]
    if (candidate) { event.preventDefault(); void choose(candidate) }
  }
  return { ...toRefs(state), query, choose, turnPage, onKeydown }
}
