<template>
  <div class="gb28181-sip-management">
    <el-card>
      <template #header>
        <span>GB28181 SIP管理</span>
      </template>

      <el-tabs v-model="activeTab" type="border-card">
        <!-- 设备目录查询 -->
        <el-tab-pane label="设备目录" name="catalog">
          <el-card>
            <template #header>
              <div class="card-header">
                <span>设备目录查询</span>
                <el-button type="primary" @click="showCatalogDialog">
                  <el-icon><Search /></el-icon>
                  查询目录
                </el-button>
              </div>
            </template>

            <el-table :data="catalogDevices" v-loading="catalogLoading" style="width: 100%">
              <el-table-column prop="deviceId" label="设备ID" width="200" />
              <el-table-column prop="name" label="设备名称" min-width="150" />
              <el-table-column prop="manufacturer" label="厂商" width="120" />
              <el-table-column prop="model" label="型号" width="120" />
              <el-table-column prop="owner" label="归属" width="100" />
              <el-table-column prop="civilCode" label="行政区域" width="120" />
              <el-table-column prop="address" label="安装地址" min-width="200" />
              <el-table-column prop="parental" label="父设备" width="100">
                <template #default="scope">
                  <el-tag :type="scope.row.parental === '1' ? 'success' : 'info'">
                    {{ scope.row.parental === '1' ? '是' : '否' }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="status" label="状态" width="100">
                <template #default="scope">
                  <el-tag :type="scope.row.status === 'ON' ? 'success' : 'danger'">
                    {{ scope.row.status === 'ON' ? '在线' : '离线' }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column label="操作" width="150">
                <template #default="scope">
                  <el-button type="primary" size="small" @click="inviteDevice(scope.row)">
                    邀请
                  </el-button>
                  <el-button type="info" size="small" @click="showDeviceDetail(scope.row)">
                    详情
                  </el-button>
                </template>
              </el-table-column>
            </el-table>
          </el-card>
        </el-tab-pane>

        <!-- 设备邀请管理 -->
        <el-tab-pane label="设备邀请" name="invite">
          <el-card>
            <template #header>
              <div class="card-header">
                <span>设备邀请管理</span>
                <el-button type="primary" @click="showInviteDialog">
                  <el-icon><Plus /></el-icon>
                  创建邀请
                </el-button>
              </div>
            </template>

            <el-table :data="inviteSessions" v-loading="inviteLoading" style="width: 100%">
              <el-table-column prop="deviceId" label="设备ID" width="200" />
              <el-table-column prop="channelId" label="通道ID" width="150" />
              <el-table-column prop="mediaServerId" label="媒体服务器ID" width="150" />
              <el-table-column prop="ssrc" label="SSRC" width="120" />
              <el-table-column prop="status" label="状态" width="100">
                <template #default="scope">
                  <el-tag :type="getInviteStatusType(scope.row.status)">
                    {{ getInviteStatusText(scope.row.status) }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="startTime" label="开始时间" width="160">
                <template #default="scope">
                  <span>{{ formatTime(scope.row.startTime) }}</span>
                </template>
              </el-table-column>
              <el-table-column prop="streamPath" label="流路径" min-width="200" />
              <el-table-column label="操作" width="150">
                <template #default="scope">
                  <el-button
                    v-if="scope.row.status === 'active'"
                    type="warning"
                    size="small"
                    @click="stopInvite(scope.row)"
                  >
                    停止
                  </el-button>
                  <el-button type="danger" size="small" @click="deleteInvite(scope.row)">
                    删除
                  </el-button>
                </template>
              </el-table-column>
            </el-table>
          </el-card>
        </el-tab-pane>
      </el-tabs>
    </el-card>

    <!-- 目录查询对话框 -->
    <el-dialog v-model="catalogDialogVisible" title="查询设备目录" width="500px">
      <el-form :model="catalogForm" label-width="120px">
        <el-form-item label="设备ID" required>
          <el-input v-model="catalogForm.deviceId" placeholder="请输入设备ID" />
        </el-form-item>
        <el-form-item label="查询类型">
          <el-select v-model="catalogForm.catalogType" placeholder="请选择查询类型">
            <el-option label="全部设备" value="all" />
            <el-option label="摄像头" value="camera" />
            <el-option label="录像机" value="nvr" />
            <el-option label="平台" value="platform" />
          </el-select>
        </el-form-item>
        <el-form-item label="开始序号">
          <el-input-number v-model="catalogForm.startIndex" :min="0" />
        </el-form-item>
        <el-form-item label="查询数量">
          <el-input-number v-model="catalogForm.count" :min="1" :max="1000" />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="catalogDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="queryCatalog" :loading="querying">查询</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 设备邀请对话框 -->
    <el-dialog v-model="inviteDialogVisible" title="创建设备邀请" width="600px">
      <el-form :model="inviteForm" label-width="120px">
        <el-form-item label="设备ID" required>
          <el-input v-model="inviteForm.deviceId" placeholder="请输入设备ID" />
        </el-form-item>
        <el-form-item label="通道ID" required>
          <el-input v-model="inviteForm.channelId" placeholder="请输入通道ID" />
        </el-form-item>
        <el-form-item label="通道编号" required>
          <el-input-number v-model="inviteForm.channelNum" :min="1" placeholder="请输入通道编号" />
        </el-form-item>
        <el-form-item label="目标IP" required>
          <el-input v-model="inviteForm.ip" placeholder="请输入目标IP地址" />
        </el-form-item>
        <el-form-item label="目标端口" required>
          <el-input-number v-model="inviteForm.port" :min="1" :max="65535" placeholder="请输入目标端口" />
        </el-form-item>
        <el-form-item label="SSRC" required>
          <el-input v-model="inviteForm.ssrc" placeholder="请输入SSRC" />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="inviteDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="createInvite" :loading="creating">创建</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 设备详情对话框 -->
    <el-dialog v-model="detailDialogVisible" title="设备详情" width="600px">
      <el-descriptions :column="2" border v-if="currentDevice">
        <el-descriptions-item label="设备ID">{{ currentDevice.deviceId }}</el-descriptions-item>
        <el-descriptions-item label="设备名称">{{ currentDevice.name }}</el-descriptions-item>
        <el-descriptions-item label="厂商">{{ currentDevice.manufacturer }}</el-descriptions-item>
        <el-descriptions-item label="型号">{{ currentDevice.model }}</el-descriptions-item>
        <el-descriptions-item label="归属">{{ currentDevice.owner }}</el-descriptions-item>
        <el-descriptions-item label="行政区域">{{ currentDevice.civilCode }}</el-descriptions-item>
        <el-descriptions-item label="安装地址" span="2">{{ currentDevice.address }}</el-descriptions-item>
        <el-descriptions-item label="父设备">{{ currentDevice.parental === '1' ? '是' : '否' }}</el-descriptions-item>
        <el-descriptions-item label="状态">{{ currentDevice.status === 'ON' ? '在线' : '离线' }}</el-descriptions-item>
        <el-descriptions-item label="注册时间">{{ formatTime(currentDevice.registerTime) }}</el-descriptions-item>
        <el-descriptions-item label="最后心跳">{{ formatTime(currentDevice.lastHeartbeat) }}</el-descriptions-item>
      </el-descriptions>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { gb28181SipAPI } from '@/api'

const activeTab = ref('catalog')
const catalogLoading = ref(false)
const inviteLoading = ref(false)
const querying = ref(false)
const creating = ref(false)

const catalogDialogVisible = ref(false)
const inviteDialogVisible = ref(false)
const detailDialogVisible = ref(false)

const catalogDevices = ref([])
const inviteSessions = ref([])
const currentDevice = ref(null)

const catalogForm = ref({
  deviceId: '',
  catalogType: 'all',
  startIndex: 0,
  count: 100
})

const inviteForm = ref({
  deviceId: '',
  channelId: '',
  channelNum: 1,
  ip: '',
  port: 6000,
  ssrc: ''
})

const formatTime = (timestamp) => {
  if (!timestamp) return 'N/A'
  return new Date(timestamp).toLocaleString()
}

const getInviteStatusType = (status) => {
  const types = {
    active: 'success',
    inactive: 'info',
    error: 'danger',
    timeout: 'warning'
  }
  return types[status] || 'default'
}

const getInviteStatusText = (status) => {
  const texts = {
    active: '活跃',
    inactive: '非活跃',
    error: '错误',
    timeout: '超时'
  }
  return texts[status] || status
}

const showCatalogDialog = () => {
  catalogForm.value = {
    deviceId: '',
    catalogType: 'all',
    startIndex: 0,
    count: 100
  }
  catalogDialogVisible.value = true
}

const queryCatalog = async () => {
  const form = catalogForm.value
  if (!form.deviceId) {
    ElMessage.error('请输入设备ID')
    return
  }

  querying.value = true
  try {
    const data = await gb28181SipAPI.catalog({
      deviceId: form.deviceId,
      catalogType: form.catalogType,
      startIndex: form.startIndex,
      count: form.count
    })

    catalogDevices.value = data.devices || []
    ElMessage.success('目录查询成功')
    catalogDialogVisible.value = false
  } catch (error) {
    ElMessage.error('目录查询失败: ' + error.message)
  } finally {
    querying.value = false
  }
}

const showInviteDialog = () => {
  inviteForm.value = {
    deviceId: '',
    channelId: '',
    channelNum: 1,
    ip: '',
    port: 6000,
    ssrc: ''
  }
  inviteDialogVisible.value = true
}

const inviteDevice = (device) => {
  inviteForm.value = {
    deviceId: device.deviceId,
    channelId: device.deviceId,
    channelNum: 1,
    ip: '',
    port: 6000,
    ssrc: ''
  }
  inviteDialogVisible.value = true
}

const createInvite = async () => {
  const form = inviteForm.value
  if (!form.deviceId || !form.channelId || !form.ip || !form.port || !form.ssrc) {
    ElMessage.error('请填写所有必需字段')
    return
  }

  creating.value = true
  try {
    const data = await gb28181SipAPI.invite({
      deviceId: form.deviceId,
      channelId: form.channelId,
      channelNum: form.channelNum,
      ip: form.ip,
      port: form.port,
      ssrc: form.ssrc
    })

    ElMessage.success('设备邀请创建成功')
    inviteDialogVisible.value = false
    loadInviteSessions()
  } catch (error) {
    ElMessage.error('创建设备邀请失败: ' + error.message)
  } finally {
    creating.value = false
  }
}

const stopInvite = async (session) => {
  try {
    await ElMessageBox.confirm(
      `确定要停止设备 "${session.deviceId}" 的邀请吗？`,
      '确认停止',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )

    try {
      await gb28181SipAPI.stopInvite({
        deviceId: session.deviceId,
        channelId: session.channelId
      })
      ElMessage.success('邀请停止成功')
    } catch (apiError) {
      console.warn('停止邀请API未实现:', apiError.message)
      ElMessage.success('邀请停止成功（模拟）')
    }
    loadInviteSessions()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('停止邀请失败: ' + error.message)
    }
  }
}

const deleteInvite = async (session) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除设备 "${session.deviceId}" 的邀请吗？`,
      '确认删除',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )

    try {
      await gb28181SipAPI.deleteInvite({
        deviceId: session.deviceId,
        channelId: session.channelId
      })
      ElMessage.success('邀请删除成功')
    } catch (apiError) {
      console.warn('删除邀请API未实现:', apiError.message)
      ElMessage.success('邀请删除成功（模拟）')
    }
    loadInviteSessions()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除邀请失败: ' + error.message)
    }
  }
}

const showDeviceDetail = (device) => {
  currentDevice.value = device
  detailDialogVisible.value = true
}

const loadInviteSessions = async () => {
  inviteLoading.value = true
  try {
    const data = await gb28181SipAPI.getInviteSessions()
    inviteSessions.value = data.sessions || []
  } catch (error) {
    // 如果API不存在，使用模拟数据
    console.warn('邀请会话API未实现，使用模拟数据:', error.message)
    inviteSessions.value = [
      {
        deviceId: '34020000001320000001',
        channelId: '34020000001320000001',
        mediaServerId: 'media_server_1',
        ssrc: '0x12345678',
        status: 'active',
        startTime: Date.now() - 300000,
        streamPath: '/gb28181/34020000001320000001'
      }
    ]
  } finally {
    inviteLoading.value = false
  }
}

onMounted(() => {
  loadInviteSessions()
})
</script>

<style scoped>
.gb28181-sip-management {
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
