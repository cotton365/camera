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

    // 获取一帧图像（超时，单位ms）
    const int CAPTURE_TIMEOUT_MS = 1000;
    int status = CameraGetImageBuffer(m_hCamera, &sFrameInfo, &pbyBuffer, CAPTURE_TIMEOUT_MS);

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
