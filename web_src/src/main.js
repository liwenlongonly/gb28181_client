import './assets/main.scss'

import { createApp } from 'vue'
import router from "@/router/index.js";
import App from './App.vue'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import locale from 'element-plus/dist/locale/zh-cn.js'

const app = createApp(App);
app.use(router)
app.use(ElementPlus,{locale});
app.mount('#app')
