<template>
  <div class="protocols-config">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>协议配置</span>
          <div>
            <el-button @click="loadConfig">重新加载</el-button>
            <el-button type="primary" @click="saveConfig" :loading="saving">保存配置</el-button>
          </div>
        </div>
      </template>

      <el-tabs v-model="activeTab" type="border-card">
        <!-- HTTP配置 -->
        <el-tab-pane label="HTTP" name="http">
          <el-form :model="config.Http" label-width="150px">
            <el-card class="config-section">
              <template #header>HTTP API配置</template>
              <div v-for="(api, index) in config.Http.Api" :key="index" class="server-item">
                <el-form-item :label="`API服务器${index + 1}`">
                  <el-row :gutter="10">
                    <el-col :span="6">
                      <el-input v-model="api.ip" placeholder="IP地址" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="api.port" placeholder="端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="api.sslPort" placeholder="SSL端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="api.timeout" placeholder="超时(ms)" :min="1000" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="api.threads" placeholder="线程数" :min="1" :max="32" />
                    </el-col>
                    <el-col :span="2">
                      <el-button type="danger" @click="removeHttpApi(index)">删除</el-button>
                    </el-col>
                  </el-row>
                </el-form-item>
              </div>
              <el-button @click="addHttpApi">添加API服务器</el-button>
            </el-card>

            <el-card class="config-section">
              <template #header>HTTP文件服务配置</template>
              <div v-for="(server, index) in config.Http.Server" :key="index" class="server-item">
                <el-form-item :label="`文件服务器${index + 1}`">
                  <el-row :gutter="10">
                    <el-col :span="4">
                      <el-input v-model="server.ip" placeholder="IP地址" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.port" placeholder="端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.sslPort" placeholder="SSL端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.timeout" placeholder="超时(ms)" :min="1000" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.threads" placeholder="线程数" :min="1" :max="32" />
                    </el-col>
                    <el-col :span="4">
                      <el-input v-model="server.rootPath" placeholder="根目录" />
                    </el-col>
                    <el-col :span="2">
                      <el-switch v-model="server.enableViewDir" />
                    </el-col>
                    <el-col :span="2">
                      <el-button type="danger" @click="removeHttpServer(index)">删除</el-button>
                    </el-col>
                  </el-row>
                </el-form-item>
              </div>
              <el-button @click="addHttpServer">添加文件服务器</el-button>
            </el-card>
          </el-form>
        </el-tab-pane>

        <!-- RTSP配置 -->
        <el-tab-pane label="RTSP" name="rtsp">
          <el-form :model="config.Rtsp" label-width="150px">
            <el-card class="config-section">
              <template #header>RTSP服务器配置</template>
              <div v-for="(server, index) in config.Rtsp.Server" :key="index" class="server-item">
                <el-form-item :label="`RTSP服务器${index + 1}`">
                  <el-row :gutter="10">
                    <el-col :span="4">
                      <el-input v-model="server.ip" placeholder="IP地址" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.port" placeholder="端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.sslPort" placeholder="SSL端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.timeout" placeholder="超时(ms)" :min="1000" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.threads" placeholder="线程数" :min="1" :max="32" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.udpPortMin" placeholder="UDP最小端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.udpPortMax" placeholder="UDP最大端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="2">
                      <el-switch v-model="server.rtspAuth" />
                    </el-col>
                  </el-row>
                </el-form-item>
              </div>
              <el-button @click="addRtspServer">添加RTSP服务器</el-button>
            </el-card>
          </el-form>
        </el-tab-pane>

        <!-- RTMP配置 -->
        <el-tab-pane label="RTMP" name="rtmp">
          <el-form :model="config.Rtmp" label-width="150px">
            <el-card class="config-section">
              <template #header>RTMP服务器配置</template>
              <div v-for="(server, index) in config.Rtmp.Server" :key="index" class="server-item">
                <el-form-item :label="`RTMP服务器${index + 1}`">
                  <el-row :gutter="10">
                    <el-col :span="6">
                      <el-input v-model="server.ip" placeholder="IP地址" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.port" placeholder="端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.sslPort" placeholder="SSL端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.timeout" placeholder="超时(ms)" :min="1000" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.threads" placeholder="线程数" :min="1" :max="32" />
                    </el-col>
                    <el-col :span="2">
                      <el-switch v-model="server.enableAddMute" />
                    </el-col>
                  </el-row>
                </el-form-item>
              </div>
              <el-button @click="addRtmpServer">添加RTMP服务器</el-button>
            </el-card>
          </el-form>
        </el-tab-pane>

        <!-- WebRTC配置 -->
        <el-tab-pane label="WebRTC" name="webrtc">
          <el-form :model="config.Webrtc" label-width="150px">
            <el-card class="config-section">
              <template #header>WebRTC服务器配置</template>
              <div v-for="(server, index) in config.Webrtc.Server" :key="index" class="server-item">
                <el-form-item :label="`WebRTC服务器${index + 1}`">
                  <el-row :gutter="10">
                    <el-col :span="4">
                      <el-input v-model="server.ip" placeholder="IP地址" />
                    </el-col>
                    <el-col :span="4">
                      <el-input v-model="server.candidateIp" placeholder="候选IP" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.port" placeholder="端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.timeout" placeholder="超时(ms)" :min="1000" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.threads" placeholder="线程数" :min="1" :max="32" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.udpPortMin" placeholder="UDP最小端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="3">
                      <el-input-number v-model="server.udpPortMax" placeholder="UDP最大端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="1">
                      <el-button type="danger" @click="removeWebrtcServer(index)">删除</el-button>
                    </el-col>
                  </el-row>
                </el-form-item>
                <el-form-item label="功能开关">
                  <el-row :gutter="10">
                    <el-col :span="4">
                      <el-checkbox v-model="server.enableTcp">启用TCP</el-checkbox>
                    </el-col>
                    <el-col :span="4">
                      <el-checkbox v-model="server.enableNack">启用NACK</el-checkbox>
                    </el-col>
                    <el-col :span="4">
                      <el-checkbox v-model="server.enableTwcc">启用TWCC</el-checkbox>
                    </el-col>
                    <el-col :span="4">
                      <el-checkbox v-model="server.enableRtx">启用RTX</el-checkbox>
                    </el-col>
                    <el-col :span="4">
                      <el-checkbox v-model="server.enableRed">启用RED</el-checkbox>
                    </el-col>
                    <el-col :span="4">
                      <el-checkbox v-model="server.enableUlpfec">启用ULPFEC</el-checkbox>
                    </el-col>
                  </el-row>
                </el-form-item>
              </div>
              <el-button @click="addWebrtcServer">添加WebRTC服务器</el-button>
            </el-card>
          </el-form>
        </el-tab-pane>

        <!-- SRT配置 -->
        <el-tab-pane label="SRT" name="srt">
          <el-form :model="config.Srt" label-width="150px">
            <el-card class="config-section">
              <template #header>SRT服务器配置</template>
              <div v-for="(server, index) in config.Srt.Server" :key="index" class="server-item">
                <el-form-item :label="`SRT服务器${index + 1}`">
                  <el-row :gutter="10">
                    <el-col :span="6">
                      <el-input v-model="server.ip" placeholder="IP地址" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.port" placeholder="端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.timeout" placeholder="超时(ms)" :min="1000" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.threads" placeholder="线程数" :min="1" :max="32" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.udpPortMin" placeholder="UDP最小端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="2">
                      <el-input-number v-model="server.udpPortMax" placeholder="UDP最大端口" :min="1" :max="65535" />
                    </el-col>
                  </el-row>
                </el-form-item>
              </div>
              <el-button @click="addSrtServer">添加SRT服务器</el-button>
            </el-card>
          </el-form>
        </el-tab-pane>

        <!-- WebSocket配置 -->
        <el-tab-pane label="WebSocket" name="websocket">
          <el-form :model="config.Websocket" label-width="150px">
            <el-card class="config-section">
              <template #header>WebSocket服务器配置</template>
              <div v-for="(server, index) in config.Websocket.Server" :key="index" class="server-item">
                <el-form-item :label="`WebSocket服务器${index + 1}`">
                  <el-row :gutter="10">
                    <el-col :span="6">
                      <el-input v-model="server.ip" placeholder="IP地址" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.port" placeholder="端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.sslPort" placeholder="SSL端口" :min="1" :max="65535" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.timeout" placeholder="超时(ms)" :min="1000" />
                    </el-col>
                    <el-col :span="4">
                      <el-input-number v-model="server.threads" placeholder="线程数" :min="1" :max="32" />
                    </el-col>
                    <el-col :span="2">
                      <el-button type="danger" @click="removeWebsocketServer(index)">删除</el-button>
                    </el-col>
                  </el-row>
                </el-form-item>
              </div>
              <el-button @click="addWebsocketServer">添加WebSocket服务器</el-button>
            </el-card>
          </el-form>
        </el-tab-pane>
      </el-tabs>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { configAPI } from '@/api'

const activeTab = ref('http')
const saving = ref(false)

const config = ref({
  Http: {
    Api: [],
    Server: []
  },
  Rtsp: {
    Server: []
  },
  Rtmp: {
    Server: []
  },
  Webrtc: {
    Server: []
  },
  Srt: {
    Server: []
  },
  Websocket: {
    Server: []
  }
})

// 默认服务器配置模板
const defaultHttpApi = () => ({
  ip: '0.0.0.0',
  port: 80,
  sslPort: 443,
  timeout: 5000,
  threads: 1
})

const defaultHttpServer = () => ({
  ip: '0.0.0.0',
  port: 8080,
  sslPort: 18080,
  timeout: 5000,
  threads: 1,
  rootPath: './',
  enableViewDir: true
})

const defaultRtspServer = () => ({
  ip: '0.0.0.0',
  port: 554,
  sslPort: 1554,
  timeout: 5000,
  threads: 1,
  udpPortMin: 10000,
  udpPortMax: 20000,
  rtspAuth: false
})

const defaultRtmpServer = () => ({
  ip: '0.0.0.0',
  port: 1935,
  sslPort: 11935,
  timeout: 5000,
  threads: 1,
  enableAddMute: false
})

const defaultWebrtcServer = () => ({
  ip: '0.0.0.0',
  candidateIp: '192.168.0.104',
  port: 7000,
  timeout: 5000,
  threads: 1,
  udpPortMin: 6000,
  udpPortMax: 10000,
  enableTcp: false,
  enableNack: true,
  enableTwcc: false,
  enableRtx: true,
  enableRed: false,
  enableUlpfec: false
})

const defaultSrtServer = () => ({
  ip: '0.0.0.0',
  port: 6666,
  timeout: 5000,
  threads: 2,
  udpPortMin: 6000,
  udpPortMax: 10000
})

const defaultWebsocketServer = () => ({
  ip: '0.0.0.0',
  port: 1080,
  sslPort: 1443,
  timeout: 5000,
  threads: 1
})

// 添加服务器方法
const addHttpApi = () => {
  config.value.Http.Api.push(defaultHttpApi())
}

const addHttpServer = () => {
  config.value.Http.Server.push(defaultHttpServer())
}

const addRtspServer = () => {
  config.value.Rtsp.Server.push(defaultRtspServer())
}

const addRtmpServer = () => {
  config.value.Rtmp.Server.push(defaultRtmpServer())
}

const addWebrtcServer = () => {
  config.value.Webrtc.Server.push(defaultWebrtcServer())
}

const addSrtServer = () => {
  config.value.Srt.Server.push(defaultSrtServer())
}

const addWebsocketServer = () => {
  config.value.Websocket.Server.push(defaultWebsocketServer())
}

// 删除服务器方法
const removeHttpApi = (index) => {
  config.value.Http.Api.splice(index, 1)
}

const removeHttpServer = (index) => {
  config.value.Http.Server.splice(index, 1)
}

const removeRtspServer = (index) => {
  config.value.Rtsp.Server.splice(index, 1)
}

const removeRtmpServer = (index) => {
  config.value.Rtmp.Server.splice(index, 1)
}

const removeWebrtcServer = (index) => {
  config.value.Webrtc.Server.splice(index, 1)
}

const removeSrtServer = (index) => {
  config.value.Srt.Server.splice(index, 1)
}

const removeWebsocketServer = (index) => {
  config.value.Websocket.Server.splice(index, 1)
}

const loadConfig = async () => {
  try {
    const data = await configAPI.getConfig()
    
    // 确保数组结构存在
    config.value.Http.Api = data.Http?.Api || []
    config.value.Http.Server = data.Http?.Server || []
    config.value.Rtsp.Server = data.Rtsp?.Server || []
    config.value.Rtmp.Server = data.Rtmp?.Server || []
    config.value.Webrtc.Server = data.Webrtc?.Server || []
    config.value.Srt.Server = data.Srt?.Server || []
    config.value.Websocket.Server = data.Websocket?.Server || []
    
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
.protocols-config {
  max-width: 1400px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.config-section {
  margin-bottom: 20px;
}

.server-item {
  margin-bottom: 15px;
  padding: 15px;
  border: 1px solid #e4e7ed;
  border-radius: 4px;
  background-color: #fafafa;
}

:deep(.el-tabs__content) {
  padding: 20px;
}

:deep(.el-card__body) {
  padding: 20px;
}
</style>
