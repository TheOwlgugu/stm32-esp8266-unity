/**
  * @file    Millis.c
  * @brief   毫秒计数器，基于 TIM2 中断，用于非阻塞计时
  * @note    不影响 Delay.c 的 SysTick 轮询式延时
  */

#include "stm32f10x.h"

/* 全局毫秒计数器 */
static volatile uint32_t ms_counter = 0;

/**
  * @brief  初始化 TIM2，配置为每 1ms 中断一次
  * @note   必须在 main 里调用一次
  *         TIM2 挂在 APB1，时钟 72MHz
  */
void Millis_Init(void)
{
    /* 1. 开启 TIM2 时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* 2. 配置 TIM2：72MHz / 72 = 1MHz，再 / 1000 = 1kHz（1ms） */
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    TIM_InitStructure.TIM_Prescaler = 72 - 1;      // 预分频：72MHz → 1MHz
    TIM_InitStructure.TIM_Period = 1000 - 1;       // 自动重装：1MHz / 1000 = 1kHz
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);

    /* 3. 开启更新中断 */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    /* 4. NVIC 配置 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;   // 比 USART1 低
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);

    /* 5. 启动 TIM2 */
    TIM_Cmd(TIM2, ENABLE);
}

/**
  * @brief  获取系统运行毫秒数
  * @retval 自 Millis_Init 以来的毫秒数，约 49 天溢出一次
  */
uint32_t millis(void)
{
    return ms_counter;
}

/**
  * @brief  TIM2 中断服务函数（每 1ms 触发一次）
  */
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        ms_counter++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
