#ifndef MVCAMERA_H
#define MVCAMERA_H

#include <opencv2/opencv.hpp>
#include "CameraApi.h"
#include <string>

/**
 * @brief MVCamera类 - 封装MVSDK相机功能并与OpenCV集成
 *
 * 这个类提供了简单的接口来：
 * - 初始化和打开MVSDK相机
 * - 捕获图像并转换为OpenCV Mat格式
 * - 设置相机参数（曝光、增益、分辨率等）
 * - 管理相机资源
 */
class MVCamera {
public:
    /**
     * @brief 构造函数
     */
    MVCamera();

    /**
     * @brief 析构函数 - 自动释放所有资源
     */
    ~MVCamera();

    // ========== 基础功能 ==========

    /**
     * @brief 初始化相机
     * @param camera_index 相机索引（0表示第一个相机）
     * @return 成功返回true，失败返回false
     */
    bool init(int camera_index = 0);

    /**
     * @brief 打开相机并开始采集
     * @return 成功返回true，失败返回false
     */
    bool open();

    /**
     * @brief 捕获一帧图像到OpenCV Mat
     * @param frame 输出的OpenCV Mat对象
     * @return 成功返回true，失败返回false
     */
    bool capture(cv::Mat& frame);

    /**
     * @brief 关闭相机（停止采集）
     * @return 成功返回true，失败返回false
     */
    bool close();

    /**
     * @brief 释放所有资源
     * @return 成功返回true，失败返回false
     */
    bool release();

    // ========== 参数设置 ==========

    /**
     * @brief 设置曝光时间
     * @param exposure_time_us 曝光时间（微秒）
     * @return 成功返回true，失败返回false
     */
    bool setExposure(double exposure_time_us);

    /**
     * @brief 设置模拟增益
     * @param gain 增益值
     * @return 成功返回true，失败返回false
     */
    bool setGain(int gain);

    /**
     * @brief 设置分辨率
     * @param width 宽度
     * @param height 高度
     * @return 成功返回true，失败返回false
     */
    bool setResolution(int width, int height);

    // ========== 信息获取 ==========

    /**
     * @brief 检查相机是否已打开
     * @return 已打开返回true，否则返回false
     */
    bool isOpen() const { return m_bIsOpen; }

    /**
     * @brief 获取图像宽度
     * @return 图像宽度（像素）
     */
    int getWidth() const { return m_nWidth; }

    /**
     * @brief 获取图像高度
     * @return 图像高度（像素）
     */
    int getHeight() const { return m_nHeight; }

    /**
     * @brief 检查是否为彩色相机
     * @return 彩色相机返回true，黑白相机返回false
     */
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
