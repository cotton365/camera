# 相机功能迁移教程 - 集成到OpenCV项目

## 📋 目录
1. [概述](#概述)
2. [迁移准备](#迁移准备)
3. [核心功能提取](#核心功能提取)
4. [OpenCV集成方案](#opencv集成方案)
5. [完整实现代码](#完整实现代码)
6. [编译和运行](#编译和运行)
7. [常见问题](#常见问题)

---

## 概述

本教程将帮助你把当前相机程序的核心功能（调用相机、打开相机、拍摄、关闭相机）迁移到新的OpenCV项目中。

### 当前架构
```
原始程序 (GTK_Demo)
├── MVSDK相机库 (libMVSDK.so)
├── GTK界面
└── 相机核心代码 (Demo.cpp)
```

### 目标架构
```
新项目
├── MVSDK相机库 (libMVSDK.so)
├── OpenCV库 (libopencv_*.so)
└── 相机+OpenCV封装类
```

---

## 迁移准备

### 1. 需要的文件和依赖

#### 从原项目需要的文件：
- **参考文件**（不需要完全复制，需要提取核心逻辑）：
  - `/home/runner/work/camera/camera/GTK_Demo/src/Demo.cpp` - 核心相机操作
  - `/home/runner/work/camera/camera/GTK_Demo/inc/Demo.h` - 头文件
  - `/home/runner/work/camera/camera/GTK_Demo/GTK_demo.cpp` - 初始化流程参考

#### 必需的依赖库：
```bash
# MVSDK相机库（最重要！）
libMVSDK.so          # 位于 ../../lib/
CameraApi.h          # 位于 ../../include/
CameraDefine.h       # 位于 ../../include/
CameraStatus.h       # 位于 ../../include/

# OpenCV库
libopencv_core.so
libopencv_highgui.so
libopencv_imgproc.so
libopencv_imgcodecs.so

# 系统库
pthread              # 多线程支持
librt                # 实时库
```

### 2. 安装OpenCV

```bash
# Ubuntu/Debian
sudo apt-get install libopencv-dev

# 或者从源码编译
# https://opencv.org/releases/
```

---

## 核心功能提取

### 原始相机操作的关键步骤

从 `GTK_demo.cpp` 和 `Demo.cpp` 中提取的核心流程：

```cpp
// 1. 初始化SDK
CameraSdkInit(1);

// 2. 枚举相机设备
tSdkCameraDevInfo tCameraEnumList[64];
int iCameraCounts = 64;
CameraEnumerateDevice(tCameraEnumList, &iCameraCounts);

// 3. 选择并初始化相机
CameraInit(&tCameraEnumList[0], -1, -1, &g_hCamera);

// 4. 获取相机能力
CameraGetCapability(g_hCamera, &g_tCapability);

// 5. 设置输出格式
CameraSetIspOutFormat(g_hCamera, CAMERA_MEDIA_TYPE_RGB8);

// 6. 分配图像缓冲区
g_pRgbBuffer = (unsigned char*)CameraAlignMalloc(max_w * max_h * 3, 16);

// 7. 开始采集
CameraPlay(g_hCamera);

// 8. 循环获取帧
while(running) {
    tSdkFrameHead sFrameInfo;
    BYTE* pbyBuffer;

    if(CameraGetImageBuffer(g_hCamera, &sFrameInfo, &pbyBuffer, 1000) == CAMERA_STATUS_SUCCESS) {
        // 处理图像（RAW转RGB）
        CameraImageProcess(g_hCamera, pbyBuffer, g_pRgbBuffer, &sFrameInfo);

        // 这里可以转换为OpenCV的Mat

        // 释放缓冲区
        CameraReleaseImageBuffer(g_hCamera, pbyBuffer);
    }
}

// 9. 释放资源
CameraUnInit(g_hCamera);
CameraAlignFree(g_pRgbBuffer);
```

---

## OpenCV集成方案

### 关键：MVSDK数据到OpenCV Mat的转换

MVSDK获取的图像数据是 `unsigned char*` 格式，需要转换为OpenCV的 `cv::Mat`：

```cpp
// MVSDK RGB8格式 → OpenCV Mat (BGR格式)
unsigned char* rgb_buffer;  // 从MVSDK获取
int width = sFrameInfo.iWidth;
int height = sFrameInfo.iHeight;

// 创建OpenCV Mat（注意：MVSDK的RGB8实际上是BGR8格式）
cv::Mat frame(height, width, CV_8UC3, rgb_buffer);

// 如果需要RGB格式（OpenCV默认BGR）
cv::Mat frame_rgb;
cv::cvtColor(frame, frame_rgb, cv::COLOR_BGR2RGB);

// 如果是黑白相机（MONO8）
cv::Mat frame_gray(height, width, CV_8UC1, mono_buffer);
```

---

## 完整实现代码

### 方案1：创建相机类封装（推荐）

#### 文件：`MVCamera.h`

```cpp
#ifndef MVCAMERA_H
#define MVCAMERA_H

#include <opencv2/opencv.hpp>
#include "CameraApi.h"
#include <string>

class MVCamera {
public:
    MVCamera();
    ~MVCamera();

    // 基础功能
    bool init(int camera_index = 0);           // 初始化相机
    bool open();                                // 打开相机（开始采集）
    bool capture(cv::Mat& frame);               // 捕获一帧到OpenCV Mat
    bool close();                               // 关闭相机
    bool release();                             // 释放所有资源

    // 参数设置
    bool setExposure(double exposure_time_us);  // 设置曝光时间（微秒）
    bool setGain(int gain);                     // 设置增益
    bool setResolution(int width, int height);  // 设置分辨率

    // 信息获取
    bool isOpen() const { return m_bIsOpen; }
    int getWidth() const { return m_nWidth; }
    int getHeight() const { return m_nHeight; }
    bool isColorCamera() const { return !m_bMonoCamera; }

private:
    int m_hCamera;                              // 相机句柄
    tSdkCameraCapbility m_tCapability;          // 相机能力
    unsigned char* m_pRgbBuffer;                // RGB缓冲区
    bool m_bIsOpen;                             // 是否已打开
    bool m_bMonoCamera;                         // 是否黑白相机
    int m_nWidth;                               // 图像宽度
    int m_nHeight;                              // 图像高度
};

#endif // MVCAMERA_H
```

#### 文件：`MVCamera.cpp`

```cpp
#include "MVCamera.h"
#include <iostream>

MVCamera::MVCamera()
    : m_hCamera(-1)
    , m_pRgbBuffer(nullptr)
    , m_bIsOpen(false)
    , m_bMonoCamera(false)
    , m_nWidth(0)
    , m_nHeight(0)
{
}

MVCamera::~MVCamera() {
    release();
}

bool MVCamera::init(int camera_index) {
    // 1. 初始化SDK（1表示中文提示）
    if (CameraSdkInit(1) != CAMERA_STATUS_SUCCESS) {
        std::cerr << "错误: SDK初始化失败！" << std::endl;
        return false;
    }

    // 2. 枚举相机设备
    tSdkCameraDevInfo tCameraEnumList[10];
    int iCameraCounts = 10;

    if (CameraEnumerateDevice(tCameraEnumList, &iCameraCounts) != CAMERA_STATUS_SUCCESS) {
        std::cerr << "错误: 枚举相机失败！" << std::endl;
        return false;
    }

    if (iCameraCounts == 0) {
        std::cerr << "错误: 没有找到相机设备！" << std::endl;
        return false;
    }

    if (camera_index >= iCameraCounts) {
        std::cerr << "错误: 相机索引超出范围！" << std::endl;
        return false;
    }

    std::cout << "找到 " << iCameraCounts << " 个相机设备" << std::endl;
    std::cout << "正在初始化相机 " << camera_index << ": "
              << tCameraEnumList[camera_index].acFriendlyName << std::endl;

    // 3. 初始化相机
    if (CameraInit(&tCameraEnumList[camera_index], -1, -1, &m_hCamera) != CAMERA_STATUS_SUCCESS) {
        std::cerr << "错误: 相机初始化失败！" << std::endl;
        return false;
    }

    // 4. 获取相机能力
    if (CameraGetCapability(m_hCamera, &m_tCapability) != CAMERA_STATUS_SUCCESS) {
        std::cerr << "错误: 获取相机能力失败！" << std::endl;
        CameraUnInit(m_hCamera);
        m_hCamera = -1;
        return false;
    }

    // 5. 检查是否为黑白相机
    m_bMonoCamera = m_tCapability.sIspCapacity.bMonoSensor;

    // 6. 设置输出格式
    if (m_bMonoCamera) {
        CameraSetIspOutFormat(m_hCamera, CAMERA_MEDIA_TYPE_MONO8);
    } else {
        CameraSetIspOutFormat(m_hCamera, CAMERA_MEDIA_TYPE_RGB8);
    }

    // 7. 分配图像缓冲区
    int max_w = m_tCapability.sResolutionRange.iWidthMax;
    int max_h = m_tCapability.sResolutionRange.iHeightMax;
    int buffer_size = max_w * max_h * (m_bMonoCamera ? 1 : 3);

    m_pRgbBuffer = (unsigned char*)CameraAlignMalloc(buffer_size, 16);
    if (m_pRgbBuffer == nullptr) {
        std::cerr << "错误: 分配图像缓冲区失败！" << std::endl;
        CameraUnInit(m_hCamera);
        m_hCamera = -1;
        return false;
    }

    m_nWidth = max_w;
    m_nHeight = max_h;

    std::cout << "相机初始化成功！" << std::endl;
    std::cout << "分辨率: " << m_nWidth << "x" << m_nHeight << std::endl;
    std::cout << "类型: " << (m_bMonoCamera ? "黑白" : "彩色") << std::endl;

    return true;
}

bool MVCamera::open() {
    if (m_hCamera < 0) {
        std::cerr << "错误: 相机未初始化！" << std::endl;
        return false;
    }

    if (m_bIsOpen) {
        std::cout << "警告: 相机已经打开！" << std::endl;
        return true;
    }

    // 让SDK进入工作模式，开始接收图像数据
    if (CameraPlay(m_hCamera) != CAMERA_STATUS_SUCCESS) {
        std::cerr << "错误: 启动相机失败！" << std::endl;
        return false;
    }

    m_bIsOpen = true;
    std::cout << "相机已打开，开始采集..." << std::endl;
    return true;
}

bool MVCamera::capture(cv::Mat& frame) {
    if (!m_bIsOpen) {
        std::cerr << "错误: 相机未打开！" << std::endl;
        return false;
    }

    tSdkFrameHead sFrameInfo;
    BYTE* pbyBuffer = nullptr;

    // 获取一帧图像（超时1000ms）
    int status = CameraGetImageBuffer(m_hCamera, &sFrameInfo, &pbyBuffer, 1000);

    if (status != CAMERA_STATUS_SUCCESS) {
        // 超时或其他错误
        return false;
    }

    // 图像处理（RAW转RGB/MONO）
    CameraImageProcess(m_hCamera, pbyBuffer, m_pRgbBuffer, &sFrameInfo);

    // 释放SDK的图像缓冲区
    CameraReleaseImageBuffer(m_hCamera, pbyBuffer);

    // 转换为OpenCV Mat
    int width = sFrameInfo.iWidth;
    int height = sFrameInfo.iHeight;

    if (m_bMonoCamera) {
        // 黑白图像
        frame = cv::Mat(height, width, CV_8UC1, m_pRgbBuffer).clone();
    } else {
        // 彩色图像（MVSDK的RGB8实际是BGR格式，正好是OpenCV的默认格式）
        frame = cv::Mat(height, width, CV_8UC3, m_pRgbBuffer).clone();
    }

    return true;
}

bool MVCamera::close() {
    if (!m_bIsOpen) {
        return true;
    }

    // 停止采集（可选，因为CameraUnInit会自动停止）
    // CameraStop(m_hCamera);

    m_bIsOpen = false;
    std::cout << "相机已关闭" << std::endl;
    return true;
}

bool MVCamera::release() {
    close();

    if (m_hCamera >= 0) {
        CameraUnInit(m_hCamera);
        m_hCamera = -1;
    }

    if (m_pRgbBuffer != nullptr) {
        CameraAlignFree(m_pRgbBuffer);
        m_pRgbBuffer = nullptr;
    }

    std::cout << "相机资源已释放" << std::endl;
    return true;
}

bool MVCamera::setExposure(double exposure_time_us) {
    if (m_hCamera < 0) {
        return false;
    }

    return (CameraSetExposureTime(m_hCamera, exposure_time_us) == CAMERA_STATUS_SUCCESS);
}

bool MVCamera::setGain(int gain) {
    if (m_hCamera < 0) {
        return false;
    }

    return (CameraSetAnalogGain(m_hCamera, gain) == CAMERA_STATUS_SUCCESS);
}

bool MVCamera::setResolution(int width, int height) {
    if (m_hCamera < 0) {
        return false;
    }

    tSdkImageResolution sResolution;

    // 查找匹配的分辨率
    for (int i = 0; i < m_tCapability.iImageSizeDesc; i++) {
        if (m_tCapability.pImageSizeDesc[i].iWidth == width &&
            m_tCapability.pImageSizeDesc[i].iHeight == height) {
            sResolution = m_tCapability.pImageSizeDesc[i];

            if (CameraSetImageResolution(m_hCamera, &sResolution) == CAMERA_STATUS_SUCCESS) {
                m_nWidth = width;
                m_nHeight = height;
                return true;
            }
        }
    }

    return false;
}
```

#### 文件：`example_opencv_camera.cpp` - 使用示例

```cpp
#include "MVCamera.h"
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    std::cout << "=== MVSDK相机 + OpenCV集成示例 ===" << std::endl;
    std::cout << std::endl;

    // 1. 创建相机对象
    MVCamera camera;

    // 2. 初始化相机（使用第一个相机，索引为0）
    if (!camera.init(0)) {
        std::cerr << "初始化相机失败！" << std::endl;
        return -1;
    }

    // 3. 打开相机（开始采集）
    if (!camera.open()) {
        std::cerr << "打开相机失败！" << std::endl;
        return -1;
    }

    // 4. 设置参数（可选）
    camera.setExposure(10000.0);  // 曝光时间 10ms
    camera.setGain(100);           // 增益 100

    // 5. 创建OpenCV窗口
    cv::namedWindow("MVSDK Camera", cv::WINDOW_AUTOSIZE);

    std::cout << std::endl;
    std::cout << "开始实时预览..." << std::endl;
    std::cout << "按 ESC 退出" << std::endl;
    std::cout << "按 S 保存图像" << std::endl;
    std::cout << "按 +/- 调整曝光" << std::endl;
    std::cout << std::endl;

    cv::Mat frame;
    int frame_count = 0;
    double exposure = 10000.0;

    // 6. 主循环 - 捕获并显示图像
    while (true) {
        // 捕获一帧
        if (camera.capture(frame)) {
            frame_count++;

            // 在图像上显示帧数
            cv::putText(frame, "Frame: " + std::to_string(frame_count),
                       cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX,
                       1.0, cv::Scalar(0, 255, 0), 2);

            // 显示图像
            cv::imshow("MVSDK Camera", frame);

            // OpenCV图像处理示例
            // cv::Mat gray;
            // cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            // cv::Mat edges;
            // cv::Canny(gray, edges, 50, 150);
            // cv::imshow("Edges", edges);
        }

        // 处理键盘输入
        int key = cv::waitKey(1);

        if (key == 27) {  // ESC键
            std::cout << "用户按下ESC，退出..." << std::endl;
            break;
        } else if (key == 's' || key == 'S') {  // S键 - 保存图像
            if (!frame.empty()) {
                std::string filename = "capture_" + std::to_string(frame_count) + ".jpg";
                cv::imwrite(filename, frame);
                std::cout << "已保存图像: " << filename << std::endl;
            }
        } else if (key == '+' || key == '=') {  // +键 - 增加曝光
            exposure *= 1.2;
            camera.setExposure(exposure);
            std::cout << "曝光时间: " << exposure << " us" << std::endl;
        } else if (key == '-' || key == '_') {  // -键 - 减少曝光
            exposure /= 1.2;
            camera.setExposure(exposure);
            std::cout << "曝光时间: " << exposure << " us" << std::endl;
        }
    }

    // 7. 关闭相机
    camera.close();

    // 8. 释放资源
    camera.release();

    // 9. 销毁OpenCV窗口
    cv::destroyAllWindows();

    std::cout << std::endl;
    std::cout << "程序结束，总共捕获 " << frame_count << " 帧" << std::endl;

    return 0;
}
```

---

## 编译和运行

### 方案1：使用Makefile

创建 `Makefile`：

```makefile
# 编译器
CXX = g++

# MVSDK库路径（根据实际情况修改）
MVSDK_INCLUDE = ../../include
MVSDK_LIB = ../../lib

# 编译选项
CXXFLAGS = -Wall -O2 -std=c++11
CXXFLAGS += -I$(MVSDK_INCLUDE)
CXXFLAGS += $(shell pkg-config --cflags opencv4)

# 链接选项
LDFLAGS = -L$(MVSDK_LIB)
LDFLAGS += -lMVSDK
LDFLAGS += -lpthread
LDFLAGS += -lrt
LDFLAGS += $(shell pkg-config --libs opencv4)

# 如果pkg-config找不到opencv4，尝试opencv
ifeq ($(shell pkg-config --exists opencv4 && echo yes),)
    CXXFLAGS += $(shell pkg-config --cflags opencv)
    LDFLAGS += $(shell pkg-config --libs opencv)
endif

# 目标
TARGET = example_opencv_camera

# 源文件
SOURCES = example_opencv_camera.cpp MVCamera.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# 默认目标
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) *.jpg

run: $(TARGET)
	export LD_LIBRARY_PATH=$(MVSDK_LIB):$$LD_LIBRARY_PATH && ./$(TARGET)

.PHONY: all clean run
```

### 方案2：使用CMake（推荐）

创建 `CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.10)
project(MVCamera_OpenCV)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# MVSDK路径（根据实际情况修改）
set(MVSDK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../include" CACHE PATH "MVSDK include directory")
set(MVSDK_LIB_DIR "${CMAKE_SOURCE_DIR}/../../lib" CACHE PATH "MVSDK library directory")

# 包含MVSDK头文件
include_directories(${MVSDK_INCLUDE_DIR})

# 添加MVSDK库
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
```

### 编译步骤

#### 使用Makefile：
```bash
# 编译
make

# 运行
make run

# 或者直接运行（需要设置库路径）
export LD_LIBRARY_PATH=../../lib:$LD_LIBRARY_PATH
./example_opencv_camera

# 清理
make clean
```

#### 使用CMake：
```bash
# 创建build目录
mkdir build
cd build

# 配置
cmake ..

# 如果MVSDK路径不对，可以手动指定
# cmake -DMVSDK_INCLUDE_DIR=/path/to/include -DMVSDK_LIB_DIR=/path/to/lib ..

# 编译
make

# 运行（需要设置库路径）
export LD_LIBRARY_PATH=../../../lib:$LD_LIBRARY_PATH
./example_opencv_camera
```

---

## 常见问题

### Q1: 找不到libMVSDK.so

**问题：**
```
error while loading shared libraries: libMVSDK.so: cannot open shared object file
```

**解决方案：**
```bash
# 方法1：临时设置库路径
export LD_LIBRARY_PATH=/path/to/mvsdk/lib:$LD_LIBRARY_PATH

# 方法2：将库复制到系统路径
sudo cp /path/to/libMVSDK.so /usr/local/lib/
sudo ldconfig

# 方法3：在运行时指定
LD_LIBRARY_PATH=/path/to/lib ./example_opencv_camera
```

### Q2: 找不到CameraApi.h

**问题：**
```
fatal error: CameraApi.h: No such file or directory
```

**解决方案：**
```bash
# 在编译时指定正确的include路径
g++ -I/path/to/mvsdk/include ...

# 或修改Makefile/CMakeLists.txt中的路径
```

### Q3: 相机权限不足

**问题：**
```
错误: 相机初始化失败！
```

**解决方案：**
```bash
# 方法1：使用sudo运行
sudo ./example_opencv_camera

# 方法2：添加udev规则（永久解决）
# 创建 /etc/udev/rules.d/99-mvusb.rules
SUBSYSTEM=="usb", ATTRS{idVendor}=="YOUR_VENDOR_ID", MODE="0666"

# 重新加载udev规则
sudo udevadm control --reload-rules
sudo udevadm trigger

# 查看USB设备的vendor ID
lsusb
```

### Q4: OpenCV显示窗口无法关闭

**问题：**
窗口卡死或无法响应

**解决方案：**
```cpp
// 确保在循环中调用cv::waitKey()
while(true) {
    if(camera.capture(frame)) {
        cv::imshow("Camera", frame);
    }

    // 必须调用waitKey，否则窗口无法响应
    int key = cv::waitKey(1);  // 1ms延迟
    if(key == 27) break;  // ESC退出
}
```

### Q5: 图像颜色不对

**问题：**
OpenCV显示的图像颜色偏蓝/红

**解决方案：**
```cpp
// MVSDK的RGB8格式可能需要转换
cv::Mat frame_bgr(height, width, CV_8UC3, m_pRgbBuffer);

// 如果颜色不对，尝试转换
cv::Mat frame_rgb;
cv::cvtColor(frame_bgr, frame_rgb, cv::COLOR_BGR2RGB);
cv::imshow("Camera", frame_rgb);
```

### Q6: 帧率太低

**问题：**
实时显示时帧率很低

**解决方案：**
```cpp
// 1. 减小图像分辨率
camera.setResolution(640, 480);

// 2. 调整曝光时间
camera.setExposure(5000.0);  // 减少曝光时间

// 3. 使用多线程
// 一个线程捕获，一个线程显示

// 4. 减少OpenCV处理
// 只在需要时进行图像处理
```

---

## 高级功能

### 1. 保存视频

```cpp
#include "MVCamera.h"
#include <opencv2/opencv.hpp>

int main() {
    MVCamera camera;
    camera.init(0);
    camera.open();

    // 创建视频写入器
    cv::VideoWriter writer("output.avi",
                          cv::VideoWriter::fourcc('M','J','P','G'),
                          30.0,  // 帧率
                          cv::Size(camera.getWidth(), camera.getHeight()));

    cv::Mat frame;
    for(int i = 0; i < 300; i++) {  // 录制300帧
        if(camera.capture(frame)) {
            writer.write(frame);
            std::cout << "录制帧: " << i << std::endl;
        }
    }

    writer.release();
    camera.close();
    camera.release();

    return 0;
}
```

### 2. 多线程采集

```cpp
#include "MVCamera.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <atomic>

std::mutex g_mutex;
cv::Mat g_latest_frame;
std::atomic<bool> g_running(true);

void capture_thread(MVCamera* camera) {
    cv::Mat frame;
    while(g_running) {
        if(camera->capture(frame)) {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_latest_frame = frame.clone();
        }
    }
}

int main() {
    MVCamera camera;
    camera.init(0);
    camera.open();

    // 启动采集线程
    std::thread t(capture_thread, &camera);

    // 主线程显示
    cv::namedWindow("Camera", cv::WINDOW_AUTOSIZE);

    while(true) {
        cv::Mat frame;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            if(!g_latest_frame.empty()) {
                frame = g_latest_frame.clone();
            }
        }

        if(!frame.empty()) {
            cv::imshow("Camera", frame);
        }

        if(cv::waitKey(1) == 27) break;
    }

    g_running = false;
    t.join();

    camera.close();
    camera.release();

    return 0;
}
```

### 3. OpenCV图像处理集成

```cpp
#include "MVCamera.h"
#include <opencv2/opencv.hpp>

int main() {
    MVCamera camera;
    camera.init(0);
    camera.open();

    cv::Mat frame, gray, edges, blurred;

    while(true) {
        if(camera.capture(frame)) {
            // 转灰度
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

            // 高斯模糊
            cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 1.5);

            // 边缘检测
            cv::Canny(blurred, edges, 50, 150);

            // 显示原图和处理结果
            cv::imshow("Original", frame);
            cv::imshow("Edges", edges);
        }

        if(cv::waitKey(1) == 27) break;
    }

    camera.close();
    camera.release();
    cv::destroyAllWindows();

    return 0;
}
```

---

## 总结

### 迁移要点

1. **核心流程**：
   - 初始化SDK → 枚举相机 → 初始化相机 → 开始采集 → 循环获取帧 → 释放资源

2. **关键转换**：
   - MVSDK的 `unsigned char*` 转换为OpenCV的 `cv::Mat`
   - 注意BGR/RGB格式差异

3. **依赖管理**：
   - MVSDK库（libMVSDK.so）必须正确链接
   - OpenCV库必须正确安装

4. **资源管理**：
   - 使用RAII模式（在类的析构函数中释放资源）
   - 确保调用 `CameraUnInit()` 和 `CameraAlignFree()`

### 优势

- ✅ 完全脱离GTK界面
- ✅ 可以使用OpenCV强大的图像处理功能
- ✅ 跨平台（OpenCV支持Windows/Linux/Mac）
- ✅ 代码简洁易维护

### 下一步

1. 根据你的项目需求调整分辨率和参数
2. 添加错误处理和日志记录
3. 实现更多相机功能（触发模式、ROI等）
4. 集成到你的OpenCV应用中

祝迁移顺利！🎉
