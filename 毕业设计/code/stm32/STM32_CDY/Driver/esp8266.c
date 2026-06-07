#include "esp8266.h"

static uint8_t  rx_buf[ESP8266_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
static uint8_t wifi_connected = 0;
static uint8_t mqtt_connected = 0;

static void UART3_Init(uint32_t baudrate);
static void ESP8266_ClearRxBuf(void);
static uint8_t ESP8266_WaitResponse(const char *expect, uint32_t timeout_ms);
static void ESP8266_SendRaw(const char *data, uint16_t len);

static void UART3_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin   = ESP8266_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ESP8266_TX_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = ESP8266_RX_PIN;
    GPIO_Init(ESP8266_RX_PORT, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate            = baudrate;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_Init(ESP8266_USART, &USART_InitStructure);

    USART_ITConfig(ESP8266_USART, USART_IT_RXNE, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel                   = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    USART_Cmd(ESP8266_USART, ENABLE);
    rx_head = 0;
    rx_tail = 0;
    printf("[ESP8266] UART3 OK, baud=%d\r\n", baudrate);
}

void ESP8266_UART3_IRQHandler(void)
{
    if (USART_GetITStatus(ESP8266_USART, USART_IT_RXNE) == SET)
    {
        uint8_t byte = USART_ReceiveData(ESP8266_USART);
        uint16_t next = (rx_head + 1) % ESP8266_RX_BUF_SIZE;
        if (next != rx_tail) { rx_buf[rx_head] = byte; rx_head = next; }
    }
}

static void ESP8266_ClearRxBuf(void) { rx_head = 0; rx_tail = 0; }

uint8_t ESP8266_SendCmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    ESP8266_ClearRxBuf();
    for (uint16_t i = 0; cmd[i] != '\0'; i++)
    {
        USART_SendData(ESP8266_USART, (uint8_t)cmd[i]);
        while (USART_GetFlagStatus(ESP8266_USART, USART_FLAG_TXE) == RESET);
    }
    USART_SendData(ESP8266_USART, '\r');
    while (USART_GetFlagStatus(ESP8266_USART, USART_FLAG_TXE) == RESET);
    USART_SendData(ESP8266_USART, '\n');
    while (USART_GetFlagStatus(ESP8266_USART, USART_FLAG_TXE) == RESET);
    return ESP8266_WaitResponse(expect, timeout_ms);
}

static uint8_t ESP8266_WaitResponse(const char *expect, uint32_t timeout_ms)
{
    volatile uint32_t wait = 0;
    char recv_buf[256];
    uint16_t idx, t;
    while (wait < timeout_ms * 8000)
    {
        wait++;
        if (rx_tail != rx_head)
        {
            idx = 0; t = rx_tail;
            while (t != rx_head && idx < 255)
            {
                recv_buf[idx++] = (char)rx_buf[t];
                t = (t + 1) % ESP8266_RX_BUF_SIZE;
            }
            recv_buf[idx] = '\0';
            if (strstr(recv_buf, expect) != NULL) return 1;
            if (strstr(recv_buf, "ERROR") != NULL) return 0;
        }
    }
    return 0;
}

static void ESP8266_SendRaw(const char *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        USART_SendData(ESP8266_USART, (uint8_t)data[i]);
        while (USART_GetFlagStatus(ESP8266_USART, USART_FLAG_TXE) == RESET);
    }
}

void ESP8266_Init(void)
{
    printf("\r\n[ESP8266] ==== Init Start ====\r\n");
    UART3_Init(ESP8266_BAUDRATE);
    if (!ESP8266_SendCmd("AT", "OK", 2000))
    {
        printf("[ESP8266] AT FAIL!\r\n");
        return;
    }
    printf("[ESP8266] AT OK\r\n");
    ESP8266_SendCmd("AT+CWMODE=1", "OK", 2000);
    ESP8266_SendCmd("ATE0", "OK", 1000);
    printf("[ESP8266] ==== Init Done ====\r\n\r\n");
}

uint8_t ESP8266_ConnectWiFi(void)
{
    char cmd[128];
    printf("[ESP8266] WiFi: %s ...\r\n", WIFI_SSID);
    // 先断开可能存在的旧连接，避免状态冲突
    ESP8266_SendCmd("AT+CWQAP", "OK", 3000);
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASSWORD);
    if (!ESP8266_SendCmd(cmd, "WIFI GOT IP", 15000))
    {
        printf("[ESP8266] WiFi FAIL!\r\n");
        wifi_connected = 0;
        return 0;
    }
    wifi_connected = 1;
    printf("[ESP8266] WiFi OK\r\n");
    return 1;
}

uint8_t ESP8266_ConnectMQTT(void)
{
    char cmd[256];
    if (!wifi_connected) { printf("[ESP8266] WiFi not connected\r\n"); return 0; }
    printf("[ESP8266] Broker: %s:%d\r\n", EMQX_BROKER_HOST, EMQX_BROKER_PORT);
    ESP8266_SendCmd("AT+MQTTCLEAN=0", "OK", 2000);  // clear old MQTT state

    snprintf(cmd, sizeof(cmd), "AT+MQTTUSERCFG=0,2,\"%s\",\"%s\",\"%s\",0,0,\"\"",
             EMQX_CLIENT_ID, EMQX_MQTT_USERNAME, EMQX_MQTT_PASSWORD);
    if (!ESP8266_SendCmd(cmd, "OK", 3000)) { printf("[ESP8266] USERCFG FAIL\r\n"); return 0; }
    printf("[ESP8266] USERCFG OK\r\n");

    snprintf(cmd, sizeof(cmd), "AT+MQTTCONNCFG=0,60,0,\"\",\"\",0,0");
    if (!ESP8266_SendCmd(cmd, "OK", 3000)) { printf("[ESP8266] CONNCFG FAIL\r\n"); return 0; }
    printf("[ESP8266] CONNCFG OK\r\n");

    printf("[ESP8266] Connecting EMQX Cloud...\r\n");
    snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%d,0",
             EMQX_BROKER_HOST, EMQX_BROKER_PORT);
    if (!ESP8266_SendCmd(cmd, "OK", 15000))
    {
        printf("[ESP8266] MQTT CONNECT FAIL!\r\n");
        mqtt_connected = 0;
        return 0;
    }
    mqtt_connected = 1;
    printf("[ESP8266] MQTT Connected!\r\n");
    return 1;
}

uint8_t ESP8266_PublishData(const char *json_str)
{
    char cmd[128];
    uint16_t json_len;
    if (!mqtt_connected) return 0;
    json_len = strlen(json_str);
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%d,1,0", EMQX_PUB_TOPIC, json_len);
    if (!ESP8266_SendCmd(cmd, ">", 5000))
    {
        printf("[ESP8266] PUBRAW prompt fail\r\n");
        return 0;
    }
    ESP8266_SendRaw(json_str, json_len);
    return ESP8266_WaitResponse("OK", 5000);
}

uint8_t ESP8266_IsConnected(void) { return wifi_connected && mqtt_connected; }
