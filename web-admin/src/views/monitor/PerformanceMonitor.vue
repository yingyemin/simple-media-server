<template>
  <div class="performance-monitor">
    <el-card>
      <template #header>
        <span>性能统计</span>
      </template>
      
      <!-- 性能概览 -->
      <el-row :gutter="20" style="margin-bottom: 20px;">
        <el-col :span="6">
          <el-statistic title="平均响应时间" :value="performanceStats.avgResponseTime" suffix="ms" />
        </el-col>
        <el-col :span="6">
          <el-statistic title="请求成功率" :value="performanceStats.successRate" suffix="%" />
        </el-col>
        <el-col :span="6">
          <el-statistic title="并发连接数" :value="performanceStats.concurrentConnections" />
        </el-col>
        <el-col :span="6">
          <el-statistic title="错误率" :value="performanceStats.errorRate" suffix="%" />
        </el-col>
      </el-row>

      <!-- 性能图表 -->
      <el-row :gutter="20">
        <el-col :span="12">
          <div class="chart-container">
            <h4>响应时间趋势</h4>
            <v-chart :option="responseTimeChartOption" style="height: 300px;" />
          </div>
        </el-col>
        <el-col :span="12">
          <div class="chart-container">
            <h4>吞吐量统计</h4>
            <v-chart :option="throughputChartOption" style="height: 300px;" />
          </div>
        </el-col>
      </el-row>

      <!-- API性能统计 -->
      <div style="margin-top: 20px;">
        <h4>API性能统计</h4>
        <el-table :data="apiStats" style="width: 100%">
          <el-table-column prop="api" label="API接口" min-width="200" />
          <el-table-column prop="requestCount" label="请求次数" width="120" />
          <el-table-column prop="avgResponseTime" label="平均响应时间" width="150">
            <template #default="scope">
              <span>{{ scope.row.avgResponseTime }}ms</span>
            </template>
          </el-table-column>
          <el-table-column prop="successRate" label="成功率" width="100">
            <template #default="scope">
              <el-progress :percentage="scope.row.successRate" :color="getSuccessRateColor(scope.row.successRate)" />
            </template>
          </el-table-column>
          <el-table-column prop="errorCount" label="错误次数" width="120" />
          <el-table-column label="状态" width="100">
            <template #default="scope">
              <el-tag :type="getApiStatusType(scope.row)">
                {{ getApiStatusText(scope.row) }}
              </el-tag>
            </template>
          </el-table-column>
        </el-table>
      </div>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { use } from 'echarts/core'
import { CanvasRenderer } from 'echarts/renderers'
import { LineChart, BarChart } from 'echarts/charts'
import {
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  GridComponent
} from 'echarts/components'
import VChart from 'vue-echarts'

use([
  CanvasRenderer,
  LineChart,
  BarChart,
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  GridComponent
])

const performanceStats = ref({
  avgResponseTime: 0,
  successRate: 0,
  concurrentConnections: 0,
  errorRate: 0
})

const apiStats = ref([])

// 图表配置
const responseTimeChartOption = ref({
  tooltip: { trigger: 'axis' },
  xAxis: { type: 'category', data: [] },
  yAxis: { type: 'value', name: '毫秒' },
  series: [{
    name: '响应时间',
    type: 'line',
    smooth: true,
    data: []
  }]
})

const throughputChartOption = ref({
  tooltip: { trigger: 'axis' },
  xAxis: { type: 'category', data: [] },
  yAxis: { type: 'value', name: '请求/秒' },
  series: [{
    name: '吞吐量',
    type: 'bar',
    data: []
  }]
})

let refreshTimer = null

const getSuccessRateColor = (rate) => {
  if (rate >= 95) return '#67c23a'
  if (rate >= 90) return '#e6a23c'
  return '#f56c6c'
}

const getApiStatusType = (api) => {
  if (api.successRate >= 95 && api.avgResponseTime < 100) return 'success'
  if (api.successRate >= 90 && api.avgResponseTime < 200) return 'warning'
  return 'danger'
}

const getApiStatusText = (api) => {
  if (api.successRate >= 95 && api.avgResponseTime < 100) return '优秀'
  if (api.successRate >= 90 && api.avgResponseTime < 200) return '良好'
  return '需优化'
}

const loadPerformanceData = () => {
  // 模拟性能数据
  performanceStats.value = {
    avgResponseTime: Math.floor(Math.random() * 200) + 50,
    successRate: Math.floor(Math.random() * 10) + 90,
    concurrentConnections: Math.floor(Math.random() * 1000) + 100,
    errorRate: Math.floor(Math.random() * 5) + 1
  }

  // 模拟API统计数据
  apiStats.value = [
    {
      api: '/api/v1/getSourceList',
      requestCount: Math.floor(Math.random() * 10000) + 1000,
      avgResponseTime: Math.floor(Math.random() * 100) + 20,
      successRate: Math.floor(Math.random() * 10) + 90,
      errorCount: Math.floor(Math.random() * 100)
    },
    {
      api: '/api/v1/config',
      requestCount: Math.floor(Math.random() * 5000) + 500,
      avgResponseTime: Math.floor(Math.random() * 150) + 30,
      successRate: Math.floor(Math.random() * 15) + 85,
      errorCount: Math.floor(Math.random() * 50)
    },
    {
      api: '/api/v1/rtsp/play/start',
      requestCount: Math.floor(Math.random() * 3000) + 300,
      avgResponseTime: Math.floor(Math.random() * 200) + 50,
      successRate: Math.floor(Math.random() * 20) + 80,
      errorCount: Math.floor(Math.random() * 80)
    },
    {
      api: '/api/v1/rtmp/publish/start',
      requestCount: Math.floor(Math.random() * 2000) + 200,
      avgResponseTime: Math.floor(Math.random() * 180) + 40,
      successRate: Math.floor(Math.random() * 12) + 88,
      errorCount: Math.floor(Math.random() * 60)
    }
  ]

  updateCharts()
}

const updateCharts = () => {
  const now = new Date()
  const times = []
  const responseTimeData = []
  const throughputData = []
  
  // 生成最近10分钟的数据
  for (let i = 9; i >= 0; i--) {
    const time = new Date(now.getTime() - i * 60 * 1000)
    times.push(time.getHours() + ':' + time.getMinutes().toString().padStart(2, '0'))
    responseTimeData.push(Math.floor(Math.random() * 200) + 50)
    throughputData.push(Math.floor(Math.random() * 1000) + 100)
  }
  
  responseTimeChartOption.value.xAxis.data = times
  responseTimeChartOption.value.series[0].data = responseTimeData
  
  throughputChartOption.value.xAxis.data = times
  throughputChartOption.value.series[0].data = throughputData
}

onMounted(() => {
  loadPerformanceData()
  
  // 设置定时刷新
  refreshTimer = setInterval(() => {
    loadPerformanceData()
  }, 10000) // 10秒刷新一次
})

onUnmounted(() => {
  if (refreshTimer) {
    clearInterval(refreshTimer)
  }
})
</script>

<style scoped>
.performance-monitor {
  max-width: 100%;
}

.chart-container {
  text-align: center;
}

.chart-container h4 {
  margin: 0 0 10px 0;
  color: #303133;
}
</style>
