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
    // 支持新的错误码格式（数字）和旧的格式（字符串）
    const code = data.code
    const isSuccess = code === '200' || code === 200 || code === '成功'

    if (code && !isSuccess) {
      const errorMsg = data.msg || '请求失败'

      // 根据错误码类型显示不同的错误信息
      if (code >= 600 && code < 700) {
        // 业务错误，显示详细信息
        ElMessage.error(`参数错误: ${errorMsg}`)
      } else if (code >= 700 && code < 800) {
        // 媒体流错误
        ElMessage.error(`流媒体错误: ${errorMsg}`)
      } else if (code >= 800 && code < 900) {
        // 服务器错误
        ElMessage.error(`服务器错误: ${errorMsg}`)
      } else if (code >= 900 && code < 1000) {
        // 协议错误
        ElMessage.error(`协议错误: ${errorMsg}`)
      } else {
        // 其他错误
        ElMessage.error(errorMsg)
      }

      return Promise.reject(new Error(errorMsg))
    }
    return data
  },
  error => {
    let errorMsg = '网络错误'

    if (error.response) {
      const { status, data } = error.response
      if (data && data.msg) {
        errorMsg = data.msg
      } else {
        switch (status) {
          case 400:
            errorMsg = '请求参数错误'
            break
          case 401:
            errorMsg = '未授权访问'
            break
          case 403:
            errorMsg = '禁止访问'
            break
          case 404:
            errorMsg = '请求的资源不存在'
            break
          case 500:
            errorMsg = '服务器内部错误'
            break
          case 502:
            errorMsg = '网关错误'
            break
          case 503:
            errorMsg = '服务不可用'
            break
          default:
            errorMsg = `请求失败 (${status})`
        }
      }
    } else if (error.request) {
      errorMsg = '网络连接失败'
    }

    ElMessage.error(errorMsg)
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
  getServerInfo: () => api.get('/server/info'),
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
  control: (data) => api.post('/vod/control', data),
  // 获取录制列表
  getVodStatusAndProgress: (data) => api.get('/record/query', data)
}

// HTTP流媒体API
export const httpStreamAPI = {
  // FLV播放相关
  startFlvPlay: (data) => api.post('/http/flv/play/start', data),
  stopFlvPlay: (data) => api.post('/http/flv/play/stop', data),
  getFlvPlayList: () => api.get('/http/flv/play/list'),

  // HLS播放相关
  startHlsPlay: (data) => api.post('/http/hls/play/start', data),
  stopHlsPlay: (data) => api.post('/http/hls/play/stop', data),
  getHlsPlayList: () => api.get('/http/hls/play/list'),

  // PS点播相关
  startPsVodPlay: (data) => api.post('/http/ps/vod/play/start', data),
  stopPsVodPlay: (data) => api.post('/http/ps/vod/play/stop', data),
  getPsVodPlayList: () => api.get('/http/ps/vod/play/list')
}

// RTSP服务器管理API
export const rtspServerAPI = {
  createServer: (data) => api.post('/rtsp/server/create', data),
  stopServer: (data) => api.post('/rtsp/server/stop', data),
  getServerList: () => api.get('/rtsp/server/list')
}

// RTMP流创建API
export const rtmpStreamAPI = {
  createStream: (data) => api.post('/rtmp/create', data)
}

// WebRTC WHEP/WHIP API
export const webrtcWhepWhipAPI = {
  whep: (data) => {
    const params = new URLSearchParams({
      appName: data.appName,
      streamName: data.streamName
    })
    return api.post(`/rtc/whep?${params}`, data.sdp, {
      headers: {
        'Content-Type': 'application/sdp'
      }
    })
  },
  whip: (data) => {
    const params = new URLSearchParams({
      appName: data.appName,
      streamName: data.streamName
    })
    return api.post(`/rtc/whip?${params}`, data.sdp, {
      headers: {
        'Content-Type': 'application/sdp'
      }
    })
  }
}

// GB28181 SIP API
export const gb28181SipAPI = {
  catalog: (data) => api.post('/catalog', data),
  invite: (data) => api.post('/invite', data),
  // 获取邀请会话列表
  getInviteSessions: () => api.get('/invite/sessions'),
  // 停止邀请
  stopInvite: (data) => api.post('/invite/stop', data),
  // 删除邀请
  deleteInvite: (data) => api.post('/invite/delete', data)
}

export default api
