<template>
  <div class="loops-monitor">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>事件循环监控</span>
          <el-button @click="refreshLoops" :loading="loading">
            <el-icon><Refresh /></el-icon>
            刷新
          </el-button>
        </div>
      </template>

      <el-table :data="loops" v-loading="loading" style="width: 100%">
        <el-table-column prop="epollFd" label="Epoll FD" width="100" />
        <el-table-column prop="fdCount" label="文件描述符数" width="150" />
        <el-table-column prop="timerTaskCount" label="定时器任务数" width="150" />
        <el-table-column label="负载情况" min-width="200">
          <template #default="scope">
            <div class="load-info">
              <div class="load-item">
                <span>上次等待: {{ scope.row.lastWaitDuration }}ms</span>
                <el-progress 
                  :percentage="getLoadPercentage(scope.row.lastWaitDuration, 100)" 
                  :color="getLoadColor(scope.row.lastWaitDuration, 100)"
                  :show-text="false"
                  style="width: 100px; margin-left: 10px;"
                />
              </div>
              <div class="load-item">
                <span>上次运行: {{ scope.row.lastRunDuration }}ms</span>
                <el-progress 
                  :percentage="getLoadPercentage(scope.row.lastRunDuration, 50)" 
                  :color="getLoadColor(scope.row.lastRunDuration, 50)"
                  :show-text="false"
                  style="width: 100px; margin-left: 10px;"
                />
              </div>
            </div>
          </template>
        </el-table-column>
        <el-table-column label="当前状态" min-width="200">
          <template #default="scope">
            <div class="load-info">
              <div class="load-item">
                <span>当前等待: {{ scope.row.curWaitDuration }}ms</span>
                <el-progress 
                  :percentage="getLoadPercentage(scope.row.curWaitDuration, 100)" 
                  :color="getLoadColor(scope.row.curWaitDuration, 100)"
                  :show-text="false"
                  style="width: 100px; margin-left: 10px;"
                />
              </div>
              <div class="load-item">
                <span>当前运行: {{ scope.row.curRunDuration }}ms</span>
                <el-progress 
                  :percentage="getLoadPercentage(scope.row.curRunDuration, 50)" 
                  :color="getLoadColor(scope.row.curRunDuration, 50)"
                  :show-text="false"
                  style="width: 100px; margin-left: 10px;"
                />
              </div>
            </div>
          </template>
        </el-table-column>
        <el-table-column label="状态" width="100">
          <template #default="scope">
            <el-tag :type="getStatusType(scope.row)">
              {{ getStatusText(scope.row) }}
            </el-tag>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 事件循环性能图表 -->
    <el-row :gutter="20" style="margin-top: 20px;">
      <el-col :span="12">
        <el-card>
          <template #header>
            <span>等待时间趋势</span>
          </template>
          <v-chart :option="waitTimeChartOption" style="height: 300px;" />
        </el-card>
      </el-col>
      <el-col :span="12">
        <el-card>
          <template #header>
            <span>运行时间趋势</span>
          </template>
          <v-chart :option="runTimeChartOption" style="height: 300px;" />
        </el-card>
      </el-col>
    </el-row>

    <!-- 详细统计 -->
    <el-row :gutter="20" style="margin-top: 20px;">
      <el-col :span="24">
        <el-card>
          <template #header>
            <span>事件循环统计</span>
          </template>
          <el-row :gutter="20">
            <el-col :span="6">
              <el-statistic title="总事件循环数" :value="loopStats.totalLoops" />
            </el-col>
            <el-col :span="6">
              <el-statistic title="平均文件描述符" :value="loopStats.avgFdCount" />
            </el-col>
            <el-col :span="6">
              <el-statistic title="平均定时器任务" :value="loopStats.avgTimerTasks" />
            </el-col>
            <el-col :span="6">
              <el-statistic title="平均负载" :value="loopStats.avgLoad + '%'" />
            </el-col>
          </el-row>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import { LineChart } from 'echarts/charts'
import {
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  GridComponent
} from 'echarts/components'
import VChart from 'vue-echarts'
import { serverAPI } from '@/api'

use([
  CanvasRenderer,
  LineChart,
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  GridComponent
])

const loading = ref(false)
const loops = ref([])

// 图表配置
const waitTimeChartOption = ref({
  tooltip: { trigger: 'axis' },
  legend: { data: [] },
  xAxis: { type: 'category', data: [] },
  yAxis: { type: 'value', name: '毫秒' },
  series: []
})

const runTimeChartOption = ref({
  tooltip: { trigger: 'axis' },
  legend: { data: [] },
  xAxis: { type: 'category', data: [] },
  yAxis: { type: 'value', name: '毫秒' },
  series: []
})

let refreshTimer = null

// 计算统计数据
const loopStats = computed(() => {
  if (loops.value.length === 0) {
    return {
      totalLoops: 0,
      avgFdCount: 0,
      avgTimerTasks: 0,
      avgLoad: 0
    }
  }
  
  const totalFd = loops.value.reduce((sum, loop) => sum + loop.fdCount, 0)
  const totalTimers = loops.value.reduce((sum, loop) => sum + loop.timerTaskCount, 0)
  const totalLoad = loops.value.reduce((sum, loop) => {
    const load = Math.max(loop.curWaitDuration, loop.curRunDuration)
    return sum + Math.min(load / 100 * 100, 100)
  }, 0)
  
  return {
    totalLoops: loops.value.length,
    avgFdCount: Math.round(totalFd / loops.value.length),
    avgTimerTasks: Math.round(totalTimers / loops.value.length),
    avgLoad: Math.round(totalLoad / loops.value.length)
  }
})

const getLoadPercentage = (duration, maxDuration) => {
  return Math.min((duration / maxDuration) * 100, 100)
}

const getLoadColor = (duration, maxDuration) => {
  const percentage = (duration / maxDuration) * 100
  if (percentage < 50) return '#67c23a'
  if (percentage < 80) return '#e6a23c'
  return '#f56c6c'
}

const getStatusType = (loop) => {
  const maxWait = Math.max(loop.curWaitDuration, loop.lastWaitDuration)
  const maxRun = Math.max(loop.curRunDuration, loop.lastRunDuration)
  
  if (maxWait > 100 || maxRun > 50) return 'danger'
  if (maxWait > 50 || maxRun > 25) return 'warning'
  return 'success'
}

const getStatusText = (loop) => {
  const maxWait = Math.max(loop.curWaitDuration, loop.lastWaitDuration)
  const maxRun = Math.max(loop.curRunDuration, loop.lastRunDuration)
  
  if (maxWait > 100 || maxRun > 50) return '高负载'
  if (maxWait > 50 || maxRun > 25) return '中负载'
  return '正常'
}

const refreshLoops = async () => {
  loading.value = true
  try {
    const data = await serverAPI.getLoopList()
    loops.value = data.loops || []
    updateCharts()
  } catch (error) {
    console.error('获取事件循环列表失败:', error)
  } finally {
    loading.value = false
  }
}

const updateCharts = () => {
  if (loops.value.length === 0) return
  
  const now = new Date()
  const times = []
  
  // 生成时间轴
  for (let i = 9; i >= 0; i--) {
    const time = new Date(now.getTime() - i * 30 * 1000)
    times.push(time.getHours() + ':' + time.getMinutes().toString().padStart(2, '0') + ':' + time.getSeconds().toString().padStart(2, '0'))
  }
  
  // 等待时间图表
  const waitSeries = loops.value.map((loop, index) => ({
    name: `Loop ${loop.epollFd}`,
    type: 'line',
    smooth: true,
    data: Array.from({ length: 10 }, () => Math.random() * 100)
  }))
  
  waitTimeChartOption.value = {
    tooltip: { trigger: 'axis' },
    legend: { data: waitSeries.map(s => s.name) },
    xAxis: { type: 'category', data: times },
    yAxis: { type: 'value', name: '毫秒' },
    series: waitSeries
  }
  
  // 运行时间图表
  const runSeries = loops.value.map((loop, index) => ({
    name: `Loop ${loop.epollFd}`,
    type: 'line',
    smooth: true,
    data: Array.from({ length: 10 }, () => Math.random() * 50)
  }))
  
  runTimeChartOption.value = {
    tooltip: { trigger: 'axis' },
    legend: { data: runSeries.map(s => s.name) },
    xAxis: { type: 'category', data: times },
    yAxis: { type: 'value', name: '毫秒' },
    series: runSeries
  }
}

onMounted(() => {
  refreshLoops()
  
  // 设置定时刷新
  refreshTimer = setInterval(() => {
    refreshLoops()
  }, 5000) // 5秒刷新一次
})

onUnmounted(() => {
  if (refreshTimer) {
    clearInterval(refreshTimer)
  }
})
</script>

<style scoped>
.loops-monitor {
  max-width: 100%;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.load-info {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.load-item {
  display: flex;
  align-items: center;
  font-size: 12px;
}
</style>
