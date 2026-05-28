#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"
#include <stdio.h>

// ================== 串口引脚定义 ==================
#define UART1_TX_GPIO_PORT    GPIOA
#define UART1_TX_GPIO_PIN     GPIO_Pin_9
#define UART1_RX_GPIO_PORT    GPIOA
#define UART1_RX_GPIO_PIN     GPIO_Pin_10

// ================== 串口初始化与发送 ==================
void UART1_Init(uint32_t baudrate);        // 串口初始化，推荐115200
void UART1_SendByte(uint8_t byte);         // 发送单个字节
void UART1_SendString(char *str);          // 发送字符串

// ================== 单字节接收 API（极简版） ==================
uint8_t UART1_GetRxFlag(void);             // 获取接收完成标志位
uint8_t UART1_GetRxData(void);             // 获取接收到的字节
void UART1_ClearRxFlag(void);              // 清除接收标志位

#endif /* __UART_H */
