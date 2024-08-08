cmake_minimum_required(VERSION 3.8)
project(MMP_H26X DESCRIPTION "Multi Media Plugin - H26X Deserialize and Serialize")

option(MMP_H26X_DEBUG_MODE "Enable debug mode" ON)
option(ENBALE_MMP_H26X_SAMPLE "Enbale MMP H26X Sampele" ON)

set(MMP_H26X_SRCS)
set(MMP_H26X_INCS)

list(APPEND MMP_H26X_INCS
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Common
list(APPEND MMP_H26X_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/AbstractH26xByteReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/H26xBinaryReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/H26xBinaryReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/H26xUltis.h
    ${CMAKE_CURRENT_SOURCE_DIR}/H26xUltis.cpp
)

# H264
list(APPEND MMP_H26X_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/H264Common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/H264Common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/H264Deserialize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/H264Deserialize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/H264SliceDecodingProcess.h
    ${CMAKE_CURRENT_SOURCE_DIR}/H264SliceDecodingProcess.cpp
)

# H265
list(APPEND MMP_H26X_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/H265Common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/H265Common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/H265Deserialize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/H265Deserialize.cpp
)

add_library(MMP_H26X STATIC ${MMP_H26X_SRCS})
add_library(MMP::H26x ALIAS MMP_H26X)
target_include_directories(MMP_H26X PUBLIC ${MMP_H26X_INCS})
if (MMP_H26X_DEBUG_MODE)
    target_compile_definitions(MMP_H26X PUBLIC MMP_H26X_DEBUG_MODE)
endif()

if (ENBALE_MMP_H26X_SAMPLE)
    add_executable(Sample ${MMP_H26X_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
    target_include_directories(Sample PUBLIC ${MMP_H26X_INCS})
    if (MMP_H26X_DEBUG_MODE)
        target_compile_definitions(Sample PUBLIC MMP_H26X_DEBUG_MODE)
    endif()
    if (UNIX)
        target_link_libraries(Sample asan)
        target_compile_options(Sample PUBLIC -fsanitize=address)
    endif()
endif()