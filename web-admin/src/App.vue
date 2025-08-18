<template>
  <div id="app">
    <el-container class="layout-container">
      <!-- 侧边栏 -->
      <el-aside width="250px" class="sidebar">
        <div class="logo">
          <h2>SimpleMediaServer</h2>
        </div>
        <el-menu
          :default-active="$route.path"
          router
          class="sidebar-menu"
          background-color="#304156"
          text-color="#bfcbd9"
          active-text-color="#409EFF"
        >
          <el-menu-item index="/dashboard">
            <el-icon><Monitor /></el-icon>
            <span>仪表盘</span>
          </el-menu-item>

          <el-sub-menu index="config">
            <template #title>
              <el-icon><Setting /></el-icon>
              <span>系统配置</span>
            </template>
            <el-menu-item index="/config/server">服务器配置</el-menu-item>
            <el-menu-item index="/config/protocols">协议配置</el-menu-item>
            <el-menu-item index="/config/hook">Hook配置</el-menu-item>
            <el-menu-item index="/config/cdn">CDN配置</el-menu-item>
          </el-sub-menu>

          <el-sub-menu index="streams">
            <template #title>
              <el-icon><VideoPlay /></el-icon>
              <span>流媒体管理</span>
            </template>
            <el-menu-item index="/streams/list">流列表</el-menu-item>
            <!-- <el-menu-item index="/streams/clients">客户端管理</el-menu-item> -->
          </el-sub-menu>

          <el-sub-menu index="protocols">
            <template #title>
              <el-icon><Connection /></el-icon>
              <span>协议管理</span>
            </template>
            <el-menu-item index="/protocols/rtsp">RTSP</el-menu-item>
            <el-menu-item index="/protocols/rtmp">RTMP</el-menu-item>
            <el-menu-item index="/protocols/webrtc">WebRTC</el-menu-item>
            <el-menu-item index="/protocols/srt">SRT</el-menu-item>
            <el-menu-item index="/protocols/gb28181">GB28181</el-menu-item>
            <el-menu-item index="/protocols/gb28181-sip">GB28181 SIP</el-menu-item>
            <el-menu-item index="/protocols/jt1078">JT1078</el-menu-item>
            <el-menu-item index="/protocols/httpstream">HTTP流媒体</el-menu-item>
          </el-sub-menu>

          <el-sub-menu index="record">
            <template #title>
              <el-icon><VideoCamera /></el-icon>
              <span>录制点播</span>
            </template>
            <el-menu-item index="/record/tasks">录制任务</el-menu-item>
            <el-menu-item index="/record/vod">点播管理</el-menu-item>
          </el-sub-menu>

          <!-- <el-sub-menu index="monitor">
            <template #title>
              <el-icon><DataAnalysis /></el-icon>
              <span>系统监控</span>
            </template>
            <el-menu-item index="/monitor/server">服务器状态</el-menu-item>
            <el-menu-item index="/monitor/loops">事件循环</el-menu-item>
            <el-menu-item index="/monitor/performance">性能统计</el-menu-item>
          </el-sub-menu> -->
        </el-menu>
      </el-aside>

      <!-- 主内容区 -->
      <el-container>
        <!-- 顶部导航 -->
        <el-header class="header">
          <div class="header-left">
            <el-breadcrumb separator="/">
              <el-breadcrumb-item :to="{ path: '/' }">首页</el-breadcrumb-item>
              <el-breadcrumb-item v-for="item in breadcrumbs" :key="item.path" :to="{ path: item.path }">
                {{ item.name }}
              </el-breadcrumb-item>
            </el-breadcrumb>
          </div>
          <div class="header-right">
            <el-button type="primary" @click="refreshData">
              <el-icon><Refresh /></el-icon>
              刷新
            </el-button>
            <el-dropdown>
              <span class="el-dropdown-link">
                <el-icon><User /></el-icon>
                管理员
                <el-icon class="el-icon--right"><arrow-down /></el-icon>
              </span>
              <template #dropdown>
                <el-dropdown-menu>
                  <el-dropdown-item>个人设置</el-dropdown-item>
                  <el-dropdown-item divided>退出登录</el-dropdown-item>
                </el-dropdown-menu>
              </template>
            </el-dropdown>
          </div>
        </el-header>

        <!-- 主内容 -->
        <el-main class="main-content">
          <router-view />
        </el-main>
      </el-container>
    </el-container>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { useRoute } from 'vue-router'

const route = useRoute()

const breadcrumbs = computed(() => {
  const matched = route.matched.filter(item => item.meta && item.meta.title)
  return matched.map(item => ({
    name: item.meta.title,
    path: item.path
  }))
})

const refreshData = () => {
  window.location.reload()
}
</script>

<style scoped>
.layout-container {
  height: 100vh;
}

.sidebar {
  background-color: #304156;
  overflow: hidden;
}

.logo {
  height: 60px;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: #2b3a4b;
  color: white;
  margin-bottom: 0;
}

.logo h2 {
  margin: 0;
  font-size: 18px;
}

.sidebar-menu {
  border: none;
  height: calc(100vh - 60px);
  overflow-y: auto;
}

.header {
  background-color: #fff;
  border-bottom: 1px solid #e6e6e6;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 20px;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 15px;
}

.el-dropdown-link {
  cursor: pointer;
  color: #409EFF;
  display: flex;
  align-items: center;
}

.main-content {
  background-color: #f5f5f5;
  padding: 20px;
}
</style>

<style>
body {
  margin: 0;
  font-family: 'Helvetica Neue', Helvetica, 'PingFang SC', 'Hiragino Sans GB', 'Microsoft YaHei', '微软雅黑', Arial, sans-serif;
}

#app {
  height: 100vh;
}
</style>
