import type { IconName } from '@/components/ui/icons'
export const NAVIGATION: { path: string; label: string; icon: IconName; group: string }[] = [
  { path: '/', label: '概览', icon: 'grid', group: '工作台' },
  { path: '/input', label: '输入设置', icon: 'keyboard', group: '输入与语言' },
  { path: '/languages', label: '语言与引擎', icon: 'globe', group: '输入与语言' },
  { path: '/dictionary', label: '词库管理', icon: 'book', group: '输入与语言' },
  { path: '/learning', label: '英语学习', icon: 'star', group: '学习空间' },
  { path: '/statistics', label: '学习统计', icon: 'chart', group: '学习空间' },
  { path: '/shortcuts', label: '快捷键', icon: 'command', group: '偏好设置' },
  { path: '/appearance', label: '外观', icon: 'palette', group: '偏好设置' },
  { path: '/privacy', label: '隐私与数据', icon: 'shield', group: '偏好设置' },
  { path: '/about', label: '关于', icon: 'info', group: '偏好设置' },
]
