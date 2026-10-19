import { expect, test } from '@playwright/test'

test('candidate keyboard selection and real collection', async ({ page }) => {
  await page.goto('/')
  await expect(
    page.getByRole('heading', { name: '让表达，自然发生。' })
  ).toBeVisible()
  await expect(page.getByRole('option').first()).toContainText('学习')
  await page.getByRole('button', { name: '收藏选中词' }).click()
  await expect(page.getByRole('button', { name: '已收藏' })).toBeDisabled()
  await page.getByLabel('输入预览拼音').press('Enter')
  await expect(page.getByLabel('输入预览拼音')).toHaveValue('')
  await page
    .getByRole('navigation')
    .getByRole('link', { name: '英语学习' })
    .click()
  await expect(
    page.getByRole('heading', { name: '学习', exact: true })
  ).toBeVisible()
  await page.getByRole('button', { name: '标记掌握' }).click()
  await expect(page.getByText('已掌握', { exact: true }).last()).toBeVisible()
  await page.reload()
  await expect(page.getByRole('button', { name: '重新复习' })).toBeVisible()
})

test('settings draft survives navigation, cancellation and restart persistence', async ({
  page,
}) => {
  await page.goto('/#/appearance')
  await page.getByRole('button', { name: '深色', exact: true }).click()
  await expect(page.locator('html')).toHaveClass('dark')
  await page
    .getByRole('navigation')
    .getByRole('link', { name: '输入设置' })
    .click()
  await expect(page.getByText('有未保存的更改')).toBeVisible()
  await page.getByRole('button', { name: '取消', exact: true }).click()
  await expect(page.locator('html')).not.toHaveClass('dark')
  await page.getByRole('button', { name: '双语详细', exact: true }).click()
  await page.getByRole('button', { name: '保存设置' }).click()
  await expect(page.getByText('有未保存的更改')).toBeHidden()
  await page.reload()
  await expect(
    page.getByRole('button', { name: '双语详细', exact: true })
  ).toHaveAttribute('aria-pressed', 'true')
})

test('personal dictionary CRUD, import validation and deliberate data clearing', async ({
  page,
}) => {
  await page.goto('/#/dictionary')
  await page.getByRole('button', { name: '添加词条' }).click()
  await page.getByLabel('中文词条').fill('项目管理')
  await page.getByLabel('全拼', { exact: true }).fill('xiang mu guan li')
  await page.getByLabel('英文释义').fill('project management')
  await page.getByRole('button', { name: '保存词条' }).click()
  await expect(page.getByRole('dialog')).toBeHidden()
  await page.getByRole('button', { name: /个人词库 · 1/ }).click()
  await expect(page.getByRole('cell', { name: '项目管理' })).toBeVisible()
  await page.getByRole('button', { name: '编辑项目管理' }).click()
  await page.getByLabel('英文释义').fill('managing a project')
  await page.getByRole('button', { name: '保存词条' }).click()
  await expect(
    page.getByRole('cell', { name: 'managing a project' })
  ).toBeVisible()
  await page
    .getByLabel('导入个人词库 JSON')
    .setInputFiles({
      name: 'words.json',
      mimeType: 'application/json',
      buffer: Buffer.from('[{"text":"语言连接","reading":"yu yan lian jie"}]'),
    })
  await expect(
    page.getByRole('heading', { name: '确认导入个人词库' })
  ).toBeVisible()
  await page.getByRole('button', { name: '导入 1 条' }).click()
  await expect(page.getByRole('cell', { name: '语言连接' })).toBeVisible()
  await page.getByRole('button', { name: '删除项目管理' }).click()
  await page
    .getByRole('dialog')
    .getByRole('button', { name: '删除词条' })
    .click()
  await expect(page.getByRole('cell', { name: '项目管理' })).toBeHidden()
  await page
    .getByRole('navigation')
    .getByRole('link', { name: '隐私与数据' })
    .click()
  await page.getByRole('button', { name: '重置数据', exact: true }).click()
  await page.getByRole('dialog').getByRole('button', { name: '取消' }).click()
  await page
    .getByRole('navigation')
    .getByRole('link', { name: '词库管理' })
    .click()
  await page.getByRole('button', { name: /个人词库 · 1/ }).click()
  await expect(page.getByRole('cell', { name: '语言连接' })).toBeVisible()
})

test('all routes load, desktop and compact layouts have no page overflow', async ({
  page,
}) => {
  const errors: string[] = []
  page.on('pageerror', error => errors.push(error.message))
  for (const route of [
    '',
    'input',
    'languages',
    'dictionary',
    'learning',
    'statistics',
    'shortcuts',
    'appearance',
    'privacy',
    'about',
  ]) {
    await page.goto(`/#/${route}`)
    await expect(page.locator('h1')).toBeVisible()
    expect(
      await page.evaluate(
        () => document.documentElement.scrollWidth <= window.innerWidth
      )
    ).toBeTruthy()
  }
  await page.goto('/')
  await page.screenshot({
    path: 'test-results/overview-light.png',
    fullPage: true,
  })
  await page.setViewportSize({ width: 768, height: 1024 })
  expect(
    await page.evaluate(
      () => document.documentElement.scrollWidth <= window.innerWidth
    )
  ).toBeTruthy()
  await page.setViewportSize({ width: 390, height: 844 })
  expect(
    await page.evaluate(
      () => document.documentElement.scrollWidth <= window.innerWidth
    )
  ).toBeTruthy()
  await page.goto('/#/appearance')
  await page.getByRole('button', { name: '深色', exact: true }).click()
  await page.setViewportSize({ width: 1440, height: 1000 })
  await page.screenshot({
    path: 'test-results/appearance-dark.png',
    fullPage: true,
  })
  expect(errors).toEqual([])
})
