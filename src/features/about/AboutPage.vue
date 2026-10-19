<script setup lang="ts">
import {
  UiBadge,
  UiIcon,
  UiPageHeader,
  UiPanel,
  UiSettingRow,
} from '@/components/ui'
import { desktop } from '@/services'
</script>
<template>
  <UiPageHeader
    title="关于 LinguaIME"
    description="以熟悉的方式输入，以自然的方式学习。"
    eyebrow="BUILT FOR EVERY EXPRESSION"
  />
  <div class="max-w-4xl space-y-5">
    <UiPanel>
      <div class="flex flex-wrap items-center gap-5 p-7">
        <span
          class="grid size-16 place-items-center rounded-2xl bg-accent text-surface"
        >
          <UiIcon
            name="leaf"
            :size="36"
          />
        </span>
        <div>
          <h2 class="text-2xl font-semibold tracking-tight">
            LinguaIME
            <span class="text-sm font-normal text-muted">0.1.0</span>
          </h2>
          <p class="mt-2 text-xs text-muted">
            中文为主，英文直显。让语言，自然连接。
          </p>
          <div class="mt-3 flex gap-2">
            <UiBadge tone="green">Windows Preview</UiBadge>
            <UiBadge>
              {{ desktop ? 'Tauri 管理应用' : '浏览器交互预览' }}
            </UiBadge>
          </div>
        </div>
      </div>
    </UiPanel>
    <UiPanel title="当前版本">
      <UiSettingRow
        title="已提供"
        description="本地全拼检索、三种候选模式、个人词条、词频学习、收藏复习、SQLite 设置与数据管理。"
      />
      <UiSettingRow
        title="Windows 原生接入"
        description="独立 C++ TSF/COM 宿主与 Win32 候选窗，通过版本化 C ABI 调用 Rust。宿主不依赖管理应用运行。目标应用兼容性、高 DPI 和多显示器仍需实机验收。"
      >
        <UiBadge tone="amber">开发预览</UiBadge>
      </UiSettingRow>
      <UiSettingRow
        title="后续计划"
        description="完善安装签名与兼容性、敏感输入检查、性能基准；之后接入主动翻译、发音与独立日语引擎。"
      >
        <UiBadge>规划中</UiBadge>
      </UiSettingRow>
    </UiPanel>
    <UiPanel title="首次使用原生输入法">
      <ol class="space-y-4 p-5 text-xs leading-6 text-muted">
        <li>
          <span class="mr-3 font-semibold text-accent">01</span>
          构建：在项目根目录运行
          <code class="rounded bg-subtle px-1.5 py-1">pnpm native:build</code>
          。
        </li>
        <li>
          <span class="mr-3 font-semibold text-accent">02</span>
          注册：运行
          <code class="rounded bg-subtle px-1.5 py-1">
            pnpm native:register
          </code>
          。如遇权限问题，查看项目原生接入文档。
        </li>
        <li>
          <span class="mr-3 font-semibold text-accent">03</span>
          使用
          <kbd class="keycap">Win</kbd>
          +
          <kbd class="keycap">Space</kbd>
          切换到 LinguaIME，在记事本中验证输入。
        </li>
        <li>
          <span class="mr-3 font-semibold text-accent">04</span>
          卸载注册：运行
          <code class="rounded bg-subtle px-1.5 py-1">
            pnpm native:unregister
          </code>
          ，个人数据会保留。
        </li>
      </ol>
    </UiPanel>
    <UiPanel title="数据来源与开源致谢">
      <div class="space-y-3 px-5 py-5 text-xs leading-6 text-muted">
        <p>
          <a
            href="https://www.mdbg.net/chinese/dictionary?page=cedict"
            target="_blank"
            rel="noreferrer"
            class="text-link"
          >
            CC-CEDICT / MDBG
            <UiIcon
              name="arrow"
              :size="12"
            />
          </a>
          · CC-CEDICT contributors，原始 CEDICT 由 Paul Denisowski
          发起。数据遵循
          <a
            href="https://creativecommons.org/licenses/by-sa/4.0/"
            target="_blank"
            rel="noreferrer"
            class="text-link"
          >
            CC BY-SA 4.0
          </a>
          。词典数据及其衍生数据保留同一许可。
        </p>
        <p>
          Vue · TypeScript · Tailwind CSS v4 · Pinia · Tauri 2 · Rust · SQLite
        </p>
        <p>
          静态词频来自
          jieba（MIT），用于本地候选排序；不包含在线上下文模型。未提供释义、词性或例句时，不显示推测结果。
        </p>
      </div>
    </UiPanel>
  </div>
</template>
