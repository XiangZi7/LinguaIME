import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import './assets/styles/main.css'
import { router } from './router'
const app = createApp(App)
app.use(createPinia())
app.use(router)
app.config.errorHandler = error => {
  console.error(error instanceof Error ? error.name : 'UI_ERROR')
}
app.mount('#app')
