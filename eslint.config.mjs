import antfu from '@antfu/eslint-config'

export default antfu({
  vue: true,
  typescript: true,
  stylistic: false,
  jsonc: false,
  yaml: false,
  markdown: false,
  ignores: [
    'build/**',
    'target/**',
    'data/**',
    'public/**',
    'src-tauri/gen/**',
    'prompt/**',
    '.claude/**',
    '.trae/**',
  ],
  rules: {
    // Prettier is the single formatter; ESLint enforces correctness and maintainability.
    'vue/one-component-per-file': 'error',
    'vue/block-order': ['error', { order: ['script', 'template', 'style'] }],
  },
})
