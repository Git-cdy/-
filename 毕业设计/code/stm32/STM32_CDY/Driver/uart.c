#include "uart.h"

// ================== 串口接收全局变量（极简版） ==================
uint8_t UART1_RxData = 0;      // 接收到的单字节数据
uint8_t UART1_RxFlag = 0;      // 接收完成标志位（1=接收到数据）

// ================== 串口初始化 ==================
void UART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    // -------- 使能时钟 --------
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // -------- TX (PA9)配置为复用推挽输出 --------
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin   = UART1_TX_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(UART1_TX_GPIO_PORT, &GPIO_InitStructure);

    // -------- RX (PA10)配置为输入(参考单片机硬件设计) --------
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

    // -------- NVIC 配置：设置中断优先级 --------
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);   // 分组2
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

// ================== 单字节接收 API（极简版） ==================
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

// ================== printf 重定向（需勾选MicroLIB） ==================
int fputc(int ch, FILE *f)
{
    UART1_SendByte((uint8_t)ch);
    return ch;
}

// ================== USART1 中断服务函数（极简版） ==================
// 每收到一个字节，直接赋值给 UART1_RxData，置位标志位
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        UART1_RxData = USART_ReceiveData(USART1);
        UART1_RxFlag = 1;
    }
}
