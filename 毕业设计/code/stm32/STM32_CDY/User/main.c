#include "stm32f10x.h"
#include "uart.h"
#include "uart_frame.h"
#include "scheduler.h"
#include "OLED.h"
#include "sht30.h"
#include "bh1750.h"
#include "soil_moisture.h"
#include "motor.h"
#include "buzzer.h"
#include "relay.h"
#include "esp8266.h"
#include <string.h>
#include <stdio.h>

// ================== 全局变量定义 ==================
uint8_t Page_Index = 0;   //OLED显示界面 0代表一个界面，目前没有1
int8_t Current_Temp = 0;  //温度值（支持负数）
uint8_t Current_Humi = 0;  //湿度值
uint16_t Current_Lux = 0;  //光照强度（lux）
uint16_t Current_Soil_ADC = 0;  //土壤湿度 ADC 值
uint8_t Current_Soil_Moisture = 0;  //土壤湿度（%）
uint8_t System_Status = 0;  // 0: 正常，1: 警告，2: 告警
uint8_t Control_Mode = 0;   // 0: 自动模式，1: 手动模式
uint8_t SHT30_Error = 0;    // 0: 正常，1: 通信失败

// K210 UART frame receiver
FrameReceiver_t K210_FrameRx;
uint8_t K210_DiseaseType = 0;
uint8_t K210_Confidence = 0;
uint32_t K210_LastRxTime = 0;
#define K210_TIMEOUT_MS  5000

// 病害中文名查找
static const char* K210_DiseaseNames[] = {
    "",          // 0x00 - 未使用
    "早疫病",    // 0x01 - 早疫病
    "晚疫病",    // 0x02 - 晚疫病
    "叶斑病",    // 0x03 - 叶斑病
    "健康",      // 0x04 - 健康
};
static uint8_t K210_FrameCount = 0;
static uint8_t K210_ErrorCount = 0;

// K210 UART frame receiver task (called every 50ms by scheduler)
void K210_FrameRx_Task(void)
{
    if (UART2_GetRxFlag())
    {
        uint8_t byte = UART2_GetRxData();
        UART2_ClearRxFlag();
        if (Frame_Receiver_Feed(&K210_FrameRx, byte))
        {
            K210Frame_t *frame = Frame_Receiver_GetFrame(&K210_FrameRx);
            if (frame)
            {
                K210_DiseaseType = frame->data.disease_type;
                K210_Confidence = frame->data.confidence;
                K210_LastRxTime = System_Tick;
                K210_FrameCount++;
                
                // 打印中文病害识别结果
                if (K210_DiseaseType <= 0x04)
                {
                    printf("[K210] 识别: %s, 置信度:%d%%\r\n",
                           K210_DiseaseNames[K210_DiseaseType], K210_Confidence);
                }
                else
                {
                    printf("[K210] 未知病害类型 0x%02X\r\n", K210_DiseaseType);
                }
                Frame_Receiver_ClearFlag(&K210_FrameRx);
            }
        }
    }
    
    // 超时检测（静默，避免刷屏）
    if (System_Tick - K210_LastRxTime > K210_TIMEOUT_MS)
    {
        K210_ErrorCount++;
        K210_LastRxTime = System_Tick;
    }
}

// ================== SHT30 采集与控制任务 ==================
// 执行周期：2000ms（2秒）
void SHT30_Task(void)
{
    int8_t temp;
    uint8_t humi;
    static uint8_t Last_Status = 0;

    // 尝试读取 SHT30 数据（最多重试 3 次）
    uint8_t retry = 0;
    while (retry < 3 && !SHT30_Read_Data(&temp, &humi))
    {
        retry++;
        printf("[SHT30_Task] 读取失败，重试 %d/3\r\n", retry);
    }

    if (retry >= 3)
    {
        printf("[SHT30_Task] 读取失败 3 次，传感器无响应\r\n");
        SHT30_Error = 1;  // 标记错误状态
        return;
    }

    // 读取成功，清除错误标志
    SHT30_Error = 0;
    Current_Temp = temp;
    Current_Humi = humi;

    // -------- 仅在自动模式下执行温度控制逻辑 --------
    if (Control_Mode == 0)
    {
        if (Current_Temp >= 35)       // 告警阈值 35℃
        {
            Relay_SetState(1, 1);   // 风机继电器 ON
            Motor_SetSpeed(100);
            Buzzer_On();
            System_Status = 2;
        }
        else if (Current_Temp >= 30)  // 警告阈值 30℃
        {
            Relay_SetState(1, 1);   // 风机继电器 ON
            Motor_SetSpeed(60);
            Buzzer_Off();
            System_Status = 1;
        }
        else
        {
            Relay_SetState(1, 0);   // 风机继电器 OFF
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

void OLED_Task(void)
{
    switch (Page_Index)
    {
        case 0:
            // -------- 第0页：温度、湿度、状态 --------
            // 第一行：智慧大棚（居中）
            OLED_ShowChinese(1, 5, "智慧大棚");

            // 第二行：温度
            OLED_ShowChinese(2, 1, "温");
            OLED_ShowChinese(2, 3, "度");
            OLED_ShowString(2, 5, ":");
            if (SHT30_Error)
            {
                OLED_ShowString(2, 6, "ERR");
            }
            else
            {
                if (Current_Temp < 0)
                {
                    OLED_ShowString(2, 6, "-");
                    OLED_ShowNum(2, 7, -Current_Temp, 2);
                }
                else
                {
                    OLED_ShowNum(2, 6, Current_Temp, 2);
                }
                OLED_ShowString(2, 8, "C");
            }

            // 第三行：湿度
            OLED_ShowChinese(3, 1, "湿");
            OLED_ShowChinese(3, 3, "度");
            OLED_ShowString(3, 5, ":");
            if (SHT30_Error)
            {
                OLED_ShowString(3, 6, "ERR");
            }
            else
            {
                OLED_ShowNum(3, 6, Current_Humi, 2);
                OLED_ShowString(3, 8, "%");
            }

            // 第四行：状态
            OLED_ShowChinese(4, 1, "状");
            OLED_ShowChinese(4, 3, "态");
            OLED_ShowString(4, 5, ":");
            if (System_Status == 0)
            {
                OLED_ShowChinese(4, 6, "正");
                OLED_ShowChinese(4, 8, "常");
            }
            else if (System_Status == 1)
            {
                OLED_ShowChinese(4, 6, "警");
                OLED_ShowChinese(4, 8, "告");
            }
            else
            {
                OLED_ShowChinese(4, 6, "告");
                OLED_ShowChinese(4, 8, "警");
            }
            break;

        case 1:
            // -------- 第1页：光照、土壤、状态 --------
            // 第一行：智慧大棚（居中）
						OLED_ShowChinese(1, 5, "智慧大棚");

            // 第二行：光照
            OLED_ShowChinese(2, 1, "光");
            OLED_ShowChinese(2, 3, "照");
            OLED_ShowString(2, 5, ":");
            OLED_ShowNum(2, 6, Current_Lux, 5);
            OLED_ShowString(2, 11, "lux");

            // 第三行：土壤
            OLED_ShowChinese(3, 1, "土");
            OLED_ShowChinese(3, 3, "壤");
            OLED_ShowString(3, 5, ":");
            OLED_ShowNum(3, 6, Current_Soil_Moisture, 2);
            OLED_ShowString(3, 8, "%");

            // 第四行：状态
            OLED_ShowChinese(4, 1, "状");
            OLED_ShowChinese(4, 3, "态");
            OLED_ShowString(4, 5, ":");
            if (System_Status == 0)
            {
                OLED_ShowChinese(4, 6, "正");
                OLED_ShowChinese(4, 8, "常");
            }
            else if (System_Status == 1)
            {
                OLED_ShowChinese(4, 6, "警");
                OLED_ShowChinese(4, 8, "告");
            }
            else
            {
                OLED_ShowChinese(4, 6, "告");
                OLED_ShowChinese(4, 8, "警");
            }
            break;

        case 2:
            // -------- 第2页：继电器状态 --------
            // 第一行：智慧大棚（居中）
            OLED_ShowChinese(1, 5, "智慧大棚");

            // 第二行：风机
            OLED_ShowChinese(2, 1, "风");
            OLED_ShowChinese(2, 3, "机");
            OLED_ShowString(2, 5, ":[");
            OLED_ShowString(2, 7, "]");

            // 第三行：水阀
            OLED_ShowChinese(3, 1, "水");
            OLED_ShowChinese(3, 3, "阀");
            OLED_ShowString(3, 5, ":[");
            OLED_ShowString(3, 7, "]");

            // 第四行：补光
            OLED_ShowChinese(4, 1, "补");
            OLED_ShowChinese(4, 3, "光");
            OLED_ShowString(4, 5, ":[");
            OLED_ShowString(4, 7, "]");
            break;

        default:
            Page_Index = 0;
            break;
    }
}

// ================== BH1750 光照采集任务 ==================
// 执行周期：1000ms（1秒）
void BH1750_Task(void)
{
    uint16_t lux;

    // 尝试读取 BH1750 数据（最多重试 3 次）
    uint8_t retry = 0;
    while (retry < 3 && !BH1750_Read_Lux(&lux))
    {
        retry++;
        printf("[BH1750_Task] 读取失败，重试 %d/3\r\n", retry);
    }

    if (retry >= 3)
    {
        printf("[BH1750_Task] 读取失败 3 次，放弃本次采集\r\n");
        return;
    }

    Current_Lux = lux;

    // 自动补光：光照 < 100 lux 开灯，> 500 lux 关灯
    if (Control_Mode == 0)
    {
        if (Current_Lux < 100)
            Relay_SetState(3, 1);
        else if (Current_Lux > 500)
            Relay_SetState(3, 0);
    }
}

// ================== 土壤湿度采集任务 ==================
// 执行周期：1000ms（1秒）
void Soil_Moisture_Task(void)
{
    uint16_t adc_value;
    uint8_t moisture;

    if (Soil_Moisture_Read(&adc_value, &moisture))
    {
        Current_Soil_ADC = adc_value;
        Current_Soil_Moisture = moisture;

        // 自动浇水：土壤湿度 < 20% 开水阀，> 50% 关水阀
        if (Control_Mode == 0)
        {
            if (Current_Soil_Moisture < 20)
                Relay_SetState(2, 1);
            else if (Current_Soil_Moisture > 50)
                Relay_SetState(2, 0);
        }
    }
    else
    {
        printf("[Soil_Moisture_Task] 读取失败\r\n");
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
        if (SHT30_Error)
        {
            printf("[系统数据] 温度:ERR, 湿度:ERR, 光照:%d lux, 土壤:%d%%, 病害:%s(%d%%), 模式:%s, 状态:传感器故障\r\n",
                   Current_Lux, Current_Soil_Moisture,
                   (K210_DiseaseType <= 0x04) ? K210_DiseaseNames[K210_DiseaseType] : "?",
                   K210_Confidence,
                   (Control_Mode == 0) ? "自动" : "手动");
        }
        else
        {
            printf("[系统数据] 温度:%d度, 湿度:%d%%, 光照:%d lux, 土壤:%d%%, 病害:%s(%d%%), 模式:%s, 状态:%s\r\n",
                   Current_Temp, Current_Humi, Current_Lux, Current_Soil_Moisture,
                   (K210_DiseaseType <= 0x04) ? K210_DiseaseNames[K210_DiseaseType] : "?",
                   K210_Confidence,
                   (Control_Mode == 0) ? "自动" : "手动",
                   (System_Status == 0) ? "正常" : ((System_Status == 1) ? "警告" : "告警"));
        }
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
    // '6' -> 继电器 1（风机）开启
    // '7' -> 继电器 1（风机）关闭
    // '8' -> 继电器 2（水阀）开启
    // '9' -> 继电器 2（水阀）关闭
    // 'a' -> 继电器 3（补光灯）开启
    // 'b' -> 继电器 3（补光灯）关闭
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
    else if (cmd == '6')
    {
        Relay_SetState(1, 1);  printf(">> 风机已开启\r\n");
    }
    else if (cmd == '7')
    {
        Relay_SetState(1, 0);  printf(">> 风机已关闭\r\n");
    }
    else if (cmd == '8')
    {
        Relay_SetState(2, 1);  printf(">> 水阀已开启\r\n");
    }
    else if (cmd == '9')
    {
        Relay_SetState(2, 0);  printf(">> 水阀已关闭\r\n");
    }
    else if (cmd == 'a' || cmd == 'A')
    {
        Relay_SetState(3, 1);  printf(">> 补光灯已开启\r\n");
    }
    else if (cmd == 'b' || cmd == 'B')
    {
        Relay_SetState(3, 0);  printf(">> 补光灯已关闭\r\n");
    }
    else if (cmd == '+')
    {
        Page_Index = (Page_Index + 1) % 3;
        OLED_Clear();
        printf(">> 切换至 OLED 第%d页\r\n", Page_Index);
    }
    else if (cmd == '-')
    {
        Page_Index = (Page_Index == 0) ? 2 : (Page_Index - 1);
        OLED_Clear();
        printf(">> 切换至 OLED 第%d页\r\n", Page_Index);
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

// ================== ESP8266 云平台通信任务 ==================
void ESP8266_Task(void)
{
    char json[256];
    
    if (!ESP8266_IsConnected())
    {
        printf("[ESP8266_Task] 连接WiFi...\r\n");
        if (ESP8266_ConnectWiFi())
        {
            printf("[ESP8266_Task] WiFi已连接，连接MQTT...\r\n");
            if (ESP8266_ConnectMQTT())
            {
                printf("[ESP8266_Task] 云平台就绪！\r\n");
            }
            else
            {
                printf("[ESP8266_Task] MQTT连接失败\r\n");
            }
        }
        else
        {
            printf("[ESP8266_Task] WiFi连接失败\r\n");
        }
        return;
    }
    
    // 上报数据（UTF-8 JSON，MQTTX 可正确显示中文）
    {
        // UTF-8 编码的病害名
        static const char* disease_names_utf8[] = {
            "",
            "\xE6\x97\xA9\xE7\x96\xAB\xE7\x97\x85",
            "\xE6\x99\x9A\xE7\x96\xAB\xE7\x97\x85",
            "\xE5\x8F\xB6\xE6\x96\x91\xE7\x97\x85",
            "\xE5\x81\xA5\xE5\xBA\xB7",
        };
        const char* disease_name = (K210_DiseaseType <= 0x04 && K210_DiseaseType >= 0x01) 
            ? disease_names_utf8[K210_DiseaseType] : "\xE6\x9C\xAA\xE7\x9F\xA5";
        const char* mode_name = (Control_Mode == 0) ? "\xE8\x87\xAA\xE5\x8A\xA8" : "\xE6\x89\x8B\xE5\x8A\xA8";
        const char* status_name = (System_Status == 0) ? "\xE6\xAD\xA3\xE5\xB8\xB8" 
            : ((System_Status == 1) ? "\xE8\xAD\xA6\xE5\x91\x8A" : "\xE5\x91\x8A\xE8\xAD\xA6");
        sprintf(json,
            "{\"\xE6\xB8\xA9\xE5\xBA\xA6\":%d,\"\xE6\xB9\xBF\xE5\xBA\xA6\":%d,"
            "\"\xE5\x85\x89\xE7\x85\xA7\":%d,\"\xE5\x9C\x9F\xE5\xA3\xA4\":%d,"
            "\"\xE7\x97\x85\xE5\xAE\xB3\":\"%s\",\"\xE7\xBD\xAE\xE4\xBF\xA1\xE5\xBA\xA6\":%d,"
            "\"\xE7\x8A\xB6\xE6\x80\x81\":\"%s\",\"\xE6\xA8\xA1\xE5\xBC\x8F\":\"%s\"}",
            Current_Temp, Current_Humi, Current_Lux, Current_Soil_Moisture,
            disease_name, K210_Confidence,
            status_name, mode_name);
    }
    
    // 娌跺彇 MQTT 涓嬭鍛戒护
    {
        char mqtt_cmd[16];
        if (ESP8266_CheckCommand(mqtt_cmd, sizeof(mqtt_cmd)))
        {
            printf("[MQTT鍛戒护] 鏀跺埌: %s (闀峰害=%d)\r\n", mqtt_cmd, strlen(mqtt_cmd));
            uint8_t c = mqtt_cmd[0];
            if (c == '1') { Control_Mode = 0; printf(">> [MQTT] 鍒囨崲鑷姩鏃舵櫠\r\n"); }
            else if (c == '3') { Page_Index = 1; OLED_Clear(); printf(">> [MQTT] 鏄剧ず绗?椤礬r\n"); }
            else if (c == '4') { Page_Index = 2; OLED_Clear(); printf(">> [MQTT] 鏄剧ず绗?椤礬r\n"); }
            else if (c == '6') { Relay_SetState(1, 1); printf(">> [MQTT] 椋庢満鎵撳紑\r\n"); }
            else if (c == '7') { Relay_SetState(1, 0); printf(">> [MQTT] 椋庢満鍏抽棴\r\n"); }
            else if (c == '8') { Relay_SetState(2, 1); printf(">> [MQTT] 姘撮榾鎵撳紑\r\n"); }
            else if (c == '9') { Relay_SetState(2, 0); printf(">> [MQTT] 姘撮榾鍏抽棴\r\n"); }
            else if (c == 'a' || c == 'A') { Relay_SetState(3, 1); printf(">> [MQTT] 瑁滃厜鐏墦寮�\r\n"); }
            else if (c == 'b' || c == 'B') { Relay_SetState(3, 0); printf(">> [MQTT] 瑁滃厜鐏叧闂璡r\n"); }
            else if (c == 'p' || c == 'P') { Page_Index = (Page_Index + 1) % 3; OLED_Clear(); printf(">> [MQTT] 寰幆鍒囨崲椤甸潰鍒?%d\r\n", Page_Index); }
            else if (c == '+') { Page_Index = (Page_Index + 1) % 3; OLED_Clear(); printf(">> [MQTT] 涓嬩竴椤甸潰 %d\r\n", Page_Index); }
            else if (c == '-') { Page_Index = (Page_Index == 0) ? 2 : (Page_Index - 1); OLED_Clear(); printf(">> [MQTT] 涓婁竴椤甸潰 %d\r\n", Page_Index); }
            else { printf(">> [MQTT] 鏈煡鍛戒护: 0x%02X ('%c')\r\n", c, (c >= 32 && c < 127) ? c : '?'); }
        }
    }


    if (ESP8266_PublishData(json))
    {
        printf("[ESP8266_Task] 数据上报成功\r\n");
    }
    else
    {
        printf("[ESP8266_Task] 数据上报失败，尝试重连...\r\n");
    }
}// ================== 主程序 ==================
// ================== 主程序 ==================
int main(void)
{
    // 配置系统中断优先级分组
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // 初始化系统各外设模块
    UART1_Init(115200);
    UART2_Init(115200);
    Frame_Receiver_Init(&K210_FrameRx);
    SHT30_Init();
    BH1750_Init();
    Soil_Moisture_Init();
    Motor_Init();
    Buzzer_Init();
    Relay_Init();
    OLED_Init();
    OLED_Clear();
    Scheduler_Init();
    ESP8266_Init();       

    // 中文启动菜单信息
    printf("==================================\r\n");
    printf(" 系统启动成功\r\n");
    printf(" 智慧大棚环境监测节点 v2.3\r\n");
    printf(" SHT30温湿度传感器 ... [正常]\r\n");
    printf(" BH1750光照传感器 ... [正常]\r\n");
    printf(" 土壤湿度传感器 ..... [正常]\r\n");
    printf(" 风扇驱动及蜂鸣器 ..... [正常]\r\n");
    printf(" 4路继电器控制 ...... [正常]\r\n");
    printf(" 串口双向通信接口 ..... [正常]\r\n");
    printf(" K210 UART2 接口 .... [正常]\r\n");
    printf(" K210病虫害识别 .... [等待数据...]\r\n");
    printf(" ESP8266云平台通信 .... [初始化中...]\r\n");
    printf("==================================\r\n");

    while (1)
    {
        Scheduler_Run();
    }
}
