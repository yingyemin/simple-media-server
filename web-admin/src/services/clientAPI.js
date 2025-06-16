import axios from 'axios';

// 假设客户端列表接口地址
const CLIENT_LIST_API = '/api/v1/client/list';

const clientAPI = {
  /**
   * 获取客户端列表信息
   * @returns {Promise<object>} 包含客户端列表信息的 Promise
   */
  async getClientList() {
    try {
      const response = await axios.get(CLIENT_LIST_API);
      return response;
    } catch (error) {
      console.error('Failed to fetch client list:', error);
      throw error;
    }
  }
};

export default clientAPI;
