import axios from 'axios';

// 创建 axios 实例
const service = axios.create({
  baseURL: "", // 从环境变量中获取 API 基础路径
  timeout: 5000 // 请求超时时间
});

// 请求拦截器
service.interceptors.request.use(
  config => {
    // 在发送请求之前做些什么，例如添加 token
    // const token = getToken();
    // if (token) {
    //   config.headers['Authorization'] = `Bearer ${token}`;
    // }
    return config;
  },
  error => {
    // 处理请求错误
    console.log(error); // for debug
    return Promise.reject(error);
  }
);

// 响应拦截器
service.interceptors.response.use(
  response => {
    const res = response.data;
    // 根据具体的业务状态码进行处理
    if (res.code !== 200) {
      // 处理错误信息
      console.error(res.message || 'Error');
      return Promise.reject(new Error(res.message || 'Error'));
    } else {
      return res;
    }
  },
  error => {
    // 处理响应错误
    console.log('err' + error); // for debug
    return Promise.reject(error);
  }
);

export default service;
