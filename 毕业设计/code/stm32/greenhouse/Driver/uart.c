#include "uart.h"

/* ========== UART1 (K210) 接收变量 ========== */
K210_Frame_t K210_Data = {0, 0, 0, 0};
volatile uint8_t K210_NewData = 0;

static uint8_t uart1_rx_buf[4];
static uint8_t uart1_rx_idx = 0;

/* ========== UART2 (ESP8266) 接收变量 ========== */
uint8_t UART2_RxData = 0;
uint8_t UART2_RxFlag = 0;

/* ========== UART1 初始化 (PA9=TX, PA10=RX, 连接K210) ========== */
void UART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = UART1_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(UART1_TX_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = UART1_RX_PIN;
    GPIO_Init(UART1_RX_PORT, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

/* ========== UART2 初始化 (PA2=TX, PA3=RX, 连接ESP8266) ========== */
void UART2_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = UART2_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(UART2_TX_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = UART2_RX_PIN;
    GPIO_Init(UART2_RX_PORT, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2, &USART_InitStructure);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);
}

void UART_InitAll(void)
{
    UART1_Init(115200);
    UART2_Init(115200);
}

/* ========== UART1 发送 (K210) ========== */
void UART1_SendByte(uint8_t byte)
{
    USART_SendData(USART1, byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART1_SendString(char *str)
{
    while (*str != '\0')
    {
        UART1_SendByte((uint8_t)*str++);
    }
}

/* ========== UART2 发送 (ESP8266) ========== */
void UART2_SendByte(uint8_t byte)
{
    USART_SendData(USART2, byte);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

void UART2_SendString(char *str)
{
    while (*str != '\0')
    {
        UART2_SendByte((uint8_t)*str++);
    }
}

/* ========== UART2 单字节接收API ========== */
uint8_t UART2_GetRxFlag(void) { return UART2_RxFlag; }
uint8_t UART2_GetRxData(void) { return UART2_RxData; }
void UART2_ClearRxFlag(void)  { UART2_RxFlag = 0; }

/* ========== printf 重定向到 UART1（调试输出） ========== */
int fputc(int ch, FILE *f)
{
    UART1_SendByte((uint8_t)ch);
    return ch;
}

/* ========== USART1 中断：多字节帧接收状态机 (K210协议) ========== */
void USART1_IRQHandler(void)
{
    uint8_t byte, checksum_calc;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        byte = USART_ReceiveData(USART1);

        if (uart1_rx_idx == 0)
        {
            if (byte == UART1_FRAME_HEADER)
            {
                uart1_rx_buf[0] = byte;
                uart1_rx_idx = 1;
            }
        }
        else
        {
            uart1_rx_buf[uart1_rx_idx] = byte;
            uart1_rx_idx++;

            if (uart1_rx_idx >= UART1_FRAME_LEN)
            {
                uart1_rx_idx = 0;

                checksum_calc = (uart1_rx_buf[1] + uart1_rx_buf[2]) & 0xFF;
                if (checksum_calc == uart1_rx_buf[3])
                {
                    K210_Data.disease_class = uart1_rx_buf[1];
                    K210_Data.confidence = uart1_rx_buf[2];
                    K210_Data.checksum = uart1_rx_buf[3];
                    K210_Data.valid = 1;
                    K210_NewData = 1;
                }
            }
        }
    }
}

/* ========== USART2 中断：单字节接收 (ESP8266 AT响应) ========== */
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
    {
        UART2_RxData = USART_ReceiveData(USART2);
        UART2_RxFlag = 1;
    }
}
