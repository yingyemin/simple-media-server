<template>
  <div class="clients-list">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>客户端管理</span>
          <el-button @click="refreshClients" :loading="loading">
            <el-icon><Refresh /></el-icon>
            刷新
          </el-button>
        </div>
      </template>

      <el-table :data="clients" v-loading="loading" style="width: 100%">
        <el-table-column prop="ip" label="IP地址" width="150" />
        <el-table-column prop="port" label="端口" width="100" />
        <el-table-column prop="protocol" label="协议" width="100">
          <template #default="scope">
            <el-tag>{{ scope.row.protocol.toUpperCase() }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="stream" label="流路径" min-width="200" />
        <el-table-column prop="bitrate" label="码率" width="120">
          <template #default="scope">
            <span>{{ formatBitrate(scope.row.bitrate) }}</span>
          </template>
        </el-table-column>
        <el-table-column prop="connectTime" label="连接时间" width="160">
          <template #default="scope">
            <span>{{ formatTime(scope.row.connectTime) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="100">
          <template #default="scope">
            <el-button type="danger" size="small" @click="disconnectClient(scope.row)">
              断开
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { streamAPI } from '@/api'

const loading = ref(false)
const clients = ref([])

const formatBitrate = (bitrate) => {
  if (!bitrate) return '0 bps'
  if (bitrate < 1024) return bitrate + ' bps'
  if (bitrate < 1024 * 1024) return (bitrate / 1024).toFixed(1) + ' Kbps'
  return (bitrate / 1024 / 1024).toFixed(1) + ' Mbps'
}

const formatTime = (timestamp) => {
  if (!timestamp) return 'N/A'
  return new Date(timestamp).toLocaleString()
}

const refreshClients = async () => {
  loading.value = true
  try {
    // 模拟获取客户端列表
    await new Promise(resolve => setTimeout(resolve, 1000))
    clients.value = [
      {
        ip: '192.168.1.100',
        port: 12345,
        protocol: 'rtsp',
        stream: '/live/stream1',
        bitrate: 1024000,
        connectTime: Date.now() - 300000
      },
      {
        ip: '192.168.1.101',
        port: 12346,
        protocol: 'rtmp',
        stream: '/live/stream2',
        bitrate: 2048000,
        connectTime: Date.now() - 600000
      }
    ]
  } catch (error) {
    ElMessage.error('获取客户端列表失败: ' + error.message)
  } finally {
    loading.value = false
  }
}

const disconnectClient = async (client) => {
  try {
    await ElMessageBox.confirm(
      `确定要断开客户端 ${client.ip}:${client.port} 吗？`,
      '确认断开',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )
    
    await streamAPI.closeClient({ ip: client.ip, port: client.port })
    ElMessage.success('客户端断开成功')
    refreshClients()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('断开客户端失败: ' + error.message)
    }
  }
}

onMounted(() => {
  refreshClients()
})
</script>

<style scoped>
.clients-list {
  max-width: 100%;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>
