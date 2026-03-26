cmake_minimum_required(VERSION 3.10)
project(CameraApp)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找OpenCV
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")

include_directories(${OpenCV_INCLUDE_DIRS})

# MVSDK路径：默认使用relay目录内的本地include/和lib/
# 也可以通过命令行参数覆盖:
#   cmake -DMVSDK_INCLUDE_DIR=/other/path/include -DMVSDK_LIB_DIR=/other/path/lib ..
set(MVSDK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include" CACHE PATH "MVSDK include directory")
set(MVSDK_LIB_DIR     "${CMAKE_SOURCE_DIR}/lib"     CACHE PATH "MVSDK library directory")

message(STATUS "MVSDK include directory: ${MVSDK_INCLUDE_DIR}")
message(STATUS "MVSDK library directory: ${MVSDK_LIB_DIR}")

# 检查MVSDK头文件是否存在
if(NOT EXISTS "${MVSDK_INCLUDE_DIR}/CameraApi.h")
    message(WARNING "CameraApi.h not found in ${MVSDK_INCLUDE_DIR}")
    message(WARNING "Please copy MVSDK headers into relay/include/")
    message(WARNING "Or specify the path: cmake -DMVSDK_INCLUDE_DIR=/path/to/include ..")
endif()

# 检查MVSDK库文件是否存在
if(NOT EXISTS "${MVSDK_LIB_DIR}/libMVSDK.so")
    message(WARNING "libMVSDK.so not found in ${MVSDK_LIB_DIR}")
    message(WARNING "Please copy libMVSDK.so into relay/lib/")
    message(WARNING "Or specify the path: cmake -DMVSDK_LIB_DIR=/path/to/lib ..")
endif()

# 包含MVSDK头文件
include_directories(${MVSDK_INCLUDE_DIR})

# 添加MVSDK库路径，并设置rpath使程序运行时能自动找到库
link_directories(${MVSDK_LIB_DIR})
set(CMAKE_INSTALL_RPATH "${MVSDK_LIB_DIR}")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# 源文件
set(SOURCES
    MVCamera.cpp
    main.cpp
)

# 创建可执行文件
add_executable(camera_app ${SOURCES})

# 链接库
target_link_libraries(camera_app
    ${OpenCV_LIBS}
    MVSDK
    pthread
    rt
)

# 打印编译信息
message(STATUS "==================================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "MVSDK include: ${MVSDK_INCLUDE_DIR}")
message(STATUS "MVSDK library: ${MVSDK_LIB_DIR}")
message(STATUS "==================================================")
