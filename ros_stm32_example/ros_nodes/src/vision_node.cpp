#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <std_msgs/String.h>
#include "camera_control/DetectionResult.h"

/**
 * @brief 视觉识别节点
 * 功能：接收图像，进行目标识别，发布识别结果
 */
class VisionNode {
private:
    ros::NodeHandle nh_;
    image_transport::ImageTransport it_;
    image_transport::Subscriber image_sub_;
    ros::Publisher result_pub_;

    // 识别参数
    cv::Scalar lower_color_;
    cv::Scalar upper_color_;

public:
    VisionNode() : it_(nh_) {
        // 订阅图像话题
        image_sub_ = it_.subscribe("camera/image_raw", 1,
                                   &VisionNode::imageCallback, this);

        // 发布识别结果话题
        result_pub_ = nh_.advertise<camera_control::DetectionResult>(
            "vision/detection_result", 1);

        // 颜色阈值（示例：红色物体）
        lower_color_ = cv::Scalar(0, 100, 100);    // HSV下限
        upper_color_ = cv::Scalar(10, 255, 255);   // HSV上限

        ROS_INFO("Vision node started");
    }

    void imageCallback(const sensor_msgs::ImageConstPtr& msg) {
        // 转换ROS图像消息为OpenCV Mat
        cv_bridge::CvImagePtr cv_ptr;
        try {
            cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
        } catch (cv_bridge::Exception& e) {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return;
        }

        // 进行目标检测
        cv::Mat hsv, mask;
        cv::cvtColor(cv_ptr->image, hsv, cv::COLOR_BGR2HSV);
        cv::inRange(hsv, lower_color_, upper_color_, mask);

        // 形态学操作去噪
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);

        // 查找轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL,
                        cv::CHAIN_APPROX_SIMPLE);

        // 发布最大的目标
        if (!contours.empty()) {
            // 找最大轮廓
            double max_area = 0;
            int max_idx = 0;
            for (size_t i = 0; i < contours.size(); i++) {
                double area = cv::contourArea(contours[i]);
                if (area > max_area) {
                    max_area = area;
                    max_idx = i;
                }
            }

            // 计算中心点
            cv::Moments m = cv::moments(contours[max_idx]);
            int center_x = int(m.m10 / m.m00);
            int center_y = int(m.m01 / m.m00);

            // 发布结果
            camera_control::DetectionResult result;
            result.header.stamp = ros::Time::now();
            result.detected = true;
            result.x = center_x;
            result.y = center_y;
            result.width = cv_ptr->image.cols;
            result.height = cv_ptr->image.rows;
            result.object_type = "red_object";
            result.confidence = max_area / (cv_ptr->image.cols * cv_ptr->image.rows);

            result_pub_.publish(result);

            ROS_INFO("Detected at (%d, %d), area: %.0f",
                     center_x, center_y, max_area);
        }
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "vision_node");

    VisionNode node;
    ros::spin();

    return 0;
}
