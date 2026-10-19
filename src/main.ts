import { createPinia } from 'pinia'
import { createApp } from 'vue'
import App from './App.vue'
import { router } from './router'
import './assets/styles/main.css'
const app = createApp(App)
app.use(createPinia())
app.use(router)
app.config.errorHandler = error => {
  console.error(error instanceof Error ? error.name : 'UI_ERROR')
}
app.mount('#app')
