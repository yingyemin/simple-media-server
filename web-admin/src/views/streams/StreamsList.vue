<template>
  <div class="streams-list">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>流列表管理</span>
          <div>
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
        <el-table-column label="操作" width="200" fixed="right">
          <template #default="scope">
            <el-button type="primary" size="small" @click="showStreamDetail(scope.row)">
              详情
            </el-button>
            <el-button type="warning" size="small" @click="showClientList(scope.row)">
              客户端
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
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { streamAPI, rtspAPI, rtmpAPI, webrtcAPI, srtAPI } from '@/api'

// 响应式数据
const loading = ref(false)
const clientLoading = ref(false)
const creating = ref(false)
const streams = ref([])
const streamClients = ref([])
const selectedStreams = ref([])
const currentStream = ref(null)

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
  let result = streams.value

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
})

// 方法
const refreshStreams = async () => {
  loading.value = true
  try {
    const data = await streamAPI.getSourceList()
    streams.value = data.sources || []
    totalStreams.value = data.count || 0
  } catch (error) {
    ElMessage.error('获取流列表失败: ' + error.message)
  } finally {
    loading.value = false
  }
}

const getTotalPlayerCount = (stream) => {
  if (!stream.muxer) return 0
  return stream.muxer.reduce((total, mux) => total + (mux.playerCount || 0), 0)
}

const getStreamStatus = (stream) => {
  return getTotalPlayerCount(stream) > 0 ? '活跃' : '空闲'
}

const getStatusTagType = (stream) => {
  return getTotalPlayerCount(stream) > 0 ? 'success' : 'info'
}

const getProtocolTagType = (protocol) => {
  const types = {
    rtsp: 'primary',
    rtmp: 'success',
    webrtc: 'warning',
    srt: 'danger',
    gb28181: 'info',
    jt1078: 'primary'
  }
  return types[protocol] || 'default'
}

const formatBitrate = (bitrate) => {
  if (!bitrate) return '0 bps'
  if (bitrate < 1024) return bitrate + ' bps'
  if (bitrate < 1024 * 1024) return (bitrate / 1024).toFixed(1) + ' Kbps'
  return (bitrate / 1024 / 1024).toFixed(1) + ' Mbps'
}

const formatTime = (timestamp) => {
  if (!timestamp) return 'N/A'
  return new Date(timestamp).toLocaleString()
}

const showStreamDetail = (stream) => {
  currentStream.value = stream
  detailDialogVisible.value = true
}

const showClientList = async (stream) => {
  currentStream.value = stream
  clientLoading.value = true
  clientDialogVisible.value = true
  
  try {
    const data = await streamAPI.getClientList({ path: stream.path })
    streamClients.value = data.client || []
  } catch (error) {
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
      ElMessage.error('批量关闭失败: ' + error.message)
    }
  }
}

const showCreateDialog = () => {
  createForm.value = {
    protocol: '',
    appName: '',
    streamName: '',
    url: ''
  }
  createDialogVisible.value = true
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
    ElMessage.error('创建流失败: ' + error.message)
  } finally {
    creating.value = false
  }
}

const handleSearch = () => {
  currentPage.value = 1
}

const handleFilter = () => {
  currentPage.value = 1
}

const handleSelectionChange = (selection) => {
  selectedStreams.value = selection
}

const handleSizeChange = (size) => {
  pageSize.value = size
  currentPage.value = 1
}

const handleCurrentChange = (page) => {
  currentPage.value = page
}

onMounted(() => {
  refreshStreams()
  
  // 设置定时刷新
  refreshTimer = setInterval(() => {
    refreshStreams()
  }, 10000) // 10秒刷新一次
})

onUnmounted(() => {
  if (refreshTimer) {
    clearInterval(refreshTimer)
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
