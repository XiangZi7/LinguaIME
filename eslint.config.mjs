import antfu from '@antfu/eslint-config'

export default antfu({
  vue: true,
  typescript: true,
  test: { overrides: { 'test/no-import-node-test': 'off' } },
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
    'vue/singleline-html-element-content-newline': 'off',
    'vue/html-self-closing': 'off',
    'vue/block-order': ['error', { order: ['script', 'template', 'style'] }],
  },
})
