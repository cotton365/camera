#include <ros/ros.h>
#include <serial/serial.h>
#include "camera_control/DetectionResult.h"
#include <vector>

/**
 * @brief 串口通信节点
 * 功能：接收识别结果，通过串口发送给STM32
 */
class SerialNode {
private:
    ros::NodeHandle nh_;
    ros::Subscriber result_sub_;
    serial::Serial serial_port_;

    std::string port_name_;
    int baudrate_;

    // 数据包结构
    struct DataPacket {
        uint8_t header1;      // 帧头1: 0xAA
        uint8_t header2;      // 帧头2: 0x55
        uint8_t cmd;          // 命令字
        int16_t x;            // X坐标
        int16_t y;            // Y坐标
        uint8_t obj_type;     // 物体类型
        uint8_t checksum;     // 校验和
    } __attribute__((packed));

public:
    SerialNode() : port_name_("/dev/ttyUSB0"), baudrate_(115200) {
        // 获取串口参数
        nh_.param<std::string>("serial_port", port_name_, "/dev/ttyUSB0");
        nh_.param("baudrate", baudrate_, 115200);

        // 订阅识别结果
        result_sub_ = nh_.subscribe("vision/detection_result", 1,
                                    &SerialNode::resultCallback, this);

        // 配置串口
        try {
            serial_port_.setPort(port_name_);
            serial_port_.setBaudrate(baudrate_);
            serial::Timeout timeout = serial::Timeout::simpleTimeout(1000);
            serial_port_.setTimeout(timeout);

            serial_port_.open();

            if (serial_port_.isOpen()) {
                ROS_INFO("Serial port %s opened at %d baud",
                         port_name_.c_str(), baudrate_);
            } else {
                ROS_ERROR("Failed to open serial port");
                ros::shutdown();
            }
        } catch (serial::IOException& e) {
            ROS_ERROR("Unable to open port: %s", e.what());
            ros::shutdown();
        }
    }

    ~SerialNode() {
        if (serial_port_.isOpen()) {
            serial_port_.close();
        }
    }

    void resultCallback(const camera_control::DetectionResult::ConstPtr& msg) {
        if (!msg->detected) {
            return;
        }

        // 构建数据包
        DataPacket packet;
        packet.header1 = 0xAA;
        packet.header2 = 0x55;
        packet.cmd = 0x01;  // 命令：目标位置
        packet.x = static_cast<int16_t>(msg->x);
        packet.y = static_cast<int16_t>(msg->y);
        packet.obj_type = getObjectType(msg->object_type);

        // 计算校验和（简单累加校验）
        packet.checksum = calculateChecksum(packet);

        // 发送数据
        try {
            std::vector<uint8_t> data(sizeof(DataPacket));
            memcpy(data.data(), &packet, sizeof(DataPacket));

            size_t bytes_written = serial_port_.write(data);

            ROS_INFO("Sent to STM32: x=%d, y=%d, type=%d [%zu bytes]",
                     packet.x, packet.y, packet.obj_type, bytes_written);

        } catch (serial::IOException& e) {
            ROS_ERROR("Serial write error: %s", e.what());
        }
    }

private:
    uint8_t getObjectType(const std::string& type) {
        if (type == "red_object") return 1;
        if (type == "blue_object") return 2;
        if (type == "green_object") return 3;
        return 0;  // 未知类型
    }

    uint8_t calculateChecksum(const DataPacket& packet) {
        uint8_t sum = 0;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&packet);
        // 校验和不包括最后一个字节（校验和本身）
        for (size_t i = 0; i < sizeof(DataPacket) - 1; i++) {
            sum += data[i];
        }
        return sum;
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "serial_node");

    SerialNode node;
    ros::spin();

    return 0;
}
