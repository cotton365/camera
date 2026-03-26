# 在 relay/ 目录下继续开发

这是你的相机项目工作目录。你已经准备好了相机库，接下来按照以下步骤编译和运行。

---

## 📁 目录结构

```
relay/
├── include/          ← 将 MVSDK 头文件放这里（CameraApi.h 等）
├── lib/              ← 将 libMVSDK.so 放这里
├── MVCamera.h        ← 相机封装类头文件（已提供）
├── MVCamera.cpp      ← 相机封装类实现（已提供）
├── main.cpp          ← 主程序（已提供）
├── Makefile          ← Make 编译配置（已提供）
└── CMakeLists.txt    ← CMake 编译配置（已提供）
```

---

## 🚀 接下来怎么办

### 第 1 步：复制 MVSDK 库文件

将你准备好的 MVSDK 相机库文件分别放入对应目录：

```bash
# 进入 relay 目录
cd relay/

# 将头文件复制到 include/（至少需要 CameraApi.h）
cp /你的MVSDK路径/include/CameraApi.h   ./include/
cp /你的MVSDK路径/include/CameraDefine.h ./include/
cp /你的MVSDK路径/include/CameraStatus.h ./include/
# 或者直接复制整个 include 目录内容：
# cp /你的MVSDK路径/include/* ./include/

# 将动态库复制到 lib/
cp /你的MVSDK路径/lib/libMVSDK.so ./lib/
```

### 第 2 步：检查依赖

```bash
make check-deps
```

应该看到所有依赖项都显示 ✓。如果 OpenCV 未找到：

```bash
# Ubuntu/Debian 安装 OpenCV
make install-deps
```

### 第 3 步：编译

```bash
make
```

### 第 4 步：运行

```bash
make run
```

或者手动运行：

```bash
export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
./camera_app
```

---

## 💡 程序操作说明

程序运行后会打开相机并显示实时图像，支持以下键盘操作：

| 按键 | 功能 |
|------|------|
| ESC  | 退出程序 |
| S    | 保存当前图像为 JPG |
| +    | 增加曝光时间（×1.2） |
| -    | 减少曝光时间（÷1.2） |
| E    | 切换到边缘检测模式 |
| O    | 切换回原始图像模式 |

---

## 🔧 使用 CMake 编译（可选）

如果你更喜欢 CMake：

```bash
mkdir build && cd build
cmake ..
make
./camera_app
```

---

## 🐛 常见问题

### ❌ 找不到 CameraApi.h

```
fatal error: CameraApi.h: No such file or directory
```

**解决**：将 MVSDK 头文件复制到 `relay/include/` 目录。

### ❌ 找不到 libMVSDK.so

```
error while loading shared libraries: libMVSDK.so: cannot open shared object file
```

**解决**：

```bash
export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
./camera_app
```

或使用 `make run`（已内置设置）。

### ❌ 没有找到相机

```
错误: 没有找到相机设备！
```

**解决**：

```bash
# 检查 USB 连接
lsusb

# 可能需要 sudo 权限
sudo ./camera_app
```

### ❌ 权限不足

```bash
# 临时解决：使用 sudo
sudo ./camera_app

# 永久解决：配置 udev 规则（将 XXXX 替换为实际 VendorID）
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="XXXX", MODE="0666"' | \
    sudo tee /etc/udev/rules.d/99-camera.rules
sudo udevadm control --reload-rules
```

---

## 📚 更多文档

- `../迁移教程_OpenCV集成.md` — 完整迁移教程（推荐阅读）
- `../QUICK_START.md` — 快速开始指南
- `../opencv_example/` — 官方示例代码
