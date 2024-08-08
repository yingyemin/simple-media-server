cmake_minimum_required(VERSION 3.0)

# 设置项目库文件输出的路径
# set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lars_reactor/lib)

# 定义参与编译的源代码文件 
aux_source_directory(. SRC_LIST)

# 编译生成动态库liblreactor
add_library(base SHARED ${SRC_LIST})