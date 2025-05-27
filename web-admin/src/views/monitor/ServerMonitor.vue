<template>
  <div class="server-monitor">
    <el-row :gutter="20">
      <!-- 服务器基本信息 -->
      <el-col :span="12">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>服务器信息</span>
              <el-button type="text" @click="refreshServerInfo">刷新</el-button>
            </div>
          </template>
          <el-descriptions :column="2" border>
            <el-descriptions-item label="版本">{{ serverInfo.version || 'N/A' }}</el-descriptions-item>
            <el-descriptions-item label="本地IP">{{ serverInfo.localIp || 'N/A' }}</el-descriptions-item>
            <el-descriptions-item label="启动时间">{{ formatTime(serverInfo.startTime) }}</el-descriptions-item>
            <el-descriptions-item label="运行时间">{{ formatUptime(serverInfo.uptime) }}</el-descriptions-item>
            <el-descriptions-item label="CPU使用率">
              <el-progress :percentage="serverInfo.cpuUsage || 0" :color="getProgressColor(serverInfo.cpuUsage)" />
            </el-descriptions-item>
            <el-descriptions-item label="内存使用率">
              <el-progress :percentage="serverInfo.memoryUsage || 0" :color="getProgressColor(serverInfo.memoryUsage)" />
            </el-descriptions-item>
            <el-descriptions-item label="磁盘使用率">
              <el-progress :percentage="serverInfo.diskUsage || 0" :color="getProgressColor(serverInfo.diskUsage)" />
            </el-descriptions-item>
            <el-descriptions-item label="网络连接数">{{ serverInfo.connectionCount || 0 }}</el-descriptions-item>
          </el-descriptions>
        </el-card>
      </el-col>

      <!-- 实时统计 -->
      <el-col :span="12">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>实时统计</span>
              <el-button type="text" @click="refreshStats">刷新</el-button>
            </div>
          </template>
          <el-row :gutter="20">
            <el-col :span="12">
              <el-statistic title="活跃流数量" :value="stats.activeStreams" />
            </el-col>
            <el-col :span="12">
              <el-statistic title="在线客户端" :value="stats.onlineClients" />
            </el-col>
            <el-col :span="12">
              <el-statistic title="总带宽" :value="formatBandwidth(stats.totalBandwidth)" />
            </el-col>
            <el-col :span="12">
              <el-statistic title="总流量" :value="formatTraffic(stats.totalTraffic)" />
            </el-col>
          </el-row>
        </el-card>
      </el-col>
    </el-row>

    <!-- 性能图表 -->
    <el-row :gutter="20" style="margin-top: 20px;">
      <el-col :span="24">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>性能监控</span>
              <el-button type="text" @click="refreshCharts">刷新</el-button>
            </div>
          </template>
          <el-row :gutter="20">
            <el-col :span="8">
              <div class="chart-container">
                <h4>CPU使用率</h4>
                <v-chart :option="cpuChartOption" style="height: 200px;" />
              </div>
            </el-col>
            <el-col :span="8">
              <div class="chart-container">
                <h4>内存使用率</h4>
                <v-chart :option="memoryChartOption" style="height: 200px;" />
              </div>
            </el-col>
            <el-col :span="8">
              <div class="chart-container">
                <h4>网络流量</h4>
                <v-chart :option="networkChartOption" style="height: 200px;" />
              </div>
            </el-col>
          </el-row>
        </el-card>
      </el-col>
    </el-row>

    <!-- 协议统计 -->
    <el-row :gutter="20" style="margin-top: 20px;">
      <el-col :span="12">
        <el-card>
          <template #header>
            <span>协议统计</span>
          </template>
          <el-table :data="protocolStats" style="width: 100%">
            <el-table-column prop="protocol" label="协议" width="100">
              <template #default="scope">
                <el-tag>{{ scope.row.protocol.toUpperCase() }}</el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="streamCount" label="流数量" width="100" />
            <el-table-column prop="clientCount" label="客户端数" width="100" />
            <el-table-column prop="bandwidth" label="带宽" min-width="120">
              <template #default="scope">
                <span>{{ formatBandwidth(scope.row.bandwidth) }}</span>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-col>

      <!-- 服务状态 -->
      <el-col :span="12">
        <el-card>
          <template #header>
            <span>服务状态</span>
          </template>
          <el-table :data="serviceStatus" style="width: 100%">
            <el-table-column prop="service" label="服务" width="120" />
            <el-table-column prop="port" label="端口" width="80" />
            <el-table-column prop="status" label="状态" width="100">
              <template #default="scope">
                <el-tag :type="scope.row.status === 'running' ? 'success' : 'danger'">
                  {{ scope.row.status === 'running' ? '运行中' : '停止' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="connections" label="连接数" min-width="100" />
          </el-table>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, computed, watch } from 'vue'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import { LineChart } from 'echarts/charts'
import {
  TitleComponent,
  TooltipComponent,
  GridComponent
} from 'echarts/components'
import VChart from 'vue-echarts'
import { serverAPI } from '@/api'
import { useRealtime } from '@/composables/useRealtime'

use([
  CanvasRenderer,
  LineChart,
  TitleComponent,
  TooltipComponent,
  GridComponent
])

// 使用实时数据管理
const { realtimeStore, getUpdateMode, refreshData } = useRealtime({
  enableWebSocket: true,
  enablePolling: true,
  pollingInterval: 5000, // ServerMonitor需要更频繁的更新
  autoConnect: true
})

const serverInfo = ref({
  version: '',
  localIp: '',
  startTime: '',
  uptime: 0,
  cpuUsage: 0,
  memoryUsage: 0,
  diskUsage: 0,
  connectionCount: 0
})

// 使用computed从realtime store获取统计数据
const stats = computed(() => ({
  activeStreams: realtimeStore.serverStats.activeStreams,
  onlineClients: realtimeStore.serverStats.onlineClients,
  totalBandwidth: realtimeStore.serverStats.totalBandwidth,
  totalTraffic: realtimeStore.serverStats.totalBandwidth * 3600 // 估算每小时流量
}))

// 连接状态
const connectionStatus = computed(() => realtimeStore.connectionStatus)
const updateMode = ref('none')

const protocolStats = ref([])
const serviceStatus = ref([])

// 图表配置
const cpuChartOption = ref({
  tooltip: { trigger: 'axis' },
  xAxis: { type: 'category', data: [] },
  yAxis: { type: 'value', max: 100 },
  series: [{ data: [], type: 'line', smooth: true }]
})

const memoryChartOption = ref({
  tooltip: { trigger: 'axis' },
  xAxis: { type: 'category', data: [] },
  yAxis: { type: 'value', max: 100 },
  series: [{ data: [], type: 'line', smooth: true }]
})

const networkChartOption = ref({
  tooltip: { trigger: 'axis' },
  xAxis: { type: 'category', data: [] },
  yAxis: { type: 'value' },
  series: [
    { name: '入流量', data: [], type: 'line', smooth: true },
    { name: '出流量', data: [], type: 'line', smooth: true }
  ]
})

let refreshTimer = null

const formatTime = (timestamp) => {
  if (!timestamp) return 'N/A'
  return new Date(timestamp).toLocaleString()
}

const formatUptime = (uptime) => {
  if (!uptime) return '0天'
  const days = Math.floor(uptime / 86400)
  const hours = Math.floor((uptime % 86400) / 3600)
  const minutes = Math.floor((uptime % 3600) / 60)
  return `${days}天${hours}小时${minutes}分钟`
}

const formatBandwidth = (bandwidth) => {
  if (!bandwidth) return '0 bps'
  if (bandwidth < 1024) return bandwidth + ' bps'
  if (bandwidth < 1024 * 1024) return (bandwidth / 1024).toFixed(1) + ' Kbps'
  return (bandwidth / 1024 / 1024).toFixed(1) + ' Mbps'
}

const formatTraffic = (traffic) => {
  if (!traffic) return '0 B'
  const units = ['B', 'KB', 'MB', 'GB', 'TB']
  let index = 0
  while (traffic >= 1024 && index < units.length - 1) {
    traffic /= 1024
    index++
  }
  return `${traffic.toFixed(2)} ${units[index]}`
}

const getProgressColor = (percentage) => {
  if (percentage < 50) return '#67c23a'
  if (percentage < 80) return '#e6a23c'
  return '#f56c6c'
}

const refreshServerInfo = async () => {
  try {
    const data = await serverAPI.getServerInfo()
    serverInfo.value = {
      ...data,
      uptime: Math.floor((Date.now() - (data.startTime || Date.now())) / 1000)
    }
  } catch (error) {
    console.error('获取服务器信息失败:', error)
  }
}

const refreshStats = async () => {
  try {
    // 模拟获取统计数据
    stats.value = {
      activeStreams: Math.floor(Math.random() * 100),
      onlineClients: Math.floor(Math.random() * 500),
      totalBandwidth: Math.floor(Math.random() * 1000000000),
      totalTraffic: Math.floor(Math.random() * 10000000000)
    }
  } catch (error) {
    console.error('获取统计数据失败:', error)
  }
}

const refreshCharts = () => {
  // 使用实时历史数据
  const cpuHistory = realtimeStore.getCpuHistory(10)
  const memoryHistory = realtimeStore.getMemoryHistory(10)
  const bandwidthHistory = realtimeStore.getBandwidthHistory(10)

  if (cpuHistory.timestamps.length > 0) {
    // 使用真实的历史数据
    const times = cpuHistory.timestamps.map(time =>
      time.getHours().toString().padStart(2, '0') + ':' +
      time.getMinutes().toString().padStart(2, '0')
    )

    cpuChartOption.value.xAxis.data = times
    cpuChartOption.value.series[0].data = cpuHistory.values

    memoryChartOption.value.xAxis.data = times
    memoryChartOption.value.series[0].data = memoryHistory.values

    networkChartOption.value.xAxis.data = times
    networkChartOption.value.series[0].data = bandwidthHistory.values.map(v => v / 1024 / 1024) // 转换为Mbps
    networkChartOption.value.series[1].data = bandwidthHistory.values.map(v => v / 1024 / 1024 * 0.8) // 模拟出流量
  } else {
    // 如果没有历史数据，使用模拟数据
    const now = new Date()
    const times = []
    const cpuData = []
    const memoryData = []
    const inTraffic = []
    const outTraffic = []

    for (let i = 9; i >= 0; i--) {
      const time = new Date(now.getTime() - i * 60 * 1000)
      times.push(time.getHours() + ':' + time.getMinutes().toString().padStart(2, '0'))
      cpuData.push(Math.floor(Math.random() * 100))
      memoryData.push(Math.floor(Math.random() * 100))
      inTraffic.push(Math.floor(Math.random() * 1000))
      outTraffic.push(Math.floor(Math.random() * 1000))
    }

    cpuChartOption.value.xAxis.data = times
    cpuChartOption.value.series[0].data = cpuData

    memoryChartOption.value.xAxis.data = times
    memoryChartOption.value.series[0].data = memoryData

    networkChartOption.value.xAxis.data = times
    networkChartOption.value.series[0].data = inTraffic
    networkChartOption.value.series[1].data = outTraffic
  }
}

const loadProtocolStats = () => {
  protocolStats.value = [
    { protocol: 'rtsp', streamCount: 15, clientCount: 45, bandwidth: 50000000 },
    { protocol: 'rtmp', streamCount: 8, clientCount: 32, bandwidth: 30000000 },
    { protocol: 'webrtc', streamCount: 5, clientCount: 20, bandwidth: 25000000 },
    { protocol: 'srt', streamCount: 3, clientCount: 12, bandwidth: 15000000 }
  ]
}

const loadServiceStatus = () => {
  serviceStatus.value = [
    { service: 'RTSP', port: 554, status: 'running', connections: 45 },
    { service: 'RTMP', port: 1935, status: 'running', connections: 32 },
    { service: 'HTTP', port: 80, status: 'running', connections: 128 },
    { service: 'WebRTC', port: 7000, status: 'running', connections: 20 },
    { service: 'SRT', port: 6666, status: 'running', connections: 12 }
  ]
}

const refreshAllData = async () => {
  refreshServerInfo()
  refreshCharts()
  loadProtocolStats()
  loadServiceStatus()
  // 手动触发实时数据刷新
  await refreshData()
}

// 监听实时数据变化，自动更新图表
watch(() => realtimeStore.serverStats, () => {
  refreshCharts()
  updateMode.value = getUpdateMode()
}, { deep: true })

onMounted(() => {
  refreshAllData()
  updateMode.value = getUpdateMode()
})

onUnmounted(() => {
  // 实时数据管理会自动清理，无需手动清理定时器
})
</script>

<style scoped>
.server-monitor {
  max-width: 100%;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.chart-container {
  text-align: center;
}

.chart-container h4 {
  margin: 0 0 10px 0;
  color: #303133;
}
</style>
