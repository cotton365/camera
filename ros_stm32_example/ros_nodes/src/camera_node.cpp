#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include "MVCamera.h"

/**
 * @brief 相机发布节点
 * 功能：从MVSDK相机采集图像并发布到ROS话题
 */
class CameraNode {
private:
    ros::NodeHandle nh_;
    image_transport::ImageTransport it_;
    image_transport::Publisher image_pub_;

    MVCamera camera_;
    int camera_index_;
    double exposure_time_;
    int gain_;

public:
    CameraNode() : it_(nh_), camera_index_(0),
                   exposure_time_(10000.0), gain_(100) {

        // 获取参数
        nh_.param("camera_index", camera_index_, 0);
        nh_.param("exposure_time", exposure_time_, 10000.0);
        nh_.param("gain", gain_, 100);

        // 发布图像话题
        image_pub_ = it_.advertise("camera/image_raw", 1);

        // 初始化相机
        if (!camera_.init(camera_index_)) {
            ROS_ERROR("Failed to initialize camera %d", camera_index_);
            ros::shutdown();
            return;
        }

        // 设置参数
        camera_.setExposure(exposure_time_);
        camera_.setGain(gain_);

        // 打开相机
        if (!camera_.open()) {
            ROS_ERROR("Failed to open camera");
            ros::shutdown();
            return;
        }

        ROS_INFO("Camera node initialized successfully");
    }

    ~CameraNode() {
        camera_.close();
        camera_.release();
    }

    void spin() {
        ros::Rate loop_rate(30); // 30 FPS
        cv::Mat frame;

        while (ros::ok()) {
            // 捕获图像
            if (camera_.capture(frame)) {
                // 转换为ROS消息
                sensor_msgs::ImagePtr msg =
                    cv_bridge::CvImage(std_msgs::Header(),
                                      "bgr8",
                                      frame).toImageMsg();

                msg->header.stamp = ros::Time::now();
                msg->header.frame_id = "camera_frame";

                // 发布
                image_pub_.publish(msg);
            }

            ros::spinOnce();
            loop_rate.sleep();
        }
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "camera_node");

    CameraNode node;
    node.spin();

    return 0;
}
