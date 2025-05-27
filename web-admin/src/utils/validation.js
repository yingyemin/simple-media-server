// 表单验证工具

// 常用正则表达式
export const patterns = {
  // 应用名和流名：只允许字母、数字、下划线、连字符
  appName: /^[a-zA-Z0-9_-]+$/,
  streamName: /^[a-zA-Z0-9_-]+$/,
  
  // IP地址
  ip: /^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/,
  
  // 端口号
  port: /^([1-9]|[1-9]\d{1,3}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5])$/,
  
  // 分辨率格式
  resolution: /^\d+x\d+$/,
  
  // URL格式
  url: /^(https?|rtmp|rtsp|rtp):\/\/[^\s]+$/,
  
  // SSRC格式（十六进制）
  ssrc: /^(0x)?[0-9a-fA-F]+$/
}

// 验证规则生成器
export const createValidationRules = {
  // 必填字段
  required: (message = '此字段为必填项') => ({
    required: true,
    message,
    trigger: 'blur'
  }),

  // 字符串长度
  length: (min, max, message) => ({
    min,
    max,
    message: message || `长度应在 ${min} 到 ${max} 个字符之间`,
    trigger: 'blur'
  }),

  // 正则表达式验证
  pattern: (pattern, message) => ({
    pattern,
    message,
    trigger: 'blur'
  }),

  // 数值范围
  range: (min, max, message) => ({
    type: 'number',
    min,
    max,
    message: message || `数值应在 ${min} 到 ${max} 之间`,
    trigger: 'blur'
  }),

  // 自定义验证器
  custom: (validator, message) => ({
    validator: (rule, value, callback) => {
      if (validator(value)) {
        callback()
      } else {
        callback(new Error(message))
      }
    },
    trigger: 'blur'
  })
}

// 常用验证规则
export const commonRules = {
  // 应用名验证
  appName: [
    createValidationRules.required('请输入应用名'),
    createValidationRules.length(1, 50),
    createValidationRules.pattern(patterns.appName, '应用名只能包含字母、数字、下划线和连字符')
  ],

  // 流名验证
  streamName: [
    createValidationRules.required('请输入流名'),
    createValidationRules.length(1, 100),
    createValidationRules.pattern(patterns.streamName, '流名只能包含字母、数字、下划线和连字符')
  ],

  // IP地址验证
  ip: [
    createValidationRules.required('请输入IP地址'),
    createValidationRules.pattern(patterns.ip, '请输入有效的IP地址')
  ],

  // 端口验证
  port: [
    createValidationRules.required('请输入端口号'),
    createValidationRules.range(1, 65535, '端口号应在 1 到 65535 之间')
  ],

  // URL验证
  url: [
    createValidationRules.required('请输入URL'),
    createValidationRules.pattern(patterns.url, '请输入有效的URL地址')
  ],

  // 分辨率验证
  resolution: [
    createValidationRules.pattern(patterns.resolution, '分辨率格式应为 宽x高，如 1920x1080')
  ],

  // 比特率验证
  bitrate: [
    createValidationRules.range(100, 50000, '比特率应在 100 到 50000 kbps 之间')
  ],

  // 帧率验证
  framerate: [
    createValidationRules.range(1, 120, '帧率应在 1 到 120 fps 之间')
  ],

  // SSRC验证
  ssrc: [
    createValidationRules.required('请输入SSRC'),
    createValidationRules.pattern(patterns.ssrc, 'SSRC格式不正确')
  ]
}

// 表单验证辅助函数
export const validateForm = (formRef) => {
  return new Promise((resolve, reject) => {
    if (!formRef) {
      reject(new Error('表单引用不存在'))
      return
    }

    formRef.validate((valid, fields) => {
      if (valid) {
        resolve(true)
      } else {
        // 获取第一个错误信息
        const firstError = Object.values(fields)[0][0]
        reject(new Error(firstError.message))
      }
    })
  })
}

// 重置表单
export const resetForm = (formRef) => {
  if (formRef) {
    formRef.resetFields()
  }
}

// 清除表单验证
export const clearValidation = (formRef) => {
  if (formRef) {
    formRef.clearValidate()
  }
}

// 验证单个字段
export const validateField = (formRef, fieldName) => {
  return new Promise((resolve, reject) => {
    if (!formRef) {
      reject(new Error('表单引用不存在'))
      return
    }

    formRef.validateField(fieldName, (errorMessage) => {
      if (errorMessage) {
        reject(new Error(errorMessage))
      } else {
        resolve(true)
      }
    })
  })
}

// 错误信息格式化
export const formatErrorMessage = (error) => {
  if (typeof error === 'string') {
    return error
  }
  
  if (error && error.message) {
    return error.message
  }
  
  return '未知错误'
}

// 表单数据验证（在提交前进行额外验证）
export const validateFormData = (data, rules) => {
  const errors = []
  
  for (const [field, fieldRules] of Object.entries(rules)) {
    const value = data[field]
    
    for (const rule of fieldRules) {
      if (rule.required && (!value || value === '')) {
        errors.push(`${field}: ${rule.message || '此字段为必填项'}`)
        break
      }
      
      if (value && rule.pattern && !rule.pattern.test(value)) {
        errors.push(`${field}: ${rule.message || '格式不正确'}`)
        break
      }
      
      if (value && rule.min !== undefined && value.length < rule.min) {
        errors.push(`${field}: ${rule.message || `长度不能少于${rule.min}个字符`}`)
        break
      }
      
      if (value && rule.max !== undefined && value.length > rule.max) {
        errors.push(`${field}: ${rule.message || `长度不能超过${rule.max}个字符`}`)
        break
      }
    }
  }
  
  return errors
}
