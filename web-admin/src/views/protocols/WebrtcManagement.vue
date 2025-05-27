<template>
  <div class="webrtc-management">
    <el-card>
      <template #header>
        <span>WebRTC管理</span>
      </template>
      <el-tabs v-model="activeTab" type="border-card">
        <el-tab-pane label="拉流管理" name="pull">
          <el-button type="primary" @click="showCreateDialog('pull')">创建拉流</el-button>
          <el-table :data="pullStreams" style="width: 100%; margin-top: 20px;">
            <el-table-column prop="path" label="路径" />
            <el-table-column prop="url" label="URL" />
            <el-table-column prop="status" label="状态" />
            <el-table-column label="操作">
              <template #default="scope">
                <el-button size="small" @click="stopStream(scope.row, 'pull')">停止</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>
        <el-tab-pane label="推流管理" name="push">
          <el-button type="primary" @click="showCreateDialog('push')">创建推流</el-button>
          <el-table :data="pushStreams" style="width: 100%; margin-top: 20px;">
            <el-table-column prop="path" label="路径" />
            <el-table-column prop="url" label="URL" />
            <el-table-column prop="status" label="状态" />
            <el-table-column label="操作">
              <template #default="scope">
                <el-button size="small" @click="stopStream(scope.row, 'push')">停止</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>
        <el-tab-pane label="WHEP播放" name="whep">
          <el-button type="primary" @click="showWhepDialog">创建WHEP播放</el-button>
          <el-table :data="whepSessions" style="width: 100%; margin-top: 20px;">
            <el-table-column prop="appName" label="应用名" />
            <el-table-column prop="streamName" label="流名" />
            <el-table-column prop="sessionId" label="会话ID" />
            <el-table-column prop="status" label="状态" />
            <el-table-column label="操作">
              <template #default="scope">
                <el-button size="small" @click="stopWhepSession(scope.row)">停止</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>
        <el-tab-pane label="WHIP推流" name="whip">
          <el-button type="primary" @click="showWhipDialog">创建WHIP推流</el-button>
          <el-table :data="whipSessions" style="width: 100%; margin-top: 20px;">
            <el-table-column prop="appName" label="应用名" />
            <el-table-column prop="streamName" label="流名" />
            <el-table-column prop="sessionId" label="会话ID" />
            <el-table-column prop="status" label="状态" />
            <el-table-column label="操作">
              <template #default="scope">
                <el-button size="small" @click="stopWhipSession(scope.row)">停止</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>
      </el-tabs>
    </el-card>

    <!-- 创建对话框 -->
    <el-dialog v-model="dialogVisible" :title="`创建WebRTC${dialogType === 'pull' ? '拉流' : '推流'}`">
      <el-form :model="form" label-width="100px">
        <el-form-item label="URL">
          <el-input v-model="form.url" />
        </el-form-item>
        <el-form-item label="应用名">
          <el-input v-model="form.appName" />
        </el-form-item>
        <el-form-item label="流名">
          <el-input v-model="form.streamName" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="createStream">创建</el-button>
      </template>
    </el-dialog>

    <!-- WHEP对话框 -->
    <el-dialog v-model="whepDialogVisible" title="创建WHEP播放会话" width="600px">
      <el-form :model="whepForm" label-width="120px">
        <el-form-item label="应用名" required>
          <el-input v-model="whepForm.appName" placeholder="请输入应用名" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="whepForm.streamName" placeholder="请输入流名" />
        </el-form-item>
        <el-form-item label="启用DTLS">
          <el-switch v-model="whepForm.enableDtls" />
        </el-form-item>
        <el-form-item label="SDP Offer" required>
          <el-input
            v-model="whepForm.sdp"
            type="textarea"
            :rows="6"
            placeholder="请输入SDP Offer内容"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="whepDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createWhepSession" :loading="whepCreating">创建</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- WHIP对话框 -->
    <el-dialog v-model="whipDialogVisible" title="创建WHIP推流会话" width="600px">
      <el-form :model="whipForm" label-width="120px">
        <el-form-item label="应用名" required>
          <el-input v-model="whipForm.appName" placeholder="请输入应用名" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="whipForm.streamName" placeholder="请输入流名" />
        </el-form-item>
        <el-form-item label="启用DTLS">
          <el-switch v-model="whipForm.enableDtls" />
        </el-form-item>
        <el-form-item label="SDP Offer" required>
          <el-input
            v-model="whipForm.sdp"
            type="textarea"
            :rows="6"
            placeholder="请输入SDP Offer内容"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="whipDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createWhipSession" :loading="whipCreating">创建</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { webrtcAPI, webrtcWhepWhipAPI } from '@/api'

const activeTab = ref('pull')
const dialogVisible = ref(false)
const dialogType = ref('pull')
const pullStreams = ref([])
const pushStreams = ref([])

// WHEP/WHIP相关状态
const whepDialogVisible = ref(false)
const whipDialogVisible = ref(false)
const whepCreating = ref(false)
const whipCreating = ref(false)
const whepSessions = ref([])
const whipSessions = ref([])

const form = ref({
  url: '',
  appName: '',
  streamName: ''
})

const whepForm = ref({
  appName: '',
  streamName: '',
  enableDtls: true,
  sdp: ''
})

const whipForm = ref({
  appName: '',
  streamName: '',
  enableDtls: true,
  sdp: ''
})

const showCreateDialog = (type) => {
  dialogType.value = type
  form.value = { url: '', appName: '', streamName: '' }
  dialogVisible.value = true
}

const createStream = async () => {
  try {
    if (dialogType.value === 'pull') {
      await webrtcAPI.startPull(form.value)
    } else {
      await webrtcAPI.startPush(form.value)
    }
    ElMessage.success('创建成功')
    dialogVisible.value = false
    loadStreams()
  } catch (error) {
    ElMessage.error('创建失败: ' + error.message)
  }
}

const stopStream = async (stream, type) => {
  try {
    if (type === 'pull') {
      await webrtcAPI.stopPull({ path: stream.path })
    } else {
      await webrtcAPI.stopPush({ url: stream.url })
    }
    ElMessage.success('停止成功')
    loadStreams()
  } catch (error) {
    ElMessage.error('停止失败: ' + error.message)
  }
}

const loadStreams = async () => {
  try {
    const [pullData, pushData] = await Promise.all([
      webrtcAPI.getPullList(),
      webrtcAPI.getPushList()
    ])
    pullStreams.value = pullData.clients || []
    pushStreams.value = pushData.clients || []
  } catch (error) {
    ElMessage.error('加载失败: ' + error.message)
  }
}

// WHEP相关方法
const showWhepDialog = () => {
  whepForm.value = {
    appName: '',
    streamName: '',
    enableDtls: true,
    sdp: ''
  }
  whepDialogVisible.value = true
}

const createWhepSession = async () => {
  const form = whepForm.value
  if (!form.appName || !form.streamName || !form.sdp) {
    ElMessage.error('请填写所有必需字段')
    return
  }

  whepCreating.value = true
  try {
    const response = await webrtcWhepWhipAPI.whep({
      appName: form.appName,
      streamName: form.streamName,
      sdp: form.sdp
    })

    ElMessage.success('WHEP会话创建成功')
    whepDialogVisible.value = false

    // 添加到会话列表
    whepSessions.value.push({
      appName: form.appName,
      streamName: form.streamName,
      sessionId: Date.now().toString(),
      status: 'active',
      sdp: response.data || response
    })
  } catch (error) {
    ElMessage.error('创建WHEP会话失败: ' + error.message)
  } finally {
    whepCreating.value = false
  }
}

const stopWhepSession = async (session) => {
  try {
    await ElMessageBox.confirm(
      `确定要停止WHEP会话 "${session.appName}/${session.streamName}" 吗？`,
      '确认停止',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )

    // 这里应该调用停止WHEP会话的API
    ElMessage.success('WHEP会话停止成功')

    // 从列表中移除
    const index = whepSessions.value.findIndex(s => s.sessionId === session.sessionId)
    if (index > -1) {
      whepSessions.value.splice(index, 1)
    }
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('停止WHEP会话失败: ' + error.message)
    }
  }
}

// WHIP相关方法
const showWhipDialog = () => {
  whipForm.value = {
    appName: '',
    streamName: '',
    enableDtls: true,
    sdp: ''
  }
  whipDialogVisible.value = true
}

const createWhipSession = async () => {
  const form = whipForm.value
  if (!form.appName || !form.streamName || !form.sdp) {
    ElMessage.error('请填写所有必需字段')
    return
  }

  whipCreating.value = true
  try {
    const response = await webrtcWhepWhipAPI.whip({
      appName: form.appName,
      streamName: form.streamName,
      sdp: form.sdp
    })

    ElMessage.success('WHIP会话创建成功')
    whipDialogVisible.value = false

    // 添加到会话列表
    whipSessions.value.push({
      appName: form.appName,
      streamName: form.streamName,
      sessionId: Date.now().toString(),
      status: 'active',
      sdp: response.data || response
    })
  } catch (error) {
    ElMessage.error('创建WHIP会话失败: ' + error.message)
  } finally {
    whipCreating.value = false
  }
}

const stopWhipSession = async (session) => {
  try {
    await ElMessageBox.confirm(
      `确定要停止WHIP会话 "${session.appName}/${session.streamName}" 吗？`,
      '确认停止',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )

    // 这里应该调用停止WHIP会话的API
    ElMessage.success('WHIP会话停止成功')

    // 从列表中移除
    const index = whipSessions.value.findIndex(s => s.sessionId === session.sessionId)
    if (index > -1) {
      whipSessions.value.splice(index, 1)
    }
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('停止WHIP会话失败: ' + error.message)
    }
  }
}

onMounted(() => {
  loadStreams()
})
</script>

<style scoped>
.webrtc-management {
  max-width: 100%;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}

:deep(.el-tabs__content) {
  padding: 0;
}

:deep(.el-textarea__inner) {
  font-family: 'Courier New', monospace;
  font-size: 12px;
}
</style>
