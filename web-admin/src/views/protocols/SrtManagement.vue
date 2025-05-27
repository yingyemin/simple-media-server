<template>
  <div class="srt-management">
    <el-card>
      <template #header>
        <span>SRT管理</span>
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
      </el-tabs>
    </el-card>

    <!-- 创建对话框 -->
    <el-dialog v-model="dialogVisible" :title="`创建SRT${dialogType === 'pull' ? '拉流' : '推流'}`">
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
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { srtAPI } from '@/api'

const activeTab = ref('pull')
const dialogVisible = ref(false)
const dialogType = ref('pull')
const pullStreams = ref([])
const pushStreams = ref([])

const form = ref({
  url: '',
  appName: '',
  streamName: ''
})

const showCreateDialog = (type) => {
  dialogType.value = type
  form.value = { url: '', appName: '', streamName: '' }
  dialogVisible.value = true
}

const createStream = async () => {
  try {
    if (dialogType.value === 'pull') {
      await srtAPI.startPull(form.value)
    } else {
      await srtAPI.startPush(form.value)
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
      await srtAPI.stopPull({ path: stream.path })
    } else {
      await srtAPI.stopPush({ url: stream.url })
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
      srtAPI.getPullList(),
      srtAPI.getPushList()
    ])
    pullStreams.value = pullData.clients || []
    pushStreams.value = pushData.clients || []
  } catch (error) {
    ElMessage.error('加载失败: ' + error.message)
  }
}

onMounted(() => {
  loadStreams()
})
</script>
