#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "stm32f10x.h"

#define TASK_MAX_NUM    8

typedef struct
{
    void     (*task)(void);
    uint32_t   interval_ms;
    uint32_t   last_time;
} Task_t;

extern volatile uint32_t System_Tick;

void Scheduler_Init(void);
void Scheduler_Run(void);

#endif /* __SCHEDULER_H */
