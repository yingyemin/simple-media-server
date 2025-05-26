import axios from 'axios'
import { ElMessage } from 'element-plus'

// 创建axios实例
const api = axios.create({
  baseURL: '/api/v1',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// 请求拦截器
api.interceptors.request.use(
  config => {
    return config
  },
  error => {
    return Promise.reject(error)
  }
)

// 响应拦截器
api.interceptors.response.use(
  response => {
    const { data } = response
    if (data.code && data.code !== '200') {
      ElMessage.error(data.msg || '请求失败')
      return Promise.reject(new Error(data.msg || '请求失败'))
    }
    return data
  },
  error => {
    ElMessage.error(error.message || '网络错误')
    return Promise.reject(error)
  }
)

// API接口定义
export const configAPI = {
  // 获取配置
  getConfig: () => api.get('/config'),
  // 更新配置
  updateConfig: (data) => api.post('/config', data)
}

export const streamAPI = {
  // 获取流列表
  getSourceList: () => api.get('/getSourceList'),
  // 获取流信息
  getSourceInfo: (params) => api.get('/getSourceInfo', { params }),
  // 关闭流
  closeSource: (data) => api.post('/closeSource', data),
  // 获取客户端列表
  getClientList: (params) => api.get('/getClientList', { params }),
  // 关闭客户端
  closeClient: (data) => api.post('/closeClient', data)
}

export const serverAPI = {
  // 获取服务器信息
  getServerInfo: () => api.get('/getServerInfo'),
  // 获取版本信息
  getVersion: () => api.get('/version'),
  // 获取事件循环列表
  getLoopList: () => api.get('/getLoopList'),
  // 退出服务器
  exitServer: () => api.post('/exitServer')
}

export const rtspAPI = {
  // 创建RTSP流
  createStream: (data) => api.post('/rtsp/streams/create', data),
  // 开始RTSP拉流
  startPlay: (data) => api.post('/rtsp/play/start', data),
  // 停止RTSP拉流
  stopPlay: (data) => api.post('/rtsp/play/stop', data),
  // 获取RTSP拉流列表
  getPlayList: () => api.get('/rtsp/play/list'),
  // 开始RTSP推流
  startPublish: (data) => api.post('/rtsp/publish/start', data),
  // 停止RTSP推流
  stopPublish: (data) => api.post('/rtsp/publish/stop', data),
  // 获取RTSP推流列表
  getPublishList: () => api.get('/rtsp/publish/list')
}

export const rtmpAPI = {
  // 开始RTMP拉流
  startPlay: (data) => api.post('/rtmp/play/start', data),
  // 停止RTMP拉流
  stopPlay: (data) => api.post('/rtmp/play/stop', data),
  // 获取RTMP拉流列表
  getPlayList: () => api.get('/rtmp/play/list'),
  // 开始RTMP推流
  startPublish: (data) => api.post('/rtmp/publish/start', data),
  // 停止RTMP推流
  stopPublish: (data) => api.post('/rtmp/publish/stop', data),
  // 获取RTMP推流列表
  getPublishList: () => api.get('/rtmp/publish/list')
}

export const webrtcAPI = {
  // WebRTC播放
  play: (data) => api.post('/rtc/play', data),
  // WebRTC推流
  publish: (data) => api.post('/rtc/publish', data),
  // 开始WebRTC拉流
  startPull: (data) => api.post('/rtc/pull/start', data),
  // 停止WebRTC拉流
  stopPull: (data) => api.post('/rtc/pull/stop', data),
  // 获取WebRTC拉流列表
  getPullList: () => api.get('/rtc/pull/list'),
  // 开始WebRTC推流
  startPush: (data) => api.post('/rtc/push/start', data),
  // 停止WebRTC推流
  stopPush: (data) => api.post('/rtc/push/stop', data),
  // 获取WebRTC推流列表
  getPushList: () => api.get('/rtc/push/list')
}

export const srtAPI = {
  // 开始SRT拉流
  startPull: (data) => api.post('/srt/pull/start', data),
  // 停止SRT拉流
  stopPull: (data) => api.post('/srt/pull/stop', data),
  // 获取SRT拉流列表
  getPullList: () => api.get('/srt/pull/list'),
  // 开始SRT推流
  startPush: (data) => api.post('/srt/push/start', data),
  // 停止SRT推流
  stopPush: (data) => api.post('/srt/push/stop', data),
  // 获取SRT推流列表
  getPushList: () => api.get('/srt/push/list')
}

export const gb28181API = {
  // 创建GB28181接收器
  createReceiver: (data) => api.post('/gb28181/recv/create', data),
  // 创建GB28181发送器
  createSender: (data) => api.post('/gb28181/send/create', data),
  // 停止GB28181接收器
  stopReceiver: (data) => api.post('/gb28181/recv/stop', data),
  // 停止GB28181发送器
  stopSender: (data) => api.post('/gb28181/send/stop', data)
}

export const jt1078API = {
  // 创建JT1078
  create: (data) => api.post('/jt1078/create', data),
  // 删除JT1078
  delete: (data) => api.post('/jt1078/delete', data),
  // 开启JT1078服务器
  openServer: (data) => api.post('/jt1078/server/open', data),
  // 关闭JT1078服务器
  closeServer: (data) => api.post('/jt1078/server/close', data),
  // 开始对讲
  startTalk: (data) => api.post('/jt1078/talk/start', data),
  // 停止对讲
  stopTalk: (data) => api.post('/jt1078/talk/stop', data),
  // 开始发送
  startSend: (data) => api.post('/jt1078/send/start', data),
  // 停止发送
  stopSend: (data) => api.post('/jt1078/send/stop', data),
  // 获取端口信息
  getPortInfo: () => api.get('/jt1078/port/info')
}

export const recordAPI = {
  // 开始录制
  startRecord: (data) => api.post('/record/start', data),
  // 停止录制
  stopRecord: (data) => api.post('/record/stop', data),
  // 获取录制列表
  getRecordList: () => api.get('/record/list')
}

export const vodAPI = {
  // 开始点播
  start: (data) => api.post('/vod/start', data),
  // 停止点播
  stop: (data) => api.post('/vod/stop', data),
  // 控制点播
  control: (data) => api.post('/vod/control', data)
}

export default api
