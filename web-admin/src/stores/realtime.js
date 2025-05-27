/**
 * 实时数据状态管理
 * 管理WebSocket接收的实时数据，包括服务器状态、流列表、客户端列表等
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export const useRealtimeStore = defineStore('realtime', () => {
  // 连接状态
  const isConnected = ref(false)
  const lastUpdateTime = ref(null)
  
  // 服务器统计数据
  const serverStats = ref({
    activeStreams: 0,
    onlineClients: 0,
    totalBandwidth: 0,
    uptime: 0,
    cpuUsage: 0,
    memoryUsage: 0
  })
  
  // 流列表数据
  const streamList = ref([])
  
  // 客户端列表数据
  const clientList = ref([])
  
  // 历史数据（用于图表）
  const historyData = ref({
    serverStats: [],
    maxHistoryLength: 50 // 保留最近50个数据点
  })

  // 计算属性
  const connectionStatus = computed(() => {
    return isConnected.value ? 'connected' : 'disconnected'
  })

  const formattedBandwidth = computed(() => {
    const bandwidth = serverStats.value.totalBandwidth
    if (bandwidth < 1024) return `${bandwidth} B/s`
    if (bandwidth < 1024 * 1024) return `${(bandwidth / 1024).toFixed(1)} KB/s`
    if (bandwidth < 1024 * 1024 * 1024) return `${(bandwidth / (1024 * 1024)).toFixed(1)} MB/s`
    return `${(bandwidth / (1024 * 1024 * 1024)).toFixed(1)} GB/s`
  })

  const formattedUptime = computed(() => {
    const uptime = serverStats.value.uptime
    const hours = Math.floor(uptime / 3600)
    const minutes = Math.floor((uptime % 3600) / 60)
    const seconds = uptime % 60
    return `${hours}h ${minutes}m ${seconds}s`
  })

  // Actions
  const setConnectionStatus = (status) => {
    isConnected.value = status
    if (status) {
      lastUpdateTime.value = new Date()
    }
  }

  const updateServerStats = (data) => {
    // 更新服务器统计数据
    serverStats.value = {
      ...serverStats.value,
      ...data
    }
    
    // 添加到历史数据
    const timestamp = Date.now()
    historyData.value.serverStats.push({
      timestamp,
      ...data
    })
    
    // 保持历史数据长度限制
    if (historyData.value.serverStats.length > historyData.value.maxHistoryLength) {
      historyData.value.serverStats.shift()
    }
    
    lastUpdateTime.value = new Date()
    console.log('Server stats updated:', data)
  }

  const updateStreamList = (data) => {
    streamList.value = Array.isArray(data) ? data : []
    lastUpdateTime.value = new Date()
    console.log('Stream list updated:', data)
  }

  const updateClientList = (data) => {
    clientList.value = Array.isArray(data) ? data : []
    lastUpdateTime.value = new Date()
    console.log('Client list updated:', data)
  }

  // 获取特定协议的流数量
  const getStreamCountByProtocol = (protocol) => {
    return streamList.value.filter(stream => 
      stream.protocol && stream.protocol.toLowerCase() === protocol.toLowerCase()
    ).length
  }

  // 获取特定状态的流数量
  const getStreamCountByStatus = (status) => {
    return streamList.value.filter(stream => 
      stream.status === status
    ).length
  }

  // 获取协议分布数据（用于图表）
  const getProtocolDistribution = () => {
    const distribution = {}
    streamList.value.forEach(stream => {
      const protocol = stream.protocol || 'unknown'
      distribution[protocol] = (distribution[protocol] || 0) + 1
    })
    return distribution
  }

  // 获取客户端协议分布
  const getClientProtocolDistribution = () => {
    const distribution = {}
    clientList.value.forEach(client => {
      const protocol = client.protocol || 'unknown'
      distribution[protocol] = (distribution[protocol] || 0) + 1
    })
    return distribution
  }

  // 获取历史数据（用于图表）
  const getHistoryData = (field, limit = 20) => {
    const data = historyData.value.serverStats.slice(-limit)
    return {
      timestamps: data.map(item => new Date(item.timestamp)),
      values: data.map(item => item[field] || 0)
    }
  }

  // 获取CPU使用率历史数据
  const getCpuHistory = (limit = 20) => {
    return getHistoryData('cpuUsage', limit)
  }

  // 获取内存使用率历史数据
  const getMemoryHistory = (limit = 20) => {
    return getHistoryData('memoryUsage', limit)
  }

  // 获取带宽历史数据
  const getBandwidthHistory = (limit = 20) => {
    return getHistoryData('totalBandwidth', limit)
  }

  // 获取流数量历史数据
  const getStreamCountHistory = (limit = 20) => {
    return getHistoryData('activeStreams', limit)
  }

  // 清空历史数据
  const clearHistoryData = () => {
    historyData.value.serverStats = []
  }

  // 重置所有数据
  const resetAllData = () => {
    isConnected.value = false
    lastUpdateTime.value = null
    serverStats.value = {
      activeStreams: 0,
      onlineClients: 0,
      totalBandwidth: 0,
      uptime: 0,
      cpuUsage: 0,
      memoryUsage: 0
    }
    streamList.value = []
    clientList.value = []
    clearHistoryData()
  }

  // 模拟数据更新（用于测试）
  const simulateDataUpdate = () => {
    updateServerStats({
      activeStreams: Math.floor(Math.random() * 100),
      onlineClients: Math.floor(Math.random() * 500),
      totalBandwidth: Math.floor(Math.random() * 1000000000),
      uptime: Math.floor(Math.random() * 86400),
      cpuUsage: Math.random() * 100,
      memoryUsage: Math.random() * 100
    })
  }

  return {
    // State
    isConnected,
    lastUpdateTime,
    serverStats,
    streamList,
    clientList,
    historyData,
    
    // Computed
    connectionStatus,
    formattedBandwidth,
    formattedUptime,
    
    // Actions
    setConnectionStatus,
    updateServerStats,
    updateStreamList,
    updateClientList,
    getStreamCountByProtocol,
    getStreamCountByStatus,
    getProtocolDistribution,
    getClientProtocolDistribution,
    getHistoryData,
    getCpuHistory,
    getMemoryHistory,
    getBandwidthHistory,
    getStreamCountHistory,
    clearHistoryData,
    resetAllData,
    simulateDataUpdate
  }
})
