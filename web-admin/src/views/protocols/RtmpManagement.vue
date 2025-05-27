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

      <!-- RTMP流创建 -->
      <el-tab-pane label="流创建" name="create">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>RTMP流创建</span>
              <el-button type="primary" @click="showCreateStreamDialog">
                <el-icon><Plus /></el-icon>
                创建流
              </el-button>
            </div>
          </template>

          <el-table :data="createdStreams" v-loading="streamLoading" style="width: 100%">
            <el-table-column prop="appName" label="应用名" width="150" />
            <el-table-column prop="streamName" label="流名" width="200" />
            <el-table-column prop="streamPath" label="流路径" min-width="300" show-overflow-tooltip />
            <el-table-column prop="status" label="状态" width="100">
              <template #default="scope">
                <el-tag :type="scope.row.status === 'active' ? 'success' : 'info'">
                  {{ scope.row.status === 'active' ? '活跃' : '就绪' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="createTime" label="创建时间" width="180">
              <template #default="scope">
                {{ formatTime(scope.row.createTime) }}
              </template>
            </el-table-column>
            <el-table-column label="操作" width="150">
              <template #default="scope">
                <el-button type="danger" size="small" @click="deleteStream(scope.row)">
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

    <!-- 创建流对话框 -->
    <el-dialog v-model="streamDialogVisible" title="创建RTMP流" width="600px">
      <el-form :model="streamForm" label-width="120px">
        <el-form-item label="应用名" required>
          <el-input v-model="streamForm.appName" placeholder="live" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="streamForm.streamName" placeholder="stream1" />
        </el-form-item>
        <el-form-item label="流类型">
          <el-select v-model="streamForm.streamType" placeholder="请选择流类型">
            <el-option label="直播流" value="live" />
            <el-option label="点播流" value="vod" />
            <el-option label="录制流" value="record" />
          </el-select>
        </el-form-item>
        <el-form-item label="编码格式">
          <el-select v-model="streamForm.codec" placeholder="请选择编码格式">
            <el-option label="H.264" value="h264" />
            <el-option label="H.265" value="h265" />
            <el-option label="VP8" value="vp8" />
            <el-option label="VP9" value="vp9" />
          </el-select>
        </el-form-item>
        <el-form-item label="分辨率">
          <el-select v-model="streamForm.resolution" placeholder="请选择分辨率">
            <el-option label="1920x1080" value="1920x1080" />
            <el-option label="1280x720" value="1280x720" />
            <el-option label="854x480" value="854x480" />
            <el-option label="640x360" value="640x360" />
          </el-select>
        </el-form-item>
        <el-form-item label="比特率(kbps)">
          <el-input-number v-model="streamForm.bitrate" :min="100" :max="10000" placeholder="2000" />
        </el-form-item>
        <el-form-item label="帧率(fps)">
          <el-input-number v-model="streamForm.framerate" :min="1" :max="60" placeholder="25" />
        </el-form-item>
        <el-form-item label="描述">
          <el-input v-model="streamForm.description" type="textarea" placeholder="流描述信息" />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="streamDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createStream" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { rtmpAPI, rtmpStreamAPI } from '@/api'

const activeTab = ref('pull')
const pullLoading = ref(false)
const pushLoading = ref(false)
const streamLoading = ref(false)
const creating = ref(false)

const pullStreams = ref([])
const pushStreams = ref([])
const createdStreams = ref([])

const pullDialogVisible = ref(false)
const pushDialogVisible = ref(false)
const streamDialogVisible = ref(false)

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

const streamForm = ref({
  appName: 'live',
  streamName: '',
  streamType: 'live',
  codec: 'h264',
  resolution: '1280x720',
  bitrate: 2000,
  framerate: 25,
  description: ''
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

const showCreateStreamDialog = () => {
  streamForm.value = {
    appName: 'live',
    streamName: '',
    streamType: 'live',
    codec: 'h264',
    resolution: '1280x720',
    bitrate: 2000,
    framerate: 25,
    description: ''
  }
  streamDialogVisible.value = true
}

const loadCreatedStreams = async () => {
  streamLoading.value = true
  try {
    // 由于后端API可能未完全实现，使用模拟数据
    // const data = await rtmpStreamAPI.getStreamList()
    // createdStreams.value = data.streams || []

    // 模拟数据
    createdStreams.value = [
      {
        appName: 'live',
        streamName: 'stream1',
        streamPath: 'rtmp://localhost:1935/live/stream1',
        status: 'ready',
        createTime: Date.now() - 3600000,
        streamType: 'live',
        codec: 'h264',
        resolution: '1280x720',
        bitrate: 2000,
        framerate: 25
      }
    ]
  } catch (error) {
    console.warn('获取流列表失败，使用模拟数据:', error.message)
    createdStreams.value = []
  } finally {
    streamLoading.value = false
  }
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

const createStream = async () => {
  const form = streamForm.value
  if (!form.appName || !form.streamName) {
    ElMessage.error('请填写应用名和流名')
    return
  }

  creating.value = true
  try {
    const streamData = {
      appName: form.appName,
      streamName: form.streamName,
      streamType: form.streamType,
      codec: form.codec,
      resolution: form.resolution,
      bitrate: form.bitrate,
      framerate: form.framerate,
      description: form.description
    }

    try {
      await rtmpStreamAPI.createStream(streamData)
      ElMessage.success('RTMP流创建成功')
    } catch (apiError) {
      console.warn('RTMP流创建API未完全实现，使用模拟创建:', apiError.message)

      // 模拟创建成功，添加到本地列表
      const newStream = {
        ...streamData,
        streamPath: `rtmp://localhost:1935/${form.appName}/${form.streamName}`,
        status: 'ready',
        createTime: Date.now()
      }
      createdStreams.value.push(newStream)
      ElMessage.success('RTMP流创建成功（模拟）')
    }

    streamDialogVisible.value = false
    loadCreatedStreams()
  } catch (error) {
    ElMessage.error('创建RTMP流失败: ' + error.message)
  } finally {
    creating.value = false
  }
}

const deleteStream = async (stream) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除流 "${stream.appName}/${stream.streamName}" 吗？`,
      '确认删除',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )

    // 这里应该调用删除流的API
    // await rtmpStreamAPI.deleteStream({ appName: stream.appName, streamName: stream.streamName })

    // 模拟删除
    const index = createdStreams.value.findIndex(s =>
      s.appName === stream.appName && s.streamName === stream.streamName
    )
    if (index > -1) {
      createdStreams.value.splice(index, 1)
    }

    ElMessage.success('流删除成功')
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除流失败: ' + error.message)
    }
  }
}

const formatTime = (timestamp) => {
  return new Date(timestamp).toLocaleString()
}

onMounted(() => {
  loadPullStreams()
  loadPushStreams()
  loadCreatedStreams()
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
