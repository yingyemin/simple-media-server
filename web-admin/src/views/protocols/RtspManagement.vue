<template>
  <div class="rtsp-management">
    <el-tabs v-model="activeTab" type="border-card">
      <!-- RTSP拉流管理 -->
      <el-tab-pane label="拉流管理" name="pull">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>RTSP拉流管理</span>
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
            <el-table-column prop="bitrate" label="码率" width="120">
              <template #default="scope">
                <span>{{ formatBitrate(scope.row.bitrate) }}</span>
              </template>
            </el-table-column>
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

      <!-- RTSP推流管理 -->
      <el-tab-pane label="推流管理" name="push">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>RTSP推流管理</span>
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
            <el-table-column prop="bitrate" label="码率" width="120">
              <template #default="scope">
                <span>{{ formatBitrate(scope.row.bitrate) }}</span>
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

      <!-- RTSP服务器管理 -->
      <el-tab-pane label="服务器管理" name="server">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>RTSP服务器管理</span>
              <el-button type="primary" @click="showCreateServerDialog">
                <el-icon><Plus /></el-icon>
                创建服务器
              </el-button>
            </div>
          </template>

          <el-table :data="rtspServers" v-loading="serverLoading" style="width: 100%">
            <el-table-column prop="name" label="服务器名称" width="150" />
            <el-table-column prop="ip" label="IP地址" width="150" />
            <el-table-column prop="port" label="端口" width="100" />
            <el-table-column prop="sslPort" label="SSL端口" width="100" />
            <el-table-column prop="status" label="状态" width="100">
              <template #default="scope">
                <el-tag :type="scope.row.status === 'running' ? 'success' : 'danger'">
                  {{ scope.row.status === 'running' ? '运行中' : '停止' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="clientCount" label="连接数" width="100" />
            <el-table-column label="操作" width="200">
              <template #default="scope">
                <el-button
                  :type="scope.row.status === 'running' ? 'warning' : 'success'"
                  size="small"
                  @click="toggleServer(scope.row)"
                >
                  {{ scope.row.status === 'running' ? '停止' : '启动' }}
                </el-button>
                <el-button type="danger" size="small" @click="deleteServer(scope.row)">
                  删除
                </el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-tab-pane>
    </el-tabs>

    <!-- 创建拉流对话框 -->
    <el-dialog v-model="pullDialogVisible" title="创建RTSP拉流" width="600px">
      <el-form :model="pullForm" label-width="100px" ref="pullFormRef">
        <el-form-item label="源URL" required>
          <el-input v-model="pullForm.url" placeholder="rtsp://example.com/stream" />
        </el-form-item>
        <el-form-item label="应用名" required>
          <el-input v-model="pullForm.appName" placeholder="live" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="pullForm.streamName" placeholder="stream1" />
        </el-form-item>
        <el-form-item label="传输协议">
          <el-select v-model="pullForm.transport" placeholder="请选择传输协议">
            <el-option label="UDP" value="udp" />
            <el-option label="TCP" value="tcp" />
            <el-option label="自动" value="auto" />
          </el-select>
        </el-form-item>
        <el-form-item label="超时时间(秒)">
          <el-input-number v-model="pullForm.timeout" :min="5" :max="300" />
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
    <el-dialog v-model="pushDialogVisible" title="创建RTSP推流" width="600px">
      <el-form :model="pushForm" label-width="100px" ref="pushFormRef">
        <el-form-item label="本地路径" required>
          <el-input v-model="pushForm.localPath" placeholder="/live/stream1" />
        </el-form-item>
        <el-form-item label="目标URL" required>
          <el-input v-model="pushForm.url" placeholder="rtsp://target.com/stream" />
        </el-form-item>
        <el-form-item label="应用名" required>
          <el-input v-model="pushForm.appName" placeholder="live" />
        </el-form-item>
        <el-form-item label="流名" required>
          <el-input v-model="pushForm.streamName" placeholder="stream1" />
        </el-form-item>
        <el-form-item label="传输协议">
          <el-select v-model="pushForm.transport" placeholder="请选择传输协议">
            <el-option label="UDP" value="udp" />
            <el-option label="TCP" value="tcp" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="pushDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createPush" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 创建服务器对话框 -->
    <el-dialog v-model="serverDialogVisible" title="创建RTSP服务器" width="600px">
      <el-form :model="serverForm" label-width="100px" ref="serverFormRef">
        <el-form-item label="服务器名称" required>
          <el-input v-model="serverForm.name" placeholder="RTSP Server 1" />
        </el-form-item>
        <el-form-item label="IP地址" required>
          <el-input v-model="serverForm.ip" placeholder="0.0.0.0" />
        </el-form-item>
        <el-form-item label="端口" required>
          <el-input-number v-model="serverForm.port" :min="1" :max="65535" />
        </el-form-item>
        <el-form-item label="SSL端口">
          <el-input-number v-model="serverForm.sslPort" :min="1" :max="65535" />
        </el-form-item>
        <el-form-item label="线程数">
          <el-input-number v-model="serverForm.threads" :min="1" :max="32" />
        </el-form-item>
        <el-form-item label="UDP端口范围">
          <el-row :gutter="10">
            <el-col :span="12">
              <el-input-number v-model="serverForm.udpPortMin" placeholder="最小端口" :min="1" :max="65535" />
            </el-col>
            <el-col :span="12">
              <el-input-number v-model="serverForm.udpPortMax" placeholder="最大端口" :min="1" :max="65535" />
            </el-col>
          </el-row>
        </el-form-item>
        <el-form-item label="启用认证">
          <el-switch v-model="serverForm.rtspAuth" />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="serverDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createServer" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { rtspAPI, rtspServerAPI } from '@/api'

const activeTab = ref('pull')
const pullLoading = ref(false)
const pushLoading = ref(false)
const serverLoading = ref(false)
const creating = ref(false)

const pullStreams = ref([])
const pushStreams = ref([])
const rtspServers = ref([])

// 对话框状态
const pullDialogVisible = ref(false)
const pushDialogVisible = ref(false)
const serverDialogVisible = ref(false)

// 表单数据
const pullForm = ref({
  url: '',
  appName: 'live',
  streamName: '',
  transport: 'auto',
  timeout: 30
})

const pushForm = ref({
  localPath: '',
  url: '',
  appName: 'live',
  streamName: '',
  transport: 'tcp'
})

const serverForm = ref({
  name: '',
  ip: '0.0.0.0',
  port: 554,
  sslPort: 1554,
  threads: 1,
  udpPortMin: 10000,
  udpPortMax: 20000,
  rtspAuth: false
})

const formatBitrate = (bitrate) => {
  if (!bitrate) return '0 bps'
  if (bitrate < 1024) return bitrate + ' bps'
  if (bitrate < 1024 * 1024) return (bitrate / 1024).toFixed(1) + ' Kbps'
  return (bitrate / 1024 / 1024).toFixed(1) + ' Mbps'
}

const loadPullStreams = async () => {
  pullLoading.value = true
  try {
    const data = await rtspAPI.getPlayList()
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
    const data = await rtspAPI.getPublishList()
    pushStreams.value = data.clients || []
  } catch (error) {
    ElMessage.error('获取推流列表失败: ' + error.message)
  } finally {
    pushLoading.value = false
  }
}

const loadRtspServers = async () => {
  serverLoading.value = true
  try {
    const data = await rtspServerAPI.getServerList()
    rtspServers.value = data.servers || []
  } catch (error) {
    ElMessage.error('获取服务器列表失败: ' + error.message)
    // 降级处理：使用模拟数据
    rtspServers.value = [
      {
        name: 'RTSP Server 1',
        ip: '0.0.0.0',
        port: 554,
        sslPort: 1554,
        status: 'running',
        clientCount: 0
      }
    ]
  } finally {
    serverLoading.value = false
  }
}

const showCreatePullDialog = () => {
  pullForm.value = {
    url: '',
    appName: 'live',
    streamName: '',
    transport: 'auto',
    timeout: 30
  }
  pullDialogVisible.value = true
}

const showCreatePushDialog = () => {
  pushForm.value = {
    localPath: '',
    url: '',
    appName: 'live',
    streamName: '',
    transport: 'tcp'
  }
  pushDialogVisible.value = true
}

const showCreateServerDialog = () => {
  serverForm.value = {
    name: '',
    ip: '0.0.0.0',
    port: 554,
    sslPort: 1554,
    threads: 1,
    udpPortMin: 10000,
    udpPortMax: 20000,
    rtspAuth: false
  }
  serverDialogVisible.value = true
}

const createPull = async () => {
  const form = pullForm.value
  if (!form.url || !form.appName || !form.streamName) {
    ElMessage.error('请填写完整信息')
    return
  }

  creating.value = true
  try {
    await rtspAPI.startPlay({
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
    await rtspAPI.startPublish({
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

const createServer = async () => {
  const form = serverForm.value
  if (!form.ip || !form.port) {
    ElMessage.error('请填写IP和端口信息')
    return
  }

  creating.value = true
  try {
    await rtspServerAPI.createServer({
      ip: form.ip,
      port: form.port,
      count: form.threads || 0
    })
    ElMessage.success('服务器创建成功')
    serverDialogVisible.value = false
    loadRtspServers()
  } catch (error) {
    ElMessage.error('创建服务器失败: ' + error.message)
  } finally {
    creating.value = false
  }
}

const stopPull = async (stream) => {
  try {
    await rtspAPI.stopPlay({ path: stream.path })
    ElMessage.success('拉流停止成功')
    loadPullStreams()
  } catch (error) {
    ElMessage.error('停止拉流失败: ' + error.message)
  }
}

const stopPush = async (stream) => {
  try {
    await rtspAPI.stopPublish({ url: stream.url })
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

    await rtspAPI.stopPlay({ path: stream.path })
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

    await rtspAPI.stopPublish({ url: stream.url })
    ElMessage.success('推流删除成功')
    loadPushStreams()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除推流失败: ' + error.message)
    }
  }
}

const toggleServer = async (server) => {
  try {
    if (server.status === 'running') {
      await rtspServerAPI.stopServer({
        port: server.port,
        count: 0
      })
      server.status = 'stopped'
      ElMessage.success('服务器停止成功')
    } else {
      await rtspServerAPI.createServer({
        ip: server.ip,
        port: server.port,
        count: 0
      })
      server.status = 'running'
      ElMessage.success('服务器启动成功')
    }
    loadRtspServers() // 重新加载服务器列表
  } catch (error) {
    ElMessage.error('操作失败: ' + error.message)
  }
}

const deleteServer = async (server) => {
  try {
    await ElMessageBox.confirm('确定要删除此服务器吗？', '确认删除', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
    })

    await rtspServerAPI.stopServer({
      port: server.port,
      count: 0
    })
    ElMessage.success('服务器删除成功')
    loadRtspServers()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除服务器失败: ' + error.message)
    }
  }
}

onMounted(() => {
  loadPullStreams()
  loadPushStreams()
  loadRtspServers()
})
</script>

<style scoped>
.rtsp-management {
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
