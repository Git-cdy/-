#include "uart.h"

// ================== UART1 全局变量 ==================
uint8_t UART1_RxData = 0;      // 接收的单个字节缓存
uint8_t UART1_RxFlag = 0;      // 接收完成标志位（1=接收到数据）

// ================== UART2 接收缓冲区 ==================
uint8_t UART2_RxBuf[UART2_RX_BUF_SIZE];   // 环形接收缓冲区
volatile uint8_t UART2_RxHead = 0;        // 写入指针（ISR 写入）
volatile uint8_t UART2_RxTail = 0;        // 读取指针（主循环读取）
volatile uint32_t UART2_ErrorCount = 0;   // 接收错误计数

// ================== 串口初始化 ==================
void UART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    // -------- 使能时钟 --------
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // -------- TX (PA9) 配置为推挽输出 --------
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin   = UART1_TX_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(UART1_TX_GPIO_PORT, &GPIO_InitStructure);

    // -------- RX (PA10) 配置为输入 --------
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;   // 上拉输入
    GPIO_InitStructure.GPIO_Pin   = UART1_RX_GPIO_PIN;
    GPIO_Init(UART1_RX_GPIO_PORT, &GPIO_InitStructure);

    // -------- USART1 参数配置 --------
    USART_InitStructure.USART_BaudRate            = baudrate;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);

    // -------- 使能接收中断 --------
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    // -------- NVIC 配置，设置中断优先级 --------
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);   // 分组 2
    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // -------- 使能 USART1 --------
    USART_Cmd(USART1, ENABLE);
}

// ================== 发送单个字节 ==================
void UART1_SendByte(uint8_t byte)
{
    USART_SendData(USART1, byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

// ================== 发送字符串 ==================
void UART1_SendString(char *str)
{
    while (*str != '\0')
    {
        UART1_SendByte((uint8_t)*str++);
    }
}

// ================== 接收字节 API（中断模式）==================
uint8_t UART1_GetRxFlag(void)
{
    return UART1_RxFlag;
}

uint8_t UART1_GetRxData(void)
{
    return UART1_RxData;
}

void UART1_ClearRxFlag(void)
{
    UART1_RxFlag = 0;
}

// ================== printf 重定向（选择 MicroLIB）==================
int fputc(int ch, FILE *f)
{
    UART1_SendByte((uint8_t)ch);
    return ch;
}

// ================== USART1 中断处理（单字节模式）==================
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        UART1_RxData = USART_ReceiveData(USART1);
        UART1_RxFlag = 1;
    }
}

// ================== UART2 初始化 ==================
void UART2_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    // -------- 使能时钟 --------
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // -------- TX (PA2) 配置为推挽输出 --------
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin   = UART2_TX_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(UART2_TX_GPIO_PORT, &GPIO_InitStructure);

    // -------- RX (PA3) 配置为输入 --------
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;   // 上拉输入
    GPIO_InitStructure.GPIO_Pin   = UART2_RX_GPIO_PIN;
    GPIO_Init(UART2_RX_GPIO_PORT, &GPIO_InitStructure);

    // -------- USART2 参数配置 --------
    USART_InitStructure.USART_BaudRate            = baudrate;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_Init(USART2, &USART_InitStructure);

    // -------- 使能接收中断 --------
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    // -------- NVIC 配置，设置中断优先级 --------
    NVIC_InitStructure.NVIC_IRQChannel                   = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // -------- 使能 USART2 --------
    USART_Cmd(USART2, ENABLE);

    // -------- 初始化环形缓冲区 --------
    UART2_RxHead = 0;
    UART2_RxTail = 0;
    UART2_ErrorCount = 0;

    printf("[UART2] 初始化成功，波特率=%d, 缓冲区=%d字节\r\n", baudrate, UART2_RX_BUF_SIZE);
}

// ================== UART2 发送单个字节 ==================
void UART2_SendByte(uint8_t byte)
{
    USART_SendData(USART2, byte);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

// ================== UART2 发送字符串 ==================
void UART2_SendString(char *str)
{
    while (*str != '\0')
    {
        UART2_SendByte((uint8_t)*str++);
    }
}

// ================== UART2 接收缓冲区 API（环形缓冲）==================

// 检查缓冲区是否有数据
uint8_t UART2_GetRxFlag(void)
{
    return (UART2_RxHead != UART2_RxTail);
}

// 从缓冲区取出一个字节
uint8_t UART2_GetRxData(void)
{
    uint8_t byte = UART2_RxBuf[UART2_RxTail];
    UART2_RxTail = (UART2_RxTail + 1) % UART2_RX_BUF_SIZE;
    return byte;
}

// 清除接收标志位（自动管理，通常不需要）
void UART2_ClearRxFlag(void)
{
    // 环形缓冲区自动管理，GetRxData 会自动推进 tail
}

// ================== UART2 统计和诊断 API ==================

// 获取接收缓冲区中的字节数
uint16_t UART2_GetRxCount(void)
{
    uint8_t head = UART2_RxHead;
    uint8_t tail = UART2_RxTail;

    if (head >= tail)
        return head - tail;
    else
        return UART2_RX_BUF_SIZE - tail + head;
}

// 清空接收缓冲区（用于错误恢复）
void UART2_ClearRxBuffer(void)
{
    UART2_RxHead = 0;
    UART2_RxTail = 0;
}

// 获取接收错误计数
uint32_t UART2_GetErrorCount(void)
{
    return UART2_ErrorCount;
}

// ================== USART2 中断处理（环形缓冲区模式）==================
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
    {
        uint8_t byte = USART_ReceiveData(USART2);
        uint8_t next_head = (UART2_RxHead + 1) % UART2_RX_BUF_SIZE;

        // 检查缓冲区是否满
        if (next_head != UART2_RxTail)
        {
            UART2_RxBuf[UART2_RxHead] = byte;
            UART2_RxHead = next_head;
        }
        else
        {
            // 缓冲区满，丢弃数据并计数
            UART2_ErrorCount++;
        }
    }
}
