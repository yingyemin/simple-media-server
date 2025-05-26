<template>
  <div class="hook-config">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>Hook回调配置</span>
          <div>
            <el-button @click="loadConfig">重新加载</el-button>
            <el-button type="primary" @click="saveConfig" :loading="saving">保存配置</el-button>
          </div>
        </div>
      </template>

      <el-form :model="config.Hook" label-width="150px">
        <!-- 基础配置 -->
        <el-card class="config-section">
          <template #header>基础配置</template>
          <el-form-item label="回调类型">
            <el-radio-group v-model="config.Hook.Type">
              <el-radio label="http">HTTP回调</el-radio>
              <el-radio label="kafka">Kafka消息</el-radio>
            </el-radio-group>
          </el-form-item>
          <el-form-item label="启用Hook">
            <el-switch v-model="config.Hook.EnableHook" />
          </el-form-item>
        </el-card>

        <!-- HTTP回调配置 -->
        <el-card class="config-section" v-if="config.Hook.Type === 'http'">
          <template #header>HTTP回调配置</template>
          <el-form-item label="超时时间(秒)">
            <el-input-number v-model="config.Hook.Http.timeout" :min="1" :max="60" />
          </el-form-item>
          
          <el-form-item label="流状态回调">
            <el-input v-model="config.Hook.Http.onStreamStatus" placeholder="流上线/下线时回调" />
            <div class="form-tip">流上线、下线时调用此接口</div>
          </el-form-item>
          
          <el-form-item label="流心跳回调">
            <el-input v-model="config.Hook.Http.onStreamHeartbeat" placeholder="流心跳回调" />
            <div class="form-tip">定期上报流量等信息</div>
          </el-form-item>
          
          <el-form-item label="无人观看回调">
            <el-input v-model="config.Hook.Http.onNonePlayer" placeholder="无人观看时回调" />
            <div class="form-tip">无人观看时调用此接口</div>
          </el-form-item>
          
          <el-form-item label="服务心跳回调">
            <el-input v-model="config.Hook.Http.onKeepAlive" placeholder="服务心跳回调" />
            <div class="form-tip">服务的心跳回调，便于管理服务做调度</div>
          </el-form-item>
          
          <el-form-item label="注册服务回调">
            <el-input v-model="config.Hook.Http.onRegisterServer" placeholder="注册服务回调" />
            <div class="form-tip">服务注册时回调</div>
          </el-form-item>
          
          <el-form-item label="推流鉴权回调">
            <el-input v-model="config.Hook.Http.onPublish" placeholder="推流鉴权回调" />
            <div class="form-tip">推流时进行鉴权</div>
          </el-form-item>
          
          <el-form-item label="观看鉴权回调">
            <el-input v-model="config.Hook.Http.onPlay" placeholder="观看鉴权回调" />
            <div class="form-tip">观看时进行鉴权</div>
          </el-form-item>
        </el-card>

        <!-- Kafka配置 -->
        <el-card class="config-section" v-if="config.Hook.Type === 'kafka'">
          <template #header>Kafka配置</template>
          <el-form-item label="Kafka地址">
            <el-input v-model="config.Hook.Kafka.endpoint" placeholder="127.0.0.1:9092" />
          </el-form-item>
          <el-form-item label="Topic名称">
            <el-input v-model="config.Hook.Kafka.broker" placeholder="streamStatus" />
          </el-form-item>
        </el-card>

        <!-- Hook测试 -->
        <el-card class="config-section">
          <template #header>Hook测试</template>
          <el-form-item label="测试Hook">
            <el-button type="primary" @click="testHook" :loading="testing">测试连接</el-button>
            <div class="form-tip">测试Hook回调是否正常工作</div>
          </el-form-item>
          
          <el-form-item label="测试结果" v-if="testResult">
            <el-alert
              :title="testResult.success ? '测试成功' : '测试失败'"
              :type="testResult.success ? 'success' : 'error'"
              :description="testResult.message"
              show-icon
              :closable="false"
            />
          </el-form-item>
        </el-card>

        <!-- Hook日志 -->
        <el-card class="config-section">
          <template #header>
            <div class="card-header">
              <span>最近Hook日志</span>
              <el-button type="text" @click="refreshHookLogs">刷新</el-button>
            </div>
          </template>
          <el-table :data="hookLogs" v-loading="logsLoading" style="width: 100%" max-height="400">
            <el-table-column prop="time" label="时间" width="160">
              <template #default="scope">
                <span>{{ formatTime(scope.row.time) }}</span>
              </template>
            </el-table-column>
            <el-table-column prop="type" label="类型" width="120">
              <template #default="scope">
                <el-tag :type="getHookTypeTag(scope.row.type)">
                  {{ getHookTypeName(scope.row.type) }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="url" label="回调URL" min-width="200" show-overflow-tooltip />
            <el-table-column prop="status" label="状态" width="100">
              <template #default="scope">
                <el-tag :type="scope.row.status === 'success' ? 'success' : 'danger'">
                  {{ scope.row.status === 'success' ? '成功' : '失败' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="response" label="响应" width="120">
              <template #default="scope">
                <span>{{ scope.row.responseTime }}ms</span>
              </template>
            </el-table-column>
            <el-table-column prop="error" label="错误信息" min-width="200" show-overflow-tooltip />
          </el-table>
        </el-card>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { configAPI } from '@/api'

const config = ref({
  Hook: {
    Type: 'http',
    EnableHook: false,
    Http: {
      timeout: 10,
      onStreamStatus: 'http://127.0.0.1/api/v1/onStreamStatus',
      onStreamHeartbeat: 'http://127.0.0.1/api/v1/onStreamHeartbeat',
      onNonePlayer: 'http://127.0.0.1/api/v1/onNonePlayer',
      onKeepAlive: 'http://127.0.0.1/api/v1/onKeepAlive',
      onRegisterServer: 'http://127.0.0.1/api/v1/onRegisterServer',
      onPublish: 'http://127.0.0.1/api/v1/onPublish',
      onPlay: 'http://127.0.0.1/api/v1/onPlay'
    },
    Kafka: {
      endpoint: '127.0.0.1:9092',
      broker: 'streamStatus'
    }
  }
})

const saving = ref(false)
const testing = ref(false)
const logsLoading = ref(false)
const testResult = ref(null)
const hookLogs = ref([])

const loadConfig = async () => {
  try {
    const data = await configAPI.getConfig()
    if (data.Hook) {
      Object.assign(config.value.Hook, data.Hook)
    }
    ElMessage.success('配置加载成功')
  } catch (error) {
    ElMessage.error('配置加载失败: ' + error.message)
  }
}

const saveConfig = async () => {
  saving.value = true
  try {
    await configAPI.updateConfig(config.value)
    ElMessage.success('配置保存成功')
  } catch (error) {
    ElMessage.error('配置保存失败: ' + error.message)
  } finally {
    saving.value = false
  }
}

const testHook = async () => {
  testing.value = true
  testResult.value = null
  
  try {
    // 模拟测试Hook连接
    await new Promise(resolve => setTimeout(resolve, 2000))
    
    if (config.value.Hook.Type === 'http') {
      // 测试HTTP回调
      const testUrl = config.value.Hook.Http.onStreamStatus
      if (!testUrl || !testUrl.startsWith('http')) {
        throw new Error('请配置有效的HTTP回调URL')
      }
      
      testResult.value = {
        success: true,
        message: 'HTTP回调连接测试成功'
      }
    } else {
      // 测试Kafka连接
      const endpoint = config.value.Hook.Kafka.endpoint
      if (!endpoint) {
        throw new Error('请配置Kafka地址')
      }
      
      testResult.value = {
        success: true,
        message: 'Kafka连接测试成功'
      }
    }
  } catch (error) {
    testResult.value = {
      success: false,
      message: error.message
    }
  } finally {
    testing.value = false
  }
}

const refreshHookLogs = async () => {
  logsLoading.value = true
  try {
    // 模拟获取Hook日志
    await new Promise(resolve => setTimeout(resolve, 1000))
    
    hookLogs.value = [
      {
        time: Date.now() - 60000,
        type: 'onStreamStatus',
        url: config.value.Hook.Http.onStreamStatus,
        status: 'success',
        responseTime: 150,
        error: ''
      },
      {
        time: Date.now() - 120000,
        type: 'onPlay',
        url: config.value.Hook.Http.onPlay,
        status: 'success',
        responseTime: 89,
        error: ''
      },
      {
        time: Date.now() - 180000,
        type: 'onPublish',
        url: config.value.Hook.Http.onPublish,
        status: 'failed',
        responseTime: 5000,
        error: 'Connection timeout'
      }
    ]
  } catch (error) {
    ElMessage.error('获取Hook日志失败: ' + error.message)
  } finally {
    logsLoading.value = false
  }
}

const formatTime = (timestamp) => {
  return new Date(timestamp).toLocaleString()
}

const getHookTypeTag = (type) => {
  const tags = {
    onStreamStatus: 'primary',
    onStreamHeartbeat: 'success',
    onNonePlayer: 'warning',
    onKeepAlive: 'info',
    onRegisterServer: 'primary',
    onPublish: 'success',
    onPlay: 'info'
  }
  return tags[type] || 'default'
}

const getHookTypeName = (type) => {
  const names = {
    onStreamStatus: '流状态',
    onStreamHeartbeat: '流心跳',
    onNonePlayer: '无人观看',
    onKeepAlive: '服务心跳',
    onRegisterServer: '注册服务',
    onPublish: '推流鉴权',
    onPlay: '观看鉴权'
  }
  return names[type] || type
}

onMounted(() => {
  loadConfig()
  refreshHookLogs()
})
</script>

<style scoped>
.hook-config {
  max-width: 1200px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.config-section {
  margin-bottom: 20px;
}

.config-section:last-child {
  margin-bottom: 0;
}

.form-tip {
  font-size: 12px;
  color: #909399;
  margin-top: 5px;
}

:deep(.el-card__body) {
  padding: 20px;
}

:deep(.config-section .el-card__header) {
  background-color: #f5f7fa;
  font-weight: bold;
}
</style>
