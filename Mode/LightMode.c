#include "LightMode.h"

/**
  * @brief  光照传感器初始化
  * @note   数字 DO 输出，PA1 上拉输入
  */
void LightMode_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
  * @brief  读取光照 DO
  * @retval 0/1
  */
uint8_t LightMode_Read(void)
{
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1);
}
