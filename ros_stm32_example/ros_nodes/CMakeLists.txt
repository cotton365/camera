cmake_minimum_required(VERSION 3.0.2)
project(camera_control)

## 编译选项
add_compile_options(-std=c++11)

## 查找依赖包
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  sensor_msgs
  cv_bridge
  image_transport
  message_generation
  serial
)

find_package(OpenCV REQUIRED)

## 消息文件
add_message_files(
  FILES
  DetectionResult.msg
)

generate_messages(
  DEPENDENCIES
  std_msgs
)

## catkin配置
catkin_package(
  CATKIN_DEPENDS
    roscpp rospy std_msgs sensor_msgs
    cv_bridge image_transport message_runtime serial
  DEPENDS OpenCV
)

## 包含目录
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../opencv_example
  ${CMAKE_CURRENT_SOURCE_DIR}/../../GTK_Demo/inc
)

## MVSDK库路径（根据实际路径修改）
# 常见路径：/opt/MVS/lib 或 /usr/lib
link_directories(
  /opt/MVS/lib
  /usr/lib
)

## 相机节点
add_executable(camera_node
  src/camera_node.cpp
  ../../opencv_example/MVCamera.cpp
)
target_link_libraries(camera_node
  ${catkin_LIBRARIES}
  ${OpenCV_LIBS}
  MVSDK
  pthread
  rt
)
add_dependencies(camera_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## 视觉节点
add_executable(vision_node src/vision_node.cpp)
target_link_libraries(vision_node
  ${catkin_LIBRARIES}
  ${OpenCV_LIBS}
)
add_dependencies(vision_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## 串口节点
add_executable(serial_node src/serial_node.cpp)
target_link_libraries(serial_node
  ${catkin_LIBRARIES}
)
add_dependencies(serial_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
