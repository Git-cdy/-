#include "scheduler.h"

volatile uint32_t System_Tick = 0;

extern void SensorTask(void);
extern void MultiModalFusion(void);
extern void ActuatorControl(void);
extern void UART_Task(void);
extern void OLED_Task(void);

static Task_t Task_List[] =
{
    { SensorTask,        2000, 0 },   // 传感器采集：每2秒
    { MultiModalFusion,  2000, 0 },   // 多模态融合：每2秒
    { ActuatorControl,   2000, 0 },   // 执行器控制：每2秒
    { UART_Task,           10, 0 },   // 串口处理：每10ms
    { OLED_Task,          200, 0 },   // OLED刷新：每200ms
};

static uint8_t Task_Count;

void Scheduler_Init(void)
{
    Task_Count = sizeof(Task_List) / sizeof(Task_t);
    SysTick_Config(SystemCoreClock / 1000);
}

void Scheduler_Run(void)
{
    uint8_t  i;
    uint32_t current = System_Tick;

    for (i = 0; i < Task_Count; i++)
    {
        if (current - Task_List[i].last_time >= Task_List[i].interval_ms)
        {
            Task_List[i].last_time = current;
            Task_List[i].task();
        }
    }
}

void SysTick_Handler(void)
{
    System_Tick++;
}
