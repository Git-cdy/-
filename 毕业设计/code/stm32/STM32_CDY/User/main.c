#include "stm32f10x.h"
#include "uart.h"
#include "scheduler.h"
#include "OLED.h"
#include "sht30.h"
#include "motor.h"
#include "buzzer.h"
#include <string.h>
#include <stdio.h>

// ================== 全局变量定义 ==================
uint8_t Page_Index = 0;   //OLED显示界面 0代表一个界面，目前没有1
int8_t Current_Temp = 0;  //温度值（支持负数）
uint8_t Current_Humi = 0;  //湿度值
uint8_t System_Status = 0;  // 0: 正常，1: 警告，2: 告警
uint8_t Control_Mode = 0;   // 0: 自动模式，1: 手动模式

// ================== SHT30 采集与控制任务 ==================
// 执行周期：2000ms（2秒）
void SHT30_Task(void)
{
    int8_t temp;
    uint8_t humi;
    static uint8_t Last_Status = 0; 

    // 尝试读取 SHT30 数据
    if (SHT30_Read_Data(&temp, &humi))
    {
        Current_Temp = temp;
        Current_Humi = humi;
    }

    // -------- 仅在自动模式下执行温度控制逻辑 --------
    if (Control_Mode == 0)
    {
        if (Current_Temp >= 35)       // 告警阈值 35℃
        {
            Motor_SetSpeed(100);
            Buzzer_On();
            System_Status = 2;  
        }
        else if (Current_Temp >= 30)  // 警告阈值 30℃
        {
            Motor_SetSpeed(60);
            Buzzer_Off();
            System_Status = 1;  
        }
        else
        {
            Motor_SetSpeed(0);
            Buzzer_Off();
            System_Status = 0;  
        }

// -------- 状态突变时，串口立刻主动向电脑弹窗中文告警 --------
        if (System_Status != Last_Status)
        {
            if (System_Status == 2)
            {
                printf("\r\n[紧急告警] !!! 机房温度极高，全面启动最高防御 !!!\r\n");
            }
            else if (System_Status == 1)
            {
                if (Last_Status == 0) // 从 正常(0) 升温到 警告(1)
                {
                    printf("\r\n[系统警告] 机房温度升高，辅助降温风扇已启动。\r\n");
                }
                else if (Last_Status == 2) // 从 告警(2) 降温到 警告(1)
                {
                    printf("\r\n[警报降级] 温度离开极度危险区，蜂鸣器解除，风扇继续降温。\r\n");
                }
            }
            else if (System_Status == 0)
            {
                printf("\r\n[恢复正常] 机房温度已完全回落，系统恢复安全状态。\r\n");
            }
            Last_Status = System_Status; 
        }
    }
}

// ================== OLED 显示任务 ==================
// 执行周期：100ms
void OLED_Task(void)
{
    switch (Page_Index)
    {
        case 0:
            // -------- 第一行：居中显示 == 机房监控 == --------
            OLED_ShowString(1, 1, "== ");
            OLED_ShowChinese(1, 4, "机房监控");  
            OLED_ShowString(1, 12, " ==");

            // -------- 第二行：温度 : xx C --------
            OLED_ShowChinese(2, 1, "温度");
            OLED_ShowString(2, 5, " : ");
            if (Current_Temp < 0)
            {
                OLED_ShowString(2, 8, "-");
                OLED_ShowNum(2, 9, -Current_Temp, 2);
            }
            else
            {
                OLED_ShowNum(2, 8, Current_Temp, 2);
            }
            OLED_ShowString(2, 11, " C  ");   

            // -------- 第三行：湿度 : xx % --------
            OLED_ShowChinese(3, 1, "湿度");     
            OLED_ShowString(3, 5, " : ");
            OLED_ShowNum(3, 8, Current_Humi, 2);
            OLED_ShowString(3, 11, " %  ");   

            // -------- 第四行：状态 : [对应中文] --------
            OLED_ShowChinese(4, 1, "状态");     
            OLED_ShowString(4, 5, " : ");

            if (System_Status == 2)
            {
                OLED_ShowChinese(4, 8, "告警");  
                OLED_ShowString(4, 12, "  ");   
            }
            else if (System_Status == 1)
            {
                OLED_ShowChinese(4, 8, "警告");  
                OLED_ShowString(4, 12, "  ");   
            }
            else
            {
                OLED_ShowChinese(4, 8, "正常");  
                OLED_ShowString(4, 12, "  ");   
            }
            break;

        default:
            Page_Index = 0;
            break;
    }
}

// ================== 串口通信任务 ==================
// 执行周期：10ms
void UART_Task(void)
{
    static uint32_t report_timer = 0;

    // -------- 定时上报数据（每 2000ms 上报一次，纯中文输出） --------
    report_timer++;
    if (report_timer >= 200)
    {
        report_timer = 0;
        printf("[系统数据] 温度:%d度, 湿度:%d%%, 模式:%s, 状态:%s\r\n",
               Current_Temp, Current_Humi,
               (Control_Mode == 0) ? "自动" : "手动",
               (System_Status == 0) ? "正常" : ((System_Status == 1) ? "警告" : "告警"));
    }

    // -------- 处理接收到的单字节命令 --------
    if (UART1_GetRxFlag() == 0) return;

    uint8_t cmd = UART1_GetRxData();

    // -------- 单字节指令映射表 --------
    // '1' -> 自动模式
    // '2' -> 风扇开启
    // '3' -> 风扇关闭
    // '4' -> 警报开启
    // '5' -> 警报关闭
    // \r, \n, 空格 -> 静默忽略
    // 其他 -> 无效指令

    if (cmd == '1')
    {
        Control_Mode = 0;
        printf(">> 执行成功: 系统已切换回自动温控模式\r\n");
    }
    else if (cmd == '2')
    {
        Control_Mode = 1;
        Motor_SetSpeed(100);
        printf(">> 执行成功: 强制切入手动模式，风扇全速开启\r\n");
    }
    else if (cmd == '3')
    {
        Control_Mode = 1;
        Motor_SetSpeed(0);
        printf(">> 执行成功: 强制切入手动模式，风扇已关闭\r\n");
    }
    else if (cmd == '4')
    {
        Control_Mode = 1;
        Buzzer_On();
				Motor_SetSpeed(100);
        printf(">> 执行成功: 强制切入手动模式，物理警报开启\r\n");
    }
    else if (cmd == '5')
    {
        Control_Mode = 1;
        Buzzer_Off();
				Motor_SetSpeed(0);
        printf(">> 执行成功: 强制切入手动模式，物理警报关闭\r\n");
    }
    else if (cmd == '\r' || cmd == '\n' || cmd == ' ')
    {
        // 防干扰：静默忽略回车、换行、空格
    }
    else
    {
        printf(">> 错误: 无效指令 (0x%02X)\r\n", cmd);
    }

    // -------- 清除接收标志位，准备接收下一条命令 --------
    UART1_ClearRxFlag();
}

// ================== 主程序 ==================
int main(void)
{
    // 配置系统中断优先级分组
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // 初始化系统各外设模块
    UART1_Init(115200);
    SHT30_Init();
    Motor_Init();           
    Buzzer_Init();          
    OLED_Init();            
    OLED_Clear();           
    Scheduler_Init();       

    // 中文启动菜单信息
    printf("==================================\r\n");
    printf(" 系统启动成功\r\n");
    printf(" 智慧大棚环境监测节点 v2.0\r\n");
    printf(" SHT30温湿度传感器 ... [正常]\r\n");
    printf(" 风扇驱动及蜂鸣器 ..... [正常]\r\n");
    printf(" 串口双向通信接口 ..... [正常]\r\n");
    printf("==================================\r\n");

    while (1)
    {
        Scheduler_Run();
    }
}
