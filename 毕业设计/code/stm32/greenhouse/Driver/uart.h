#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"
#include "pin_config.h"
#include <stdio.h>

/* ========== UART1 (K210) 接收帧结构 ========== */
#define UART1_FRAME_LEN     4
#define UART1_FRAME_HEADER  0xAA

typedef struct {
    uint8_t disease_class;
    uint8_t confidence;
    uint8_t checksum;
    uint8_t valid;
} K210_Frame_t;

extern K210_Frame_t K210_Data;
extern volatile uint8_t K210_NewData;

/* ========== UART2 (ESP8266) ========== */
extern uint8_t UART2_RxData;
extern uint8_t UART2_RxFlag;

/* ========== API ========== */
void UART1_Init(uint32_t baudrate);
void UART2_Init(uint32_t baudrate);
void UART_InitAll(void);

/* UART1 发送 */
void UART1_SendByte(uint8_t byte);
void UART1_SendString(char *str);

/* UART2 发送 (ESP8266 AT命令) */
void UART2_SendByte(uint8_t byte);
void UART2_SendString(char *str);
uint8_t UART2_GetRxFlag(void);
uint8_t UART2_GetRxData(void);
void UART2_ClearRxFlag(void);

#endif /* __UART_H */
