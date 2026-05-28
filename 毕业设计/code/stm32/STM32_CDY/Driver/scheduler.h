#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "stm32f10x.h"

// ================== 调度器配置 ==================
#define TASK_MAX_NUM    8              // 最大任务数量上限（静态数组预留）

// ================== 任务结构体 ==================
// task         : 任务函数指针
// interval_ms  : 执行周期（单位：ms）
// last_time    : 上次执行时的时间戳（单位：ms，来自 System_Tick）
typedef struct
{
    void     (*task)(void);            // 任务函数指针
    uint32_t   interval_ms;           // 执行周期（ms）
    uint32_t   last_time;             // 上次执行时间戳（ms）
} Task_t;

// ================== 系统时基（1ms 自增，SysTick 中断维护） ==================
extern volatile uint32_t System_Tick;

// ================== 函数声明 ==================
void Scheduler_Init(void);             // 调度器初始化（配置SysTick为1ms）
void Scheduler_Run(void);              // 调度器运行（主循环中持续调用）

#endif /* __SCHEDULER_H */
