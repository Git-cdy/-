#include "stm32f10x.h"

// ================== DHT11 引脚配置 ==================
#define DHT11_GPIO_PORT    GPIOA
#define DHT11_GPIO_PIN     GPIO_Pin_4

// ================== DHT11 引脚操作宏 ==================
// 设置为输出模式
#define DHT11_DQ_OUT()     { GPIO_InitTypeDef GPIO_InitStructure; \
                            GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN; \
                            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; \
                            GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; \
                            GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure); }

// 设置为输入模式
#define DHT11_DQ_IN()      { GPIO_InitTypeDef GPIO_InitStructure; \
                            GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN; \
                            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; \
                            GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure); }

// 拉高数据线
#define DHT11_DQ_SET()     GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

// 拉低数据线
#define DHT11_DQ_CLR()     GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

// 读取数据线状态
#define DHT11_DQ_READ()    GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

// ================== 微秒延时函数（原始版本） ==================
// 对于 STM32F103 72MHz，每个循环约 1us
static void DHT11_Delay_us(uint32_t us)
{
    uint32_t i;
    while (us--)
    {
        for (i = 0; i < 8; i++);
    }
}

// ================== DHT11 初始化 ==================
void DHT11_Init(void)
{
    // 启用 GPIOA 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 设置 PA4 为输出模式，初始状态拉高
    DHT11_DQ_OUT();
    DHT11_DQ_SET();
}

// ================== DHT11 单总线协议层 ==================
// 发送起始信号：主机拉低至少 18ms，然后释放
static void DHT11_Start(void)
{
    DHT11_DQ_OUT();
    DHT11_DQ_CLR();
    DHT11_Delay_us(18000);  // 拉低 18ms
    DHT11_DQ_SET();
    DHT11_Delay_us(30);     // 释放后等待 30us
}

// 等待 DHT11 响应：DHT11 会拉低 80us，然后拉高 80us
static uint8_t DHT11_Wait_Response(void)
{
    uint8_t retry = 0;
    DHT11_DQ_IN();

    // 等待 DHT11 拉低（超时 100us）
    while (DHT11_DQ_READ() && retry < 100)
    {
        retry++;
        DHT11_Delay_us(1);
    }
    if (retry >= 100) return 0;

    retry = 0;
    // 等待 DHT11 释放（超时 100us）
    while (!DHT11_DQ_READ() && retry < 100)
    {
        retry++;
        DHT11_Delay_us(1);
    }
    if (retry >= 100) return 0;

    return 1;
}

// 读取一位数据：DHT11 拉低 50us，然后拉高
// 如果拉高时间 > 30us，则为 1；否则为 0
static uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;

    // 等待信号拉低（超时 100us）
    while (DHT11_DQ_READ() && retry < 100)
    {
        retry++;
        DHT11_Delay_us(1);
    }

    retry = 0;
    // 等待信号拉高（超时 100us）
    while (!DHT11_DQ_READ() && retry < 100)
    {
        retry++;
        DHT11_Delay_us(1);
    }

    // 延时 30us 后读取，判断是 0 还是 1
    DHT11_Delay_us(30);

    if (DHT11_DQ_READ())
        return 1;
    else
        return 0;
}

// 读取一字节数据（8 位）
static uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, byte = 0;
    for (i = 0; i < 8; i++)
    {
        byte <<= 1;
        byte |= DHT11_Read_Bit();
    }
    return byte;
}

// ================== DHT11 数据读取接口 ==================
// 返回值：1 表示读取成功，0 表示读取失败
// 参数：temp 指向温度变量，humi 指向湿度变量
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    uint8_t i;
    uint32_t primask;

    // 禁用中断，确保时序精确（避免 SysTick 中断打断）
    primask = __get_PRIMASK();
    __disable_irq();

    // 发送起始信号
    DHT11_Start();

    // 等待 DHT11 响应
    if (!DHT11_Wait_Response())
    {
        __set_PRIMASK(primask);
        return 0;
    }

    // 读取 5 字节数据：湿度整数 + 湿度小数 + 温度整数 + 温度小数 + 校验和
    for (i = 0; i < 5; i++)
    {
        buf[i] = DHT11_Read_Byte();
    }

    // 重新启用中断
    __set_PRIMASK(primask);

    // 校验：前 4 字节之和应等于第 5 字节
    if ((buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
        return 0;

    // 提取温度和湿度（只取整数部分）
    *humi = buf[0];
    *temp = buf[2];

    return 1;
}
