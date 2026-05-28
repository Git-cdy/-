/**
 * step0_main.c — 第0步测试：系统时钟 + 调度器 + 串口打印
 *
 * 接线：ST-Link（下载+供电）+ USB-TTL接PA9/PA10
 * 预期：串口助手 115200 波特率看到启动信息 + 心跳
 *
 * 此文件不含任何传感器/OLED/继电器初始化。
 * 编译时替换 User/main.c 即可。
 */

#include "stm32f10x.h"
#include "scheduler.h"
#include "uart.h"
#include <stdio.h>

/* ========== 系统时钟 (HSE 8MHz → PLL x9 → 72MHz) ========== */
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
    else
    {
        /* HSE 失败 → 降级 HSI (8MHz / 2 * 16 = 64MHz) */
        RCC_HSICmd(ENABLE);
        while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK2Config(RCC_HCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div2);
        RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_16);
        RCC_PLLCmd(ENABLE);
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        while (RCC_GetSYSCLKSource() != 0x08);
    }
}

/* ========== 心跳任务 (每1000ms) ========== */
void Heartbeat_Task(void)
{
    static uint32_t count = 0;
    count++;
    printf("[STEP0] Tick=%lu | System running OK\r\n", System_Tick);

    /* 前3次额外打印诊断信息 */
    if (count <= 3)
    {
        printf("  └─ SystemCoreClock = %lu Hz\r\n", SystemCoreClock);
        printf("  └─ SysTick 1ms = %s\r\n", (System_Tick > 100) ? "OK" : "CHECK!");
    }
}

/* ========== 调度器任务列表 ========== */
extern volatile uint32_t System_Tick;

void Scheduler_Run(void);   /* 来自 scheduler.c */
void Scheduler_Init(void);  /* 来自 scheduler.c */

/* 极简版：只跑心跳 */
void Task_Runner(void)
{
    static uint32_t last_heartbeat = 0;
    uint32_t now = System_Tick;

    if (now - last_heartbeat >= 1000)
    {
        last_heartbeat = now;
        Heartbeat_Task();
    }
}

/* ========== 主程序 ========== */
int main(void)
{
    SystemClock_Init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 只初始化 UART1 (printf 重定向到 PA9/PA10) */
    UART1_Init(115200);

    /* 初始化调度器 (SysTick 1ms) */
    Scheduler_Init();

    /* 启动信息 */
    printf("\r\n");
    printf("==================================\r\n");
    printf(" 智慧大棚 第0步测试\r\n");
    printf(" 系统时钟 + 调度器 + 串口\r\n");
    printf("==================================\r\n");
    printf(" SystemCoreClock = %lu Hz\r\n", SystemCoreClock);
    printf(" 预期: 72 000 000 Hz (72MHz)\r\n");
    printf("==================================\r\n\r\n");

    /* 主循环 */
    while (1)
    {
        Task_Runner();  /* 轻量版调度：只跑心跳 */
    }
}
