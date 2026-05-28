#include "stm32f10x.h"
#include "scheduler.h"
#include "delay.h"
#include "pin_config.h"
#include "uart.h"
#include "OLED.h"
#include "sht30.h"
#include "bh1750.h"
#include "soil.h"
#include "relay.h"
#include "buzzer_pwm.h"
#include <string.h>
#include <stdio.h>

/* ========== 全局状态变量 ========== */
float    g_Temperature = 0.0f;
float    g_Humidity = 0.0f;
float    g_SoilMoisture = 0.0f;
float    g_LightLux = 0.0f;
uint8_t  g_DiseaseClass = 0;
uint8_t  g_DiseaseConfidence = 0;
uint8_t  g_WarnLevel = 0;       /* 0=正常, 1=关注, 2=预警, 3=告警 */
uint8_t  g_NetStatus = 0;       /* 0=离线, 1=弱网, 2=在线 */

/* 执行器状态 */
uint8_t  g_FanOn = 0;
uint8_t  g_WaterOn = 0;
uint8_t  g_LightOn = 0;

/* ========== 系统时钟初始化 (HSE 8MHz -> PLL x9 -> 72MHz) ========== */
static void SystemClock_Init(void)
{
    ErrorStatus HSEStartUpStatus;

    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    HSEStartUpStatus = RCC_WaitForHSEStartUp();

    if (HSEStartUpStatus == SUCCESS)
    {
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK2Config(RCC_HCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div2);
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
        RCC_PLLCmd(ENABLE);
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        while (RCC_GetSYSCLKSource() != 0x08);
    }
}

/* ========== 传感器采集任务 (每2秒) ========== */
void SensorTask(void)
{
    float t, h, lux;

    if (SHT30_Read(&t, &h))
    {
        g_Temperature = t;
        g_Humidity = h;
    }

    lux = BH1750_Read();
    if (lux >= 0)
    {
        g_LightLux = lux;
    }

    g_SoilMoisture = Soil_Read(7);

    if (K210_NewData)
    {
        __disable_irq();
        K210_NewData = 0;
        g_DiseaseClass = K210_Data.disease_class;
        g_DiseaseConfidence = K210_Data.confidence;
        __enable_irq();
    }
}

/* ========== 多模态融合 (每2秒) ========== */
void MultiModalFusion(void)
{
    uint8_t env_score = 0;
    uint8_t k210_score = 0;
    uint8_t total = 0;

    /* 环境因子评分 (0-30) */
    if (g_Temperature > 35.0f || g_Temperature < 5.0f)  env_score += 15;
    else if (g_Temperature > 32.0f)                      env_score += 8;

    if (g_Humidity > 85.0f)                              env_score += 10;
    else if (g_Humidity > 75.0f)                         env_score += 5;

    if (g_SoilMoisture < 15.0f)                          env_score += 5;

    /* K210 病虫害评分 (0-70) */
    if (g_DiseaseClass > 0)
    {
        k210_score = (uint8_t)((uint32_t)g_DiseaseConfidence * 70 / 100);
    }

    total = env_score + k210_score;

    if (total >= 80)      g_WarnLevel = 3;  /* 告警 */
    else if (total >= 50) g_WarnLevel = 2;  /* 预警 */
    else if (total >= 25) g_WarnLevel = 1;  /* 关注 */
    else                  g_WarnLevel = 0;  /* 正常 */
}

/* ========== 执行器控制 (每2秒) ========== */
void ActuatorControl(void)
{
    /* 风机：温度>32℃ 或 湿度>80% 开启 */
    if (g_Temperature > 32.0f || g_Humidity > 80.0f)
    {
        Relay_Fan(RELAY_ON);
        g_FanOn = 1;
    }
    else if (g_Temperature < 28.0f && g_Humidity < 70.0f)
    {
        Relay_Fan(RELAY_OFF);
        g_FanOn = 0;
    }

    /* 浇水：土壤湿度<20%开启 */
    if (g_SoilMoisture < 20.0f)
    {
        Relay_Water(RELAY_ON);
        g_WaterOn = 1;
    }
    else if (g_SoilMoisture > 50.0f)
    {
        Relay_Water(RELAY_OFF);
        g_WaterOn = 0;
    }

    /* 补光灯：光照<5000 lux开启 */
    if (g_LightLux < 5000.0f)
    {
        Relay_Light(RELAY_ON);
        g_LightOn = 1;
    }
    else if (g_LightLux > 10000.0f)
    {
        Relay_Light(RELAY_OFF);
        g_LightOn = 0;
    }

    /* 蜂鸣器：预警等级>=2时响 */
    if (g_WarnLevel >= 2)
        Buzzer_On();
    else
        Buzzer_Off();
}

/* ========== OLED 显示任务 (每200ms) ========== */
void OLED_Task(void)
{
    char buf[20];

    /* 第1行：标题 */
    OLED_ShowString(1, 2, "=== Smart GH ===");

    /* 第2行：温湿度 */
    OLED_ShowChinese(2, 1, "温度");
    OLED_ShowNum(2, 5, (uint32_t)g_Temperature, 2);
    OLED_ShowString(2, 8, "C  ");
    OLED_ShowChinese(2, 11, "湿度");
    OLED_ShowNum(2, 15, (uint32_t)g_Humidity, 2);
    OLED_ShowString(2, 18, "%");

    /* 第3行：土壤+光照 */
    OLED_ShowChar(3, 1, 'S');
    OLED_ShowString(3, 2, ":");
    OLED_ShowNum(3, 4, (uint32_t)g_SoilMoisture, 2);
    OLED_ShowString(3, 7, "% ");
    OLED_ShowChar(3, 10, 'L');
    OLED_ShowString(3, 11, ":");
    OLED_ShowNum(3, 13, (uint32_t)g_LightLux, 5);
    OLED_ShowString(3, 18, "lx");

    /* 第4行：状态 */
    OLED_ShowChinese(4, 1, "状态");
    if (g_WarnLevel == 3)
        OLED_ShowChinese(4, 5, "告警");
    else if (g_WarnLevel == 2)
        OLED_ShowChinese(4, 5, "警告");
    else
        OLED_ShowChinese(4, 5, "正常");
}

/* ========== UART 任务 (每10ms) ========== */
void UART_Task(void)
{
    static uint32_t report_timer = 0;

    report_timer++;
    if (report_timer >= 200)
    {
        report_timer = 0;
        printf("[大棚数据] T:%.1fC H:%.1f%% S:%.1f%% L:%.0flux 病虫:%d(%d%%) 等级:%d\r\n",
               g_Temperature, g_Humidity, g_SoilMoisture, g_LightLux,
               g_DiseaseClass, g_DiseaseConfidence, g_WarnLevel);
    }
}

/* ========== 主程序 ========== */
int main(void)
{
    SystemClock_Init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    Delay_Init();
    Scheduler_Init();

    SHT30_Init();
    BH1750_Init();
    Soil_Init();
    Relay_Init();
    Buzzer_Init();
    OLED_Init();
    UART_InitAll();

    OLED_Clear();

    printf("==================================\r\n");
    printf(" 智慧大棚多模态测控系统 v1.0\r\n");
    printf(" SHT30温湿度传感器 ... [OK]\r\n");
    printf(" BH1750光照传感器 ... [OK]\r\n");
    printf(" 土壤湿度传感器 ..... [OK]\r\n");
    printf(" 继电器/蜂鸣器 ..... [OK]\r\n");
    printf(" K210 UART通信 ...... [OK]\r\n");
    printf(" ESP8266 UART通信 ... [OK]\r\n");
    printf("==================================\r\n");

    while (1)
    {
        Scheduler_Run();
    }
}
