<script setup lang="ts">
import { onBeforeUnmount, useId, useTemplateRef, watch } from 'vue'
import UiIcon from './UiIcon.vue'
defineProps<{ title: string; description?: string }>()
const model = defineModel<boolean>({ required: true })
const dialog = useTemplateRef<HTMLDialogElement>('dialog')
const titleId = useId()
watch(
  model,
  open => {
    if (open) dialog.value?.showModal()
    else dialog.value?.close()
  },
  { flush: 'post' }
)
onBeforeUnmount(() => dialog.value?.close())
</script>
<template>
  <Teleport to="body">
    <dialog
      ref="dialog"
      :aria-labelledby="titleId"
      class="m-auto max-h-[85vh] w-[min(560px,calc(100%-32px))] overflow-y-auto rounded-2xl border border-line bg-surface p-0 text-ink shadow-xl backdrop:bg-black/30"
      @cancel="model = false"
      @close="model = false"
      @click="
        event => {
          if (event.target === dialog) model = false
        }
      "
    >
      <header class="flex items-start justify-between border-b border-line p-6">
        <div>
          <h2
            :id="titleId"
            class="text-lg font-semibold"
          >
            {{ title }}
          </h2>
          <p
            v-if="description"
            class="mt-2 text-xs leading-6 text-muted"
          >
            {{ description }}
          </p>
        </div>
        <button
          type="button"
          aria-label="关闭对话框"
          class="-mt-1 -mr-2 grid size-9 cursor-pointer place-items-center rounded-lg text-muted hover:bg-subtle"
          @click="model = false"
        >
          <UiIcon name="close" />
        </button>
      </header>
      <div class="p-6">
        <slot />
      </div>
      <footer
        v-if="$slots.footer"
        class="flex justify-end gap-2 border-t border-line px-6 py-4"
      >
        <slot name="footer" />
      </footer>
    </dialog>
  </Teleport>
</template>
