# Camera Core Code Extraction Guide
# 相机核心代码提取指南

## Quick Summary | 快速总结

### Core Files to Extract | 需要提取的核心文件

```
核心代码位置：
/home/runner/work/camera/camera/GTK_Demo/
├── src/Demo.cpp          ← 最核心！所有相机操作都在这里
├── inc/Demo.h            ← 头文件声明
└── GTK_demo.cpp          ← 主程序入口（参考初始化流程）
```

---

## Method 1: Minimal Camera Library | 方法1：最小化相机库

### Step 1: Create Standalone Header | 创建独立头文件

**File:** `camera_lib.h`

```cpp
#ifndef _CAMERA_LIB_H_
#define _CAMERA_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

// 初始化相机 (Initialize camera)
// camera_index: 相机索引 (0表示第一个相机)
// 返回: 0=成功, -1=失败
int camera_lib_init(int camera_index);

// 释放相机 (Release camera)
void camera_lib_uninit();

// 获取一帧图像 (Get one frame)
// rgb_buffer: 输出RGB图像数据 (output RGB image data)
// width: 输出图像宽度 (output image width)
// height: 输出图像高度 (output image height)
// 返回: 0=成功, -1=失败
int camera_lib_get_frame(unsigned char** rgb_buffer, int* width, int* height);

// 设置曝光时间 (Set exposure time)
// exposure_us: 曝光时间（微秒）(exposure time in microseconds)
int camera_lib_set_exposure(double exposure_us);

// 设置增益 (Set gain)
// gain: 增益值 (gain value)
int camera_lib_set_gain(int gain);

// 保存当前帧 (Save current frame)
// filename: 文件名 (filename)
int camera_lib_save_image(const char* filename);

// 开始连续采集 (Start continuous capture)
int camera_lib_start_capture();

// 停止连续采集 (Stop continuous capture)
void camera_lib_stop_capture();

#ifdef __cplusplus
}
#endif

#endif // _CAMERA_LIB_H_
```

---

### Step 2: Implementation File | 实现文件

**File:** `camera_lib.cpp`

```cpp
#include "camera_lib.h"
#include "CameraApi.h"
#include <pthread.h>
#include <string.h>

// 全局变量 (Global variables)
static int g_hCamera = -1;
static tSdkCameraCapbility g_tCapability;
static unsigned char* g_pRgbBuffer = NULL;
static int g_frame_width = 0;
static int g_frame_height = 0;
static int g_is_capturing = 0;
static pthread_mutex_t g_frame_lock = PTHREAD_MUTEX_INITIALIZER;

// 初始化相机
int camera_lib_init(int camera_index) {
    // 1. 初始化SDK
    CameraSdkInit(1);

    // 2. 枚举相机
    tSdkCameraDevInfo tCameraEnumList[10];
    int iCameraCounts = 10;

    if(CameraEnumerateDevice(tCameraEnumList, &iCameraCounts) != CAMERA_STATUS_SUCCESS) {
        return -1;
    }

    if(iCameraCounts == 0 || camera_index >= iCameraCounts) {
        return -1;
    }

    // 3. 初始化指定相机
    if(CameraInit(&tCameraEnumList[camera_index], -1, -1, &g_hCamera) != CAMERA_STATUS_SUCCESS) {
        return -1;
    }

    // 4. 获取相机能力
    if(CameraGetCapability(g_hCamera, &g_tCapability) != CAMERA_STATUS_SUCCESS) {
        CameraUnInit(g_hCamera);
        return -1;
    }

    // 5. 分配图像缓冲区
    int max_w = g_tCapability.sResolutionRange.iWidthMax;
    int max_h = g_tCapability.sResolutionRange.iHeightMax;

    g_pRgbBuffer = (unsigned char*)CameraAlignMalloc(max_w * max_h * 3, 16);
    if(g_pRgbBuffer == NULL) {
        CameraUnInit(g_hCamera);
        return -1;
    }

    // 6. 设置输出格式为RGB
    CameraSetIspOutFormat(g_hCamera, CAMERA_MEDIA_TYPE_RGB8);

    // 7. 开始采集
    CameraPlay(g_hCamera);

    g_frame_width = max_w;
    g_frame_height = max_h;

    return 0;
}

// 释放相机
void camera_lib_uninit() {
    if(g_hCamera >= 0) {
        CameraUnInit(g_hCamera);
        g_hCamera = -1;
    }

    if(g_pRgbBuffer) {
        CameraAlignFree(g_pRgbBuffer);
        g_pRgbBuffer = NULL;
    }
}

// 获取一帧图像
int camera_lib_get_frame(unsigned char** rgb_buffer, int* width, int* height) {
    if(g_hCamera < 0) {
        return -1;
    }

    tSdkFrameHead sFrameInfo;
    BYTE* pbyBuffer;

    pthread_mutex_lock(&g_frame_lock);

    // 获取一帧
    if(CameraGetImageBuffer(g_hCamera, &sFrameInfo, &pbyBuffer, 1000) == CAMERA_STATUS_SUCCESS) {
        // 图像处理（RAW转RGB）
        CameraImageProcess(g_hCamera, pbyBuffer, g_pRgbBuffer, &sFrameInfo);

        // 释放原始缓冲区
        CameraReleaseImageBuffer(g_hCamera, pbyBuffer);

        *rgb_buffer = g_pRgbBuffer;
        *width = sFrameInfo.iWidth;
        *height = sFrameInfo.iHeight;

        pthread_mutex_unlock(&g_frame_lock);
        return 0;
    }

    pthread_mutex_unlock(&g_frame_lock);
    return -1;
}

// 设置曝光时间
int camera_lib_set_exposure(double exposure_us) {
    if(g_hCamera < 0) {
        return -1;
    }

    return CameraSetExposureTime(g_hCamera, exposure_us);
}

// 设置增益
int camera_lib_set_gain(int gain) {
    if(g_hCamera < 0) {
        return -1;
    }

    return CameraSetAnalogGain(g_hCamera, gain);
}

// 保存当前帧
int camera_lib_save_image(const char* filename) {
    if(g_hCamera < 0 || g_pRgbBuffer == NULL) {
        return -1;
    }

    tSdkFrameHead sFrameInfo;
    memset(&sFrameInfo, 0, sizeof(sFrameInfo));
    sFrameInfo.iWidth = g_frame_width;
    sFrameInfo.iHeight = g_frame_height;
    sFrameInfo.uiMediaType = CAMERA_MEDIA_TYPE_RGB8;

    return CameraSaveImage(g_hCamera, (char*)filename, g_pRgbBuffer, &sFrameInfo, FILE_BMP, 100);
}
```

---

### Step 3: Example Usage | 使用示例

**File:** `test_camera.cpp`

```cpp
#include "camera_lib.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("=== 相机测试程序 ===\n");
    printf("=== Camera Test Program ===\n\n");

    // 1. 初始化相机（使用第一个相机）
    printf("1. 初始化相机... (Initializing camera...)\n");
    if(camera_lib_init(0) != 0) {
        printf("   错误: 相机初始化失败！(Error: Camera init failed!)\n");
        return -1;
    }
    printf("   成功！(Success!)\n\n");

    // 2. 设置曝光和增益
    printf("2. 设置参数... (Setting parameters...)\n");
    camera_lib_set_exposure(10000.0);  // 10ms 曝光
    camera_lib_set_gain(100);          // 增益100
    printf("   曝光: 10ms, 增益: 100\n\n");

    // 3. 采集10帧图像
    printf("3. 采集图像... (Capturing images...)\n");
    for(int i = 0; i < 10; i++) {
        unsigned char* rgb_buffer;
        int width, height;

        if(camera_lib_get_frame(&rgb_buffer, &width, &height) == 0) {
            printf("   帧 %d: %dx%d, 数据指针: %p\n",
                   i+1, width, height, rgb_buffer);
        } else {
            printf("   帧 %d: 获取失败！(Failed!)\n", i+1);
        }

        usleep(100000);  // 等待100ms
    }
    printf("\n");

    // 4. 保存一帧图像
    printf("4. 保存图像... (Saving image...)\n");
    if(camera_lib_save_image("test_capture.bmp") == 0) {
        printf("   成功保存到 test_capture.bmp\n");
    } else {
        printf("   保存失败！(Save failed!)\n");
    }
    printf("\n");

    // 5. 释放相机
    printf("5. 释放相机... (Releasing camera...)\n");
    camera_lib_uninit();
    printf("   完成！(Done!)\n\n");

    printf("=== 测试结束 ===\n");
    printf("=== Test Complete ===\n");

    return 0;
}
```

---

### Step 4: Build Instructions | 编译说明

**File:** `Makefile`

```makefile
# 编译器
CXX = g++

# 包含路径（根据实际SDK位置调整）
INCLUDES = -I. -I../../include

# 库路径（根据实际SDK位置调整）
LIBS = -L../../lib -lMVSDK -lpthread -lrt

# 编译选项
CXXFLAGS = -Wall -O2

# 目标
all: test_camera

camera_lib.o: camera_lib.cpp camera_lib.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c camera_lib.cpp -o camera_lib.o

test_camera: test_camera.cpp camera_lib.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) test_camera.cpp camera_lib.o $(LIBS) -o test_camera

clean:
	rm -f *.o test_camera test_capture.bmp

run: test_camera
	./test_camera

.PHONY: all clean run
```

**编译和运行：**
```bash
# 编译
make

# 运行
make run

# 或直接运行
./test_camera
```

---

## Method 2: Direct Code Extraction | 方法2：直接提取代码

### Extract Core Functions from Demo.cpp | 从Demo.cpp提取核心函数

**需要提取的关键函数：**

```cpp
// 从 GTK_Demo/src/Demo.cpp 提取这些函数：

1. camera_init()           // 第12-84行：初始化相机
2. camera_uninit()         // 第86-94行：释放相机
3. read_data()             // 第96-148行：读取一帧
4. preview_thread()        // 第150-169行：预览线程
5. Gtk_SetExposure()       // 第213-309行：曝光设置
6. Gtk_SetResolution()     // 第311-355行：分辨率设置
7. Gtk_SetWB()             // 第357-418行：白平衡设置
8. Gtk_SetGamma_Contrast() // 第420-462行：伽马/对比度
9. Gtk_SetISPProce()       // 第464-506行：ISP处理
10. Gtk_SetSpeed()         // 第508-540行：帧率控制
11. Gtk_SetTrigger()       // 第542-639行：触发模式
12. Gtk_SetSnap()          // 第641-720行：图像保存
```

**修改建议：**
- 删除所有 `GtkWidget *widget` 参数
- 删除所有 `gtk_*` 函数调用
- 保留所有 `Camera*` API调用

---

## Key Camera API Functions | 关键相机API函数

### 必须使用的核心函数 (Must-use core functions):

```cpp
// SDK初始化
CameraSdkInit(1);

// 枚举相机
CameraEnumerateDevice(tSdkCameraDevInfo* pCameraList, int* piNums);

// 初始化相机
CameraInit(tSdkCameraDevInfo* pCameraInfo, int emParamLoadMode,
           int emTeam, int* pCameraHandle);

// 获取相机能力
CameraGetCapability(int hCamera, tSdkCameraCapbility* pCameraInfo);

// 开始采集
CameraPlay(int hCamera);

// 获取一帧
CameraGetImageBuffer(int hCamera, tSdkFrameHead* pFrameInfo,
                     BYTE** pbyBuffer, UINT uWaitTimeMs);

// 图像处理（RAW转RGB）
CameraImageProcess(int hCamera, BYTE* pbyIn, BYTE* pbyOut,
                   tSdkFrameHead* pFrInfo);

// 释放缓冲区
CameraReleaseImageBuffer(int hCamera, BYTE* pbyBuffer);

// 释放相机
CameraUnInit(int hCamera);
```

---

## File Structure Comparison | 文件结构对比

### 原始程序结构 (Original Structure):
```
GTK_Demo/
├── GTK_demo.cpp        (232行) - 主程序 + GTK初始化
├── src/
│   ├── Demo.cpp        (521行) - 核心相机操作 ← 最重要！
│   ├── callbacks.cpp   (848行) - GTK按钮回调
│   └── interface.cpp   (662行) - GTK界面创建
└── inc/
    ├── Demo.h          - 核心函数声明 ← 最重要！
    ├── callbacks.h     - 回调声明
    └── interface.h     - 界面声明
```

### 提取后的库结构 (Extracted Library Structure):
```
camera_lib/
├── camera_lib.h        - 简化的API接口
├── camera_lib.cpp      - 核心实现（从Demo.cpp提取）
├── test_camera.cpp     - 测试程序
└── Makefile           - 编译脚本
```

---

## Dependencies | 依赖项

### Required | 必需:
- **MVSDK Library** (libMVSDK.so) - 迈德威视相机SDK
- **pthread** - POSIX线程库
- **librt** - 实时库

### Optional | 可选:
- GTK 2.0 - 仅用于GUI显示（提取核心代码后不需要）

---

## Testing Checklist | 测试清单

```
□ 1. 相机能否正常枚举？(Can enumerate cameras?)
□ 2. 相机能否成功初始化？(Can initialize camera?)
□ 3. 能否获取相机能力信息？(Can get camera capability?)
□ 4. 能否成功采集图像？(Can capture frames?)
□ 5. 图像尺寸是否正确？(Is frame size correct?)
□ 6. 能否保存图像文件？(Can save image files?)
□ 7. 能否修改曝光时间？(Can change exposure?)
□ 8. 能否修改增益？(Can change gain?)
□ 9. 能否正常释放资源？(Can release resources?)
□ 10. 多次初始化/释放是否正常？(Multiple init/uninit cycles OK?)
```

---

## Common Issues | 常见问题

### 问题1: 找不到 libMVSDK.so
**解决方案:**
```bash
# 检查库文件位置
find / -name "libMVSDK.so" 2>/dev/null

# 添加到库路径
export LD_LIBRARY_PATH=/path/to/lib:$LD_LIBRARY_PATH

# 或复制到系统路径
sudo cp libMVSDK.so /usr/local/lib/
sudo ldconfig
```

### 问题2: 相机权限不足
**解决方案:**
```bash
# 临时方案：使用sudo运行
sudo ./test_camera

# 永久方案：添加udev规则
# 创建 /etc/udev/rules.d/99-camera.rules
SUBSYSTEM=="usb", ATTRS{idVendor}=="1234", MODE="0666"

# 重新加载udev
sudo udevadm control --reload-rules
```

### 问题3: 编译找不到头文件
**解决方案:**
```bash
# 检查SDK头文件位置
find / -name "CameraApi.h" 2>/dev/null

# 修改Makefile中的INCLUDES路径
INCLUDES = -I/actual/path/to/include
```

---

## Performance Tips | 性能提示

1. **内存对齐**: 使用 `CameraAlignMalloc()` 分配缓冲区，16字节对齐
2. **线程安全**: 使用互斥锁保护共享的图像缓冲区
3. **超时设置**: `CameraGetImageBuffer()` 建议超时1000ms
4. **缓冲区复用**: 不要频繁分配/释放图像缓冲区

---

## Summary | 总结

**最快的提取方法：**
1. 复制 `Demo.cpp` 和 `Demo.h`
2. 删除所有 GTK 相关代码
3. 保留所有 `Camera*` API 调用
4. 添加简单的 C 接口包装
5. 编译成库或直接使用

**核心只需要这些：**
- 初始化: `CameraSdkInit()` → `CameraInit()` → `CameraPlay()`
- 采集: `CameraGetImageBuffer()` → `CameraImageProcess()` → `CameraReleaseImageBuffer()`
- 释放: `CameraUnInit()`

就这么简单！
