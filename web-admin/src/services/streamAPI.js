import axios from 'axios';

// 假设流列表接口地址
const STREAM_LIST_API = '/api/v1/stream/list';

const streamAPI = {
  /**
   * 获取流列表信息
   * @returns {Promise<object>} 包含流列表信息的 Promise
   */
  async getStreamList() {
    try {
      const response = await axios.get(STREAM_LIST_API);
      return response;
    } catch (error) {
      console.error('Failed to fetch stream list:', error);
      throw error;
    }
  }
};

export default streamAPI;
