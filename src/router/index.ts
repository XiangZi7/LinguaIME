import { createRouter, createWebHashHistory } from 'vue-router'

export const router = createRouter({
  history: createWebHashHistory(),
  scrollBehavior: () => ({ top: 0 }),
  routes: [
    {
      path: '/',
      component: () => import('@/features/overview/OverviewPage.vue'),
    },
    {
      path: '/input',
      component: () =>
        import('@/features/input-settings/InputSettingsPage.vue'),
    },
    {
      path: '/languages',
      component: () => import('@/features/languages/LanguagesPage.vue'),
    },
    {
      path: '/dictionary',
      component: () => import('@/features/dictionary/DictionaryPage.vue'),
    },
    {
      path: '/learning',
      component: () => import('@/features/english-learning/LearningPage.vue'),
    },
    {
      path: '/statistics',
      component: () => import('@/features/statistics/StatisticsPage.vue'),
    },
    {
      path: '/shortcuts',
      component: () => import('@/features/shortcuts/ShortcutsPage.vue'),
    },
    {
      path: '/appearance',
      component: () => import('@/features/appearance/AppearancePage.vue'),
    },
    {
      path: '/privacy',
      component: () => import('@/features/privacy/PrivacyPage.vue'),
    },
    {
      path: '/about',
      component: () => import('@/features/about/AboutPage.vue'),
    },
    { path: '/:pathMatch(.*)*', redirect: '/' },
  ],
})
