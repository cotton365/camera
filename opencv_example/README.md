# MVSDK相机 + OpenCV集成项目

这个目录包含了将MVSDK相机集成到OpenCV项目的示例代码。

## 📁 文件列表

- `MVCamera.h` - 相机类头文件
- `MVCamera.cpp` - 相机类实现
- `example_opencv_camera.cpp` - 使用示例（主程序）
- `Makefile` - Make编译配置
- `CMakeLists.txt` - CMake编译配置

## 🚀 快速开始

### 方法1：使用Makefile

```bash
# 检查依赖
make check-deps

# 编译
make

# 运行
make run
```

### 方法2：使用CMake

```bash
# 创建build目录
mkdir build
cd build

# 配置
cmake ..

# 如果MVSDK路径不对，手动指定：
# cmake -DMVSDK_INCLUDE_DIR=/path/to/include -DMVSDK_LIB_DIR=/path/to/lib ..

# 编译
make

# 运行（需要设置库路径）
export LD_LIBRARY_PATH=../../../lib:$LD_LIBRARY_PATH
./example_opencv_camera
```

## 📋 依赖项

### 必需：
- MVSDK库（libMVSDK.so）
- OpenCV（libopencv_*.so）
- pthread
- librt

### 安装OpenCV：
```bash
# Ubuntu/Debian
sudo apt-get install libopencv-dev

# 或者从源码编译
# https://opencv.org/releases/
```

## 💡 使用说明

程序运行后会显示实时相机图像，支持以下操作：

- **ESC** - 退出程序
- **S** - 保存当前图像
- **+/-** - 调整曝光时间
- **E** - 启用边缘检测
- **O** - 切换到原始图像

## 🔧 类说明

### MVCamera类

封装了MVSDK相机的基本功能：

```cpp
MVCamera camera;

// 1. 初始化相机
camera.init(0);  // 0表示第一个相机

// 2. 打开相机
camera.open();

// 3. 捕获图像
cv::Mat frame;
if (camera.capture(frame)) {
    // 使用OpenCV处理frame
    cv::imshow("Camera", frame);
}

// 4. 设置参数
camera.setExposure(10000.0);  // 曝光10ms
camera.setGain(100);           // 增益100

// 5. 关闭相机
camera.close();
camera.release();
```

## 📖 更多信息

详细的迁移教程请参阅：
- `../迁移教程_OpenCV集成.md` - 完整的迁移教程

## ⚠️ 注意事项

1. 确保MVSDK库路径正确（默认为../../lib）
2. 运行时需要设置LD_LIBRARY_PATH
3. Linux下可能需要sudo权限或配置udev规则

## 🐛 常见问题

### 找不到libMVSDK.so
```bash
export LD_LIBRARY_PATH=/path/to/lib:$LD_LIBRARY_PATH
```

### 权限不足
```bash
sudo ./example_opencv_camera
# 或配置udev规则
```

### 找不到相机
```bash
# 检查USB连接
lsusb

# 检查权限
ls -l /dev/bus/usb/
```
