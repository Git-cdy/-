#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"
#include <stdio.h>

// ================== UART1 配置定义（调试 + 云平台通信）==================
#define UART1_TX_GPIO_PORT    GPIOA
#define UART1_TX_GPIO_PIN     GPIO_Pin_9
#define UART1_RX_GPIO_PORT    GPIOA
#define UART1_RX_GPIO_PIN     GPIO_Pin_10

// ================== UART2 配置定义（与 K210 通信）==================
#define UART2_TX_GPIO_PORT    GPIOA
#define UART2_TX_GPIO_PIN     GPIO_Pin_2
#define UART2_RX_GPIO_PORT    GPIOA
#define UART2_RX_GPIO_PIN     GPIO_Pin_3

// ================== UART2 接收缓冲区大小 ==================
#define UART2_RX_BUF_SIZE  256    // 增加到 256 字节，提高可靠性

// ================== UART1 初始化和发送 ==================
void UART1_Init(uint32_t baudrate);        // 串口初始化，推荐 115200
void UART1_SendByte(uint8_t byte);         // 发送单个字节
void UART1_SendString(char *str);          // 发送字符串

// ================== UART1 接收字节 API（中断模式）==================
uint8_t UART1_GetRxFlag(void);             // 获取接收完成标志位
uint8_t UART1_GetRxData(void);             // 获取接收的单个字节
void UART1_ClearRxFlag(void);              // 清除接收标志位

// ================== UART2 初始化和发送 ==================
void UART2_Init(uint32_t baudrate);        // UART2 初始化，用于 K210 通信
void UART2_SendByte(uint8_t byte);         // 发送单个字节
void UART2_SendString(char *str);          // 发送字符串

// ================== UART2 接收缓冲区 API（环形缓冲）==================
uint8_t UART2_GetRxFlag(void);             // 检查接收缓冲区是否有数据
uint8_t UART2_GetRxData(void);             // 从缓冲区取出一个字节
void UART2_ClearRxFlag(void);              // 清除接收标志位（自动管理，通常不需要）

// ================== UART2 统计和诊断 API ==================
uint16_t UART2_GetRxCount(void);           // 获取接收缓冲区中的字节数
void UART2_ClearRxBuffer(void);            // 清空接收缓冲区（用于错误恢复）
uint32_t UART2_GetErrorCount(void);        // 获取接收错误计数

#endif /* __UART_H */
