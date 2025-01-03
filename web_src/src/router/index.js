import { createWebHistory, createRouter } from 'vue-router'

import LayoutVue from "@/views/Layout.vue";
import MainPageVue from "@/views/MainPage.vue";

const routes = [
  {
    path: '/', redirect:'/mainPage', component: LayoutVue, children: [
        { path: '/mainPage', component: MainPageVue }
    ]
  },
  {
    path: '/mainPage', component: MainPageVue
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

export default router
