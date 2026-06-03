#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"
#include <stdio.h>

// ================== UART1 引脚定义（调试 + 与电脑通信）==================
#define UART1_TX_GPIO_PORT    GPIOA
#define UART1_TX_GPIO_PIN     GPIO_Pin_9
#define UART1_RX_GPIO_PORT    GPIOA
#define UART1_RX_GPIO_PIN     GPIO_Pin_10

// ================== UART2 引脚定义（与 K210 通信）==================
#define UART2_TX_GPIO_PORT    GPIOA
#define UART2_TX_GPIO_PIN     GPIO_Pin_2
#define UART2_RX_GPIO_PORT    GPIOA
#define UART2_RX_GPIO_PIN     GPIO_Pin_3

// ================== UART2 环形缓冲区大小 ==================
#define UART2_RX_BUF_SIZE  64

// ================== UART1 初始化与发送 ==================
void UART1_Init(uint32_t baudrate);        // 串口初始化，推荐115200
void UART1_SendByte(uint8_t byte);         // 发送单个字节
void UART1_SendString(char *str);          // 发送字符串

// ================== UART1 单字节接收 API（极简版） ==================
uint8_t UART1_GetRxFlag(void);             // 获取接收完成标志位
uint8_t UART1_GetRxData(void);             // 获取接收到的字节
void UART1_ClearRxFlag(void);              // 清除接收标志位

// ================== UART2 初始化与发送 ==================
void UART2_Init(uint32_t baudrate);        // UART2 初始化，与 K210 通信
void UART2_SendByte(uint8_t byte);         // 发送单个字节
void UART2_SendString(char *str);          // 发送字符串

// ================== UART2 环形缓冲区接收 API ==================
uint8_t UART2_GetRxFlag(void);             // 缓冲区有数据返回1
uint8_t UART2_GetRxData(void);             // 从缓冲区取出一个字节
void UART2_ClearRxFlag(void);              // 兼容保留（环形缓冲区无需调用）

#endif /* __UART_H */
