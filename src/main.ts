import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import './assets/styles/main.css'
const app = createApp(App)
app.use(createPinia())
app.config.errorHandler = error => {
  console.error(error)
}
app.mount('#app')
