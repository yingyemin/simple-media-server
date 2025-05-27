<template>
  <div class="record-tasks">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>录制任务管理</span>
          <el-button type="primary" @click="showCreateDialog">
            <el-icon><Plus /></el-icon>
            创建录制任务
          </el-button>
        </div>
      </template>

      <el-table :data="recordTasks" v-loading="loading" style="width: 100%">
        <el-table-column prop="taskId" label="任务ID" width="150" />
        <el-table-column prop="path" label="流路径" min-width="200" />
        <el-table-column prop="format" label="格式" width="100">
          <template #default="scope">
            <el-tag>{{ scope.row.format.toUpperCase() }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="status" label="状态" width="100">
          <template #default="scope">
            <el-tag :type="scope.row.status === 'recording' ? 'success' : 'danger'">
              {{ scope.row.status === 'recording' ? '录制中' : '已停止' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="duration" label="录制时长" width="120">
          <template #default="scope">
            <span>{{ formatDuration(scope.row.duration) }}</span>
          </template>
        </el-table-column>
        <el-table-column prop="fileSize" label="文件大小" width="120">
          <template #default="scope">
            <span>{{ formatFileSize(scope.row.fileSize) }}</span>
          </template>
        </el-table-column>
        <el-table-column prop="startTime" label="开始时间" width="160">
          <template #default="scope">
            <span>{{ formatTime(scope.row.startTime) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="150">
          <template #default="scope">
            <el-button 
              v-if="scope.row.status === 'recording'"
              type="warning" 
              size="small" 
              @click="stopRecord(scope.row)"
            >
              停止录制
            </el-button>
            <el-button type="danger" size="small" @click="deleteRecord(scope.row)">
              删除
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 创建录制任务对话框 -->
    <el-dialog v-model="dialogVisible" title="创建录制任务" width="600px">
      <el-form :model="recordForm" label-width="120px" ref="recordFormRef">
        <el-form-item label="应用名" required>
          <el-input v-model="recordForm.appName" placeholder="live" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="recordForm.streamName" placeholder="stream1" />
        </el-form-item>
        <el-form-item label="录制格式" required>
          <el-select v-model="recordForm.format" placeholder="请选择录制格式">
            <el-option label="MP4" value="mp4" />
            <el-option label="PS" value="ps" />
          </el-select>
        </el-form-item>
        <el-form-item label="录制模板">
          <el-card>
            <el-form-item label="录制时长(秒)">
              <el-input-number v-model="recordForm.recordTemplate.duration" :min="0" />
              <div class="form-tip">0表示不限制时长</div>
            </el-form-item>
            <el-form-item label="分段时长(秒)">
              <el-input-number v-model="recordForm.recordTemplate.segmentDuration" :min="0" />
              <div class="form-tip">0表示不分段</div>
            </el-form-item>
            <el-form-item label="分段数量">
              <el-input-number v-model="recordForm.recordTemplate.segmentCount" :min="0" />
              <div class="form-tip">0表示不限制分段数量</div>
            </el-form-item>
          </el-card>
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="dialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createRecord" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { recordAPI } from '@/api'

const loading = ref(false)
const creating = ref(false)
const dialogVisible = ref(false)
const recordTasks = ref([])

const recordForm = ref({
  appName: 'live',
  streamName: '',
  format: 'mp4',
  recordTemplate: {
    duration: 0,
    segmentDuration: 0,
    segmentCount: 0
  }
})

let refreshTimer = null

const formatDuration = (duration) => {
  if (!duration) return '0秒'
  const hours = Math.floor(duration / 3600)
  const minutes = Math.floor((duration % 3600) / 60)
  const seconds = duration % 60
  
  if (hours > 0) {
    return `${hours}时${minutes}分${seconds}秒`
  } else if (minutes > 0) {
    return `${minutes}分${seconds}秒`
  } else {
    return `${seconds}秒`
  }
}

const formatFileSize = (size) => {
  if (!size) return '0 B'
  const units = ['B', 'KB', 'MB', 'GB']
  let index = 0
  while (size >= 1024 && index < units.length - 1) {
    size /= 1024
    index++
  }
  return `${size.toFixed(2)} ${units[index]}`
}

const formatTime = (timestamp) => {
  if (!timestamp) return 'N/A'
  return new Date(timestamp).toLocaleString()
}

const loadRecordTasks = async () => {
  loading.value = true
  try {
    const data = await recordAPI.getRecordList()
    recordTasks.value = data.records || []
  } catch (error) {
    ElMessage.error('获取录制任务失败: ' + error.message)
  } finally {
    loading.value = false
  }
}

const showCreateDialog = () => {
  recordForm.value = {
    appName: 'live',
    streamName: '',
    format: 'mp4',
    recordTemplate: {
      duration: 0,
      segmentDuration: 0,
      segmentCount: 0
    }
  }
  dialogVisible.value = true
}

const createRecord = async () => {
  const form = recordForm.value
  if (!form.appName || !form.streamName || !form.format) {
    ElMessage.error('请填写完整信息')
    return
  }
  
  creating.value = true
  try {
    await recordAPI.startRecord({
      appName: form.appName,
      streamName: form.streamName,
      format: form.format,
      recordTemplate: form.recordTemplate
    })
    ElMessage.success('录制任务创建成功')
    dialogVisible.value = false
    loadRecordTasks()
  } catch (error) {
    ElMessage.error('创建录制任务失败: ' + error.message)
  } finally {
    creating.value = false
  }
}

const stopRecord = async (task) => {
  try {
    await ElMessageBox.confirm(
      `确定要停止录制任务 "${task.taskId}" 吗？`,
      '确认停止',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )
    
    await recordAPI.stopRecord({ taskId: task.taskId })
    ElMessage.success('录制任务停止成功')
    loadRecordTasks()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('停止录制任务失败: ' + error.message)
    }
  }
}

const deleteRecord = async (task) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除录制任务 "${task.taskId}" 吗？`,
      '确认删除',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )
    
    // 如果正在录制，先停止
    if (task.status === 'recording') {
      await recordAPI.stopRecord({ taskId: task.taskId })
    }
    
    ElMessage.success('录制任务删除成功')
    loadRecordTasks()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除录制任务失败: ' + error.message)
    }
  }
}

onMounted(() => {
  loadRecordTasks()
  
  // 设置定时刷新
  refreshTimer = setInterval(() => {
    loadRecordTasks()
  }, 5000) // 5秒刷新一次
})

onUnmounted(() => {
  if (refreshTimer) {
    clearInterval(refreshTimer)
  }
})
</script>

<style scoped>
.record-tasks {
  max-width: 100%;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.form-tip {
  font-size: 12px;
  color: #909399;
  margin-top: 5px;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}
</style>
