cmake_minimum_required(VERSION 3.10)
project(MVCamera_OpenCV)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找OpenCV
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")

include_directories(${OpenCV_INCLUDE_DIRS})

# MVSDK路径（可以通过命令行参数 -DMVSDK_INCLUDE_DIR=... 指定）
set(MVSDK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../include" CACHE PATH "MVSDK include directory")
set(MVSDK_LIB_DIR "${CMAKE_SOURCE_DIR}/../../lib" CACHE PATH "MVSDK library directory")

message(STATUS "MVSDK include directory: ${MVSDK_INCLUDE_DIR}")
message(STATUS "MVSDK library directory: ${MVSDK_LIB_DIR}")

# 检查MVSDK头文件是否存在
if(NOT EXISTS "${MVSDK_INCLUDE_DIR}/CameraApi.h")
    message(WARNING "CameraApi.h not found in ${MVSDK_INCLUDE_DIR}")
    message(WARNING "You may need to specify correct MVSDK path using:")
    message(WARNING "  cmake -DMVSDK_INCLUDE_DIR=/path/to/include -DMVSDK_LIB_DIR=/path/to/lib ..")
endif()

# 包含MVSDK头文件
include_directories(${MVSDK_INCLUDE_DIR})

# 添加MVSDK库路径
link_directories(${MVSDK_LIB_DIR})

# 源文件
set(SOURCES
    MVCamera.cpp
    example_opencv_camera.cpp
)

# 创建可执行文件
add_executable(example_opencv_camera ${SOURCES})

# 链接库
target_link_libraries(example_opencv_camera
    ${OpenCV_LIBS}
    MVSDK
    pthread
    rt
)

# 安装规则（可选）
install(TARGETS example_opencv_camera DESTINATION bin)

# 打印编译信息
message(STATUS "==================================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "MVSDK include: ${MVSDK_INCLUDE_DIR}")
message(STATUS "MVSDK library: ${MVSDK_LIB_DIR}")
message(STATUS "==================================================")
