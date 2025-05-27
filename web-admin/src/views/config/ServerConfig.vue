<template>
  <div class="server-config">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>服务器配置</span>
          <div>
            <el-button @click="loadConfig">重新加载</el-button>
            <el-button type="primary" @click="saveConfig" :loading="saving">保存配置</el-button>
          </div>
        </div>
      </template>

      <el-form :model="config" label-width="150px" ref="configForm">
        <!-- 基础配置 -->
        <el-card class="config-section">
          <template #header>基础配置</template>
          <el-form-item label="本地IP地址">
            <el-input v-model="config.LocalIp" placeholder="请输入本地IP地址" />
          </el-form-item>
        </el-card>

        <!-- SSL配置 -->
        <el-card class="config-section">
          <template #header>SSL证书配置</template>
          <el-form-item label="私钥文件路径">
            <el-input v-model="config.Ssl.key" placeholder="请输入私钥文件路径" />
          </el-form-item>
          <el-form-item label="证书文件路径">
            <el-input v-model="config.Ssl.cert" placeholder="请输入证书文件路径" />
          </el-form-item>
        </el-card>

        <!-- 日志配置 -->
        <el-card class="config-section">
          <template #header>日志配置</template>
          <el-form-item label="日志级别">
            <el-select v-model="config.Log.logLevel" placeholder="请选择日志级别">
              <el-option label="Trace (0)" :value="0" />
              <el-option label="Debug (1)" :value="1" />
              <el-option label="Info (2)" :value="2" />
              <el-option label="Warn (3)" :value="3" />
              <el-option label="Error (4)" :value="4" />
            </el-select>
          </el-form-item>
          <el-form-item label="控制台输出">
            <el-switch v-model="config.Log.console" :active-value="1" :inactive-value="0" />
          </el-form-item>
          <el-form-item label="最大保存天数">
            <el-input-number v-model="config.Log.maxDay" :min="1" :max="365" />
          </el-form-item>
          <el-form-item label="单文件最大大小(KB)">
            <el-input-number v-model="config.Log.maxSize" :min="100" :max="10240" />
          </el-form-item>
          <el-form-item label="最大文件数量">
            <el-input-number v-model="config.Log.maxCount" :min="1" :max="1000" />
          </el-form-item>
        </el-card>

        <!-- 事件循环池配置 -->
        <el-card class="config-section">
          <template #header>事件循环池配置</template>
          <el-form-item label="线程池大小">
            <el-input-number v-model="config.EventLoopPool.size" :min="0" :max="32" />
            <div class="form-tip">0表示使用CPU核心数</div>
          </el-form-item>
        </el-card>

        <!-- 工具配置 -->
        <el-card class="config-section">
          <template #header>工具配置</template>
          <el-form-item label="无人观看停流">
            <el-switch v-model="config.Util.stopNonePlayerStream" />
          </el-form-item>
          <el-form-item label="无人观看等待时间(ms)">
            <el-input-number v-model="config.Util.nonePlayerWaitTime" :min="1000" :max="60000" />
          </el-form-item>
          <el-form-item label="启用本地文件加载">
            <el-switch v-model="config.Util.enableLoadFromFile" />
          </el-form-item>
          <el-form-item label="心跳时间(ms)">
            <el-input-number v-model="config.Util.heartbeatTime" :min="1000" :max="60000" />
          </el-form-item>
          <el-form-item label="流心跳时间(ms)">
            <el-input-number v-model="config.Util.streamHeartbeatTime" :min="1000" :max="60000" />
          </el-form-item>
          <el-form-item label="首个Track等待时间(ms)">
            <el-input-number v-model="config.Util.firstTrackWaitTime" :min="100" :max="5000" />
          </el-form-item>
          <el-form-item label="第二个Track等待时间(ms)">
            <el-input-number v-model="config.Util.sencondTrackWaitTime" :min="1000" :max="10000" />
          </el-form-item>
        </el-card>

        <!-- 录制配置 -->
        <el-card class="config-section">
          <template #header>录制配置</template>
          <el-form-item label="录制根目录">
            <el-input v-model="config.Record.rootPath" placeholder="请输入录制根目录" />
          </el-form-item>
        </el-card>

        <!-- RTP配置 -->
        <el-card class="config-section">
          <template #header>RTP配置</template>
          <el-form-item label="最大RTP包大小">
            <el-input-number v-model="config.Rtp.maxRtpSize" :min="500" :max="2000" />
          </el-form-item>
          <el-form-item label="巨帧RTP大小">
            <el-input-number v-model="config.Rtp.hugeRtpSize" :min="10000" :max="100000" />
          </el-form-item>
          <el-form-item label="超时时间(ms)">
            <el-input-number v-model="config.Rtp.Server.timeout" :min="1000" :max="30000" />
          </el-form-item>
        </el-card>

        <!-- HLS配置 -->
        <el-card class="config-section">
          <template #header>HLS配置</template>
          <el-form-item label="切片时长(ms)">
            <el-input-number v-model="config.Hls.Server.duration" :min="1000" :max="30000" />
          </el-form-item>
          <el-form-item label="切片数量">
            <el-input-number v-model="config.Hls.Server.segNum" :min="3" :max="20" />
          </el-form-item>
          <el-form-item label="播放超时(s)">
            <el-input-number v-model="config.Hls.Server.playTimeout" :min="10" :max="300" />
          </el-form-item>
          <el-form-item label="强制HLS">
            <el-switch v-model="config.Hls.Server.force" />
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
  LocalIp: '',
  Ssl: {
    key: '',
    cert: ''
  },
  Log: {
    logLevel: 2,
    console: 1,
    maxDay: 7,
    maxSize: 1024,
    maxCount: 100
  },
  EventLoopPool: {
    size: 0
  },
  Util: {
    stopNonePlayerStream: false,
    nonePlayerWaitTime: 5000,
    enableLoadFromFile: false,
    heartbeatTime: 10000,
    streamHeartbeatTime: 10000,
    firstTrackWaitTime: 500,
    sencondTrackWaitTime: 5000
  },
  Record: {
    rootPath: './'
  },
  Rtp: {
    maxRtpSize: 1400,
    hugeRtpSize: 60000,
    Server: {
      timeout: 5000
    }
  },
  Hls: {
    Server: {
      duration: 5000,
      segNum: 5,
      playTimeout: 60,
      force: false
    }
  }
})

const saving = ref(false)
const configForm = ref(null)

const loadConfig = async () => {
  try {
    const data = await configAPI.getConfig()
    // 合并配置，保持默认值
    Object.assign(config.value, data)
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

onMounted(() => {
  loadConfig()
})
</script>

<style scoped>
.server-config {
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
