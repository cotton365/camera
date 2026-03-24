# STM32代码说明

这个目录包含STM32端接收ROS数据的完整示例代码。

## 文件说明

### serial_protocol.h / serial_protocol.c
串口协议处理模块，实现：
- 数据包帧同步（0xAA 0x55帧头检测）
- 状态机接收
- 数据包校验和验证
- 可靠的字节流解析

### main.c
主程序示例，演示：
- UART中断接收
- 数据包处理
- 目标位置控制逻辑
- 电机控制框架

## 使用步骤

### 1. 创建STM32CubeMX项目

1. 打开STM32CubeMX，选择你的MCU型号（如STM32F407VGT6）
2. 配置时钟树（建议168MHz）
3. 配置USART1：
   - Mode: Asynchronous
   - Baud Rate: 115200
   - Word Length: 8 Bits
   - Stop Bits: 1
   - Parity: None
   - 启用全局中断（NVIC Settings）
4. 配置GPIO（可选）：
   - LED指示灯
   - 电机控制引脚（PWM输出）
5. 生成代码（选择HAL库）

### 2. 集成代码

```bash
# 复制文件到你的STM32项目
cp serial_protocol.h your_stm32_project/Core/Inc/
cp serial_protocol.c your_stm32_project/Core/Src/
```

然后在STM32项目的`main.c`中：

```c
/* USER CODE BEGIN Includes */
#include "serial_protocol.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
SerialProtocol serial_protocol;
uint8_t uart_rx_byte;
/* USER CODE END PV */

int main(void)
{
    /* ... HAL初始化代码 ... */

    /* USER CODE BEGIN 2 */
    SerialProtocol_Init(&serial_protocol);
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    printf("STM32 Ready!\r\n");
    /* USER CODE END 2 */

    while (1)
    {
        /* USER CODE BEGIN 3 */
        HAL_Delay(10);
        /* USER CODE END 3 */
    }
}

/* 在文件末尾添加 */
/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (SerialProtocol_ProcessByte(&serial_protocol, uart_rx_byte)) {
            if (SerialProtocol_ValidatePacket(&serial_protocol.current_packet)) {
                ProcessDetectionData(&serial_protocol.current_packet);
            }
            SerialProtocol_Init(&serial_protocol);
        }
        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}

void ProcessDetectionData(const DataPacket* packet) {
    printf("x=%d, y=%d, type=%d\r\n", packet->x, packet->y, packet->obj_type);
    // 这里添加你的控制逻辑
}
/* USER CODE END 4 */
```

### 3. 配置printf输出

在`main.c`中添加（USER CODE BEGIN 0位置）：

```c
/* USER CODE BEGIN 0 */
#ifdef __GNUC__
int _write(int fd, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
#else
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
#endif
/* USER CODE END 0 */
```

### 4. 编译和下载

1. 使用Keil MDK或STM32CubeIDE编译
2. 通过ST-Link下载到板子
3. 连接USB转串口模块：
   - STM32 PA9(TX1) → USB串口 RX
   - STM32 PA10(RX1) → USB串口 TX
   - GND → GND

### 5. 测试

```bash
# 在Linux上查看STM32输出
sudo minicom -D /dev/ttyUSB0 -b 115200

# 或使用screen
sudo screen /dev/ttyUSB0 115200
```

## 硬件连接

### 最小系统
```
USB转串口模块        STM32板子
---------------      ------------
    RX      -------> PA9  (USART1_TX)
    TX      -------> PA10 (USART1_RX)
    GND     -------> GND
    VCC              (不连接，STM32独立供电)
```

### 完整系统（带电机控制）
```
STM32引脚     连接
---------     ----
PA9           USB串口RX（数据接收）
PA10          USB串口TX（调试输出）
PB6/PB7       电机驱动PWM输出（TIM4_CH1/CH2）
PC0/PC1       电机方向控制
```

## 数据包格式

| 字节位置 | 字段 | 类型 | 说明 |
|---------|------|------|------|
| 0 | header1 | uint8 | 固定0xAA |
| 1 | header2 | uint8 | 固定0x55 |
| 2 | cmd | uint8 | 命令字 |
| 3-4 | x | int16 | X坐标（小端序） |
| 5-6 | y | int16 | Y坐标（小端序） |
| 7 | obj_type | uint8 | 物体类型 |
| 8 | checksum | uint8 | 校验和 |

### 命令定义
- 0x01: 目标位置信息
- 0x02: 电机控制命令
- 0x03: 设置工作模式
- 0x04: 心跳包
- 0x05: 紧急停止

## 调试技巧

### 1. 打印接收的原始数据

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        printf("RX: 0x%02X\r\n", uart_rx_byte);  // 调试输出
        // ... 其他处理代码
    }
}
```

### 2. 验证校验和

```c
if (!SerialProtocol_ValidatePacket(&serial_protocol.current_packet)) {
    uint8_t calc = SerialProtocol_CalculateChecksum(&serial_protocol.current_packet);
    printf("Checksum error! Calc=0x%02X, Recv=0x%02X\r\n",
           calc, serial_protocol.current_packet.checksum);
}
```

### 3. LED指示

```c
// 在main.c的循环中
HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
HAL_Delay(500);  // 闪烁表示运行正常

// 收到数据时
void ProcessDetectionData(const DataPacket* packet) {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);  // 点亮LED
    // 处理数据...
    HAL_Delay(50);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);  // 熄灭LED
}
```

## 常见问题

### Q: 收不到数据？
A: 检查以下几点：
1. 串口线连接正确（TX-RX交叉）
2. 波特率匹配（115200）
3. GND连接
4. STM32的UART中断已启用
5. ROS端串口设备路径正确（/dev/ttyUSB0）

### Q: 数据包校验失败？
A:
1. 确认ROS和STM32的数据包结构一致
2. 检查字节序（小端序）
3. 打印原始接收数据进行调试

### Q: 如何添加更多命令？
A: 在`serial_protocol.h`中定义新命令：
```c
#define CMD_YOUR_COMMAND  0x06
```
然后在`ProcessDetectionData()`中添加处理：
```c
case CMD_YOUR_COMMAND:
    // 处理你的命令
    break;
```

## 进�步扩展

### 1. 添加反馈机制
```c
void SendFeedback(uint8_t status) {
    uint8_t feedback[4] = {0xBB, 0x66, status, status};
    HAL_UART_Transmit(&huart1, feedback, 4, 100);
}
```

### 2. 实现PID控制
```c
typedef struct {
    float Kp, Ki, Kd;
    float last_error;
    float integral;
} PID_Controller;

float PID_Calculate(PID_Controller* pid, float error) {
    pid->integral += error;
    float derivative = error - pid->last_error;
    float output = pid->Kp * error + pid->Ki * pid->integral + pid->Kd * derivative;
    pid->last_error = error;
    return output;
}
```

### 3. 多电机协调控制
```c
void ControlRobot(int16_t target_x, int16_t target_y) {
    // 计算各电机速度
    int16_t motor1_speed = calculate_motor1(target_x, target_y);
    int16_t motor2_speed = calculate_motor2(target_x, target_y);
    int16_t motor3_speed = calculate_motor3(target_x, target_y);

    // 设置PWM
    SetMotorSpeed(1, motor1_speed);
    SetMotorSpeed(2, motor2_speed);
    SetMotorSpeed(3, motor3_speed);
}
```

## 参考资料

- STM32 HAL库文档
- UART通信原理
- 状态机设计模式
- ROS串口通信
