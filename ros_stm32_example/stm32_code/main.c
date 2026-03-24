/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : STM32串口接收ROS数据示例
 * @details        : 演示如何接收来自ROS的目标检测数据并控制电机
 ******************************************************************************
 */

#include "main.h"
#include "serial_protocol.h"
#include <stdio.h>

/* 私有变量 */
UART_HandleTypeDef huart1;
SerialProtocol serial_protocol;
uint8_t uart_rx_byte;

/* 私有函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void ProcessDetectionData(const DataPacket* packet);
void ControlMotor(int16_t target_x, int16_t target_y);

/**
 * @brief  主程序
 */
int main(void)
{
    /* 重置所有外设，初始化Flash接口和SysTick */
    HAL_Init();

    /* 配置系统时钟 */
    SystemClock_Config();

    /* 初始化所有配置的外设 */
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    /* 初始化协议处理器 */
    SerialProtocol_Init(&serial_protocol);

    /* 启动串口中断接收 */
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);

    printf("STM32 Ready, waiting for ROS data...\r\n");
    printf("Configured: UART1 @ 115200 baud\r\n");

    /* 无限循环 */
    while (1)
    {
        /* 主循环可以处理其他任务 */
        HAL_Delay(10);

        /* 可选：LED闪烁表示运行中 */
        // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
}

/**
 * @brief  串口接收完成中断回调函数
 * @param  huart UART句柄
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* 处理接收到的字节 */
        if (SerialProtocol_ProcessByte(&serial_protocol, uart_rx_byte))
        {
            /* 接收到完整数据包 */
            if (SerialProtocol_ValidatePacket(&serial_protocol.current_packet))
            {
                /* 数据包有效，进行处理 */
                ProcessDetectionData(&serial_protocol.current_packet);
            }
            else
            {
                printf("ERROR: Invalid packet received!\r\n");
            }

            /* 重置状态机准备接收下一个包 */
            SerialProtocol_Init(&serial_protocol);
        }

        /* 继续接收下一个字节 */
        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}

/**
 * @brief  处理检测数据
 * @param  packet 接收到的数据包
 */
void ProcessDetectionData(const DataPacket* packet)
{
    printf("Received: cmd=0x%02X, x=%d, y=%d, type=%d\r\n",
           packet->cmd, packet->x, packet->y, packet->obj_type);

    switch (packet->cmd)
    {
        case CMD_TARGET_POSITION:  // 0x01 目标位置
            printf("Target detected at (%d, %d), type=%d\r\n",
                   packet->x, packet->y, packet->obj_type);
            ControlMotor(packet->x, packet->y);
            break;

        case CMD_CONTROL_MOTOR:  // 0x02 电机控制
            printf("Motor control command\r\n");
            // 处理电机控制命令
            break;

        case CMD_SET_MODE:  // 0x03 设置模式
            printf("Set mode: %d\r\n", packet->obj_type);
            // 处理模式设置
            break;

        case CMD_HEARTBEAT:  // 0x04 心跳包
            // 心跳包，可用于检测通信状态
            break;

        case CMD_STOP:  // 0x05 紧急停止
            printf("EMERGENCY STOP!\r\n");
            // 停止所有电机
            // HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_ALL);
            break;

        default:
            printf("Unknown command: 0x%02X\r\n", packet->cmd);
            break;
    }
}

/**
 * @brief  根据目标位置控制电机
 * @param  target_x 目标X坐标
 * @param  target_y 目标Y坐标
 */
void ControlMotor(int16_t target_x, int16_t target_y)
{
    /* 这里实现你的控制逻辑 */

    // 示例：简单的比例控制
    const int16_t IMAGE_CENTER_X = 320;  // 图像中心X（根据实际分辨率调整）
    const int16_t IMAGE_CENTER_Y = 240;  // 图像中心Y

    // 计算误差
    int16_t error_x = target_x - IMAGE_CENTER_X;
    int16_t error_y = target_y - IMAGE_CENTER_Y;

    // 简单比例控制（Kp = 0.1）
    int16_t motor_speed_x = error_x / 10;
    int16_t motor_speed_y = error_y / 10;

    // 限幅
    if (motor_speed_x > 100) motor_speed_x = 100;
    if (motor_speed_x < -100) motor_speed_x = -100;
    if (motor_speed_y > 100) motor_speed_y = 100;
    if (motor_speed_y < -100) motor_speed_y = -100;

    printf("Motor control: Vx=%d, Vy=%d (error_x=%d, error_y=%d)\r\n",
           motor_speed_x, motor_speed_y, error_x, error_y);

    /* TODO: 实际的PWM/GPIO控制 */
    /*
    // 示例：使用定时器PWM控制电机
    if (motor_speed_x > 0) {
        // 正向
        HAL_GPIO_WritePin(MOTOR_X_DIR_GPIO_Port, MOTOR_X_DIR_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, motor_speed_x);
    } else {
        // 反向
        HAL_GPIO_WritePin(MOTOR_X_DIR_GPIO_Port, MOTOR_X_DIR_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, -motor_speed_x);
    }

    if (motor_speed_y > 0) {
        HAL_GPIO_WritePin(MOTOR_Y_DIR_GPIO_Port, MOTOR_Y_DIR_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, motor_speed_y);
    } else {
        HAL_GPIO_WritePin(MOTOR_Y_DIR_GPIO_Port, MOTOR_Y_DIR_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, -motor_speed_y);
    }
    */
}

/**
 * @brief  系统时钟配置（需根据你的MCU型号调整）
 */
void SystemClock_Config(void)
{
    // 这里应该使用STM32CubeMX生成的代码
    // 以下仅为示例框架
}

/**
 * @brief  USART1初始化函数
 */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief  GPIO初始化函数
 */
static void MX_GPIO_Init(void)
{
    // GPIO初始化代码（根据实际需要配置）
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/**
 * @brief  重定向printf到UART1
 */
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

/**
 * @brief  错误处理函数
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        // 错误状态：可以闪烁LED指示
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  报告发生断言错误的源文件名和行号
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Wrong parameters value: file %s on line %lu\r\n", file, line);
}
#endif
