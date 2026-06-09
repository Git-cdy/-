#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

// ================== ”≤º˛“˝Ω≈ ==================
// UART3: PB10=TX, PB11=RX
#define ESP8266_USART           USART3
#define ESP8266_TX_PORT         GPIOB
#define ESP8266_TX_PIN          GPIO_Pin_10
#define ESP8266_RX_PORT         GPIOB
#define ESP8266_RX_PIN          GPIO_Pin_11
#define ESP8266_BAUDRATE        115200
#define ESP8266_RX_BUF_SIZE     512

// ================== EMQX Cloud MQTT ≈‰÷√ ==================
#define EMQX_BROKER_HOST        "b3ae3f1a.ala.cn-shenzhen.emqxsl.cn"
#define EMQX_BROKER_PORT        8883
#define EMQX_MQTT_USERNAME      "cdy"
#define EMQX_MQTT_PASSWORD      "20050416oK"
#define EMQX_CLIENT_ID          "greenhouse_stm32"
#define EMQX_PUB_TOPIC          "greenhouse/sensor"
#define EMQX_SUB_TOPIC          "greenhouse/command"

// ================== WiFi ≈‰÷√ ==================
#define WIFI_SSID               "CDY"
#define WIFI_PASSWORD           "20050416"

// ================== API ==================
void ESP8266_Init(void);
uint8_t ESP8266_IsConnected(void);

uint8_t ESP8266_SendCmd(const char *cmd, const char *expect, uint32_t timeout_ms);
void ESP8266_SendData(const char *data, uint16_t len);

uint8_t ESP8266_ConnectWiFi(void);
uint8_t ESP8266_ConnectMQTT(void);
uint8_t ESP8266_PublishData(const char *json_str);
uint8_t ESP8266_CheckCommand(char *payload, uint16_t max_len);

void ESP8266_UART3_IRQHandler(void);

#endif
