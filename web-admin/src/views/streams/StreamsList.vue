<template>
  <div class="streams-list">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>流列表管理</span>
          <div>
            <el-button
              type="warning"
              :disabled="selectedStreams.length === 0"
              @click="showBatchOperations"
            >
              <el-icon><Operation /></el-icon>
              批量操作 ({{ selectedStreams.length }})
            </el-button>
            <el-button @click="refreshStreams" :loading="loading">
              <el-icon><Refresh /></el-icon>
              刷新
            </el-button>
            <el-button type="primary" @click="showCreateDialog">
              <el-icon><Plus /></el-icon>
              创建流
            </el-button>
          </div>
        </div>
      </template>

      <!-- 搜索和过滤 -->
      <div class="filter-section">
        <el-row :gutter="20">
          <el-col :span="6">
            <el-input
              v-model="searchText"
              placeholder="搜索流路径"
              @input="handleSearch"
              clearable
            >
              <template #prefix>
                <el-icon><Search /></el-icon>
              </template>
            </el-input>
          </el-col>
          <el-col :span="4">
            <el-select v-model="protocolFilter" placeholder="协议筛选" @change="handleFilter" clearable>
              <el-option label="全部" value="" />
              <el-option label="RTSP" value="rtsp" />
              <el-option label="RTMP" value="rtmp" />
              <el-option label="WebRTC" value="webrtc" />
              <el-option label="SRT" value="srt" />
              <el-option label="GB28181" value="gb28181" />
              <el-option label="JT1078" value="jt1078" />
            </el-select>
          </el-col>
          <el-col :span="4">
            <el-select v-model="statusFilter" placeholder="状态筛选" @change="handleFilter" clearable>
              <el-option label="全部" value="" />
              <el-option label="活跃" value="active" />
              <el-option label="空闲" value="idle" />
            </el-select>
          </el-col>
          <el-col :span="4">
            <el-select v-model="typeFilter" placeholder="类型筛选" @change="handleFilter" clearable>
              <el-option label="全部" value="" />
              <el-option label="推流" value="push" />
              <el-option label="拉流" value="pull" />
            </el-select>
          </el-col>
        </el-row>
      </div>

      <!-- 流列表表格 -->
      <el-table
        :data="filteredStreams"
        v-loading="loading"
        style="width: 100%"
        @selection-change="handleSelectionChange"
      >
        <el-table-column type="selection" width="55" />
        <el-table-column prop="path" label="流路径" min-width="200" show-overflow-tooltip />
        <el-table-column prop="protocol" label="协议" width="100">
          <template #default="scope">
            <el-tag :type="getProtocolTagType(scope.row.protocol)">
              {{ scope.row.protocol.toUpperCase() }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="type" label="类型" width="80">
          <template #default="scope">
            <el-tag :type="scope.row.type === 'push' ? 'success' : 'info'">
              {{ scope.row.type === 'push' ? '推流' : '拉流' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="vhost" label="虚拟主机" width="120" show-overflow-tooltip />
        <el-table-column label="观看数" width="100">
          <template #default="scope">
            <span class="player-count">{{ getTotalPlayerCount(scope.row) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="状态" width="100">
          <template #default="scope">
            <el-tag :type="getStatusTagType(scope.row)">
              {{ getStreamStatus(scope.row) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="码率" width="120">
          <template #default="scope">
            <span>{{ formatBitrate(scope.row.bitrate) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="创建时间" width="160">
          <template #default="scope">
            <span>{{ formatTime(scope.row.createTime) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="260" fixed="right">
          <template #default="scope">
            <el-button type="primary" size="small" @click="showStreamDetail(scope.row)">
              详情
            </el-button>
            <el-button type="warning" size="small" @click="showClientList(scope.row)">
              客户端
            </el-button>
            <!-- 新增播放按钮 -->
            <el-button type="success" size="small" @click="showPlayDialog(scope.row)">
              播放
            </el-button>
            <el-button type="danger" size="small" @click="closeStream(scope.row)">
              关闭
            </el-button>
          </template>
        </el-table-column>
      </el-table>

      <!-- 批量操作 -->
      <div class="batch-actions" v-if="selectedStreams.length > 0">
        <el-alert
          :title="`已选择 ${selectedStreams.length} 个流`"
          type="info"
          show-icon
          :closable="false"
        />
        <div class="batch-buttons">
          <el-button type="danger" @click="batchCloseStreams">批量关闭</el-button>
        </div>
      </div>

      <!-- 分页 -->
      <div class="pagination">
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :page-sizes="[10, 20, 50, 100]"
          :total="totalStreams"
          layout="total, sizes, prev, pager, next, jumper"
          @size-change="handleSizeChange"
          @current-change="handleCurrentChange"
        />
      </div>
    </el-card>

    <!-- 新增播放对话框 -->
    <el-dialog v-model="playDialogVisible" title="播放流" width="800px">
      <div id="flv-player-container" style="width: 100%; height: 400px;"></div>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="stopPlay">停止播放</el-button>
          <el-button @click="closePlayDialog">关闭</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 流详情对话框 -->
    <el-dialog v-model="detailDialogVisible" title="流详情" width="800px">
      <el-descriptions :column="2" border v-if="currentStream">
        <el-descriptions-item label="流路径">{{ currentStream.path }}</el-descriptions-item>
        <el-descriptions-item label="协议">{{ currentStream.protocol }}</el-descriptions-item>
        <el-descriptions-item label="类型">{{ currentStream.type }}</el-descriptions-item>
        <el-descriptions-item label="虚拟主机">{{ currentStream.vhost }}</el-descriptions-item>
        <el-descriptions-item label="总观看数">{{ getTotalPlayerCount(currentStream) }}</el-descriptions-item>
        <el-descriptions-item label="状态">{{ getStreamStatus(currentStream) }}</el-descriptions-item>
        <el-descriptions-item label="码率">{{ formatBitrate(currentStream.bitrate) }}</el-descriptions-item>
        <el-descriptions-item label="创建时间">{{ formatTime(currentStream.createTime) }}</el-descriptions-item>
      </el-descriptions>

      <div v-if="currentStream.muxer && currentStream.muxer.length > 0" style="margin-top: 20px;">
        <h4>多路复用器信息</h4>
        <el-table :data="currentStream.muxer" style="width: 100%">
          <el-table-column prop="protocol" label="协议" width="100" />
          <el-table-column prop="type" label="类型" width="100" />
          <el-table-column prop="playerCount" label="观看数" width="100" />
          <el-table-column label="视频信息" min-width="200">
            <template #default="scope">
              <div v-if="scope.row.video">
                <div>FPS: {{ scope.row.video.fps || 'N/A' }}</div>
                <div>最后帧时间: {{ formatTime(scope.row.video.lastFrameTime) }}</div>
              </div>
            </template>
          </el-table-column>
        </el-table>
      </div>
    </el-dialog>

    <!-- 客户端列表对话框 -->
    <el-dialog v-model="clientDialogVisible" title="客户端列表" width="800px">
      <el-table :data="streamClients" v-loading="clientLoading" style="width: 100%">
        <el-table-column prop="ip" label="IP地址" width="150" />
        <el-table-column prop="port" label="端口" width="100" />
        <el-table-column prop="bitrate" label="码率" width="120">
          <template #default="scope">
            <span>{{ formatBitrate(scope.row.bitrate) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="连接时间" width="160">
          <template #default="scope">
            <span>{{ formatTime(scope.row.connectTime) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="100">
          <template #default="scope">
            <el-button type="danger" size="small" @click="closeClient(scope.row)">
              断开
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>

    <!-- 创建流对话框 -->
    <el-dialog v-model="createDialogVisible" title="创建流" width="600px">
      <el-form :model="createForm" label-width="100px" ref="createFormRef">
        <el-form-item label="协议" required>
          <el-select v-model="createForm.protocol" placeholder="请选择协议">
            <el-option label="RTSP" value="rtsp" />
            <el-option label="RTMP" value="rtmp" />
            <el-option label="WebRTC" value="webrtc" />
            <el-option label="SRT" value="srt" />
          </el-select>
        </el-form-item>
        <el-form-item label="应用名" required>
          <el-input v-model="createForm.appName" placeholder="请输入应用名" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="createForm.streamName" placeholder="请输入流名" />
        </el-form-item>
        <el-form-item label="源URL" required>
          <el-input v-model="createForm.url" placeholder="请输入源URL" />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="createDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createStream" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 批量操作组件 -->
    <BatchOperations
      v-model="batchOperationsVisible"
      type="stream"
      :targets="selectedStreamPaths"
      @success="onBatchOperationSuccess"
      @error="onBatchOperationError"
    />
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { streamAPI, rtspAPI, rtmpAPI, webrtcAPI, srtAPI } from '@/api'
import { useRealtime } from '@/composables/useRealtime'
import BatchOperations from '@/components/BatchOperations.vue'
import flvjs from 'flv.js'

// 使用实时数据管理
const { realtimeStore, getUpdateMode, refreshData } = useRealtime({
  enableWebSocket: true,
  enablePolling: true,
  pollingInterval: 10000, // StreamsList 10秒轮询间隔
  autoConnect: true
})

// 响应式数据
const loading = ref(false)
const clientLoading = ref(false)
const creating = ref(false)
const streamClients = ref([])
const selectedStreams = ref([])
const currentStream = ref(null)

// 使用computed从realtime store获取流列表
const streams = computed(() => realtimeStore.streamList)
const connectionStatus = computed(() => realtimeStore.connectionStatus)
const updateMode = ref('none')

// 搜索和过滤
const searchText = ref('')
const protocolFilter = ref('')
const statusFilter = ref('')
const typeFilter = ref('')

// 分页
const currentPage = ref(1)
const pageSize = ref(20)
const totalStreams = ref(0)

// 对话框状态
const detailDialogVisible = ref(false)
const clientDialogVisible = ref(false)
const createDialogVisible = ref(false)
const batchOperationsVisible = ref(false)

// 创建流表单
const createForm = ref({
  protocol: '',
  appName: '',
  streamName: '',
  url: ''
})

// 定时器
let refreshTimer = null

// 计算属性
const filteredStreams = computed(() => {
  try {
    let result = streams.value || []

    // 搜索过滤
    if (searchText.value) {
      result = result.filter(stream =>
        stream.path.toLowerCase().includes(searchText.value.toLowerCase())
      )
    }

    // 协议过滤
    if (protocolFilter.value) {
      result = result.filter(stream => stream.protocol === protocolFilter.value)
    }

    // 状态过滤
    if (statusFilter.value) {
      result = result.filter(stream => {
        const status = getStreamStatus(stream)
        return (statusFilter.value === 'active' && status === '活跃') ||
               (statusFilter.value === 'idle' && status === '空闲')
      })
    }

    // 类型过滤
    if (typeFilter.value) {
      result = result.filter(stream => stream.type === typeFilter.value)
    }

    totalStreams.value = result.length

    // 分页
    const start = (currentPage.value - 1) * pageSize.value
    const end = start + pageSize.value
    return result.slice(start, end)
  } catch (error) {
    console.error('过滤流列表时出错:', error)
    ElMessage.error('过滤流列表时出错: ' + error.message)
    return []
  }
})

// 选中的流路径列表（用于批量操作）
const selectedStreamPaths = computed(() => {
  try {
    return selectedStreams.value.map(stream => stream.path)
  } catch (error) {
    console.error('获取选中流路径时出错:', error)
    ElMessage.error('获取选中流路径时出错: ' + error.message)
    return []
  }
})

// 方法
const refreshStreams = async () => {
  loading.value = true
  try {
    // 手动触发实时数据刷新
    //await refreshData()
    // 如果需要，也可以调用API获取最新数据
    const data = await streamAPI.getSourceList()
    console.log('获取流列表成功:', data)
    realtimeStore.updateStreamList(data.sources || [])
  } catch (error) {
    console.error('获取流列表失败:', error)
    ElMessage.error('获取流列表失败: ' + error.message)
  } finally {
    loading.value = false
  }
}

const getTotalPlayerCount = (stream) => {
  try {
    if (!stream.muxer) return 0
    return stream.muxer.reduce((total, mux) => total + (mux.playerCount || 0), 0)
  } catch (error) {
    console.error('计算总观看数时出错:', error)
    ElMessage.error('计算总观看数时出错: ' + error.message)
    return 0
  }
}

const getStreamStatus = (stream) => {
  try {
    return getTotalPlayerCount(stream) > 0 ? '活跃' : '空闲'
  } catch (error) {
    console.error('获取流状态时出错:', error)
    ElMessage.error('获取流状态时出错: ' + error.message)
    return '未知'
  }
}

const getStatusTagType = (stream) => {
  try {
    return getTotalPlayerCount(stream) > 0 ? 'success' : 'info'
  } catch (error) {
    console.error('获取状态标签类型时出错:', error)
    ElMessage.error('获取状态标签类型时出错: ' + error.message)
    return 'default'
  }
}

const getProtocolTagType = (protocol) => {
  try {
    const types = {
      rtsp: 'primary',
      rtmp: 'success',
      webrtc: 'warning',
      srt: 'danger',
      gb28181: 'info',
      jt1078: 'primary'
    }
    return types[protocol] || 'default'
  } catch (error) {
    console.error('获取协议标签类型时出错:', error)
    ElMessage.error('获取协议标签类型时出错: ' + error.message)
    return 'default'
  }
}

const formatBitrate = (bitrate) => {
  try {
    if (!bitrate) return '0 bps'
    if (bitrate < 1024) return bitrate + ' bps'
    if (bitrate < 1024 * 1024) return (bitrate / 1024).toFixed(1) + ' Kbps'
    return (bitrate / 1024 / 1024).toFixed(1) + ' Mbps'
  } catch (error) {
    console.error('格式化码率时出错:', error)
    ElMessage.error('格式化码率时出错: ' + error.message)
    return '未知'
  }
}

const formatTime = (timestamp) => {
  try {
    if (!timestamp) return 'N/A'
    return new Date(timestamp).toLocaleString()
  } catch (error) {
    console.error('格式化时间时出错:', error)
    ElMessage.error('格式化时间时出错: ' + error.message)
    return '未知'
  }
}

const showStreamDetail = (stream) => {
  try {
    currentStream.value = stream
    detailDialogVisible.value = true
  } catch (error) {
    console.error('显示流详情时出错:', error)
    ElMessage.error('显示流详情时出错: ' + error.message)
  }
}

const showClientList = async (stream) => {
  try {
    currentStream.value = stream
    clientLoading.value = true
    clientDialogVisible.value = true

    const data = await streamAPI.getClientList({ path: stream.path })
    streamClients.value = data.client || []
  } catch (error) {
    console.error('获取客户端列表失败:', error)
    ElMessage.error('获取客户端列表失败: ' + error.message)
  } finally {
    clientLoading.value = false
  }
}

const closeStream = async (stream) => {
  try {
    await ElMessageBox.confirm(
      `确定要关闭流 "${stream.path}" 吗？`,
      '确认关闭',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )

    await streamAPI.closeSource({ path: stream.path })
    ElMessage.success('流关闭成功')
    refreshStreams()
  } catch (error) {
    if (error !== 'cancel') {
      console.error('关闭流失败:', error)
      ElMessage.error('关闭流失败: ' + error.message)
    }
  }
}

const closeClient = async (client) => {
  try {
    await streamAPI.closeClient({
      ip: client.ip,
      port: client.port
    })
    ElMessage.success('客户端断开成功')
    showClientList(currentStream.value)
  } catch (error) {
    console.error('断开客户端失败:', error)
    ElMessage.error('断开客户端失败: ' + error.message)
  }
}

const batchCloseStreams = async () => {
  try {
    await ElMessageBox.confirm(
      `确定要关闭选中的 ${selectedStreams.value.length} 个流吗？`,
      '批量关闭确认',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )

    for (const stream of selectedStreams.value) {
      await streamAPI.closeSource({ path: stream.path })
    }

    ElMessage.success('批量关闭成功')
    selectedStreams.value = []
    refreshStreams()
  } catch (error) {
    if (error !== 'cancel') {
      console.error('批量关闭失败:', error)
      ElMessage.error('批量关闭失败: ' + error.message)
    }
  }
}

const showCreateDialog = () => {
  try {
    createForm.value = {
      protocol: '',
      appName: '',
      streamName: '',
      url: ''
    }
    createDialogVisible.value = true
  } catch (error) {
    console.error('显示创建流对话框时出错:', error)
    ElMessage.error('显示创建流对话框时出错: ' + error.message)
  }
}

const createStream = async () => {
  const form = createForm.value
  if (!form.protocol || !form.appName || !form.streamName || !form.url) {
    ElMessage.error('请填写完整信息')
    return
  }

  creating.value = true
  try {
    const data = {
      url: form.url,
      appName: form.appName,
      streamName: form.streamName
    }

    switch (form.protocol) {
      case 'rtsp':
        await rtspAPI.startPlay(data)
        break
      case 'rtmp':
        await rtmpAPI.startPlay(data)
        break
      case 'webrtc':
        await webrtcAPI.startPull(data)
        break
      case 'srt':
        await srtAPI.startPull(data)
        break
    }

    ElMessage.success('流创建成功')
    createDialogVisible.value = false
    refreshStreams()
  } catch (error) {
    console.error('创建流失败:', error)
    ElMessage.error('创建流失败: ' + error.message)
  } finally {
    creating.value = false
  }
}

const handleSearch = () => {
  try {
    currentPage.value = 1
  } catch (error) {
    console.error('处理搜索时出错:', error)
    ElMessage.error('处理搜索时出错: ' + error.message)
  }
}

const handleFilter = () => {
  try {
    currentPage.value = 1
  } catch (error) {
    console.error('处理过滤时出错:', error)
    ElMessage.error('处理过滤时出错: ' + error.message)
  }
}

const handleSelectionChange = (selection) => {
  try {
    selectedStreams.value = selection
  } catch (error) {
    console.error('处理选择变化时出错:', error)
    ElMessage.error('处理选择变化时出错: ' + error.message)
  }
}

const handleSizeChange = (size) => {
  try {
    pageSize.value = size
    currentPage.value = 1
  } catch (error) {
    console.error('处理页面大小变化时出错:', error)
    ElMessage.error('处理页面大小变化时出错: ' + error.message)
  }
}

const handleCurrentChange = (page) => {
  try {
    currentPage.value = page
  } catch (error) {
    console.error('处理当前页面变化时出错:', error)
    ElMessage.error('处理当前页面变化时出错: ' + error.message)
  }
}

// 批量操作相关方法
const showBatchOperations = () => {
  try {
    if (selectedStreams.value.length === 0) {
      ElMessage.warning('请先选择要操作的流')
      return
    }
    batchOperationsVisible.value = true
  } catch (error) {
    console.error('显示批量操作时出错:', error)
    ElMessage.error('显示批量操作时出错: ' + error.message)
  }
}

const onBatchOperationSuccess = (result) => {
  try {
    ElMessage.success(`批量操作完成，成功处理 ${result.successCount} 个流`)
    selectedStreams.value = []
    refreshStreams()
  } catch (error) {
    console.error('处理批量操作成功事件时出错:', error)
    ElMessage.error('处理批量操作成功事件时出错: ' + error.message)
  }
}

const onBatchOperationError = (error) => {
  try {
    console.error('批量操作失败:', error)
    refreshStreams()
  } catch (error) {
    console.error('处理批量操作失败事件时出错:', error)
    ElMessage.error('处理批量操作失败事件时出错: ' + error.message)
  }
}

// 响应式数据
const playDialogVisible = ref(false)
const currentPlayStream = ref(null)
let flvPlayer = null
let videoElement = null

// 显示播放对话框
const showPlayDialog = (stream) => {
  currentPlayStream.value = stream
  playDialogVisible.value = true
  // 初始化播放器前先清除上次的视频元素
  clearVideoElement()
  // 初始化播放器
  initFlvPlayer()
}

// 关闭播放对话框
const closePlayDialog = () => {
  stopPlay()
  playDialogVisible.value = false
}

// 停止播放
const stopPlay = () => {
  if (flvPlayer) {
    flvPlayer.pause()
    flvPlayer.unload()
    flvPlayer.detachMediaElement()
    flvPlayer.destroy()
    flvPlayer = null
  }
  clearVideoElement()
}

// 清除视频元素
const clearVideoElement = () => {
  const videoContainer = document.getElementById('flv-player-container')
  if (videoContainer && videoElement) {
    videoContainer.removeChild(videoElement)
    videoElement = null
  }
}

// 初始化 FLV 播放器
const initFlvPlayer = () => {
  if (!currentPlayStream.value) return

  const videoContainer = document.getElementById('flv-player-container')
  if (!videoContainer) return

  videoElement = document.createElement('video')
  videoElement.style.width = '100%'
  videoElement.style.height = '100%'
  videoElement.controls = true
  videoContainer.appendChild(videoElement)

  if (flvjs.isSupported()) {
    flvPlayer = flvjs.createPlayer({
      type: 'flv',
      // 请替换为实际的 FLV 流地址
      url: `http://172.24.6.4:8080${currentPlayStream.value.path}.flv`
    })
    flvPlayer.attachMediaElement(videoElement)
    flvPlayer.load()
    flvPlayer.play()
  } else {
    ElMessage.error('当前浏览器不支持播放 FLV 流')
  }
}

// 监听实时数据变化
watch(() => realtimeStore.streamList, () => {
  try {
    updateMode.value = getUpdateMode()
  } catch (error) {
    console.error('监听实时数据变化时出错:', error)
    ElMessage.error('监听实时数据变化时出错: ' + error.message)
  }
}, { deep: true })

onMounted(() => {
  try {
    refreshStreams()
    updateMode.value = getUpdateMode()
  } catch (error) {
    console.error('组件挂载时出错:', error)
    ElMessage.error('组件挂载时出错: ' + error.message)
  }
})

onUnmounted(() => {
  try {
    // 实时数据管理会自动清理，无需手动清理定时器
    // 组件卸载时销毁播放器
    stopPlay()
  } catch (error) {
    console.error('组件卸载时出错:', error)
    ElMessage.error('组件卸载时出错: ' + error.message)
  }
})
</script>

<style scoped>
.streams-list {
  max-width: 100%;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.filter-section {
  margin-bottom: 20px;
}

.player-count {
  font-weight: bold;
  color: #409EFF;
}

.batch-actions {
  margin-top: 20px;
  padding: 15px;
  background-color: #f5f7fa;
  border-radius: 4px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.batch-buttons {
  margin-left: 20px;
}

.pagination {
  margin-top: 20px;
  text-align: right;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}
</style>
