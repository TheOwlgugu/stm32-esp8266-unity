/**
  * @file    Serial.c
  * @brief   串口驱动（USART1，PA9=TX，PA10=RX，波特率 115200）
  * @note    接收使用环形缓冲区，中断只负责写入，主循环通过 Serial_Available/Serial_GetByte 读取
  */

#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>

/* ==================== 环形缓冲区 ==================== */
#define RX_BUFFER_SIZE  128

static uint8_t          rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;   // 写入位置（中断更新）
static volatile uint16_t rx_tail = 0;   // 读取位置（主循环更新）

/* ==================== 初始化 ==================== */

/**
  * @brief  USART1 初始化
  * @note   PA9=TX，PA10=RX，115200 波特率，8 数据位，1 停止位，无校验
  */
void Serial_Init(void)
{
    /* 开启时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* GPIO 初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;

    // PA9 → TX（复用推挽输出）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA10 → RX（上拉输入）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART 初始化 */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);

    /* 开启接收中断 */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    /* NVIC 配置 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    /* 使能 USART1 */
    USART_Cmd(USART1, ENABLE);
}

/* ==================== 发送 ==================== */

/**
  * @brief  发送一个字节
  */
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

/**
  * @brief  发送一个数组
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    for (uint16_t i = 0; i < Length; i++)
        Serial_SendByte(Array[i]);
}

/**
  * @brief  发送一个字符串
  */
void Serial_SendString(char *String)
{
    for (uint8_t i = 0; String[i] != '\0'; i++)
        Serial_SendByte(String[i]);
}

/**
  * @brief  次方函数（内部使用）
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
        Result *= X;
    return Result;
}

/**
  * @brief  发送数字（按指定位数）
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
}

/**
  * @brief  printf 底层重定向
  */
int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch);
    return ch;
}

/**
  * @brief  格式化打印（类似 printf）
  */
void Serial_Printf(char *format, ...)
{
    char String[128];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Serial_SendString(String);
}

/* ==================== 环形缓冲区接收 ==================== */

/**
  * @brief  检查缓冲区是否有数据
  * @retval 1 - 有数据；0 - 无数据
  */
uint8_t Serial_Available(void)
{
    return (rx_head != rx_tail);
}

/**
  * @brief  从缓冲区取一个字节
  * @retval 取出的字节；缓冲区空时返回 0
  * @note   调用前建议先用 Serial_Available() 判断
  */
uint8_t Serial_GetByte(void)
{
    if (rx_head == rx_tail) return 0;
    uint8_t data = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    return data;
}

/* ==================== 中断服务函数 ==================== */

/**
  * @brief  USART1 中断服务函数
  * @note   收到数据时自动执行，将数据写入环形缓冲区
  *         同时处理 ORE 溢出错误，防止中断卡死
  */
void USART1_IRQHandler(void)
{
    /* 接收中断 */
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t data = USART_ReceiveData(USART1);
        uint16_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;

        if (next_head != rx_tail)   // 缓冲区未满
        {
            rx_buffer[rx_head] = data;
            rx_head = next_head;
        }
        // 缓冲区满则丢弃该字节

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }

    /* 溢出错误处理（重要！防止中断卡死） */
    if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) == SET)
    {
        USART_ReceiveData(USART1);   // 读一次即可清除 ORE
        USART_ClearFlag(USART1, USART_FLAG_ORE);
    }
}
