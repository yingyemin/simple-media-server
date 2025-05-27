<template>
  <div class="jt1078-management">
    <el-card>
      <template #header>
        <span>JT1078管理</span>
      </template>
      <el-tabs v-model="activeTab" type="border-card">
        <el-tab-pane label="服务器管理" name="server">
          <el-button type="primary" @click="openServer">开启服务器</el-button>
          <el-button type="warning" @click="closeServer">关闭服务器</el-button>
          <el-table :data="serverInfo" style="width: 100%; margin-top: 20px;">
            <el-table-column prop="name" label="服务器名称" />
            <el-table-column prop="port" label="端口" />
            <el-table-column prop="status" label="状态" />
            <el-table-column prop="connections" label="连接数" />
          </el-table>
        </el-tab-pane>
        <el-tab-pane label="对讲管理" name="talk">
          <el-button type="primary" @click="showTalkDialog">开始对讲</el-button>
          <el-table :data="talkSessions" style="width: 100%; margin-top: 20px;">
            <el-table-column prop="deviceId" label="设备ID" />
            <el-table-column prop="channel" label="通道" />
            <el-table-column prop="status" label="状态" />
            <el-table-column label="操作">
              <template #default="scope">
                <el-button size="small" @click="stopTalk(scope.row)">停止对讲</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>
      </el-tabs>
    </el-card>

    <!-- 对讲对话框 -->
    <el-dialog v-model="talkDialogVisible" title="开始对讲">
      <el-form :model="talkForm" label-width="100px">
        <el-form-item label="设备ID">
          <el-input v-model="talkForm.deviceId" />
        </el-form-item>
        <el-form-item label="通道">
          <el-input-number v-model="talkForm.channel" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="talkDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="startTalk">开始</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { jt1078API } from '@/api'

const activeTab = ref('server')
const talkDialogVisible = ref(false)
const serverInfo = ref([])
const talkSessions = ref([])

const talkForm = ref({
  deviceId: '',
  channel: 1
})

const openServer = async () => {
  try {
    await jt1078API.openServer({})
    ElMessage.success('服务器开启成功')
    loadData()
  } catch (error) {
    ElMessage.error('开启失败: ' + error.message)
  }
}

const closeServer = async () => {
  try {
    await jt1078API.closeServer({})
    ElMessage.success('服务器关闭成功')
    loadData()
  } catch (error) {
    ElMessage.error('关闭失败: ' + error.message)
  }
}

const showTalkDialog = () => {
  talkForm.value = { deviceId: '', channel: 1 }
  talkDialogVisible.value = true
}

const startTalk = async () => {
  try {
    await jt1078API.startTalk(talkForm.value)
    ElMessage.success('对讲开始成功')
    talkDialogVisible.value = false
    loadData()
  } catch (error) {
    ElMessage.error('开始对讲失败: ' + error.message)
  }
}

const stopTalk = async (session) => {
  try {
    await jt1078API.stopTalk({ deviceId: session.deviceId })
    ElMessage.success('对讲停止成功')
    loadData()
  } catch (error) {
    ElMessage.error('停止对讲失败: ' + error.message)
  }
}

const loadData = async () => {
  // 模拟数据加载
  serverInfo.value = [
    { name: 'JT1078 Server', port: 17000, status: '运行中', connections: 3 }
  ]
  talkSessions.value = []
}

onMounted(() => {
  loadData()
})
</script>
