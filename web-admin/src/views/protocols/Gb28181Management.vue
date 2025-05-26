<template>
  <div class="gb28181-management">
    <el-card>
      <template #header>
        <span>GB28181管理</span>
      </template>
      <el-tabs v-model="activeTab" type="border-card">
        <el-tab-pane label="接收器管理" name="receiver">
          <el-button type="primary" @click="showCreateDialog('receiver')">创建接收器</el-button>
          <el-table :data="receivers" style="width: 100%; margin-top: 20px;">
            <el-table-column prop="ssrc" label="SSRC" />
            <el-table-column prop="active" label="主动模式" />
            <el-table-column prop="socketType" label="Socket类型" />
            <el-table-column prop="status" label="状态" />
            <el-table-column label="操作">
              <template #default="scope">
                <el-button size="small" @click="stopReceiver(scope.row)">停止</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>
        <el-tab-pane label="发送器管理" name="sender">
          <el-button type="primary" @click="showCreateDialog('sender')">创建发送器</el-button>
          <el-table :data="senders" style="width: 100%; margin-top: 20px;">
            <el-table-column prop="ssrc" label="SSRC" />
            <el-table-column prop="ip" label="目标IP" />
            <el-table-column prop="port" label="目标端口" />
            <el-table-column prop="status" label="状态" />
            <el-table-column label="操作">
              <template #default="scope">
                <el-button size="small" @click="stopSender(scope.row)">停止</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>
      </el-tabs>
    </el-card>

    <!-- 创建对话框 -->
    <el-dialog v-model="dialogVisible" :title="`创建GB28181${dialogType === 'receiver' ? '接收器' : '发送器'}`">
      <el-form :model="form" label-width="100px">
        <el-form-item label="SSRC">
          <el-input v-model="form.ssrc" />
        </el-form-item>
        <el-form-item label="主动模式" v-if="dialogType === 'receiver'">
          <el-switch v-model="form.active" />
        </el-form-item>
        <el-form-item label="Socket类型">
          <el-select v-model="form.socketType">
            <el-option label="UDP" value="udp" />
            <el-option label="TCP" value="tcp" />
          </el-select>
        </el-form-item>
        <el-form-item label="目标IP" v-if="dialogType === 'sender'">
          <el-input v-model="form.ip" />
        </el-form-item>
        <el-form-item label="目标端口" v-if="dialogType === 'sender'">
          <el-input-number v-model="form.port" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="createGB28181">创建</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { gb28181API } from '@/api'

const activeTab = ref('receiver')
const dialogVisible = ref(false)
const dialogType = ref('receiver')
const receivers = ref([])
const senders = ref([])

const form = ref({
  ssrc: '',
  active: false,
  socketType: 'udp',
  ip: '',
  port: 0
})

const showCreateDialog = (type) => {
  dialogType.value = type
  form.value = {
    ssrc: '',
    active: false,
    socketType: 'udp',
    ip: '',
    port: 0
  }
  dialogVisible.value = true
}

const createGB28181 = async () => {
  try {
    if (dialogType.value === 'receiver') {
      await gb28181API.createReceiver(form.value)
    } else {
      await gb28181API.createSender(form.value)
    }
    ElMessage.success('创建成功')
    dialogVisible.value = false
    loadData()
  } catch (error) {
    ElMessage.error('创建失败: ' + error.message)
  }
}

const stopReceiver = async (receiver) => {
  try {
    await gb28181API.stopReceiver({ ssrc: receiver.ssrc })
    ElMessage.success('停止成功')
    loadData()
  } catch (error) {
    ElMessage.error('停止失败: ' + error.message)
  }
}

const stopSender = async (sender) => {
  try {
    await gb28181API.stopSender({ ssrc: sender.ssrc })
    ElMessage.success('停止成功')
    loadData()
  } catch (error) {
    ElMessage.error('停止失败: ' + error.message)
  }
}

const loadData = async () => {
  // 模拟数据加载
  receivers.value = []
  senders.value = []
}

onMounted(() => {
  loadData()
})
</script>
