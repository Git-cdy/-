#include "scheduler.h"

// ================== 系统时间计数器 ==================
// SysTick 每 1ms 中断一次，System_Tick 递增1
// 外部模块通过 extern 获取当前时间
volatile uint32_t System_Tick = 0;

// ================== 外部任务声明 ==================
// 任务函数在 main.c 中实现，此处声明供任务列表使用
extern void SHT30_Task(void);          // SHT30 温湿度采集与控制任务
extern void BH1750_Task(void);         // BH1750 光照采集任务
extern void OLED_Task(void);           // OLED 显示任务
extern void UART_Task(void);           // 串口通信与指令处理任务

// ================== 任务列表 ==================
// 在此配置所有需要执行的任务，每行一个
// { 任务函数, 执行周期ms, 初始时间(固定为0) }
static Task_t Task_List[] =
{
    { SHT30_Task,   2000, 0 },   // SHT30 采集与控制：每 2000ms 执行一次
    { BH1750_Task,  2100, 0 },   // BH1750 光照采集：每 2100ms 执行一次（错开 SHT30，避免 I2C 冲突）
    { UART_Task,      10, 0 },   // 串口处理：每 10ms 执行一次，处理接收数据
    { OLED_Task,     100, 0 },   // OLED 显示：每 100ms 执行一次，刷新屏幕
};

// 任务数量自动计算，无需手动修改该变量
static uint8_t Task_Count;

// ================== 调度器初始化 ==================
// 配置 SysTick 定时器，每 1ms 产生一次中断
void Scheduler_Init(void)
{
    Task_Count = sizeof(Task_List) / sizeof(Task_t);   // 自动统计任务数

    // SystemCoreClock = 72000000（72MHz）
    // 72000000 / 1000 = 72000，每 72000 个时钟周期中断一次 = 1ms
    SysTick_Config(SystemCoreClock / 1000);
}

// ================== 调度器运行 ==================
// 在 while(1) 中不断调用
// 检查时间戳，判断是否到达执行时间，若到达则执行对应任务
void Scheduler_Run(void)
{
    uint8_t  i;
    uint32_t current = System_Tick;    // 获取当前时间，避免执行中 Tick 变化

    for (i = 0; i < Task_Count; i++)
    {
        // 时间判断：当前时间 - 上次执行时间 >= 执行周期
        // uint32_t 自然溢出，约49天循环，时间差计算仍然正确
        if (current - Task_List[i].last_time >= Task_List[i].interval_ms)
        {
            Task_List[i].last_time = current;    // 更新时间戳
            Task_List[i].task();                 // 执行任务
        }
    }
}

// ================== SysTick 中断服务函数 ==================
// 每 1ms 产生一次，只负责递增 System_Tick 计数
// 不在中断中执行任务，保证中断快速、可靠
void SysTick_Handler(void)
{
    System_Tick++;    // 系统时间递增（uint32_t，约49天循环，溢出不影响差值）
}
