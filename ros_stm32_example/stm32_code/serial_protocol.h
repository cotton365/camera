#ifndef __SERIAL_PROTOCOL_H
#define __SERIAL_PROTOCOL_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

// 数据包结构（与ROS端保持一致）
typedef struct {
    uint8_t header1;      // 0xAA
    uint8_t header2;      // 0x55
    uint8_t cmd;          // 命令字
    int16_t x;            // X坐标
    int16_t y;            // Y坐标
    uint8_t obj_type;     // 物体类型
    uint8_t checksum;     // 校验和
} __attribute__((packed)) DataPacket;

// 接收状态机
typedef enum {
    STATE_WAIT_HEADER1,
    STATE_WAIT_HEADER2,
    STATE_RECEIVE_DATA,
    STATE_COMPLETE
} ReceiveState;

// 串口协议处理器
typedef struct {
    ReceiveState state;
    uint8_t rx_buffer[sizeof(DataPacket)];
    uint8_t rx_index;
    DataPacket current_packet;
} SerialProtocol;

// 命令定义
#define CMD_TARGET_POSITION    0x01  // 目标位置信息
#define CMD_CONTROL_MOTOR      0x02  // 电机控制命令
#define CMD_SET_MODE           0x03  // 设置工作模式
#define CMD_HEARTBEAT          0x04  // 心跳包
#define CMD_STOP               0x05  // 紧急停止

// 函数声明
void SerialProtocol_Init(SerialProtocol* protocol);
bool SerialProtocol_ProcessByte(SerialProtocol* protocol, uint8_t byte);
uint8_t SerialProtocol_CalculateChecksum(const DataPacket* packet);
bool SerialProtocol_ValidatePacket(const DataPacket* packet);

#endif
