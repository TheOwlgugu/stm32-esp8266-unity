#include "Serial2.h"

/* 接收环形缓冲区：9600bps 下 512 字节约 0.5 秒的数据量 */
#define SERIAL2_RX_BUFFER_SIZE  512

static uint8_t          rx_buffer[SERIAL2_RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;   // 中断写入位置
static volatile uint16_t rx_tail = 0;   // 主循环读取位置

/**
  * @brief  USART2 初始化
  * @param  baud  波特率（PMS7003 用 9600）
  */
void Serial2_Init(uint32_t baud)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;

    /* PA2 → USART2_TX */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA3 → USART2_RX */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate            = baud;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART2, &USART_InitStructure);

    rx_head = 0;
    rx_tail = 0;

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel                   = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);
}

/**
  * @brief  发送一个字节
  */
void Serial2_SendByte(uint8_t Byte)
{
    USART_SendData(USART2, Byte);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

/**
  * @brief  发送字符串
  */
void Serial2_SendString(const char *String)
{
    while (*String != '\0')
    {
        Serial2_SendByte((uint8_t)*String++);
    }
}

/**
  * @brief  检查接收缓冲区是否有数据
  */
uint8_t Serial2_Available(void)
{
    return (rx_head != rx_tail);
}

/**
  * @brief  从接收缓冲区取一个字节
  */
uint8_t Serial2_GetByte(void)
{
    uint8_t data;

    if (rx_head == rx_tail) return 0;

    data = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % SERIAL2_RX_BUFFER_SIZE;
    return data;
}

/**
  * @brief  USART2 中断服务函数
  * @note   只负责把字节写入环形缓冲区，协议解析交给 Mode 层
  */
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
    {
        uint8_t data = (uint8_t)USART_ReceiveData(USART2);
        uint16_t next = (rx_head + 1) % SERIAL2_RX_BUFFER_SIZE;

        if (next != rx_tail)   /* 缓冲区未满，满了就丢弃 */
        {
            rx_buffer[rx_head] = data;
            rx_head = next;
        }

        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }

    /* 溢出错误处理，防止中断卡死 */
    if (USART_GetFlagStatus(USART2, USART_FLAG_ORE) == SET)
    {
        USART_ReceiveData(USART2);
        USART_ClearFlag(USART2, USART_FLAG_ORE);
    }
}
