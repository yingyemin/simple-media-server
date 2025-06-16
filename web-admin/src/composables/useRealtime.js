/**
 * 实时数据管理组合式函数
 * 提供智能刷新策略，WebSocket优先，降级到轮询
 */

import { ref, onMounted, onUnmounted, watch } from 'vue'
import { useRealtimeStore } from '@/stores/realtime'
import websocketService from '@/services/websocket'
// 引入 API 服务
import serverAPI from '@/services/serverAPI'
import streamAPI from '@/services/streamAPI'
import clientAPI from '@/services/clientAPI'

export function useRealtime(options = {}) {
  const {
    enableWebSocket = true,
    enablePolling = true,
    pollingInterval = 30000, // 30秒轮询间隔
    maxPollingInterval = 60000, // 最大轮询间隔
    autoConnect = true,
    onError = null
  } = options

  const realtimeStore = useRealtimeStore()
  const isPolling = ref(false)
  const pollingTimer = ref(null)
  const currentPollingInterval = ref(pollingInterval)
  const retryCount = ref(0)
  const maxRetries = 3

  /**
   * 启动实时数据更新
   */
  const startRealtime = () => {
    if (enableWebSocket) {
      startWebSocket()
    } else if (enablePolling) {
      startPolling()
    }
  }

  /**
   * 停止实时数据更新
   */
  const stopRealtime = () => {
    stopWebSocket()
    stopPolling()
  }

  /**
   * 启动WebSocket连接
   */
  const startWebSocket = () => {
    try {
      websocketService.connect()
      
      // 监听连接状态变化
      const checkConnection = () => {
        if (!websocketService.isConnected() && enablePolling) {
          console.log('WebSocket disconnected, falling back to polling')
          startPolling()
        } else if (websocketService.isConnected() && isPolling.value) {
          console.log('WebSocket reconnected, stopping polling')
          stopPolling()
        }
      }

      // 定期检查连接状态
      const connectionCheckInterval = setInterval(checkConnection, 5000)
      
      // 清理函数
      onUnmounted(() => {
        clearInterval(connectionCheckInterval)
      })
      
    } catch (error) {
      console.error('Failed to start WebSocket:', error)
      if (onError) onError(error)
      
      // 降级到轮询
      if (enablePolling) {
        startPolling()
      }
    }
  }

  /**
   * 停止WebSocket连接
   */
  const stopWebSocket = () => {
    websocketService.close()
  }

  /**
   * 启动轮询
   */
  const startPolling = () => {
    if (isPolling.value) return

    isPolling.value = true
    console.log(`Starting polling with interval: ${currentPollingInterval.value}ms`)
    
    // 立即执行一次
    pollData()
    
    // 设置定时器
    pollingTimer.value = setInterval(() => {
      pollData()
    }, currentPollingInterval.value)
  }

  /**
   * 停止轮询
   */
  const stopPolling = () => {
    if (pollingTimer.value) {
      clearInterval(pollingTimer.value)
      pollingTimer.value = null
    }
    isPolling.value = false
    retryCount.value = 0
    currentPollingInterval.value = pollingInterval
  }

  /**
   * 轮询数据
   */
  const pollData = async () => {
    try {
      // 调用实际的API来获取数据
      await pollServerStats()
      await pollStreamList()
      await pollClientList()
      
      // 重置重试计数和轮询间隔
      retryCount.value = 0
      currentPollingInterval.value = pollingInterval
      
    } catch (error) {
      console.error('Polling failed:', error)
      handlePollingError(error)
    }
  }

  /**
   * 轮询服务器统计数据
   */
  const pollServerStats = async () => {
    try {
      // 调用实际的API
      const response = await serverAPI.getServerInfo()
      realtimeStore.updateServerStats(response.data)
    } catch (error) {
      console.error('Failed to poll server stats:', error)
      throw error
    }
  }

  /**
   * 轮询流列表
   */
  const pollStreamList = async () => {
    try {
      // 调用实际的API
      const response = await streamAPI.getStreamList()
      console.log('pollStreamList', response.data)
      realtimeStore.updateStreamList(response.data.sources)
    } catch (error) {
      console.error('Failed to poll stream list:', error)
      throw error
    }
  }

  /**
   * 轮询客户端列表
   */
  const pollClientList = async () => {
    try {
      // 调用实际的API
      const response = await clientAPI.getClientList()
      realtimeStore.updateClientList(response.data)
    } catch (error) {
      console.error('Failed to poll client list:', error)
      throw error
    }
  }

  /**
   * 处理轮询错误
   */
  const handlePollingError = (error) => {
    retryCount.value++
    
    if (retryCount.value >= maxRetries) {
      // 增加轮询间隔
      currentPollingInterval.value = Math.min(
        currentPollingInterval.value * 2,
        maxPollingInterval
      )
      retryCount.value = 0
      
      console.log(`Polling failed ${maxRetries} times, increasing interval to ${currentPollingInterval.value}ms`)
      
      // 重新设置定时器
      if (pollingTimer.value) {
        clearInterval(pollingTimer.value)
        pollingTimer.value = setInterval(() => {
          pollData()
        }, currentPollingInterval.value)
      }
    }
    
    if (onError) onError(error)
  }

  /**
   * 手动刷新数据
   */
  const refreshData = async () => {
    if (websocketService.isConnected()) {
      // 如果WebSocket连接正常，发送刷新请求
      websocketService.send({ type: 'refresh_all' })
    } else {
      // 否则执行轮询
      await pollData()
    }
  }

  /**
   * 获取当前更新模式
   */
  const getUpdateMode = () => {
    if (websocketService.isConnected()) {
      return 'websocket'
    } else if (isPolling.value) {
      return 'polling'
    } else {
      return 'none'
    }
  }

  // 监听WebSocket连接状态变化
  watch(
    () => realtimeStore.isConnected,
    (connected) => {
      if (connected && isPolling.value) {
        // WebSocket连接成功，停止轮询
        stopPolling()
      } else if (!connected && enablePolling && !isPolling.value) {
        // WebSocket断开，启动轮询
        startPolling()
      }
    }
  )

  // 自动启动
  if (autoConnect) {
    onMounted(() => {
      startRealtime()
    })
  }

  // 清理
  onUnmounted(() => {
    stopRealtime()
  })

  return {
    // State
    isPolling,
    currentPollingInterval,
    retryCount,
    
    // Methods
    startRealtime,
    stopRealtime,
    startWebSocket,
    stopWebSocket,
    startPolling,
    stopPolling,
    refreshData,
    getUpdateMode,
    
    // Store
    realtimeStore
  }
}
