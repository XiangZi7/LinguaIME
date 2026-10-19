<script setup lang="ts">
import type { UserPhrase } from '@/types/engine'
import { reactive, toRefs, watch } from 'vue'
import { UiButton } from '@/components/ui'
import { errorMessage } from '@/services'
import { validatePhrase } from '@/services/validation'
const props = defineProps<{ phrase: UserPhrase | null; busy: boolean }>()
const emit = defineEmits<{ save: [phrase: UserPhrase]; cancel: [] }>()
// 响应式状态
const state = reactive({
  // 中文词条草稿
  text: '',
  // 拼音草稿
  reading: '',
  // 用户提供的英文释义
  translation: '',
  // 表单校验反馈
  error: '',
})
watch(
  () => props.phrase,
  phrase => {
    state.text = phrase?.text ?? ''
    state.reading = phrase?.reading ?? ''
    state.translation = phrase?.translation ?? ''
    state.error = ''
  },
  { immediate: true }
)
function submit(): void {
  const phrase: UserPhrase = {
    id: props.phrase?.id ?? crypto.randomUUID(),
    text: state.text.trim(),
    reading: state.reading.trim(),
    translation: state.translation.trim() || null,
  }
  try {
    validatePhrase(phrase)
    emit('save', phrase)
  } catch (error) {
    state.error = errorMessage(error)
  }
}
const { text, reading, translation, error } = toRefs(state)
</script>
<template>
  <form
    class="space-y-5"
    @submit.prevent="submit"
  >
    <div>
      <label
        for="phrase-text"
        class="field-label"
      >
        中文词条
        <span
          class="text-danger"
          aria-hidden="true"
        >
          *
        </span>
      </label>
      <input
        id="phrase-text"
        v-model="text"
        class="field w-full"
        required
        maxlength="32"
        placeholder="例如：项目管理"
        autofocus
      />
    </div>
    <div>
      <label
        for="phrase-reading"
        class="field-label"
      >
        全拼
        <span
          class="text-danger"
          aria-hidden="true"
        >
          *
        </span>
      </label>
      <input
        id="phrase-reading"
        v-model="reading"
        class="field w-full"
        required
        maxlength="128"
        placeholder="例如：xiang mu guan li"
      />
      <p class="mt-2 text-[11px] text-muted">
        支持空格或单引号分隔；ü 可输入 v。
      </p>
    </div>
    <div>
      <label
        for="phrase-translation"
        class="field-label"
      >
        英文释义
        <span class="font-normal text-faint">· 可选</span>
      </label>
      <input
        id="phrase-translation"
        v-model="translation"
        class="field w-full"
        maxlength="256"
        placeholder="填写你确认过的英文表达"
      />
    </div>
    <p
      v-if="error"
      role="alert"
      class="text-xs text-danger"
    >
      {{ error }}
    </p>
    <div class="flex justify-end gap-2 border-t border-line pt-5">
      <UiButton
        :disabled="busy"
        @click="$emit('cancel')"
      >
        取消
      </UiButton>
      <UiButton
        type="submit"
        variant="primary"
        :busy="busy"
      >
        保存词条
      </UiButton>
    </div>
  </form>
</template>
