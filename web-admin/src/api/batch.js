/**
 * 批量操作API
 */

import request from '@/utils/request'

export const batchAPI = {
  /**
   * 批量流操作
   * @param {Object} data - 批量操作数据
   * @param {Array} data.targets - 目标流路径列表
   * @param {string} data.operation - 操作类型 (start, stop, delete)
   * @param {Object} data.params - 操作参数
   */
  batchStreamOperation(data) {
    return request({
      url: '/api/v1/batch/streams',
      method: 'post',
      data
    })
  },

  /**
   * 批量客户端操作
   * @param {Object} data - 批量操作数据
   * @param {Array} data.targets - 目标客户端ID列表
   * @param {string} data.operation - 操作类型 (disconnect, kick)
   * @param {Object} data.params - 操作参数
   */
  batchClientOperation(data) {
    return request({
      url: '/api/v1/batch/clients',
      method: 'post',
      data
    })
  },

  /**
   * 批量配置更新
   * @param {Object} data - 批量配置数据
   * @param {Array} data.targets - 目标列表
   * @param {Object} data.params - 配置数据
   */
  batchConfigUpdate(data) {
    return request({
      url: '/api/v1/batch/config',
      method: 'post',
      data
    })
  },

  /**
   * 获取批量操作状态
   * @param {string} operationId - 操作ID
   */
  getBatchOperationStatus(operationId) {
    return request({
      url: '/api/v1/batch/status',
      method: 'get',
      params: { operationId }
    })
  }
}

export const templateAPI = {
  /**
   * 获取模板列表
   * @param {Object} params - 查询参数
   * @param {string} params.category - 模板分类
   */
  getTemplates(params = {}) {
    return request({
      url: '/api/v1/templates',
      method: 'get',
      params
    })
  },

  /**
   * 获取单个模板
   * @param {string} id - 模板ID
   */
  getTemplate(id) {
    return request({
      url: '/api/v1/templates/get',
      method: 'get',
      params: { id }
    })
  },

  /**
   * 创建模板
   * @param {Object} data - 模板数据
   */
  createTemplate(data) {
    return request({
      url: '/api/v1/templates/create',
      method: 'post',
      data
    })
  },

  /**
   * 更新模板
   * @param {string} id - 模板ID
   * @param {Object} data - 模板数据
   */
  updateTemplate(id, data) {
    return request({
      url: '/api/v1/templates/update',
      method: 'put',
      params: { id },
      data
    })
  },

  /**
   * 删除模板
   * @param {string} id - 模板ID
   */
  deleteTemplate(id) {
    return request({
      url: '/api/v1/templates/delete',
      method: 'delete',
      params: { id }
    })
  },

  /**
   * 应用模板
   * @param {Object} data - 应用数据
   * @param {string} data.templateId - 模板ID
   * @param {string} data.targetPath - 目标路径
   * @param {Object} data.params - 应用参数
   */
  applyTemplate(data) {
    return request({
      url: '/api/v1/templates/apply',
      method: 'post',
      data
    })
  },

  /**
   * 导出模板
   * @param {string} id - 模板ID
   */
  exportTemplate(id) {
    return request({
      url: '/api/v1/templates/export',
      method: 'get',
      params: { id }
    })
  },

  /**
   * 导入模板
   * @param {Object} data - 模板数据
   */
  importTemplate(data) {
    return request({
      url: '/api/v1/templates/import',
      method: 'post',
      data
    })
  },

  /**
   * 获取默认模板
   */
  getDefaultTemplates() {
    return request({
      url: '/api/v1/templates/defaults',
      method: 'get'
    })
  }
}
