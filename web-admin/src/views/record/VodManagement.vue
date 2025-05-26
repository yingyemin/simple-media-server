<template>
  <div class="vod-management">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>点播管理</span>
          <el-button type="primary" @click="showCreateDialog">
            <el-icon><Plus /></el-icon>
            创建点播
          </el-button>
        </div>
      </template>

      <el-table :data="vodSessions" v-loading="loading" style="width: 100%">
        <el-table-column prop="uri" label="点播URI" min-width="200" />
        <el-table-column prop="format" label="格式" width="100">
          <template #default="scope">
            <el-tag>{{ scope.row.format.toUpperCase() }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="status" label="状态" width="100">
          <template #default="scope">
            <el-tag :type="getStatusType(scope.row.status)">
              {{ getStatusText(scope.row.status) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="progress" label="进度" width="120">
          <template #default="scope">
            <el-progress :percentage="scope.row.progress" :show-text="false" />
            <span style="margin-left: 10px;">{{ scope.row.progress }}%</span>
          </template>
        </el-table-column>
        <el-table-column prop="duration" label="总时长" width="120">
          <template #default="scope">
            <span>{{ formatDuration(scope.row.duration) }}</span>
          </template>
        </el-table-column>
        <el-table-column prop="currentTime" label="当前时间" width="120">
          <template #default="scope">
            <span>{{ formatDuration(scope.row.currentTime) }}</span>
          </template>
        </el-table-column>
        <el-table-column prop="scale" label="播放倍速" width="100">
          <template #default="scope">
            <span>{{ scope.row.scale }}x</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="200">
          <template #default="scope">
            <el-button 
              v-if="scope.row.status === 'playing'"
              type="warning" 
              size="small" 
              @click="pauseVod(scope.row)"
            >
              暂停
            </el-button>
            <el-button 
              v-if="scope.row.status === 'paused'"
              type="success" 
              size="small" 
              @click="resumeVod(scope.row)"
            >
              继续
            </el-button>
            <el-button type="primary" size="small" @click="showControlDialog(scope.row)">
              控制
            </el-button>
            <el-button type="danger" size="small" @click="stopVod(scope.row)">
              停止
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 创建点播对话框 -->
    <el-dialog v-model="createDialogVisible" title="创建点播" width="600px">
      <el-form :model="vodForm" label-width="120px">
        <el-form-item label="点播URI" required>
          <el-input v-model="vodForm.uri" placeholder="/vod/video1" />
        </el-form-item>
        <el-form-item label="格式" required>
          <el-select v-model="vodForm.format" placeholder="请选择格式">
            <el-option label="MP4" value="mp4" />
            <el-option label="PS" value="ps" />
            <el-option label="TS" value="ts" />
          </el-select>
        </el-form-item>
        <el-form-item label="开始时间">
          <el-input v-model="vodForm.startTime" placeholder="格式: YYYY-MM-DD HH:mm:ss" />
        </el-form-item>
        <el-form-item label="结束时间">
          <el-input v-model="vodForm.endTime" placeholder="格式: YYYY-MM-DD HH:mm:ss" />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="createDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createVod" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 点播控制对话框 -->
    <el-dialog v-model="controlDialogVisible" title="点播控制" width="500px">
      <el-form :model="controlForm" label-width="120px" v-if="currentVod">
        <el-form-item label="当前URI">
          <span>{{ currentVod.uri }}</span>
        </el-form-item>
        <el-form-item label="播放倍速">
          <el-select v-model="controlForm.scale">
            <el-option label="0.25x" :value="0.25" />
            <el-option label="0.5x" :value="0.5" />
            <el-option label="1x" :value="1" />
            <el-option label="2x" :value="2" />
            <el-option label="4x" :value="4" />
            <el-option label="8x" :value="8" />
          </el-select>
        </el-form-item>
        <el-form-item label="跳转时间">
          <el-input v-model="controlForm.seekTime" placeholder="秒数" type="number" />
        </el-form-item>
        <el-form-item label="快速操作">
          <el-button @click="seekVod(-30)">后退30秒</el-button>
          <el-button @click="seekVod(-10)">后退10秒</el-button>
          <el-button @click="seekVod(10)">前进10秒</el-button>
          <el-button @click="seekVod(30)">前进30秒</el-button>
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="controlDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="applyControl">应用</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { vodAPI } from '@/api'

const loading = ref(false)
const creating = ref(false)
const createDialogVisible = ref(false)
const controlDialogVisible = ref(false)
const vodSessions = ref([])
const currentVod = ref(null)

const vodForm = ref({
  uri: '',
  format: 'mp4',
  startTime: '',
  endTime: ''
})

const controlForm = ref({
  scale: 1,
  seekTime: ''
})

let refreshTimer = null

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

const getStatusType = (status) => {
  const types = {
    playing: 'success',
    paused: 'warning',
    stopped: 'danger',
    loading: 'info'
  }
  return types[status] || 'default'
}

const getStatusText = (status) => {
  const texts = {
    playing: '播放中',
    paused: '已暂停',
    stopped: '已停止',
    loading: '加载中'
  }
  return texts[status] || status
}

const loadVodSessions = async () => {
  loading.value = true
  try {
    // 模拟获取点播会话列表
    await new Promise(resolve => setTimeout(resolve, 1000))
    vodSessions.value = [
      {
        uri: '/vod/video1',
        format: 'mp4',
        status: 'playing',
        progress: 45,
        duration: 3600,
        currentTime: 1620,
        scale: 1
      },
      {
        uri: '/vod/video2',
        format: 'ps',
        status: 'paused',
        progress: 20,
        duration: 7200,
        currentTime: 1440,
        scale: 2
      }
    ]
  } catch (error) {
    ElMessage.error('获取点播列表失败: ' + error.message)
  } finally {
    loading.value = false
  }
}

const showCreateDialog = () => {
  vodForm.value = {
    uri: '',
    format: 'mp4',
    startTime: '',
    endTime: ''
  }
  createDialogVisible.value = true
}

const createVod = async () => {
  const form = vodForm.value
  if (!form.uri || !form.format) {
    ElMessage.error('请填写完整信息')
    return
  }
  
  creating.value = true
  try {
    await vodAPI.start({
      uri: form.uri,
      format: form.format,
      startTime: form.startTime,
      endTime: form.endTime
    })
    ElMessage.success('点播创建成功')
    createDialogVisible.value = false
    loadVodSessions()
  } catch (error) {
    ElMessage.error('创建点播失败: ' + error.message)
  } finally {
    creating.value = false
  }
}

const showControlDialog = (vod) => {
  currentVod.value = vod
  controlForm.value = {
    scale: vod.scale,
    seekTime: ''
  }
  controlDialogVisible.value = true
}

const pauseVod = async (vod) => {
  try {
    await vodAPI.control({
      uri: vod.uri,
      action: 'pause'
    })
    ElMessage.success('点播暂停成功')
    loadVodSessions()
  } catch (error) {
    ElMessage.error('暂停点播失败: ' + error.message)
  }
}

const resumeVod = async (vod) => {
  try {
    await vodAPI.control({
      uri: vod.uri,
      action: 'resume'
    })
    ElMessage.success('点播继续成功')
    loadVodSessions()
  } catch (error) {
    ElMessage.error('继续点播失败: ' + error.message)
  }
}

const stopVod = async (vod) => {
  try {
    await ElMessageBox.confirm(
      `确定要停止点播 "${vod.uri}" 吗？`,
      '确认停止',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )
    
    await vodAPI.stop({ uri: vod.uri })
    ElMessage.success('点播停止成功')
    loadVodSessions()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('停止点播失败: ' + error.message)
    }
  }
}

const seekVod = (seconds) => {
  const currentTime = currentVod.value.currentTime + seconds
  controlForm.value.seekTime = Math.max(0, currentTime).toString()
}

const applyControl = async () => {
  try {
    const params = {
      uri: currentVod.value.uri
    }
    
    if (controlForm.value.scale !== currentVod.value.scale) {
      params.scale = controlForm.value.scale
    }
    
    if (controlForm.value.seekTime) {
      params.seekTime = parseInt(controlForm.value.seekTime)
    }
    
    await vodAPI.control(params)
    ElMessage.success('控制应用成功')
    controlDialogVisible.value = false
    loadVodSessions()
  } catch (error) {
    ElMessage.error('应用控制失败: ' + error.message)
  }
}

onMounted(() => {
  loadVodSessions()
  
  // 设置定时刷新
  refreshTimer = setInterval(() => {
    loadVodSessions()
  }, 2000) // 2秒刷新一次
})

onUnmounted(() => {
  if (refreshTimer) {
    clearInterval(refreshTimer)
  }
})
</script>

<style scoped>
.vod-management {
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
</style>
