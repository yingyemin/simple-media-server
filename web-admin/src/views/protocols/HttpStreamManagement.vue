<template>
  <div class="http-stream-management">
    <el-card>
      <template #header>
        <span>HTTP流媒体管理</span>
      </template>
      
      <el-tabs v-model="activeTab" type="border-card">
        <!-- FLV播放管理 -->
        <el-tab-pane label="FLV播放" name="flv">
          <el-card>
            <template #header>
              <div class="card-header">
                <span>FLV播放管理</span>
                <el-button type="primary" @click="showCreateDialog('flv')">
                  <el-icon><Plus /></el-icon>
                  创建FLV播放
                </el-button>
              </div>
            </template>

            <el-table :data="flvStreams" v-loading="flvLoading" style="width: 100%">
              <el-table-column prop="path" label="流路径" min-width="200" />
              <el-table-column prop="url" label="源URL" min-width="300" show-overflow-tooltip />
              <el-table-column prop="status" label="状态" width="100">
                <template #default="scope">
                  <el-tag :type="scope.row.status === 'playing' ? 'success' : 'danger'">
                    {{ scope.row.status === 'playing' ? '播放中' : '停止' }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="playerCount" label="观看数" width="100" />
              <el-table-column prop="bitrate" label="码率" width="120">
                <template #default="scope">
                  <span>{{ formatBitrate(scope.row.bitrate) }}</span>
                </template>
              </el-table-column>
              <el-table-column label="操作" width="150">
                <template #default="scope">
                  <el-button type="warning" size="small" @click="stopStream(scope.row, 'flv')">
                    停止
                  </el-button>
                  <el-button type="danger" size="small" @click="deleteStream(scope.row, 'flv')">
                    删除
                  </el-button>
                </template>
              </el-table-column>
            </el-table>
          </el-card>
        </el-tab-pane>

        <!-- HLS播放管理 -->
        <el-tab-pane label="HLS播放" name="hls">
          <el-card>
            <template #header>
              <div class="card-header">
                <span>HLS播放管理</span>
                <el-button type="primary" @click="showCreateDialog('hls')">
                  <el-icon><Plus /></el-icon>
                  创建HLS播放
                </el-button>
              </div>
            </template>

            <el-table :data="hlsStreams" v-loading="hlsLoading" style="width: 100%">
              <el-table-column prop="path" label="流路径" min-width="200" />
              <el-table-column prop="url" label="源URL" min-width="300" show-overflow-tooltip />
              <el-table-column prop="status" label="状态" width="100">
                <template #default="scope">
                  <el-tag :type="scope.row.status === 'playing' ? 'success' : 'danger'">
                    {{ scope.row.status === 'playing' ? '播放中' : '停止' }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="playerCount" label="观看数" width="100" />
              <el-table-column prop="segmentDuration" label="切片时长" width="120">
                <template #default="scope">
                  <span>{{ scope.row.segmentDuration }}s</span>
                </template>
              </el-table-column>
              <el-table-column label="操作" width="150">
                <template #default="scope">
                  <el-button type="warning" size="small" @click="stopStream(scope.row, 'hls')">
                    停止
                  </el-button>
                  <el-button type="danger" size="small" @click="deleteStream(scope.row, 'hls')">
                    删除
                  </el-button>
                </template>
              </el-table-column>
            </el-table>
          </el-card>
        </el-tab-pane>

        <!-- PS点播管理 -->
        <el-tab-pane label="PS点播" name="ps">
          <el-card>
            <template #header>
              <div class="card-header">
                <span>PS点播管理</span>
                <el-button type="primary" @click="showCreateDialog('ps')">
                  <el-icon><Plus /></el-icon>
                  创建PS点播
                </el-button>
              </div>
            </template>

            <el-table :data="psStreams" v-loading="psLoading" style="width: 100%">
              <el-table-column prop="path" label="流路径" min-width="200" />
              <el-table-column prop="url" label="源URL" min-width="300" show-overflow-tooltip />
              <el-table-column prop="status" label="状态" width="100">
                <template #default="scope">
                  <el-tag :type="scope.row.status === 'playing' ? 'success' : 'danger'">
                    {{ scope.row.status === 'playing' ? '播放中' : '停止' }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="duration" label="时长" width="120">
                <template #default="scope">
                  <span>{{ formatDuration(scope.row.duration) }}</span>
                </template>
              </el-table-column>
              <el-table-column prop="progress" label="进度" width="120">
                <template #default="scope">
                  <el-progress :percentage="scope.row.progress" :show-text="false" />
                  <span style="margin-left: 10px;">{{ scope.row.progress }}%</span>
                </template>
              </el-table-column>
              <el-table-column label="操作" width="150">
                <template #default="scope">
                  <el-button type="warning" size="small" @click="stopStream(scope.row, 'ps')">
                    停止
                  </el-button>
                  <el-button type="danger" size="small" @click="deleteStream(scope.row, 'ps')">
                    删除
                  </el-button>
                </template>
              </el-table-column>
            </el-table>
          </el-card>
        </el-tab-pane>
      </el-tabs>
    </el-card>

    <!-- 创建流对话框 -->
    <el-dialog v-model="dialogVisible" :title="`创建${getStreamTypeName(dialogType)}播放`" width="600px">
      <el-form :model="streamForm" label-width="120px" ref="streamFormRef">
        <el-form-item label="源URL" required>
          <el-input v-model="streamForm.url" :placeholder="getUrlPlaceholder(dialogType)" />
        </el-form-item>
        <el-form-item label="应用名" required>
          <el-input v-model="streamForm.appName" placeholder="live" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="streamForm.streamName" placeholder="stream1" />
        </el-form-item>
        
        <!-- FLV特有配置 -->
        <template v-if="dialogType === 'flv'">
          <el-form-item label="超时时间(秒)">
            <el-input-number v-model="streamForm.timeout" :min="5" :max="300" />
          </el-form-item>
        </template>
        
        <!-- HLS特有配置 -->
        <template v-if="dialogType === 'hls'">
          <el-form-item label="切片时长(秒)">
            <el-input-number v-model="streamForm.segmentDuration" :min="1" :max="30" />
          </el-form-item>
          <el-form-item label="切片数量">
            <el-input-number v-model="streamForm.segmentCount" :min="3" :max="20" />
          </el-form-item>
        </template>
        
        <!-- PS特有配置 -->
        <template v-if="dialogType === 'ps'">
          <el-form-item label="开始时间">
            <el-input v-model="streamForm.startTime" placeholder="YYYY-MM-DD HH:mm:ss" />
          </el-form-item>
          <el-form-item label="结束时间">
            <el-input v-model="streamForm.endTime" placeholder="YYYY-MM-DD HH:mm:ss" />
          </el-form-item>
        </template>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="dialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createStream" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { httpStreamAPI } from '@/api'

const activeTab = ref('flv')
const dialogVisible = ref(false)
const dialogType = ref('flv')
const creating = ref(false)

const flvLoading = ref(false)
const hlsLoading = ref(false)
const psLoading = ref(false)

const flvStreams = ref([])
const hlsStreams = ref([])
const psStreams = ref([])

const streamForm = ref({
  url: '',
  appName: 'live',
  streamName: '',
  timeout: 30,
  segmentDuration: 5,
  segmentCount: 5,
  startTime: '',
  endTime: ''
})

const formatBitrate = (bitrate) => {
  if (!bitrate) return '0 bps'
  if (bitrate < 1024) return bitrate + ' bps'
  if (bitrate < 1024 * 1024) return (bitrate / 1024).toFixed(1) + ' Kbps'
  return (bitrate / 1024 / 1024).toFixed(1) + ' Mbps'
}

const formatDuration = (seconds) => {
  if (!seconds) return '00:00'
  const hours = Math.floor(seconds / 3600)
  const minutes = Math.floor((seconds % 3600) / 60)
  const secs = seconds % 60
  
  if (hours > 0) {
    return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`
  } else {
    return `${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`
  }
}

const getStreamTypeName = (type) => {
  const names = {
    flv: 'FLV',
    hls: 'HLS',
    ps: 'PS'
  }
  return names[type] || type
}

const getUrlPlaceholder = (type) => {
  const placeholders = {
    flv: 'http://example.com/live/stream.flv',
    hls: 'http://example.com/live/stream.m3u8',
    ps: 'http://example.com/vod/video.ps'
  }
  return placeholders[type] || 'http://example.com/stream'
}

const loadStreams = async () => {
  await Promise.all([
    loadFlvStreams(),
    loadHlsStreams(),
    loadPsStreams()
  ])
}

const loadFlvStreams = async () => {
  flvLoading.value = true
  try {
    const data = await httpStreamAPI.getFlvPlayList()
    flvStreams.value = data.streams || []
  } catch (error) {
    ElMessage.error('获取FLV流列表失败: ' + error.message)
  } finally {
    flvLoading.value = false
  }
}

const loadHlsStreams = async () => {
  hlsLoading.value = true
  try {
    const data = await httpStreamAPI.getHlsPlayList()
    hlsStreams.value = data.streams || []
  } catch (error) {
    ElMessage.error('获取HLS流列表失败: ' + error.message)
  } finally {
    hlsLoading.value = false
  }
}

const loadPsStreams = async () => {
  psLoading.value = true
  try {
    const data = await httpStreamAPI.getPsVodPlayList()
    psStreams.value = data.streams || []
  } catch (error) {
    ElMessage.error('获取PS流列表失败: ' + error.message)
  } finally {
    psLoading.value = false
  }
}

const showCreateDialog = (type) => {
  dialogType.value = type
  streamForm.value = {
    url: '',
    appName: 'live',
    streamName: '',
    timeout: 30,
    segmentDuration: 5,
    segmentCount: 5,
    startTime: '',
    endTime: ''
  }
  dialogVisible.value = true
}

const createStream = async () => {
  const form = streamForm.value
  if (!form.url || !form.appName || !form.streamName) {
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
    
    if (dialogType.value === 'flv') {
      data.timeout = form.timeout
      await httpStreamAPI.startFlvPlay(data)
    } else if (dialogType.value === 'hls') {
      data.segmentDuration = form.segmentDuration
      data.segmentCount = form.segmentCount
      await httpStreamAPI.startHlsPlay(data)
    } else if (dialogType.value === 'ps') {
      data.startTime = form.startTime
      data.endTime = form.endTime
      await httpStreamAPI.startPsVodPlay(data)
    }
    
    ElMessage.success(`${getStreamTypeName(dialogType.value)}播放创建成功`)
    dialogVisible.value = false
    loadStreams()
  } catch (error) {
    ElMessage.error(`创建${getStreamTypeName(dialogType.value)}播放失败: ` + error.message)
  } finally {
    creating.value = false
  }
}

const stopStream = async (stream, type) => {
  try {
    const data = {
      appName: stream.appName || 'live',
      streamName: stream.streamName || stream.path?.split('/').pop()
    }
    
    if (type === 'flv') {
      await httpStreamAPI.stopFlvPlay(data)
    } else if (type === 'hls') {
      await httpStreamAPI.stopHlsPlay(data)
    } else if (type === 'ps') {
      await httpStreamAPI.stopPsVodPlay(data)
    }
    
    ElMessage.success(`${getStreamTypeName(type)}播放停止成功`)
    loadStreams()
  } catch (error) {
    ElMessage.error(`停止${getStreamTypeName(type)}播放失败: ` + error.message)
  }
}

const deleteStream = async (stream, type) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除此${getStreamTypeName(type)}播放吗？`,
      '确认删除',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )
    
    await stopStream(stream, type)
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error(`删除${getStreamTypeName(type)}播放失败: ` + error.message)
    }
  }
}

onMounted(() => {
  loadStreams()
})
</script>

<style scoped>
.http-stream-management {
  max-width: 100%;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}

:deep(.el-tabs__content) {
  padding: 0;
}
</style>
