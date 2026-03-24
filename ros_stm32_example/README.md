# ROS-STM32通信示例代码

本目录包含完整的ROS与STM32串口通信示例代码。

## 目录结构

```
ros_stm32_example/
├── README.md                   # 本文件
├── ros_nodes/                  # ROS节点代码
│   ├── src/                    # 源代码
│   │   ├── camera_node.cpp     # 相机采集节点
│   │   ├── vision_node.cpp     # 视觉识别节点
│   │   └── serial_node.cpp     # 串口通信节点
│   ├── msg/                    # 消息定义
│   │   └── DetectionResult.msg # 识别结果消息
│   ├── launch/                 # 启动文件
│   │   └── camera_system.launch
│   ├── CMakeLists.txt          # CMake构建文件
│   └── package.xml             # ROS包配置
└── stm32_code/                 # STM32代码
    ├── serial_protocol.h       # 串口协议头文件
    ├── serial_protocol.c       # 串口协议实现
    └── main.c                  # 主程序示例
```

## 快速开始

### 1. ROS端设置

```bash
# 复制到ROS工作空间
cp -r ros_nodes ~/catkin_ws/src/camera_control

# 编译
cd ~/catkin_ws
catkin_make

# 配置串口权限
sudo usermod -a -G dialout $USER
sudo chmod 666 /dev/ttyUSB0

# 启动系统
source devel/setup.bash
roslaunch camera_control camera_system.launch
```

### 2. STM32端设置

1. 使用STM32CubeMX创建项目，配置UART1（115200波特率）
2. 复制`stm32_code/`中的文件到你的STM32项目
3. 在`main.c`中包含并使用这些文件
4. 编译烧录到STM32板子

### 3. 测试通信

```bash
# 查看识别结果
rostopic echo /vision/detection_result

# 监控串口输出
sudo minicom -D /dev/ttyUSB0 -b 115200
```

## 数据协议

### 数据包格式（8字节）

| 字节 | 说明 | 值 |
|-----|------|---|
| 0 | 帧头1 | 0xAA |
| 1 | 帧头2 | 0x55 |
| 2 | 命令字 | 0x01 |
| 3-4 | X坐标 | int16 |
| 5-6 | Y坐标 | int16 |
| 7 | 物体类型 | uint8 |
| 8 | 校验和 | uint8 |

## 依赖项

### ROS端
- ROS Noetic/Melodic
- OpenCV 4.x
- serial库: `sudo apt-get install ros-${ROS_DISTRO}-serial`
- MVSDK相机库

### STM32端
- STM32 HAL库
- UART硬件外设

## 更多信息

详细教程请参考: `../ROS_STM32集成教程.md`
