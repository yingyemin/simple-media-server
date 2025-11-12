aux_source_directory(. TEST_SRC_LIST)

# 要遍历的根文件夹
set(INCLUDE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# 递归获取所有文件路径
file(GLOB_RECURSE ALL_FILES ${INCLUDE_ROOT_DIR}/*.h)

if (WIN32)
    find_package(OpenSSL QUIET)
	message(STATUS "找到openssl库:\"${OPENSSL_INCLUDE_DIR}\",ENABLE_OPENSSL宏已打开")
	include_directories(${OPENSSL_INCLUDE_DIR})
	add_definitions(-DENABLE_OPENSSL)
	list(APPEND BASE_LIB_LIST ${OPENSSL_LIBRARIES})
endif()
	
set(INCLUDE_DIRS "")
foreach(FILE_PATH ${ALL_FILES})
    # 打印文件路径
    message(STATUS "Found file: ${FILE_PATH}")
    get_filename_component(DIR_PATH ${FILE_PATH} DIRECTORY)
    list(FIND INCLUDE_DIRS ${DIR_PATH} DIR_FOUND)
    if(DIR_FOUND EQUAL -1)
        list(APPEND INCLUDE_DIRS ${DIR_PATH})
    endif()
endforeach()
# 全局添加包含目录
include_directories(${INCLUDE_DIRS})
message(STATUS "include path: ${INCLUDE_DIRS}")

foreach(TEST_SRC ${TEST_SRC_LIST})
	get_filename_component(TEST_EXE_NAME ${TEST_SRC} NAME_WE)
	message(STATUS "add test: ${TEST_EXE_NAME}")
	add_executable(${TEST_EXE_NAME} ${TEST_SRC})
	
	if(USE_SOLUTION_FOLDERS)
		SET_PROPERTY(TARGET ${TEST_EXE_NAME} PROPERTY FOLDER "test")
	endif()
	
	target_link_libraries(${TEST_EXE_NAME} base ${BASE_LIB_LIST})
endforeach()