#include "Serial3.h"

uint8_t Serial3_RxData;   // 接收到的数据字节
uint8_t Serial3_RxFlag;   // 接收标志位（收到数据置1，读取后清零）

/**
  * @brief  USART3 初始化（PB10 = TX，PB11 = RX，波特率 115200）
  * @param  无
  * @retval 无
  */
void Serial3_Init(void)
{
    /* 开启时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);   // USART3 挂在 APB1 上
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);    // PB10/PB11 在 GPIOB

    /* GPIO 初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;

    // PB10 → USART3_TX（复用推挽输出）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // PB11 → USART3_RX（上拉输入）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* USART3 初始化 */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART3, &USART_InitStructure);

    /* 开启接收中断 */
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    /* NVIC 中断配置 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;   // 比 USART1(1) 低
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    /* 使能 USART3 */
    USART_Cmd(USART3, ENABLE);
}

/**
  * @brief  发送一个字节
  */
void Serial3_SendByte(uint8_t Byte)
{
    USART_SendData(USART3, Byte);
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

/**
  * @brief  发送一个数组
  */
void Serial3_SendArray(uint8_t *Array, uint16_t Length)
{
    for (uint16_t i = 0; i < Length; i++)
        Serial3_SendByte(Array[i]);
}

/**
  * @brief  发送一个字符串
  */
void Serial3_SendString(char *String)
{
    for (uint8_t i = 0; String[i] != '\0'; i++)
        Serial3_SendByte(String[i]);
}

/**
  * @brief  格式化打印（类似 printf）
  */
void Serial3_Printf(char *format, ...)
{
    char String[128];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Serial3_SendString(String);
}

/**
  * @brief  获取接收标志位（读取后自动清零）
  * @retval 1 表示有新数据，0 表示无
  */
uint8_t Serial3_GetRxFlag(void)
{
    if (Serial3_RxFlag == 1)
    {
        Serial3_RxFlag = 0;
        return 1;
    }
    return 0;
}

/**
  * @brief  获取接收到的数据字节
  */
uint8_t Serial3_GetRxData(void)
{
    return Serial3_RxData;
}

/**
  * @brief  USART3 中断服务函数
  * @note   收到数据时自动执行，将数据存入 Serial3_RxData 并置标志位
  */
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
    {
        Serial3_RxData = USART_ReceiveData(USART3);
        Serial3_RxFlag = 1;
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}
