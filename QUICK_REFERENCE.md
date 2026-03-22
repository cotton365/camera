# 核心代码快速定位 | Core Code Quick Reference

## 🎯 最核心的文件（最重要！）

### 📁 `/home/runner/work/camera/camera/GTK_Demo/src/Demo.cpp` (521行)
**这是最核心的相机操作文件！包含所有关键功能。**

主要函数：
- `camera_init()` - 初始化相机
- `camera_uninit()` - 释放相机
- `read_data()` - 读取一帧图像
- `preview_thread()` - 预览线程
- `Gtk_SetExposure()` - 设置曝光
- `Gtk_SetResolution()` - 设置分辨率
- `Gtk_SetWB()` - 设置白平衡
- `Gtk_SetTrigger()` - 设置触发模式
- `Gtk_SetSnap()` - 保存图像

### 📁 `/home/runner/work/camera/camera/GTK_Demo/inc/Demo.h`
**核心函数的头文件声明**

---

## 🔧 核心API使用（最简代码）

```cpp
#include "CameraApi.h"

int main() {
    int hCamera;

    // 1. 初始化
    CameraSdkInit(1);

    // 2. 枚举相机
    tSdkCameraDevInfo camList[10];
    int count = 10;
    CameraEnumerateDevice(camList, &count);

    // 3. 打开相机
    CameraInit(&camList[0], -1, -1, &hCamera);

    // 4. 开始采集
    CameraPlay(hCamera);

    // 5. 获取一帧
    tSdkFrameHead frameInfo;
    BYTE* rawBuffer;
    unsigned char* rgbBuffer = new unsigned char[1920*1080*3];

    if(CameraGetImageBuffer(hCamera, &frameInfo, &rawBuffer, 1000) == 0) {
        // 转换RAW到RGB
        CameraImageProcess(hCamera, rawBuffer, rgbBuffer, &frameInfo);

        // 使用 rgbBuffer 中的图像...

        // 释放缓冲区
        CameraReleaseImageBuffer(hCamera, rawBuffer);
    }

    // 6. 释放相机
    CameraUnInit(hCamera);
    delete[] rgbBuffer;

    return 0;
}
```

---

## 📦 必需的依赖

1. **MVSDK库** (libMVSDK.so) - 最关键！
   - 通常在 `../../lib/libMVSDK.so`

2. **头文件** (在 `../../include/`)
   - `CameraApi.h`
   - `CameraDefine.h`
   - `CameraStatus.h`

3. **其他库**
   - pthread (线程)
   - librt (实时)

---

## 🚀 快速编译

```bash
# 编译核心代码（无GTK）
g++ your_code.cpp \
    -I../../include \
    -L../../lib \
    -lMVSDK \
    -lpthread \
    -lrt \
    -o your_app

# 运行
./your_app
```

---

## 📊 核心数据结构

```cpp
// 相机句柄
int g_hCamera;

// 相机能力
tSdkCameraCapbility g_tCapability;

// 图像缓冲区
unsigned char* g_pRgbBuffer;

// 帧信息
tSdkFrameHead sFrameInfo;
```

---

## 🔄 基本工作流程

```
1. CameraSdkInit(1)              // 初始化SDK
2. CameraEnumerateDevice()       // 枚举相机
3. CameraInit()                  // 打开相机
4. CameraGetCapability()         // 获取能力
5. CameraSetIspOutFormat()       // 设置格式（RGB/MONO）
6. CameraPlay()                  // 开始采集

   循环：
   7. CameraGetImageBuffer()     // 获取帧
   8. CameraImageProcess()       // 处理图像
   9. CameraReleaseImageBuffer() // 释放帧

10. CameraUnInit()               // 释放相机
```

---

## 📝 57个相机API函数列表

### 初始化和设备管理
- CameraSdkInit
- CameraEnumerateDevice
- CameraIdleStateDevice
- CameraInit
- CameraUnInit
- CameraGetCapability
- CameraConnectTest

### 图像采集
- CameraPlay
- CameraGetImageBuffer
- CameraReleaseImageBuffer
- CameraImageProcess
- CameraImageProcessEx
- CameraSetIspOutFormat

### 分辨率
- CameraGetImageResolution
- CameraSetImageResolution

### 曝光控制
- CameraGetAeState / CameraSetAeState
- CameraGetAeTarget / CameraSetAeTarget
- CameraGetExposureTime / CameraSetExposureTime
- CameraGetAnalogGain / CameraSetAnalogGain
- CameraGetAntiFlick / CameraSetAntiFlick
- CameraGetLightFrequency / CameraSetLightFrequency
- CameraGetExposureLineTime

### 白平衡（彩色相机）
- CameraGetGain / CameraSetGain
- CameraGetSaturation / CameraSetSaturation
- CameraSetOnceWB

### 伽马和对比度
- CameraGetGamma / CameraSetGamma
- CameraGetContrast / CameraSetContrast

### 图像处理
- CameraGetSharpness / CameraSetSharpness
- CameraGetMirror / CameraSetMirror

### 帧率
- CameraGetFrameSpeed / CameraSetFrameSpeed

### 触发模式
- CameraGetTriggerMode / CameraSetTriggerMode
- CameraSoftTrigger

### 闪光灯控制
- CameraGetStrobeMode / CameraSetStrobeMode
- CameraGetStrobePolarity / CameraSetStrobePolarity

### 图像保存
- CameraSaveImage

### 参数管理
- CameraSaveParameter
- CameraLoadParameter
- CameraReadParameterFromFile

### 内存管理
- CameraAlignMalloc
- CameraAlignFree

### 统计信息
- CameraGetFrameStatistic
- CameraGetStatisticResend
- CameraGetEyeCount

---

## 🎓 学习建议

### 第1步：理解主程序流程
阅读：`GTK_Demo/GTK_demo.cpp` (第1-232行)
了解完整的初始化流程

### 第2步：学习核心操作
阅读：`GTK_Demo/src/Demo.cpp` (第1-521行)
重点关注：
- `camera_init()` 函数
- `read_data()` 函数
- `preview_thread()` 函数

### 第3步：去除GTK依赖
- 删除所有 `GtkWidget*` 参数
- 删除所有 `gtk_*` 函数调用
- 保留所有 `Camera*` API调用

### 第4步：创建自己的包装
参考 `CORE_CODE_EXTRACTION_GUIDE.md` 中的示例代码

---

## ⚠️ 注意事项

1. **必须先安装MVSDK** - 这是迈德威视相机的官方SDK
2. **权限问题** - Linux下可能需要sudo运行或配置udev规则
3. **线程安全** - 多线程访问时使用互斥锁保护缓冲区
4. **内存对齐** - 使用 `CameraAlignMalloc()` 分配缓冲区
5. **资源释放** - 务必调用 `CameraUnInit()` 释放资源

---

## 📚 详细文档

- **中文详细说明**: `核心相机代码说明.md` (12000字详细文档)
- **英文提取指南**: `CORE_CODE_EXTRACTION_GUIDE.md` (带完整代码示例)

---

## 🎉 快速总结

**最核心的文件**: `GTK_Demo/src/Demo.cpp` + `GTK_Demo/inc/Demo.h`

**最核心的函数**:
- 初始化：`camera_init()`
- 读取：`read_data()`
- 释放：`camera_uninit()`

**最关键的依赖**: libMVSDK.so

**提取方法**: 复制核心文件 → 删除GTK代码 → 保留Camera* API → 完成！
