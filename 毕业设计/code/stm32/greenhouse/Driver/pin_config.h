#ifndef __PIN_CONFIG_H
#define __PIN_CONFIG_H

#include "stm32f10x.h"

/* ========== I2C1: SHT30 + BH1750 ========== */
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SCL_PIN        GPIO_Pin_6
#define I2C1_SDA_PORT       GPIOB
#define I2C1_SDA_PIN        GPIO_Pin_7

/* ========== I2C2: OLED SSD1306 ========== */
#define I2C2_SCL_PORT       GPIOB
#define I2C2_SCL_PIN        GPIO_Pin_10
#define I2C2_SDA_PORT       GPIOB
#define I2C2_SDA_PIN        GPIO_Pin_11

/* ========== ADC: 土壤湿度 ========== */
#define ADC_SOIL_PORT       GPIOA
#define ADC_SOIL_PIN        GPIO_Pin_0
#define ADC_SOIL_CHANNEL    ADC_Channel_0

/* ========== UART1: K210 ========== */
#define UART1_TX_PORT       GPIOA
#define UART1_TX_PIN        GPIO_Pin_9
#define UART1_RX_PORT       GPIOA
#define UART1_RX_PIN        GPIO_Pin_10

/* ========== UART2: ESP8266 ========== */
#define UART2_TX_PORT       GPIOA
#define UART2_TX_PIN        GPIO_Pin_2
#define UART2_RX_PORT       GPIOA
#define UART2_RX_PIN        GPIO_Pin_3

/* ========== 继电器 (LOW=ON, 光耦隔离) ========== */
#define RELAY_FAN_PORT      GPIOA
#define RELAY_FAN_PIN       GPIO_Pin_4
#define RELAY_WATER_PORT    GPIOA
#define RELAY_WATER_PIN     GPIO_Pin_5
#define RELAY_LIGHT_PORT    GPIOA
#define RELAY_LIGHT_PIN     GPIO_Pin_6

/* ========== 蜂鸣器: TIM3 CH2 PWM (PA7) ========== */
#define BUZZER_PORT         GPIOA
#define BUZZER_PIN          GPIO_Pin_7

/* ========== 设备地址 ========== */
#define SHT30_ADDR          0x44
#define BH1750_ADDR         0x23
#define OLED_ADDR           0x3C

/* ========== 关键规则 ========== */
/* PA13(SWDIO) / PA14(SWCLK) 绝对不能占用！ */
/* 继电器JD-VCC跳线帽必须拔掉！ */
/* ESP8266 必须独立 AMS1117-3.3 供电！ */

#endif /* __PIN_CONFIG_H */
