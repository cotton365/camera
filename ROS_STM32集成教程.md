# ROS与STM32通信集成教程

## 概述

本教程说明如何将相机识别结果从ROS框架发送到STM32电控板。完整的数据流如下：

```
相机采集 → OpenCV处理 → 识别算法 → ROS节点 → 串口通信 → STM32板子
```

## 目录

1. [系统架构](#系统架构)
2. [ROS节点开发](#ros节点开发)
3. [串口通信协议](#串口通信协议)
4. [STM32接收端代码](#stm32接收端代码)
5. [完整示例代码](#完整示例代码)
6. [常见问题解决](#常见问题解决)

---

## 系统架构

### 整体架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        ROS端（上位机）                        │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────┐     ┌──────────┐     ┌──────────────────┐    │
│  │  相机节点  │────>│  识别节点  │────>│  串口通信节点    │    │
│  │ MVCamera │     │  Vision   │     │  SerialComm     │    │
│  └──────────┘     └──────────┘     └──────────────────┘    │
│       │                 │                     │              │
│       │                 │                     │              │
│       v                 v                     v              │
│   cv::Mat          识别结果              数据包封装          │
│  (图像数据)         (坐标/类别)           (串口协议)         │
└───────────────────────────────────────────────┼─────────────┘
                                                 │
                                          USB转串口/UART
                                                 │
┌───────────────────────────────────────────────┼─────────────┐
│                                                v              │
│                        STM32端（下位机）                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────┐     ┌──────────┐     ┌──────────────────┐    │
│  │ 串口接收  │────>│  数据解析  │────>│  电机控制/执行   │    │
│  │  UART    │     │  Parser   │     │  Motor/Actuator │    │
│  └──────────┘     └──────────┘     └──────────────────┘    │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### 通信方式选择

| 通信方式 | 优点 | 缺点 | 适用场景 |
|---------|------|------|---------|
| **串口UART** | 简单可靠，硬件支持好 | 速度较慢(115200bps) | 控制命令传输 ✅推荐 |
| **USB虚拟串口** | 速度快，即插即用 | 需要USB支持 | 大数据量传输 |
| **CAN总线** | 抗干扰强，多节点 | 需要额外硬件 | 工业环境 |
| **以太网** | 速度最快 | 硬件成本高 | 复杂系统 |

**推荐方案：USB转串口（UART）**
- 成本低（￥10-30）
- 即插即用
- ROS和STM32都有成熟支持
- 满足实时控制需求

---

## ROS节点开发

### 1. ROS工作空间设置

```bash
# 创建ROS工作空间
mkdir -p ~/catkin_ws/src
cd ~/catkin_ws/src

# 创建功能包
catkin_create_pkg camera_control \
    roscpp rospy std_msgs sensor_msgs cv_bridge \
    image_transport serial

cd ~/catkin_ws
catkin_make
source devel/setup.bash
```

### 2. 相机节点 (camera_node)

创建文件: `~/catkin_ws/src/camera_control/src/camera_node.cpp`

```cpp
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include "MVCamera.h"

/**
 * @brief 相机发布节点
 * 功能：从MVSDK相机采集图像并发布到ROS话题
 */
class CameraNode {
private:
    ros::NodeHandle nh_;
    image_transport::ImageTransport it_;
    image_transport::Publisher image_pub_;

    MVCamera camera_;
    int camera_index_;
    double exposure_time_;
    int gain_;

public:
    CameraNode() : it_(nh_), camera_index_(0),
                   exposure_time_(10000.0), gain_(100) {

        // 获取参数
        nh_.param("camera_index", camera_index_, 0);
        nh_.param("exposure_time", exposure_time_, 10000.0);
        nh_.param("gain", gain_, 100);

        // 发布图像话题
        image_pub_ = it_.advertise("camera/image_raw", 1);

        // 初始化相机
        if (!camera_.init(camera_index_)) {
            ROS_ERROR("Failed to initialize camera %d", camera_index_);
            ros::shutdown();
            return;
        }

        // 设置参数
        camera_.setExposure(exposure_time_);
        camera_.setGain(gain_);

        // 打开相机
        if (!camera_.open()) {
            ROS_ERROR("Failed to open camera");
            ros::shutdown();
            return;
        }

        ROS_INFO("Camera node initialized successfully");
    }

    ~CameraNode() {
        camera_.close();
        camera_.release();
    }

    void spin() {
        ros::Rate loop_rate(30); // 30 FPS
        cv::Mat frame;

        while (ros::ok()) {
            // 捕获图像
            if (camera_.capture(frame)) {
                // 转换为ROS消息
                sensor_msgs::ImagePtr msg =
                    cv_bridge::CvImage(std_msgs::Header(),
                                      "bgr8",
                                      frame).toImageMsg();

                msg->header.stamp = ros::Time::now();
                msg->header.frame_id = "camera_frame";

                // 发布
                image_pub_.publish(msg);
            }

            ros::spinOnce();
            loop_rate.sleep();
        }
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "camera_node");

    CameraNode node;
    node.spin();

    return 0;
}
```

### 3. 视觉识别节点 (vision_node)

创建文件: `~/catkin_ws/src/camera_control/src/vision_node.cpp`

```cpp
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <std_msgs/String.h>
#include "camera_control/DetectionResult.h"

/**
 * @brief 视觉识别节点
 * 功能：接收图像，进行目标识别，发布识别结果
 */
class VisionNode {
private:
    ros::NodeHandle nh_;
    image_transport::ImageTransport it_;
    image_transport::Subscriber image_sub_;
    ros::Publisher result_pub_;

    // 识别参数
    cv::Scalar lower_color_;
    cv::Scalar upper_color_;

public:
    VisionNode() : it_(nh_) {
        // 订阅图像话题
        image_sub_ = it_.subscribe("camera/image_raw", 1,
                                   &VisionNode::imageCallback, this);

        // 发布识别结果话题
        result_pub_ = nh_.advertise<camera_control::DetectionResult>(
            "vision/detection_result", 1);

        // 颜色阈值（示例：红色物体）
        lower_color_ = cv::Scalar(0, 100, 100);    // HSV下限
        upper_color_ = cv::Scalar(10, 255, 255);   // HSV上限

        ROS_INFO("Vision node started");
    }

    void imageCallback(const sensor_msgs::ImageConstPtr& msg) {
        // 转换ROS图像消息为OpenCV Mat
        cv_bridge::CvImagePtr cv_ptr;
        try {
            cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
        } catch (cv_bridge::Exception& e) {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return;
        }

        // 进行目标检测
        cv::Mat hsv, mask;
        cv::cvtColor(cv_ptr->image, hsv, cv::COLOR_BGR2HSV);
        cv::inRange(hsv, lower_color_, upper_color_, mask);

        // 形态学操作去噪
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);

        // 查找轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL,
                        cv::CHAIN_APPROX_SIMPLE);

        // 发布最大的目标
        if (!contours.empty()) {
            // 找最大轮廓
            double max_area = 0;
            int max_idx = 0;
            for (size_t i = 0; i < contours.size(); i++) {
                double area = cv::contourArea(contours[i]);
                if (area > max_area) {
                    max_area = area;
                    max_idx = i;
                }
            }

            // 计算中心点
            cv::Moments m = cv::moments(contours[max_idx]);
            int center_x = int(m.m10 / m.m00);
            int center_y = int(m.m01 / m.m00);

            // 发布结果
            camera_control::DetectionResult result;
            result.header.stamp = ros::Time::now();
            result.detected = true;
            result.x = center_x;
            result.y = center_y;
            result.width = cv_ptr->image.cols;
            result.height = cv_ptr->image.rows;
            result.object_type = "red_object";
            result.confidence = max_area / (cv_ptr->image.cols * cv_ptr->image.rows);

            result_pub_.publish(result);

            ROS_INFO("Detected at (%d, %d), area: %.0f",
                     center_x, center_y, max_area);
        }
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "vision_node");

    VisionNode node;
    ros::spin();

    return 0;
}
```

### 4. 串口通信节点 (serial_node)

创建文件: `~/catkin_ws/src/camera_control/src/serial_node.cpp`

```cpp
#include <ros/ros.h>
#include <serial/serial.h>
#include "camera_control/DetectionResult.h"
#include <vector>

/**
 * @brief 串口通信节点
 * 功能：接收识别结果，通过串口发送给STM32
 */
class SerialNode {
private:
    ros::NodeHandle nh_;
    ros::Subscriber result_sub_;
    serial::Serial serial_port_;

    std::string port_name_;
    int baudrate_;

    // 数据包结构
    struct DataPacket {
        uint8_t header1;      // 帧头1: 0xAA
        uint8_t header2;      // 帧头2: 0x55
        uint8_t cmd;          // 命令字
        int16_t x;            // X坐标
        int16_t y;            // Y坐标
        uint8_t obj_type;     // 物体类型
        uint8_t checksum;     // 校验和
    } __attribute__((packed));

public:
    SerialNode() : port_name_("/dev/ttyUSB0"), baudrate_(115200) {
        // 获取串口参数
        nh_.param<std::string>("serial_port", port_name_, "/dev/ttyUSB0");
        nh_.param("baudrate", baudrate_, 115200);

        // 订阅识别结果
        result_sub_ = nh_.subscribe("vision/detection_result", 1,
                                    &SerialNode::resultCallback, this);

        // 配置串口
        try {
            serial_port_.setPort(port_name_);
            serial_port_.setBaudrate(baudrate_);
            serial::Timeout timeout = serial::Timeout::simpleTimeout(1000);
            serial_port_.setTimeout(timeout);

            serial_port_.open();

            if (serial_port_.isOpen()) {
                ROS_INFO("Serial port %s opened at %d baud",
                         port_name_.c_str(), baudrate_);
            } else {
                ROS_ERROR("Failed to open serial port");
                ros::shutdown();
            }
        } catch (serial::IOException& e) {
            ROS_ERROR("Unable to open port: %s", e.what());
            ros::shutdown();
        }
    }

    ~SerialNode() {
        if (serial_port_.isOpen()) {
            serial_port_.close();
        }
    }

    void resultCallback(const camera_control::DetectionResult::ConstPtr& msg) {
        if (!msg->detected) {
            return;
        }

        // 构建数据包
        DataPacket packet;
        packet.header1 = 0xAA;
        packet.header2 = 0x55;
        packet.cmd = 0x01;  // 命令：目标位置
        packet.x = static_cast<int16_t>(msg->x);
        packet.y = static_cast<int16_t>(msg->y);
        packet.obj_type = getObjectType(msg->object_type);

        // 计算校验和（简单累加校验）
        packet.checksum = calculateChecksum(packet);

        // 发送数据
        try {
            std::vector<uint8_t> data(sizeof(DataPacket));
            memcpy(data.data(), &packet, sizeof(DataPacket));

            size_t bytes_written = serial_port_.write(data);

            ROS_INFO("Sent to STM32: x=%d, y=%d, type=%d [%zu bytes]",
                     packet.x, packet.y, packet.obj_type, bytes_written);

        } catch (serial::IOException& e) {
            ROS_ERROR("Serial write error: %s", e.what());
        }
    }

private:
    uint8_t getObjectType(const std::string& type) {
        if (type == "red_object") return 1;
        if (type == "blue_object") return 2;
        if (type == "green_object") return 3;
        return 0;  // 未知类型
    }

    uint8_t calculateChecksum(const DataPacket& packet) {
        uint8_t sum = 0;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&packet);
        // 校验和不包括最后一个字节（校验和本身）
        for (size_t i = 0; i < sizeof(DataPacket) - 1; i++) {
            sum += data[i];
        }
        return sum;
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "serial_node");

    SerialNode node;
    ros::spin();

    return 0;
}
```

---

## 串口通信协议

### 数据包格式

```
┌────────┬────────┬────────┬────────┬────────┬────────┬────────┐
│ 帧头1  │ 帧头2  │ 命令字 │  X坐标  │  Y坐标  │ 类型   │ 校验和 │
│ 0xAA   │ 0x55   │ 1 byte │ 2 bytes│ 2 bytes│ 1 byte │ 1 byte │
└────────┴────────┴────────┴────────┴────────┴────────┴────────┘
总长度：8字节
```

### 字段说明

| 字段 | 长度 | 说明 | 示例 |
|-----|------|------|------|
| 帧头1 | 1字节 | 固定为0xAA，用于帧同步 | 0xAA |
| 帧头2 | 1字节 | 固定为0x55，用于帧同步 | 0x55 |
| 命令字 | 1字节 | 0x01=目标位置, 0x02=控制命令 | 0x01 |
| X坐标 | 2字节 | 目标X坐标（小端序） | 0x0140 (320) |
| Y坐标 | 2字节 | 目标Y坐标（小端序） | 0x00F0 (240) |
| 类型 | 1字节 | 目标类型：1=红色, 2=蓝色, 3=绿色 | 0x01 |
| 校验和 | 1字节 | 前7字节累加和（模256） | 0x3F |

### 命令字定义

```c
#define CMD_TARGET_POSITION    0x01  // 目标位置信息
#define CMD_CONTROL_MOTOR      0x02  // 电机控制命令
#define CMD_SET_MODE           0x03  // 设置工作模式
#define CMD_HEARTBEAT          0x04  // 心跳包
#define CMD_STOP               0x05  // 紧急停止
```

### 通信示例

**示例1：发送目标位置**
```
发送: AA 55 01 40 01 F0 00 01 3F
解析: 帧头=0xAA55, 命令=0x01, X=320, Y=240, 类型=红色, 校验=0x3F
```

**示例2：控制电机**
```
发送: AA 55 02 64 00 C8 00 00 DD
解析: 帧头=0xAA55, 命令=0x02, 速度1=100, 速度2=200, 保留=0, 校验=0xDD
```

---

## STM32接收端代码

### STM32 HAL库串口接收（推荐）

创建文件: `serial_protocol.h`

```c
#ifndef __SERIAL_PROTOCOL_H
#define __SERIAL_PROTOCOL_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

// 数据包结构（与ROS端保持一致）
typedef struct {
    uint8_t header1;      // 0xAA
    uint8_t header2;      // 0x55
    uint8_t cmd;          // 命令字
    int16_t x;            // X坐标
    int16_t y;            // Y坐标
    uint8_t obj_type;     // 物体类型
    uint8_t checksum;     // 校验和
} __attribute__((packed)) DataPacket;

// 接收状态机
typedef enum {
    STATE_WAIT_HEADER1,
    STATE_WAIT_HEADER2,
    STATE_RECEIVE_DATA,
    STATE_COMPLETE
} ReceiveState;

// 串口协议处理器
typedef struct {
    ReceiveState state;
    uint8_t rx_buffer[sizeof(DataPacket)];
    uint8_t rx_index;
    DataPacket current_packet;
} SerialProtocol;

// 函数声明
void SerialProtocol_Init(SerialProtocol* protocol);
bool SerialProtocol_ProcessByte(SerialProtocol* protocol, uint8_t byte);
uint8_t SerialProtocol_CalculateChecksum(const DataPacket* packet);
bool SerialProtocol_ValidatePacket(const DataPacket* packet);

#endif
```

创建文件: `serial_protocol.c`

```c
#include "serial_protocol.h"
#include <string.h>

/**
 * @brief 初始化串口协议处理器
 */
void SerialProtocol_Init(SerialProtocol* protocol) {
    protocol->state = STATE_WAIT_HEADER1;
    protocol->rx_index = 0;
    memset(protocol->rx_buffer, 0, sizeof(protocol->rx_buffer));
}

/**
 * @brief 处理接收到的单个字节
 * @return true表示接收到完整数据包
 */
bool SerialProtocol_ProcessByte(SerialProtocol* protocol, uint8_t byte) {
    switch (protocol->state) {
        case STATE_WAIT_HEADER1:
            if (byte == 0xAA) {
                protocol->rx_buffer[0] = byte;
                protocol->rx_index = 1;
                protocol->state = STATE_WAIT_HEADER2;
            }
            break;

        case STATE_WAIT_HEADER2:
            if (byte == 0x55) {
                protocol->rx_buffer[1] = byte;
                protocol->rx_index = 2;
                protocol->state = STATE_RECEIVE_DATA;
            } else {
                protocol->state = STATE_WAIT_HEADER1;
            }
            break;

        case STATE_RECEIVE_DATA:
            protocol->rx_buffer[protocol->rx_index++] = byte;
            if (protocol->rx_index >= sizeof(DataPacket)) {
                // 接收完整
                memcpy(&protocol->current_packet, protocol->rx_buffer,
                       sizeof(DataPacket));
                protocol->state = STATE_COMPLETE;
                return true;
            }
            break;

        case STATE_COMPLETE:
            // 重置状态机
            protocol->state = STATE_WAIT_HEADER1;
            protocol->rx_index = 0;
            break;
    }

    return false;
}

/**
 * @brief 计算校验和
 */
uint8_t SerialProtocol_CalculateChecksum(const DataPacket* packet) {
    uint8_t sum = 0;
    const uint8_t* data = (const uint8_t*)packet;
    for (size_t i = 0; i < sizeof(DataPacket) - 1; i++) {
        sum += data[i];
    }
    return sum;
}

/**
 * @brief 验证数据包
 */
bool SerialProtocol_ValidatePacket(const DataPacket* packet) {
    // 检查帧头
    if (packet->header1 != 0xAA || packet->header2 != 0x55) {
        return false;
    }

    // 检查校验和
    uint8_t calculated = SerialProtocol_CalculateChecksum(packet);
    if (calculated != packet->checksum) {
        return false;
    }

    return true;
}
```

### STM32主程序示例

创建文件: `main.c` (关键部分)

```c
#include "main.h"
#include "serial_protocol.h"
#include <stdio.h>

// 全局变量
UART_HandleTypeDef huart1;
SerialProtocol serial_protocol;
uint8_t uart_rx_byte;

// 函数声明
void SystemClock_Config(void);
void MX_USART1_UART_Init(void);
void ProcessDetectionData(const DataPacket* packet);
void ControlMotor(int16_t target_x, int16_t target_y);

int main(void) {
    // HAL库初始化
    HAL_Init();
    SystemClock_Config();

    // 初始化串口
    MX_USART1_UART_Init();

    // 初始化协议处理器
    SerialProtocol_Init(&serial_protocol);

    // 启动串口中断接收
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);

    printf("STM32 Ready, waiting for ROS data...\r\n");

    while (1) {
        // 主循环处理其他任务
        HAL_Delay(10);
    }
}

/**
 * @brief 串口接收中断回调
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // 处理接收到的字节
        if (SerialProtocol_ProcessByte(&serial_protocol, uart_rx_byte)) {
            // 接收到完整数据包
            if (SerialProtocol_ValidatePacket(&serial_protocol.current_packet)) {
                // 数据包有效，进行处理
                ProcessDetectionData(&serial_protocol.current_packet);
            } else {
                printf("Invalid packet received!\r\n");
            }

            // 重置状态机
            SerialProtocol_Init(&serial_protocol);
        }

        // 继续接收下一个字节
        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}

/**
 * @brief 处理检测数据
 */
void ProcessDetectionData(const DataPacket* packet) {
    printf("Received: cmd=0x%02X, x=%d, y=%d, type=%d\r\n",
           packet->cmd, packet->x, packet->y, packet->obj_type);

    switch (packet->cmd) {
        case 0x01:  // 目标位置
            ControlMotor(packet->x, packet->y);
            break;

        case 0x02:  // 电机控制
            // 处理电机控制命令
            break;

        case 0x05:  // 紧急停止
            // 停止所有电机
            printf("Emergency STOP!\r\n");
            break;

        default:
            printf("Unknown command: 0x%02X\r\n", packet->cmd);
            break;
    }
}

/**
 * @brief 根据目标位置控制电机
 */
void ControlMotor(int16_t target_x, int16_t target_y) {
    // 这里实现你的控制逻辑
    // 示例：简单的比例控制

    int16_t center_x = 320;  // 图像中心
    int16_t center_y = 240;

    int16_t error_x = target_x - center_x;
    int16_t error_y = target_y - center_y;

    // 计算电机速度（示例）
    int motor_speed_x = error_x / 10;  // 简单比例
    int motor_speed_y = error_y / 10;

    printf("Motor control: Vx=%d, Vy=%d\r\n", motor_speed_x, motor_speed_y);

    // TODO: 实际的PWM/GPIO控制
    // HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    // __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, motor_speed_x);
}

/**
 * @brief 串口初始化（HAL生成）
 */
void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}
```

---

## 完整示例代码

### ROS端CMakeLists.txt

创建文件: `~/catkin_ws/src/camera_control/CMakeLists.txt`

```cmake
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
  ${CMAKE_CURRENT_SOURCE_DIR}/../opencv_example
  ${CMAKE_CURRENT_SOURCE_DIR}/../GTK_Demo/inc
)

## MVSDK库路径（根据实际路径修改）
link_directories(
  /opt/MVS/lib
)

## 相机节点
add_executable(camera_node
  src/camera_node.cpp
  ../opencv_example/MVCamera.cpp
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
```

### ROS消息定义

创建文件: `~/catkin_ws/src/camera_control/msg/DetectionResult.msg`

```
Header header
bool detected
int32 x
int32 y
int32 width
int32 height
string object_type
float32 confidence
```

### Launch文件

创建文件: `~/catkin_ws/src/camera_control/launch/camera_system.launch`

```xml
<?xml version="1.0"?>
<launch>
    <!-- 相机参数 -->
    <arg name="camera_index" default="0"/>
    <arg name="exposure_time" default="10000.0"/>
    <arg name="gain" default="100"/>

    <!-- 串口参数 -->
    <arg name="serial_port" default="/dev/ttyUSB0"/>
    <arg name="baudrate" default="115200"/>

    <!-- 相机节点 -->
    <node name="camera_node" pkg="camera_control" type="camera_node" output="screen">
        <param name="camera_index" value="$(arg camera_index)"/>
        <param name="exposure_time" value="$(arg exposure_time)"/>
        <param name="gain" value="$(arg gain)"/>
    </node>

    <!-- 视觉识别节点 -->
    <node name="vision_node" pkg="camera_control" type="vision_node" output="screen"/>

    <!-- 串口通信节点 -->
    <node name="serial_node" pkg="camera_control" type="serial_node" output="screen">
        <param name="serial_port" value="$(arg serial_port)"/>
        <param name="baudrate" value="$(arg baudrate)"/>
    </node>

    <!-- RViz可视化（可选） -->
    <node name="image_view" pkg="image_view" type="image_view" respawn="false" output="screen">
        <remap from="image" to="/camera/image_raw"/>
    </node>
</launch>
```

---

## 编译和运行

### 1. 编译ROS工作空间

```bash
cd ~/catkin_ws
catkin_make

# 如果出错，清理后重新编译
catkin_make clean
catkin_make

source devel/setup.bash
```

### 2. 配置串口权限

```bash
# 查看串口设备
ls -l /dev/ttyUSB*

# 添加用户到dialout组（永久）
sudo usermod -a -G dialout $USER

# 或者临时授权
sudo chmod 666 /dev/ttyUSB0
```

### 3. 启动系统

```bash
# 启动完整系统
roslaunch camera_control camera_system.launch

# 或者分别启动各节点
roscore
rosrun camera_control camera_node
rosrun camera_control vision_node
rosrun camera_control serial_node
```

### 4. 测试串口通信

```bash
# 查看话题
rostopic list

# 查看识别结果
rostopic echo /vision/detection_result

# 测试串口（在另一个终端）
sudo cat /dev/ttyUSB0
```

---

## 常见问题解决

### 1. 串口问题

**问题：无法打开串口 `/dev/ttyUSB0`**

```bash
# 解决方案1：检查设备
ls /dev/tty*
dmesg | grep tty

# 解决方案2：权限问题
sudo chmod 666 /dev/ttyUSB0
sudo usermod -a -G dialout $USER

# 解决方案3：查看是否被占用
sudo lsof /dev/ttyUSB0
```

**问题：STM32没有收到数据**

```bash
# 检查1：ROS是否正常发送
rostopic echo /vision/detection_result

# 检查2：串口是否正常
sudo minicom -D /dev/ttyUSB0 -b 115200

# 检查3：STM32串口配置
# 确认波特率、停止位、校验位匹配
```

### 2. 相机问题

**问题：找不到相机**

```bash
# 检查相机连接
lsusb

# 检查MVSDK库
export LD_LIBRARY_PATH=/opt/MVS/lib:$LD_LIBRARY_PATH

# 运行相机测试
cd ~/catkin_ws/src/camera/../opencv_example
./example_opencv_camera
```

### 3. ROS编译问题

**问题：找不到MVCamera.h**

```cmake
# 在CMakeLists.txt中添加路径
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../opencv_example
)
```

**问题：serial库未找到**

```bash
# 安装serial包
sudo apt-get install ros-${ROS_DISTRO}-serial
```

### 4. 数据校验失败

**问题：STM32提示"Invalid packet"**

```c
// 在STM32端添加调试输出
printf("Received bytes: ");
for(int i = 0; i < sizeof(DataPacket); i++) {
    printf("%02X ", rx_buffer[i]);
}
printf("\r\n");

// 检查校验和计算是否一致
printf("Calculated: 0x%02X, Received: 0x%02X\r\n",
       calculated_sum, packet->checksum);
```

### 5. 性能优化

**优化1：降低发送频率**

```cpp
// 在serial_node中添加频率限制
ros::Rate rate(10);  // 10Hz
rate.sleep();
```

**优化2：只发送变化的数据**

```cpp
// 记录上一次的位置
static int last_x = -1, last_y = -1;
if (abs(msg->x - last_x) > 5 || abs(msg->y - last_y) > 5) {
    // 只有位置变化超过阈值才发送
    sendToSerial(msg);
    last_x = msg->x;
    last_y = msg->y;
}
```

---

## 调试技巧

### 1. ROS调试工具

```bash
# 查看节点列表
rosnode list

# 查看话题
rostopic list
rostopic echo /vision/detection_result

# 查看节点图
rqt_graph

# 查看图像
rosrun image_view image_view image:=/camera/image_raw
```

### 2. 串口监控

```bash
# 方法1：使用minicom
sudo minicom -D /dev/ttyUSB0 -b 115200

# 方法2：使用screen
sudo screen /dev/ttyUSB0 115200

# 方法3：使用cat（只读）
sudo cat /dev/ttyUSB0

# 方法4：十六进制查看
sudo hexdump -C /dev/ttyUSB0
```

### 3. Python测试脚本

创建测试脚本: `test_serial.py`

```python
#!/usr/bin/env python3
import serial
import struct
import time

def send_test_packet(ser, x, y, obj_type):
    """发送测试数据包"""
    # 构建数据包
    header1 = 0xAA
    header2 = 0x55
    cmd = 0x01

    # 打包数据（小端序）
    packet = struct.pack('<BBBB hh BB',
                        header1, header2, cmd, 0,  # 前4字节
                        x, y,                       # 坐标
                        obj_type, 0)               # 类型和校验

    # 计算校验和
    checksum = sum(packet[:-1]) % 256
    packet = packet[:-1] + bytes([checksum])

    # 发送
    ser.write(packet)
    print(f"Sent: x={x}, y={y}, type={obj_type}")
    print(f"Hex: {packet.hex()}")

if __name__ == "__main__":
    # 打开串口
    ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

    print("Testing serial communication...")

    # 发送测试数据
    for i in range(5):
        send_test_packet(ser, 320+i*10, 240+i*10, 1)
        time.sleep(1)

    ser.close()
    print("Test complete")
```

---

## 扩展功能

### 1. 添加心跳机制

```cpp
// 在serial_node中添加心跳定时器
ros::Timer heartbeat_timer = nh_.createTimer(
    ros::Duration(1.0),
    &SerialNode::sendHeartbeat, this);

void sendHeartbeat(const ros::TimerEvent&) {
    DataPacket packet;
    packet.header1 = 0xAA;
    packet.header2 = 0x55;
    packet.cmd = 0x04;  // 心跳命令
    packet.checksum = calculateChecksum(packet);
    serial_port_.write(/* packet data */);
}
```

### 2. STM32反馈机制

```c
// STM32发送反馈
void SendFeedback(uint8_t status) {
    uint8_t feedback[4] = {0xBB, 0x66, status, status};
    HAL_UART_Transmit(&huart1, feedback, 4, 100);
}
```

### 3. 多目标支持

修改消息定义支持多个目标：

```
Header header
DetectionObject[] objects

---
string object_type
int32 x
int32 y
float32 confidence
```

---

## 总结

### 完整数据流

```
1. MVCamera采集图像 → cv::Mat
2. ROS camera_node发布 → /camera/image_raw
3. vision_node订阅并识别 → /vision/detection_result
4. serial_node接收结果 → 封装数据包
5. 串口发送 → USB转串口适配器
6. STM32接收 → 状态机解析
7. 数据验证 → 执行控制逻辑
8. 电机驱动 → 实际动作
```

### 关键要点

✅ **ROS端**
- 使用`cv_bridge`转换图像
- 使用`serial`库通信
- 模块化设计（相机、识别、串口分离）

✅ **通信协议**
- 固定帧头保证同步
- 校验和保证数据完整性
- 命令字支持扩展

✅ **STM32端**
- 状态机可靠接收
- 中断处理高效
- 校验机制防错

✅ **调试方法**
- ROS工具可视化
- 串口监控数据
- 分步骤测试

### 下一步

1. 根据实际硬件调整GPIO和PWM配置
2. 优化控制算法（PID等）
3. 添加安全保护机制
4. 性能测试和优化

祝你项目顺利！🚀
