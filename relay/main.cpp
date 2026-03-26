#include "MVCamera.h"
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "=== MVSDK相机 + OpenCV集成示例 ===" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 1. 创建相机对象
    MVCamera camera;

    // 2. 初始化相机（使用第一个相机，索引为0）
    std::cout << "步骤1: 初始化相机..." << std::endl;
    if (!camera.init(0)) {
        std::cerr << "初始化相机失败！请检查：" << std::endl;
        std::cerr << "  1. 相机是否已连接？" << std::endl;
        std::cerr << "  2. MVSDK驱动是否已安装？" << std::endl;
        std::cerr << "  3. 是否有权限访问USB设备？（可能需要sudo）" << std::endl;
        return -1;
    }

    // 3. 打开相机（开始采集）
    std::cout << std::endl;
    std::cout << "步骤2: 打开相机..." << std::endl;
    if (!camera.open()) {
        std::cerr << "打开相机失败！" << std::endl;
        return -1;
    }

    // 4. 设置参数（可选）
    std::cout << std::endl;
    std::cout << "步骤3: 设置相机参数..." << std::endl;
    camera.setExposure(10000.0);  // 曝光时间 10ms
    camera.setGain(100);           // 增益 100
    std::cout << "  曝光时间: 10ms" << std::endl;
    std::cout << "  增益: 100" << std::endl;

    // 5. 创建OpenCV窗口
    std::cout << std::endl;
    std::cout << "步骤4: 创建显示窗口..." << std::endl;
    cv::namedWindow("MVSDK Camera + OpenCV", cv::WINDOW_AUTOSIZE);

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "开始实时预览..." << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "操作说明：" << std::endl;
    std::cout << "  ESC  - 退出程序" << std::endl;
    std::cout << "  S    - 保存当前图像" << std::endl;
    std::cout << "  +/-  - 调整曝光时间" << std::endl;
    std::cout << "  E    - 启用边缘检测" << std::endl;
    std::cout << "  O    - 切换到原始图像" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    cv::Mat frame;
    int frame_count = 0;
    double exposure = 10000.0;
    bool show_edges = false;

    // 6. 主循环 - 捕获并显示图像
    while (true) {
        // 捕获一帧
        if (camera.capture(frame)) {
            frame_count++;

            cv::Mat display_frame;

            if (show_edges && camera.isColorCamera()) {
                // 边缘检测模式
                cv::Mat gray, edges;
                cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
                cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.5);
                cv::Canny(gray, edges, 50, 150);

                // 转换为彩色以便显示
                cv::cvtColor(edges, display_frame, cv::COLOR_GRAY2BGR);

                // 添加文字
                cv::putText(display_frame, "Edge Detection Mode",
                           cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX,
                           0.8, cv::Scalar(0, 255, 0), 2);
            } else {
                // 原始图像模式
                display_frame = frame.clone();

                // 添加文字
                cv::putText(display_frame, "Original Mode",
                           cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX,
                           0.8, cv::Scalar(0, 255, 0), 2);
            }

            // 显示帧数
            cv::putText(display_frame, "Frame: " + std::to_string(frame_count),
                       cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX,
                       0.7, cv::Scalar(255, 255, 0), 2);

            // 显示曝光时间
            char exp_text[64];
            snprintf(exp_text, sizeof(exp_text), "Exposure: %.1f ms", exposure / 1000.0);
            cv::putText(display_frame, exp_text,
                       cv::Point(10, 90), cv::FONT_HERSHEY_SIMPLEX,
                       0.7, cv::Scalar(255, 255, 0), 2);

            // 显示图像
            cv::imshow("MVSDK Camera + OpenCV", display_frame);

            // 每隔30帧打印一次统计信息
            if (frame_count % 30 == 0) {
                std::cout << "已捕获 " << frame_count << " 帧 ("
                          << frame.cols << "x" << frame.rows << ")" << std::endl;
            }
        }

        // 处理键盘输入
        int key = cv::waitKey(1);

        if (key == 27) {  // ESC键
            std::cout << std::endl;
            std::cout << "用户按下ESC，退出..." << std::endl;
            break;
        } else if (key == 's' || key == 'S') {  // S键 - 保存图像
            if (!frame.empty()) {
                std::string filename = "capture_" + std::to_string(frame_count) + ".jpg";
                cv::imwrite(filename, frame);
                std::cout << "✓ 已保存图像: " << filename << std::endl;
            }
        } else if (key == '+' || key == '=') {  // +键 - 增加曝光
            exposure *= 1.2;
            if (exposure > 1000000.0) exposure = 1000000.0;  // 最大1秒
            camera.setExposure(exposure);
            std::cout << "曝光时间: " << exposure / 1000.0 << " ms" << std::endl;
        } else if (key == '-' || key == '_') {  // -键 - 减少曝光
            exposure /= 1.2;
            if (exposure < 100.0) exposure = 100.0;  // 最小0.1ms
            camera.setExposure(exposure);
            std::cout << "曝光时间: " << exposure / 1000.0 << " ms" << std::endl;
        } else if (key == 'e' || key == 'E') {  // E键 - 边缘检测
            show_edges = true;
            std::cout << "切换到边缘检测模式" << std::endl;
        } else if (key == 'o' || key == 'O') {  // O键 - 原始图像
            show_edges = false;
            std::cout << "切换到原始图像模式" << std::endl;
        }
    }

    // 7. 关闭相机
    std::cout << std::endl;
    std::cout << "步骤5: 关闭相机..." << std::endl;
    camera.close();

    // 8. 释放资源
    std::cout << "步骤6: 释放资源..." << std::endl;
    camera.release();

    // 9. 销毁OpenCV窗口
    cv::destroyAllWindows();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "程序结束" << std::endl;
    std::cout << "总共捕获: " << frame_count << " 帧" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
