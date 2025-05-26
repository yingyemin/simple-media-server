<template>
  <div class="rtmp-management">
    <el-tabs v-model="activeTab" type="border-card">
      <!-- RTMP拉流管理 -->
      <el-tab-pane label="拉流管理" name="pull">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>RTMP拉流管理</span>
              <el-button type="primary" @click="showCreatePullDialog">
                <el-icon><Plus /></el-icon>
                创建拉流
              </el-button>
            </div>
          </template>

          <el-table :data="pullStreams" v-loading="pullLoading" style="width: 100%">
            <el-table-column prop="path" label="本地路径" min-width="200" />
            <el-table-column prop="url" label="源URL" min-width="300" show-overflow-tooltip />
            <el-table-column prop="status" label="状态" width="100">
              <template #default="scope">
                <el-tag :type="scope.row.status === 'active' ? 'success' : 'danger'">
                  {{ scope.row.status === 'active' ? '活跃' : '停止' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="playerCount" label="观看数" width="100" />
            <el-table-column label="操作" width="150">
              <template #default="scope">
                <el-button type="warning" size="small" @click="stopPull(scope.row)">
                  停止
                </el-button>
                <el-button type="danger" size="small" @click="deletePull(scope.row)">
                  删除
                </el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-tab-pane>

      <!-- RTMP推流管理 -->
      <el-tab-pane label="推流管理" name="push">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>RTMP推流管理</span>
              <el-button type="primary" @click="showCreatePushDialog">
                <el-icon><Plus /></el-icon>
                创建推流
              </el-button>
            </div>
          </template>

          <el-table :data="pushStreams" v-loading="pushLoading" style="width: 100%">
            <el-table-column prop="path" label="本地路径" min-width="200" />
            <el-table-column prop="url" label="目标URL" min-width="300" show-overflow-tooltip />
            <el-table-column prop="status" label="状态" width="100">
              <template #default="scope">
                <el-tag :type="scope.row.status === 'active' ? 'success' : 'danger'">
                  {{ scope.row.status === 'active' ? '推流中' : '停止' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="操作" width="150">
              <template #default="scope">
                <el-button type="warning" size="small" @click="stopPush(scope.row)">
                  停止
                </el-button>
                <el-button type="danger" size="small" @click="deletePush(scope.row)">
                  删除
                </el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-tab-pane>
    </el-tabs>

    <!-- 创建拉流对话框 -->
    <el-dialog v-model="pullDialogVisible" title="创建RTMP拉流" width="600px">
      <el-form :model="pullForm" label-width="100px">
        <el-form-item label="源URL" required>
          <el-input v-model="pullForm.url" placeholder="rtmp://example.com/live/stream" />
        </el-form-item>
        <el-form-item label="应用名" required>
          <el-input v-model="pullForm.appName" placeholder="live" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="pullForm.streamName" placeholder="stream1" />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="pullDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createPull" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 创建推流对话框 -->
    <el-dialog v-model="pushDialogVisible" title="创建RTMP推流" width="600px">
      <el-form :model="pushForm" label-width="100px">
        <el-form-item label="本地路径" required>
          <el-input v-model="pushForm.localPath" placeholder="/live/stream1" />
        </el-form-item>
        <el-form-item label="目标URL" required>
          <el-input v-model="pushForm.url" placeholder="rtmp://target.com/live/stream" />
        </el-form-item>
        <el-form-item label="应用名" required>
          <el-input v-model="pushForm.appName" placeholder="live" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="pushForm.streamName" placeholder="stream1" />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="pushDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createPush" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { rtmpAPI } from '@/api'

const activeTab = ref('pull')
const pullLoading = ref(false)
const pushLoading = ref(false)
const creating = ref(false)

const pullStreams = ref([])
const pushStreams = ref([])

const pullDialogVisible = ref(false)
const pushDialogVisible = ref(false)

const pullForm = ref({
  url: '',
  appName: 'live',
  streamName: ''
})

const pushForm = ref({
  localPath: '',
  url: '',
  appName: 'live',
  streamName: ''
})

const loadPullStreams = async () => {
  pullLoading.value = true
  try {
    const data = await rtmpAPI.getPlayList()
    pullStreams.value = data.clients || []
  } catch (error) {
    ElMessage.error('获取拉流列表失败: ' + error.message)
  } finally {
    pullLoading.value = false
  }
}

const loadPushStreams = async () => {
  pushLoading.value = true
  try {
    const data = await rtmpAPI.getPublishList()
    pushStreams.value = data.clients || []
  } catch (error) {
    ElMessage.error('获取推流列表失败: ' + error.message)
  } finally {
    pushLoading.value = false
  }
}

const showCreatePullDialog = () => {
  pullForm.value = {
    url: '',
    appName: 'live',
    streamName: ''
  }
  pullDialogVisible.value = true
}

const showCreatePushDialog = () => {
  pushForm.value = {
    localPath: '',
    url: '',
    appName: 'live',
    streamName: ''
  }
  pushDialogVisible.value = true
}

const createPull = async () => {
  const form = pullForm.value
  if (!form.url || !form.appName || !form.streamName) {
    ElMessage.error('请填写完整信息')
    return
  }
  
  creating.value = true
  try {
    await rtmpAPI.startPlay({
      url: form.url,
      appName: form.appName,
      streamName: form.streamName
    })
    ElMessage.success('拉流创建成功')
    pullDialogVisible.value = false
    loadPullStreams()
  } catch (error) {
    ElMessage.error('创建拉流失败: ' + error.message)
  } finally {
    creating.value = false
  }
}

const createPush = async () => {
  const form = pushForm.value
  if (!form.localPath || !form.url || !form.appName || !form.streamName) {
    ElMessage.error('请填写完整信息')
    return
  }
  
  creating.value = true
  try {
    await rtmpAPI.startPublish({
      url: form.url,
      appName: form.appName,
      streamName: form.streamName
    })
    ElMessage.success('推流创建成功')
    pushDialogVisible.value = false
    loadPushStreams()
  } catch (error) {
    ElMessage.error('创建推流失败: ' + error.message)
  } finally {
    creating.value = false
  }
}

const stopPull = async (stream) => {
  try {
    await rtmpAPI.stopPlay({ path: stream.path })
    ElMessage.success('拉流停止成功')
    loadPullStreams()
  } catch (error) {
    ElMessage.error('停止拉流失败: ' + error.message)
  }
}

const stopPush = async (stream) => {
  try {
    await rtmpAPI.stopPublish({ url: stream.url })
    ElMessage.success('推流停止成功')
    loadPushStreams()
  } catch (error) {
    ElMessage.error('停止推流失败: ' + error.message)
  }
}

const deletePull = async (stream) => {
  try {
    await ElMessageBox.confirm('确定要删除此拉流吗？', '确认删除', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
    })
    
    await rtmpAPI.stopPlay({ path: stream.path })
    ElMessage.success('拉流删除成功')
    loadPullStreams()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除拉流失败: ' + error.message)
    }
  }
}

const deletePush = async (stream) => {
  try {
    await ElMessageBox.confirm('确定要删除此推流吗？', '确认删除', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
    })
    
    await rtmpAPI.stopPublish({ url: stream.url })
    ElMessage.success('推流删除成功')
    loadPushStreams()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除推流失败: ' + error.message)
    }
  }
}

onMounted(() => {
  loadPullStreams()
  loadPushStreams()
})
</script>

<style scoped>
.rtmp-management {
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
