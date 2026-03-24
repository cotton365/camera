#include "serial_protocol.h"
#include <string.h>

/**
 * @brief 初始化串口协议处理器
 */
void SerialProtocol_Init(SerialProtocol* protocol) {
    protocol->state = STATE_WAIT_HEADER1;
    protocol->rx_index = 0;
    memset(protocol->rx_buffer, 0, sizeof(protocol->rx_buffer));
}

/**
 * @brief 处理接收到的单个字节
 * @param protocol 协议处理器指针
 * @param byte 接收到的字节
 * @return true表示接收到完整数据包
 */
bool SerialProtocol_ProcessByte(SerialProtocol* protocol, uint8_t byte) {
    switch (protocol->state) {
        case STATE_WAIT_HEADER1:
            if (byte == 0xAA) {
                protocol->rx_buffer[0] = byte;
                protocol->rx_index = 1;
                protocol->state = STATE_WAIT_HEADER2;
            }
            break;

        case STATE_WAIT_HEADER2:
            if (byte == 0x55) {
                protocol->rx_buffer[1] = byte;
                protocol->rx_index = 2;
                protocol->state = STATE_RECEIVE_DATA;
            } else {
                // 帧头不匹配，重新开始
                protocol->state = STATE_WAIT_HEADER1;
            }
            break;

        case STATE_RECEIVE_DATA:
            protocol->rx_buffer[protocol->rx_index++] = byte;
            if (protocol->rx_index >= sizeof(DataPacket)) {
                // 接收完整
                memcpy(&protocol->current_packet, protocol->rx_buffer,
                       sizeof(DataPacket));
                protocol->state = STATE_COMPLETE;
                return true;
            }
            break;

        case STATE_COMPLETE:
            // 重置状态机
            protocol->state = STATE_WAIT_HEADER1;
            protocol->rx_index = 0;
            break;
    }

    return false;
}

/**
 * @brief 计算校验和
 * @param packet 数据包指针
 * @return 校验和值
 */
uint8_t SerialProtocol_CalculateChecksum(const DataPacket* packet) {
    uint8_t sum = 0;
    const uint8_t* data = (const uint8_t*)packet;
    // 校验和不包括最后一个字节（校验和本身）
    for (size_t i = 0; i < sizeof(DataPacket) - 1; i++) {
        sum += data[i];
    }
    return sum;
}

/**
 * @brief 验证数据包的有效性
 * @param packet 数据包指针
 * @return true表示数据包有效
 */
bool SerialProtocol_ValidatePacket(const DataPacket* packet) {
    // 检查帧头
    if (packet->header1 != 0xAA || packet->header2 != 0x55) {
        return false;
    }

    // 检查校验和
    uint8_t calculated = SerialProtocol_CalculateChecksum(packet);
    if (calculated != packet->checksum) {
        return false;
    }

    return true;
}
