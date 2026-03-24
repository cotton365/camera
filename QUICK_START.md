# 相机迁移快速指南 | Quick Start Guide

## 🎯 目标

将GTK_Demo程序的相机功能迁移到OpenCV项目中，实现：
- ✅ 打开相机
- ✅ 捕获图像
- ✅ 使用OpenCV处理和显示
- ✅ 关闭相机

---

## 📦 已提供的文件

### 1. 文档
- `迁移教程_OpenCV集成.md` - **完整教程（推荐先看！）**
- `QUICK_START.md` - 本文件

### 2. 示例代码（在 `opencv_example/` 目录）
- `MVCamera.h` - 相机类头文件
- `MVCamera.cpp` - 相机类实现
- `example_opencv_camera.cpp` - 完整示例程序
- `Makefile` - Make编译配置
- `CMakeLists.txt` - CMake编译配置
- `README.md` - 项目说明

---

## 🚀 三步快速开始

### 第1步：进入示例目录
```bash
cd opencv_example/
```

### 第2步：编译
```bash
# 检查依赖
make check-deps

# 编译项目
make
```

### 第3步：运行
```bash
# 运行示例程序
make run

# 或者手动运行
export LD_LIBRARY_PATH=../../lib:$LD_LIBRARY_PATH
./example_opencv_camera
```

---

## 💻 核心代码（最简示例）

如果你想在自己的项目中使用，只需要这几行：

```cpp
#include "MVCamera.h"
#include <opencv2/opencv.hpp>

int main() {
    // 1. 创建并初始化相机
    MVCamera camera;
    camera.init(0);    // 0 = 第一个相机
    camera.open();     // 开始采集

    // 2. 循环捕获并显示
    cv::Mat frame;
    while(true) {
        if(camera.capture(frame)) {
            cv::imshow("Camera", frame);
        }
        if(cv::waitKey(1) == 27) break;  // ESC退出
    }

    // 3. 清理
    camera.close();
    camera.release();
    return 0;
}
```

---

## 🔑 核心类：MVCamera

### 基础功能

```cpp
MVCamera camera;

// 初始化
camera.init(0);              // 初始化第0个相机

// 打开相机
camera.open();               // 开始采集

// 捕获一帧
cv::Mat frame;
camera.capture(frame);       // 捕获到OpenCV Mat

// 关闭相机
camera.close();              // 停止采集
camera.release();            // 释放所有资源
```

### 参数设置

```cpp
// 设置曝光（微秒）
camera.setExposure(10000.0);  // 10ms

// 设置增益
camera.setGain(100);

// 设置分辨率
camera.setResolution(1920, 1080);
```

### 查询信息

```cpp
bool is_open = camera.isOpen();           // 相机是否打开
int width = camera.getWidth();            // 图像宽度
int height = camera.getHeight();          // 图像高度
bool is_color = camera.isColorCamera();   // 是否彩色相机
```

---

## 📋 迁移对照表

| 原始GTK程序 | 新OpenCV程序 |
|------------|-------------|
| `camera_init()` | `camera.init(0)` |
| `CameraPlay()` | `camera.open()` |
| `read_data()` | `camera.capture(frame)` |
| `camera_uninit()` | `camera.close()` + `camera.release()` |
| GTK窗口显示 | `cv::imshow()` |
| GTK事件循环 | `cv::waitKey()` |

---

## 🔧 编译选项

### 使用Makefile
```bash
make              # 编译
make run          # 编译并运行
make clean        # 清理
make check-deps   # 检查依赖
```

### 使用CMake
```bash
mkdir build && cd build
cmake ..
make
export LD_LIBRARY_PATH=../../../lib:$LD_LIBRARY_PATH
./example_opencv_camera
```

---

## ⚙️ 依赖项

### 必需的库
1. **MVSDK** (libMVSDK.so) - 迈德威视相机SDK
2. **OpenCV** - OpenCV库
3. **pthread** - 多线程库
4. **librt** - 实时库

### 安装OpenCV
```bash
# Ubuntu/Debian
sudo apt-get install libopencv-dev

# CentOS/RedHat
sudo yum install opencv-devel

# macOS
brew install opencv
```

---

## 🐛 常见问题快速解决

### ❌ 找不到libMVSDK.so
```bash
export LD_LIBRARY_PATH=/path/to/mvsdk/lib:$LD_LIBRARY_PATH
```

### ❌ 没有找到相机
```bash
# 检查USB连接
lsusb

# 可能需要sudo权限
sudo ./example_opencv_camera
```

### ❌ 编译时找不到CameraApi.h
```bash
# 修改Makefile中的路径
MVSDK_INCLUDE = /actual/path/to/include
```

### ❌ 运行时权限不足
```bash
# 临时解决：使用sudo
sudo ./example_opencv_camera

# 永久解决：配置udev规则
# 创建 /etc/udev/rules.d/99-camera.rules
SUBSYSTEM=="usb", ATTRS{idVendor}=="XXXX", MODE="0666"
```

---

## 📖 完整功能示例

运行示例程序后支持的操作：

| 按键 | 功能 |
|-----|------|
| ESC | 退出程序 |
| S | 保存当前图像为JPG |
| + | 增加曝光时间 |
| - | 减少曝光时间 |
| E | 启用边缘检测模式 |
| O | 切换回原始图像模式 |

---

## 🎓 下一步

1. ✅ 运行示例程序熟悉API
2. ✅ 查看 `迁移教程_OpenCV集成.md` 了解详细说明
3. ✅ 复制 `MVCamera.h` 和 `MVCamera.cpp` 到你的项目
4. ✅ 开始使用！

---

## 📚 相关文档

- `迁移教程_OpenCV集成.md` - 完整的迁移教程（12000+字）
- `opencv_example/README.md` - 示例项目说明
- `CORE_CODE_EXTRACTION_GUIDE.md` - 代码提取指南
- `核心相机代码说明.md` - 原始代码详细说明

---

## 💡 提示

- MVCamera类已经处理好了所有MVSDK的复杂细节
- 返回的cv::Mat可以直接用于OpenCV的所有函数
- 支持彩色和黑白相机自动识别
- 线程安全，可以在多线程环境使用

---

## 🎉 开始使用

```bash
cd opencv_example/
make
make run
```

就这么简单！祝你使用愉快！ 🚀
