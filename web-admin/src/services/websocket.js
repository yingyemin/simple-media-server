/**
 * WebSocket服务 - 用于实时数据更新
 * 提供与MediaServer的WebSocket连接管理和实时数据处理
 */

import { ElMessage } from 'element-plus'
import { useRealtimeStore } from '@/stores/realtime'

class WebSocketService {
  constructor() {
    this.ws = null
    this.url = ''
    this.reconnectAttempts = 0
    this.maxReconnectAttempts = 5
    this.reconnectInterval = 3000
    this.heartbeatInterval = 30000
    this.heartbeatTimer = null
    this.isConnecting = false
    this.isManualClose = false
    this.messageHandlers = new Map()
    this.realtimeStore = null
  }

  /**
   * 连接WebSocket
   * @param {string} url WebSocket URL
   */
  connect(url = null) {
    if (this.isConnecting || (this.ws && this.ws.readyState === WebSocket.CONNECTING)) {
      console.log('WebSocket is already connecting')
      return
    }

    // 使用传入的URL或默认URL
    this.url = url || this.getWebSocketUrl()
    this.isConnecting = true
    this.isManualClose = false

    try {
      console.log('Connecting to WebSocket:', this.url)
      this.ws = new WebSocket(this.url)
      this.setupEventHandlers()
    } catch (error) {
      console.error('WebSocket connection error:', error)
      this.isConnecting = false
      this.handleReconnect()
    }
  }

  /**
   * 获取WebSocket URL
   */
  getWebSocketUrl() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
    const host = window.location.host
    // 使用我们创建的Admin WebSocket端点
    return `${protocol}//${host}/api/v1/admin/websocket/connect`
  }

  /**
   * 设置WebSocket事件处理器
   */
  setupEventHandlers() {
    if (!this.ws) return

    this.ws.onopen = (event) => {
      console.log('WebSocket connected:', event)
      this.isConnecting = false
      this.reconnectAttempts = 0
      this.startHeartbeat()
      
      // 初始化realtime store
      if (!this.realtimeStore) {
        this.realtimeStore = useRealtimeStore()
      }
      this.realtimeStore.setConnectionStatus(true)
      
      ElMessage.success('实时连接已建立')
    }

    this.ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data)
        this.handleMessage(data)
      } catch (error) {
        console.error('Failed to parse WebSocket message:', error, event.data)
      }
    }

    this.ws.onclose = (event) => {
      console.log('WebSocket closed:', event.code, event.reason)
      this.isConnecting = false
      this.stopHeartbeat()
      
      if (this.realtimeStore) {
        this.realtimeStore.setConnectionStatus(false)
      }

      if (!this.isManualClose) {
        ElMessage.warning('实时连接已断开，正在尝试重连...')
        this.handleReconnect()
      }
    }

    this.ws.onerror = (error) => {
      console.error('WebSocket error:', error)
      this.isConnecting = false
      
      if (this.realtimeStore) {
        this.realtimeStore.setConnectionStatus(false)
      }
    }
  }

  /**
   * 处理接收到的消息
   * @param {Object} data 消息数据
   */
  handleMessage(data) {
    console.log('Received WebSocket message:', data)
    
    if (!this.realtimeStore) {
      this.realtimeStore = useRealtimeStore()
    }

    // 根据消息类型处理数据
    switch (data.type) {
      case 'welcome':
        console.log('Welcome message:', data.message)
        break
        
      case 'server_stats':
        this.realtimeStore.updateServerStats(data.data)
        break
        
      case 'stream_list':
        this.realtimeStore.updateStreamList(data.data)
        break
        
      case 'client_list':
        this.realtimeStore.updateClientList(data.data)
        break
        
      default:
        console.log('Unknown message type:', data.type)
    }

    // 调用注册的消息处理器
    if (this.messageHandlers.has(data.type)) {
      const handler = this.messageHandlers.get(data.type)
      handler(data)
    }
  }

  /**
   * 注册消息处理器
   * @param {string} type 消息类型
   * @param {Function} handler 处理函数
   */
  onMessage(type, handler) {
    this.messageHandlers.set(type, handler)
  }

  /**
   * 移除消息处理器
   * @param {string} type 消息类型
   */
  offMessage(type) {
    this.messageHandlers.delete(type)
  }

  /**
   * 发送消息
   * @param {Object} data 要发送的数据
   */
  send(data) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(data))
    } else {
      console.warn('WebSocket is not connected')
    }
  }

  /**
   * 开始心跳
   */
  startHeartbeat() {
    this.stopHeartbeat()
    this.heartbeatTimer = setInterval(() => {
      this.send({ type: 'ping', timestamp: Date.now() })
    }, this.heartbeatInterval)
  }

  /**
   * 停止心跳
   */
  stopHeartbeat() {
    if (this.heartbeatTimer) {
      clearInterval(this.heartbeatTimer)
      this.heartbeatTimer = null
    }
  }

  /**
   * 处理重连
   */
  handleReconnect() {
    if (this.isManualClose || this.reconnectAttempts >= this.maxReconnectAttempts) {
      console.log('Max reconnect attempts reached or manual close')
      return
    }

    this.reconnectAttempts++
    console.log(`Attempting to reconnect (${this.reconnectAttempts}/${this.maxReconnectAttempts})`)
    
    setTimeout(() => {
      this.connect()
    }, this.reconnectInterval)
  }

  /**
   * 手动关闭连接
   */
  close() {
    this.isManualClose = true
    this.stopHeartbeat()
    
    if (this.ws) {
      this.ws.close()
      this.ws = null
    }
    
    if (this.realtimeStore) {
      this.realtimeStore.setConnectionStatus(false)
    }
    
    console.log('WebSocket manually closed')
  }

  /**
   * 获取连接状态
   */
  getConnectionStatus() {
    if (!this.ws) return 'CLOSED'
    
    switch (this.ws.readyState) {
      case WebSocket.CONNECTING:
        return 'CONNECTING'
      case WebSocket.OPEN:
        return 'OPEN'
      case WebSocket.CLOSING:
        return 'CLOSING'
      case WebSocket.CLOSED:
        return 'CLOSED'
      default:
        return 'UNKNOWN'
    }
  }

  /**
   * 检查是否已连接
   */
  isConnected() {
    return this.ws && this.ws.readyState === WebSocket.OPEN
  }
}

// 创建单例实例
const websocketService = new WebSocketService()

export default websocketService
