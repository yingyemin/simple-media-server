<template>
  <div class="dashboard">
    <!-- 连接状态指示器 -->
    <el-alert
      v-if="connectionStatus === 'disconnected'"
      title="实时连接已断开"
      type="warning"
      :description="`当前使用${updateMode === 'polling' ? '轮询模式' : '离线模式'}更新数据`"
      show-icon
      :closable="false"
      style="margin-bottom: 20px;"
    />
    <el-alert
      v-else-if="connectionStatus === 'connected'"
      title="实时连接正常"
      type="success"
      description="数据将实时更新"
      show-icon
      :closable="false"
      style="margin-bottom: 20px;"
    />

    <!-- 统计卡片 -->
    <el-row :gutter="20" class="stats-row">
      <el-col :span="6">
        <el-card class="stats-card">
          <div class="stats-content">
            <div class="stats-icon streams">
              <el-icon><VideoPlay /></el-icon>
            </div>
            <div class="stats-info">
              <div class="stats-number">{{ serverStats.streamCount || 0 }}</div>
              <div class="stats-label">活跃流数量</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card class="stats-card">
          <div class="stats-content">
            <div class="stats-icon clients">
              <el-icon><User /></el-icon>
            </div>
            <div class="stats-info">
              <div class="stats-number">{{ serverStats.clientCount || 0 }}</div>
              <div class="stats-label">在线客户端</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card class="stats-card">
          <div class="stats-content">
            <div class="stats-icon bandwidth">
              <el-icon><Connection /></el-icon>
            </div>
            <div class="stats-info">
              <div class="stats-number">{{ formatBandwidth(serverStats.bandwidth) }}</div>
              <div class="stats-label">总带宽</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card class="stats-card">
          <div class="stats-content">
            <div class="stats-icon uptime">
              <el-icon><Timer /></el-icon>
            </div>
            <div class="stats-info">
              <div class="stats-number">{{ formatUptime(serverStats.uptime) }}</div>
              <div class="stats-label">运行时间</div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 图表区域 -->
    <el-row :gutter="20" class="charts-row">
      <el-col :span="12">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>流量统计</span>
              <el-button type="text" @click="refreshTrafficChart">刷新</el-button>
            </div>
          </template>
          <div class="chart-container">
            <v-chart :option="trafficChartOption" style="height: 300px;" />
          </div>
        </el-card>
      </el-col>
      <el-col :span="12">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>协议分布</span>
              <el-button type="text" @click="refreshProtocolChart">刷新</el-button>
            </div>
          </template>
          <div class="chart-container">
            <v-chart :option="protocolChartOption" style="height: 300px;" />
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 系统信息 -->
    <el-row :gutter="20" class="info-row">
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
            <el-descriptions-item label="CPU使用率">{{ serverInfo.cpuUsage || 'N/A' }}%</el-descriptions-item>
            <el-descriptions-item label="内存使用">{{ formatMemory(serverInfo.memoryUsage) }}</el-descriptions-item>
            <el-descriptions-item label="磁盘使用">{{ serverInfo.diskUsage || 'N/A' }}%</el-descriptions-item>
          </el-descriptions>
        </el-card>
      </el-col>
      <el-col :span="12">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>最近活动流</span>
              <el-button type="text" @click="refreshRecentStreams">刷新</el-button>
            </div>
          </template>
          <el-table :data="recentStreams" style="width: 100%" max-height="300">
            <el-table-column prop="path" label="流路径" width="200" />
            <el-table-column prop="protocol" label="协议" width="80" />
            <el-table-column prop="totalPlayerCount" label="观看数" width="80" />
            <el-table-column prop="status" label="状态" width="80">
              <template #default="scope">
                <el-tag :type="scope.row.totalPlayerCount > 0 ? 'success' : 'danger'">
                  {{ scope.row.totalPlayerCount > 0 ? '活跃' : '停止' }}
                </el-tag>
              </template>
            </el-table-column>
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
import { LineChart, PieChart } from 'echarts/charts'
import {
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  GridComponent
} from 'echarts/components'
import VChart from 'vue-echarts'
import { serverAPI, streamAPI } from '@/api'
import { useRealtime } from '@/composables/useRealtime'
import { useRealtimeStore } from '@/stores/realtime'

use([
  CanvasRenderer,
  LineChart,
  PieChart,
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  GridComponent
])

// 使用实时数据管理
const { realtimeStore, getUpdateMode, refreshData } = useRealtime({
  enableWebSocket: true,
  enablePolling: true,
  pollingInterval: 30000,
  autoConnect: true
})

// 响应式数据 - 现在从realtime store获取
const serverStats = computed(() => ({
  streamCount: realtimeStore.serverStats.activeStreams,
  clientCount: realtimeStore.serverStats.onlineClients,
  bandwidth: realtimeStore.serverStats.totalBandwidth,
  uptime: realtimeStore.serverStats.uptime
}))

const serverInfo = ref({
  version: '',
  localIp: '',
  startTime: '',
  cpuUsage: 0,
  memoryUsage: 0,
  diskUsage: 0
})

const recentStreams = computed(() => realtimeStore.streamList.slice(0, 10))

// 连接状态指示
const connectionStatus = computed(() => realtimeStore.connectionStatus)
const updateMode = ref('none')

// 图表配置
const trafficChartOption = ref({
  title: {
    text: '流量趋势'
  },
  tooltip: {
    trigger: 'axis'
  },
  xAxis: {
    type: 'category',
    data: []
  },
  yAxis: {
    type: 'value',
    name: 'Mbps'
  },
  series: [{
    data: [],
    type: 'line',
    smooth: true,
    areaStyle: {}
  }]
})

const protocolChartOption = ref({
  title: {
    text: '协议分布',
    left: 'center'
  },
  tooltip: {
    trigger: 'item'
  },
  legend: {
    orient: 'vertical',
    left: 'left'
  },
  series: [{
    name: '协议',
    type: 'pie',
    radius: '50%',
    data: [],
    emphasis: {
      itemStyle: {
        shadowBlur: 10,
        shadowOffsetX: 0,
        shadowColor: 'rgba(0, 0, 0, 0.5)'
      }
    }
  }]
})

// 定时器
let refreshTimer = null

// 方法
const formatBandwidth = (bandwidth) => {
  if (!bandwidth) return '0 Mbps'
  return (bandwidth / 1024 / 1024).toFixed(2) + ' Mbps'
}

const formatUptime = (uptime) => {
  if (!uptime) return '0天'
  const days = Math.floor(uptime / 86400)
  const hours = Math.floor((uptime % 86400) / 3600)
  const m = Math.floor(((uptime % 86400) % 3600) / 60)
  return `${days}天${hours}小时${m}分钟`
}

const formatTime = (timestamp) => {
  if (!timestamp) return 'N/A'
  return new Date(timestamp).toLocaleString()
}

const formatMemory = (memory) => {
  if (!memory) return 'N/A'
  return (memory / 1024 / 1024).toFixed(2) + ' MB'
}

const loadServerInfo = async () => {
  try {
    const [versionData, serverData] = await Promise.all([
      serverAPI.getVersion(),
      serverAPI.getServerInfo()
    ])

    serverInfo.value = {
      version: versionData.version,
      ...serverData
    }
  } catch (error) {
    console.error('加载服务器信息失败:', error)
  }
}

const loadStreamStats = async () => {
  try {
    const data = await streamAPI.getSourceList()
    serverStats.value.streamCount = data.count || 0

    // 更新最近活动流
    if (data.sources) {
      recentStreams.value = data.sources.slice(0, 10).map(stream => ({
        path: stream.path,
        protocol: stream.protocol,
        playerCount: stream.totalPlayerCount || 0,
        status: stream.totalPlayerCount > 0 ? 'active' : 'inactive'
      }))
    }

    // 更新协议分布图表
    updateProtocolChart(data.sources || [])
  } catch (error) {
    console.error('加载流统计失败:', error)
  }
}

const updateProtocolChart = () => {
  const distribution = realtimeStore.getProtocolDistribution()
  protocolChartOption.value.series[0].data = Object.entries(distribution).map(([name, value]) => ({
    name,
    value
  }))
}

const refreshTrafficChart = () => {
  // 使用实时历史数据
  const bandwidthHistory = realtimeStore.getBandwidthHistory(24)

  if (bandwidthHistory.timestamps.length > 0) {
    trafficChartOption.value.xAxis.data = bandwidthHistory.timestamps.map(time =>
      time.getHours().toString().padStart(2, '0') + ':' +
      time.getMinutes().toString().padStart(2, '0')
    )
    trafficChartOption.value.series[0].data = bandwidthHistory.values.map(value =>
      (value / 1024 / 1024).toFixed(2) // 转换为Mbps
    )
  } else {
    // 如果没有历史数据，使用模拟数据
    const now = new Date()
    const times = []
    const values = []

    for (let i = 23; i >= 0; i--) {
      const time = new Date(now.getTime() - i * 60 * 60 * 1000)
      times.push(time.getHours() + ':00')
      values.push(Math.random() * 100)
    }

    trafficChartOption.value.xAxis.data = times
    trafficChartOption.value.series[0].data = values
  }
}

const refreshProtocolChart = () => {
  updateProtocolChart()
}

const refreshServerInfo = () => {
  loadServerInfo()
}

const refreshRecentStreams = () => {
  // 数据现在从realtime store获取，无需手动刷新
}

const refreshAllData = async () => {
  loadServerInfo()
  refreshTrafficChart()
  updateProtocolChart()
  // 手动触发实时数据刷新
  await refreshData()
}

// 监听实时数据变化，自动更新图表
watch(() => realtimeStore.streamList, () => {
  updateProtocolChart()
}, { deep: true })

watch(() => realtimeStore.serverStats, () => {
  refreshTrafficChart()
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
.dashboard {
  padding: 0;
}

.stats-row {
  margin-bottom: 20px;
}

.stats-card {
  height: 120px;
}

.stats-content {
  display: flex;
  align-items: center;
  height: 100%;
}

.stats-icon {
  width: 60px;
  height: 60px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  margin-right: 20px;
  font-size: 24px;
  color: white;
}

.stats-icon.streams {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.stats-icon.clients {
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
}

.stats-icon.bandwidth {
  background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
}

.stats-icon.uptime {
  background: linear-gradient(135deg, #43e97b 0%, #38f9d7 100%);
}

.stats-info {
  flex: 1;
}

.stats-number {
  font-size: 28px;
  font-weight: bold;
  color: #303133;
  line-height: 1;
}

.stats-label {
  font-size: 14px;
  color: #909399;
  margin-top: 5px;
}

.charts-row {
  margin-bottom: 20px;
}

.info-row {
  margin-bottom: 20px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.chart-container {
  width: 100%;
}
</style>
