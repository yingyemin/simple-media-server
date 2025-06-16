import axios from 'axios';

// 假设服务器信息接口地址
const SERVER_INFO_API = '/api/v1/server/info';

const serverAPI = {
  /**
   * 获取服务器统计信息
   * @returns {Promise<object>} 包含服务器统计信息的 Promise
   */
  async getServerInfo() {
    try {
      const response = await axios.get(SERVER_INFO_API);
      return response;
    } catch (error) {
      console.error('Failed to fetch server info:', error);
      throw error;
    }
  }
};

export default serverAPI;
