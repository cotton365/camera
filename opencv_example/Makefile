# MVSDK相机 + OpenCV 集成项目 Makefile

# 编译器
CXX = g++

# MVSDK库路径（根据实际情况修改）
# 假设MVSDK在上级目录的lib和include中
MVSDK_INCLUDE = ../../include
MVSDK_LIB = ../../lib

# 编译选项
CXXFLAGS = -Wall -O2 -std=c++11
CXXFLAGS += -I$(MVSDK_INCLUDE)

# 检测OpenCV版本
OPENCV4_EXISTS := $(shell pkg-config --exists opencv4 && echo yes)

ifeq ($(OPENCV4_EXISTS),yes)
    # OpenCV 4.x
    CXXFLAGS += $(shell pkg-config --cflags opencv4)
    LDFLAGS_OPENCV = $(shell pkg-config --libs opencv4)
else
    # OpenCV 3.x or 2.x
    CXXFLAGS += $(shell pkg-config --cflags opencv)
    LDFLAGS_OPENCV = $(shell pkg-config --libs opencv)
endif

# 链接选项
LDFLAGS = -L$(MVSDK_LIB)
LDFLAGS += -lMVSDK
LDFLAGS += -lpthread
LDFLAGS += -lrt
LDFLAGS += $(LDFLAGS_OPENCV)

# 目标可执行文件
TARGET = example_opencv_camera

# 源文件
SOURCES = example_opencv_camera.cpp MVCamera.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# 默认目标
all: $(TARGET)
	@echo ""
	@echo "========================================="
	@echo "编译完成！"
	@echo "========================================="
	@echo "运行方法:"
	@echo "  make run"
	@echo "或者:"
	@echo "  export LD_LIBRARY_PATH=$(MVSDK_LIB):\$$LD_LIBRARY_PATH"
	@echo "  ./$(TARGET)"
	@echo "========================================="

# 链接目标
$(TARGET): $(OBJECTS)
	@echo "链接 $(TARGET)..."
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

# 编译源文件
%.o: %.cpp
	@echo "编译 $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	@echo "清理编译文件..."
	rm -f $(OBJECTS) $(TARGET) *.jpg *.bmp

# 运行
run: $(TARGET)
	@echo "运行程序..."
	@export LD_LIBRARY_PATH=$(MVSDK_LIB):$$LD_LIBRARY_PATH && ./$(TARGET)

# 安装依赖（Ubuntu/Debian）
install-deps:
	@echo "安装OpenCV开发库..."
	sudo apt-get update
	sudo apt-get install -y libopencv-dev

# 检查依赖
check-deps:
	@echo "检查依赖项..."
	@echo -n "OpenCV: "
	@pkg-config --exists opencv4 && echo "✓ OpenCV 4.x" || \
	 (pkg-config --exists opencv && echo "✓ OpenCV 3.x/2.x" || echo "✗ 未找到")
	@echo -n "MVSDK头文件: "
	@[ -f $(MVSDK_INCLUDE)/CameraApi.h ] && echo "✓" || echo "✗ 未找到"
	@echo -n "MVSDK库文件: "
	@[ -f $(MVSDK_LIB)/libMVSDK.so ] && echo "✓" || echo "✗ 未找到"

# 帮助
help:
	@echo "MVSDK相机 + OpenCV 集成项目"
	@echo ""
	@echo "可用目标："
	@echo "  make              - 编译项目"
	@echo "  make run          - 编译并运行"
	@echo "  make clean        - 清理编译文件"
	@echo "  make check-deps   - 检查依赖项"
	@echo "  make install-deps - 安装OpenCV依赖（需要sudo）"
	@echo "  make help         - 显示此帮助信息"

.PHONY: all clean run install-deps check-deps help
