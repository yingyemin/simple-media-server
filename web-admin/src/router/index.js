import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  {
    path: '/',
    redirect: '/dashboard'
  },
  {
    path: '/dashboard',
    name: 'Dashboard',
    component: () => import('@/views/Dashboard.vue'),
    meta: { title: '仪表盘' }
  },
  // 系统配置
  {
    path: '/config/server',
    name: 'ServerConfig',
    component: () => import('@/views/config/ServerConfig.vue'),
    meta: { title: '服务器配置' }
  },
  {
    path: '/config/protocols',
    name: 'ProtocolsConfig',
    component: () => import('@/views/config/ProtocolsConfig.vue'),
    meta: { title: '协议配置' }
  },
  {
    path: '/config/hook',
    name: 'HookConfig',
    component: () => import('@/views/config/HookConfig.vue'),
    meta: { title: 'Hook配置' }
  },
  {
    path: '/config/cdn',
    name: 'CdnConfig',
    component: () => import('@/views/config/CdnConfig.vue'),
    meta: { title: 'CDN配置' }
  },
  // 流媒体管理
  {
    path: '/streams/list',
    name: 'StreamsList',
    component: () => import('@/views/streams/StreamsList.vue'),
    meta: { title: '流列表' }
  },
  {
    path: '/streams/clients',
    name: 'ClientsList',
    component: () => import('@/views/streams/ClientsList.vue'),
    meta: { title: '客户端管理' }
  },
  // 协议管理
  {
    path: '/protocols/rtsp',
    name: 'RtspManagement',
    component: () => import('@/views/protocols/RtspManagement.vue'),
    meta: { title: 'RTSP管理' }
  },
  {
    path: '/protocols/rtmp',
    name: 'RtmpManagement',
    component: () => import('@/views/protocols/RtmpManagement.vue'),
    meta: { title: 'RTMP管理' }
  },
  {
    path: '/protocols/webrtc',
    name: 'WebrtcManagement',
    component: () => import('@/views/protocols/WebrtcManagement.vue'),
    meta: { title: 'WebRTC管理' }
  },
  {
    path: '/protocols/srt',
    name: 'SrtManagement',
    component: () => import('@/views/protocols/SrtManagement.vue'),
    meta: { title: 'SRT管理' }
  },
  {
    path: '/protocols/gb28181',
    name: 'Gb28181Management',
    component: () => import('@/views/protocols/Gb28181Management.vue'),
    meta: { title: 'GB28181管理' }
  },
  {
    path: '/protocols/jt1078',
    name: 'Jt1078Management',
    component: () => import('@/views/protocols/Jt1078Management.vue'),
    meta: { title: 'JT1078管理' }
  },
  // 录制点播
  {
    path: '/record/tasks',
    name: 'RecordTasks',
    component: () => import('@/views/record/RecordTasks.vue'),
    meta: { title: '录制任务' }
  },
  {
    path: '/record/vod',
    name: 'VodManagement',
    component: () => import('@/views/record/VodManagement.vue'),
    meta: { title: '点播管理' }
  },
  // 系统监控
  {
    path: '/monitor/server',
    name: 'ServerMonitor',
    component: () => import('@/views/monitor/ServerMonitor.vue'),
    meta: { title: '服务器状态' }
  },
  {
    path: '/monitor/loops',
    name: 'LoopsMonitor',
    component: () => import('@/views/monitor/LoopsMonitor.vue'),
    meta: { title: '事件循环' }
  },
  {
    path: '/monitor/performance',
    name: 'PerformanceMonitor',
    component: () => import('@/views/monitor/PerformanceMonitor.vue'),
    meta: { title: '性能统计' }
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

export default router
