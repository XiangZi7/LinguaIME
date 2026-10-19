<script setup lang="ts">
import { UiBadge, UiIcon, UiPageHeader, UiPanel } from '@/components/ui'
import { desktop } from '@/services'
import { useAppStore } from '@/stores/app'
const app = useAppStore()
</script>
<template>
  <UiPageHeader
    title="语言与引擎"
    description="从中文出发，让不同语言各得其所。"
    eyebrow="LANGUAGES, CONNECTED"
  />
  <div class="max-w-4xl space-y-5">
    <UiPanel>
      <div class="flex flex-wrap items-start justify-between gap-5 p-6">
        <div class="flex gap-4">
          <span
            class="grid size-14 place-items-center rounded-xl border border-accent/10 bg-accent-soft text-2xl font-medium text-accent"
          >
            中
          </span>
          <div>
            <div class="flex flex-wrap items-center gap-3">
              <h2 class="text-lg font-semibold">中文</h2>
              <UiBadge
                tone="green"
                dot
              >
                引擎可用
              </UiBadge>
            </div>
            <p class="mt-1 text-xs text-muted">简体中文 · zh-CN</p>
            <p class="mt-3 text-xs text-muted">
              全拼输入 / 中英双语候选 / 本地词频学习
            </p>
          </div>
        </div>
        <RouterLink
          to="/input"
          class="text-link"
        >
          输入设置
          <UiIcon
            name="arrow"
            :size="14"
          />
        </RouterLink>
      </div>
      <dl
        class="grid gap-5 border-t border-line bg-canvas/50 px-6 py-5 text-xs sm:grid-cols-3"
      >
        <div>
          <dt class="text-faint">输入引擎</dt>
          <dd class="mt-2 text-ink">Lingua Core 0.1.0</dd>
        </div>
        <div>
          <dt class="text-faint">离线词典</dt>
          <dd class="mt-2 text-ink">
            CC-CEDICT · {{ app.snapshot?.dictionaryCount.toLocaleString() }} 条
          </dd>
        </div>
        <div>
          <dt class="text-faint">Windows TSF</dt>
          <dd class="mt-2">
            <UiBadge :tone="app.snapshot?.nativeRegistered ? 'green' : 'amber'">
              {{
                app.snapshot?.nativeRegistered
                  ? '已注册 · 兼容性待验收'
                  : desktop
                    ? '尚未注册'
                    : '请在桌面端验证'
              }}
            </UiBadge>
          </dd>
        </div>
      </dl>
    </UiPanel>
    <UiPanel>
      <div class="flex items-start gap-4 p-6">
        <span
          class="grid size-14 place-items-center rounded-xl bg-blue-soft text-lg font-semibold text-blue-ink"
        >
          En
        </span>
        <div>
          <div class="flex items-center gap-3">
            <h2 class="text-lg font-semibold">English</h2>
            <UiBadge tone="blue">双语辅助</UiBadge>
          </div>
          <p class="mt-1 text-xs text-muted">英语 · en</p>
          <p class="mt-3 text-xs leading-6 text-muted">
            在中文候选中显示可信英文释义，支持本地收藏与复习。
            <br />
            当前不提供独立英文预测输入、在线翻译或语音发音。
          </p>
          <RouterLink
            to="/learning"
            class="text-link mt-4"
          >
            打开我的生词本
            <UiIcon
              name="arrow"
              :size="13"
            />
          </RouterLink>
        </div>
      </div>
    </UiPanel>
    <UiPanel>
      <div class="flex items-start gap-4 p-6">
        <span
          class="grid size-14 place-items-center rounded-xl bg-subtle text-xl text-muted"
        >
          あ
        </span>
        <div>
          <div class="flex items-center gap-3">
            <h2 class="text-lg font-semibold text-muted">日本語</h2>
            <UiBadge>规划中</UiBadge>
          </div>
          <p class="mt-1 text-xs text-muted">日语 · ja-JP</p>
          <p class="mt-3 text-xs leading-6 text-muted">
            独立罗马音、假名与汉字转换引擎。
            <br />
            待引擎、词典与许可证审核完成后提供安装入口。
          </p>
        </div>
      </div>
    </UiPanel>
  </div>
</template>
