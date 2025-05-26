<template>
  <div class="cdn-config">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>CDN配置</span>
          <div>
            <el-button @click="loadConfig">重新加载</el-button>
            <el-button type="primary" @click="saveConfig" :loading="saving">保存配置</el-button>
          </div>
        </div>
      </template>

      <el-form :model="config.Cdn" label-width="150px">
        <!-- CDN模式配置 -->
        <el-card class="config-section">
          <template #header>CDN模式配置</template>
          <el-form-item label="CDN模式">
            <el-radio-group v-model="config.Cdn.mode">
              <el-radio label="disabled">禁用CDN</el-radio>
              <el-radio label="edge">边缘模式(回源)</el-radio>
              <el-radio label="forward">转推模式</el-radio>
            </el-radio-group>
            <div class="form-tip">
              <div>禁用CDN: 不启用CDN功能</div>
              <div>边缘模式: 作为CDN边缘节点，从源站拉取流</div>
              <div>转推模式: 将本地流转推到其他服务器</div>
            </div>
          </el-form-item>
        </el-card>

        <!-- CDN服务器配置 -->
        <el-card class="config-section" v-if="config.Cdn.mode !== 'disabled'">
          <template #header>CDN服务器配置</template>
          <el-form-item label="服务器地址">
            <el-input v-model="config.Cdn.endpoint" placeholder="例如: 192.168.1.100:1935" />
            <div class="form-tip">CDN服务器的IP地址和端口</div>
          </el-form-item>
          
          <el-form-item label="协议">
            <el-select v-model="config.Cdn.protocol" placeholder="请选择协议">
              <el-option label="RTMP" value="rtmp" />
              <el-option label="RTSP" value="rtsp" />
              <el-option label="SRT" value="srt" />
            </el-select>
            <div class="form-tip">与CDN服务器通信使用的协议</div>
          </el-form-item>
          
          <el-form-item label="附加参数">
            <el-input v-model="config.Cdn.params" placeholder="例如: key1=value1&key2=value2" />
            <div class="form-tip">格式：key1=value1&key2=value2</div>
          </el-form-item>
        </el-card>

        <!-- 高级配置 -->
        <el-card class="config-section" v-if="config.Cdn.mode !== 'disabled'">
          <template #header>高级配置</template>
          <el-form-item label="连接超时(秒)">
            <el-input-number v-model="cdnAdvanced.timeout" :min="5" :max="60" />
          </el-form-item>
          
          <el-form-item label="重连间隔(秒)">
            <el-input-number v-model="cdnAdvanced.retryInterval" :min="1" :max="300" />
          </el-form-item>
          
          <el-form-item label="最大重连次数">
            <el-input-number v-model="cdnAdvanced.maxRetries" :min="0" :max="100" />
          </el-form-item>
          
          <el-form-item label="启用SSL">
            <el-switch v-model="cdnAdvanced.enableSsl" />
          </el-form-item>
          
          <el-form-item label="验证证书">
            <el-switch v-model="cdnAdvanced.verifyCert" />
            <div class="form-tip">是否验证SSL证书</div>
          </el-form-item>
        </el-card>

        <!-- 流过滤配置 -->
        <el-card class="config-section" v-if="config.Cdn.mode !== 'disabled'">
          <template #header>流过滤配置</template>
          <el-form-item label="启用流过滤">
            <el-switch v-model="streamFilter.enabled" />
            <div class="form-tip">只有匹配规则的流才会进行CDN操作</div>
          </el-form-item>
          
          <div v-if="streamFilter.enabled">
            <el-form-item label="过滤规则">
              <div v-for="(rule, index) in streamFilter.rules" :key="index" class="filter-rule">
                <el-row :gutter="10">
                  <el-col :span="6">
                    <el-select v-model="rule.type" placeholder="规则类型">
                      <el-option label="应用名匹配" value="app" />
                      <el-option label="流名匹配" value="stream" />
                      <el-option label="路径匹配" value="path" />
                      <el-option label="协议匹配" value="protocol" />
                    </el-select>
                  </el-col>
                  <el-col :span="6">
                    <el-select v-model="rule.operator" placeholder="操作符">
                      <el-option label="等于" value="equals" />
                      <el-option label="包含" value="contains" />
                      <el-option label="正则匹配" value="regex" />
                      <el-option label="开头匹配" value="startsWith" />
                      <el-option label="结尾匹配" value="endsWith" />
                    </el-select>
                  </el-col>
                  <el-col :span="8">
                    <el-input v-model="rule.value" placeholder="匹配值" />
                  </el-col>
                  <el-col :span="2">
                    <el-select v-model="rule.action" placeholder="动作">
                      <el-option label="允许" value="allow" />
                      <el-option label="拒绝" value="deny" />
                    </el-select>
                  </el-col>
                  <el-col :span="2">
                    <el-button type="danger" @click="removeFilterRule(index)">删除</el-button>
                  </el-col>
                </el-row>
              </div>
              <el-button @click="addFilterRule">添加规则</el-button>
            </el-form-item>
          </div>
        </el-card>

        <!-- CDN状态监控 -->
        <el-card class="config-section" v-if="config.Cdn.mode !== 'disabled'">
          <template #header>
            <div class="card-header">
              <span>CDN状态监控</span>
              <el-button type="text" @click="refreshCdnStatus">刷新</el-button>
            </div>
          </template>
          
          <el-row :gutter="20">
            <el-col :span="8">
              <el-statistic title="连接状态" :value="cdnStatus.connected ? '已连接' : '未连接'">
                <template #suffix>
                  <el-tag :type="cdnStatus.connected ? 'success' : 'danger'">
                    {{ cdnStatus.connected ? '正常' : '异常' }}
                  </el-tag>
                </template>
              </el-statistic>
            </el-col>
            <el-col :span="8">
              <el-statistic title="活跃流数量" :value="cdnStatus.activeStreams" />
            </el-col>
            <el-col :span="8">
              <el-statistic title="总流量" :value="formatBandwidth(cdnStatus.totalBandwidth)" />
            </el-col>
          </el-row>
          
          <div style="margin-top: 20px;">
            <h4>最近CDN日志</h4>
            <el-table :data="cdnLogs" style="width: 100%" max-height="300">
              <el-table-column prop="time" label="时间" width="160">
                <template #default="scope">
                  <span>{{ formatTime(scope.row.time) }}</span>
                </template>
              </el-table-column>
              <el-table-column prop="action" label="操作" width="100">
                <template #default="scope">
                  <el-tag :type="getCdnActionTag(scope.row.action)">
                    {{ getCdnActionName(scope.row.action) }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="stream" label="流" min-width="200" />
              <el-table-column prop="status" label="状态" width="100">
                <template #default="scope">
                  <el-tag :type="scope.row.status === 'success' ? 'success' : 'danger'">
                    {{ scope.row.status === 'success' ? '成功' : '失败' }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="message" label="消息" min-width="200" />
            </el-table>
          </div>
        </el-card>

        <!-- 测试连接 -->
        <el-card class="config-section" v-if="config.Cdn.mode !== 'disabled'">
          <template #header>连接测试</template>
          <el-form-item label="测试CDN连接">
            <el-button type="primary" @click="testCdnConnection" :loading="testing">测试连接</el-button>
            <div class="form-tip">测试与CDN服务器的连接是否正常</div>
          </el-form-item>
          
          <el-form-item label="测试结果" v-if="testResult">
            <el-alert
              :title="testResult.success ? '连接成功' : '连接失败'"
              :type="testResult.success ? 'success' : 'error'"
              :description="testResult.message"
              show-icon
              :closable="false"
            />
          </el-form-item>
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
  Cdn: {
    mode: 'disabled',
    endpoint: '',
    protocol: 'rtmp',
    params: ''
  }
})

const cdnAdvanced = ref({
  timeout: 30,
  retryInterval: 5,
  maxRetries: 3,
  enableSsl: false,
  verifyCert: true
})

const streamFilter = ref({
  enabled: false,
  rules: []
})

const cdnStatus = ref({
  connected: false,
  activeStreams: 0,
  totalBandwidth: 0
})

const cdnLogs = ref([])
const saving = ref(false)
const testing = ref(false)
const testResult = ref(null)

const loadConfig = async () => {
  try {
    const data = await configAPI.getConfig()
    if (data.Cdn) {
      Object.assign(config.value.Cdn, data.Cdn)
    }
    ElMessage.success('配置加载成功')
  } catch (error) {
    ElMessage.error('配置加载失败: ' + error.message)
  }
}

const saveConfig = async () => {
  saving.value = true
  try {
    const configData = {
      ...config.value,
      CdnAdvanced: cdnAdvanced.value,
      StreamFilter: streamFilter.value
    }
    await configAPI.updateConfig(configData)
    ElMessage.success('配置保存成功')
  } catch (error) {
    ElMessage.error('配置保存失败: ' + error.message)
  } finally {
    saving.value = false
  }
}

const addFilterRule = () => {
  streamFilter.value.rules.push({
    type: 'app',
    operator: 'equals',
    value: '',
    action: 'allow'
  })
}

const removeFilterRule = (index) => {
  streamFilter.value.rules.splice(index, 1)
}

const refreshCdnStatus = async () => {
  try {
    // 模拟获取CDN状态
    await new Promise(resolve => setTimeout(resolve, 1000))
    
    cdnStatus.value = {
      connected: config.value.Cdn.mode !== 'disabled',
      activeStreams: Math.floor(Math.random() * 50),
      totalBandwidth: Math.floor(Math.random() * 1000000000)
    }
    
    // 模拟CDN日志
    cdnLogs.value = [
      {
        time: Date.now() - 60000,
        action: 'push',
        stream: '/live/stream1',
        status: 'success',
        message: '流推送成功'
      },
      {
        time: Date.now() - 120000,
        action: 'pull',
        stream: '/live/stream2',
        status: 'success',
        message: '流拉取成功'
      },
      {
        time: Date.now() - 180000,
        action: 'push',
        stream: '/live/stream3',
        status: 'failed',
        message: '连接超时'
      }
    ]
  } catch (error) {
    ElMessage.error('获取CDN状态失败: ' + error.message)
  }
}

const testCdnConnection = async () => {
  testing.value = true
  testResult.value = null
  
  try {
    if (!config.value.Cdn.endpoint) {
      throw new Error('请先配置CDN服务器地址')
    }
    
    // 模拟测试连接
    await new Promise(resolve => setTimeout(resolve, 2000))
    
    testResult.value = {
      success: true,
      message: `成功连接到 ${config.value.Cdn.endpoint}`
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

const formatBandwidth = (bandwidth) => {
  if (!bandwidth) return '0 bps'
  if (bandwidth < 1024) return bandwidth + ' bps'
  if (bandwidth < 1024 * 1024) return (bandwidth / 1024).toFixed(1) + ' Kbps'
  return (bandwidth / 1024 / 1024).toFixed(1) + ' Mbps'
}

const formatTime = (timestamp) => {
  return new Date(timestamp).toLocaleString()
}

const getCdnActionTag = (action) => {
  const tags = {
    push: 'success',
    pull: 'primary',
    stop: 'warning',
    error: 'danger'
  }
  return tags[action] || 'default'
}

const getCdnActionName = (action) => {
  const names = {
    push: '推送',
    pull: '拉取',
    stop: '停止',
    error: '错误'
  }
  return names[action] || action
}

onMounted(() => {
  loadConfig()
  refreshCdnStatus()
})
</script>

<style scoped>
.cdn-config {
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

.filter-rule {
  margin-bottom: 10px;
  padding: 10px;
  border: 1px solid #e4e7ed;
  border-radius: 4px;
  background-color: #fafafa;
}

:deep(.el-card__body) {
  padding: 20px;
}

:deep(.config-section .el-card__header) {
  background-color: #f5f7fa;
  font-weight: bold;
}
</style>
