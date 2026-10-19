<script setup lang="ts">
import type { Candidate, UserPhrase } from '@/types/engine'
import { UiBadge, UiButton, UiEmptyState, UiIcon } from '@/components/ui'
defineProps<{
  entries: (Candidate | UserPhrase)[]
  personal: boolean
  loading?: boolean
}>()
defineEmits<{
  edit: [phrase: UserPhrase]
  remove: [phrase: UserPhrase]
  favorite: [candidate: Candidate]
}>()
</script>
<template>
  <div class="overflow-x-auto">
    <table
      v-if="entries.length"
      class="w-full text-left text-xs"
    >
      <thead class="border-b border-line bg-canvas text-[10px] text-muted">
        <tr>
          <th class="px-5 py-3 font-medium">中文词条</th>
          <th class="px-4 py-3 font-medium">拼音</th>
          <th class="px-4 py-3 font-medium">英文释义</th>
          <th class="px-4 py-3 font-medium">来源</th>
          <th class="px-4 py-3 text-right font-medium">操作</th>
        </tr>
      </thead>
      <tbody
        class="divide-y divide-line"
        :class="loading ? 'opacity-40' : ''"
      >
        <tr
          v-for="entry in entries"
          :key="entry.id"
          class="hover:bg-canvas/70"
        >
          <td class="min-w-28 px-5 py-4 text-sm font-medium">
            {{ entry.text }}
          </td>
          <td class="max-w-48 px-4 py-4 text-muted">
            {{ entry.reading }}
          </td>
          <td class="max-w-xs px-4 py-4 text-translation">
            <span class="line-clamp-2">{{ entry.translation || '—' }}</span>
          </td>
          <td class="px-4 py-4 whitespace-nowrap">
            <UiBadge :tone="personal ? 'green' : 'neutral'">
              {{
                personal
                  ? '个人词库'
                  : 'source' in entry
                    ? entry.source
                    : '个人词库'
              }}
            </UiBadge>
          </td>
          <td class="px-4 py-3">
            <div class="flex justify-end gap-1">
              <template v-if="personal">
                <UiButton
                  size="sm"
                  variant="ghost"
                  :aria-label="`编辑${entry.text}`"
                  @click="$emit('edit', entry)"
                >
                  <UiIcon
                    name="edit"
                    :size="14"
                  />
                </UiButton>
                <UiButton
                  size="sm"
                  variant="ghost"
                  :aria-label="`删除${entry.text}`"
                  @click="$emit('remove', entry)"
                >
                  <UiIcon
                    name="trash"
                    :size="14"
                  />
                </UiButton>
              </template>
              <UiButton
                v-else-if="'key' in entry"
                size="sm"
                variant="ghost"
                :aria-label="`收藏${entry.text}`"
                :disabled="loading"
                @click="$emit('favorite', entry)"
              >
                <UiIcon
                  name="star"
                  :size="15"
                />
              </UiButton>
            </div>
          </td>
        </tr>
      </tbody>
    </table>
    <UiEmptyState
      v-else
      :title="loading ? '正在检索词条…' : '这里还没有匹配的词条'"
      :description="
        personal
          ? '添加常用词或导入个人词库，让输入更懂你。'
          : '输入拼音检索，例如 xuexi、nihao、zhongwen。'
      "
    />
  </div>
</template>
