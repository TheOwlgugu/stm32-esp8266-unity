/**
  * @file    Watchdog.c
  * @brief   独立看门狗（IWDG）驱动
  * @note    基于 STM32F1 标准库，使用 LSI（约 40kHz）作为时钟源
  *          超时公式：Timeout = (4 × 2^PSC × RLR) / 40000  秒
  */

#include "Watchdog.h"

/* LSI 频率（Hz），用于计算预分频和重装载值 */
#define LSI_FREQ_HZ         40000

/* IWDG 预分频系数表（对应 IWDG_Prescaler_x） */
static const uint16_t prescaler_table[7] = { 4, 8, 16, 32, 64, 128, 256 };


/**
  * @brief  初始化独立看门狗
  * @param  timeout_ms  期望的超时时间（毫秒）
  * @note   会自动选择最合适的预分频，然后计算重装载值
  */
void Watchdog_Init(uint16_t timeout_ms)
{
    uint8_t  psc;
    uint32_t rlr;

    /* 1. 使能对 IWDG 寄存器的写访问 */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    /* 2. 自动选择预分频，使 RLR 落在 1~4095 范围内 */
    for (psc = 0; psc < 7; psc++)
    {
        /* 当前分频下的时钟频率 */
        uint32_t clk_hz = LSI_FREQ_HZ / prescaler_table[psc];
        /* 当前分频下需要的重装载值 */
        rlr = (clk_hz * timeout_ms) / 1000;

        if (rlr <= 4095)
            break;   /* 找到合适的组合 */
    }

    if (rlr > 4095) rlr = 4095;   /* 上限保护 */
    if (rlr < 1)    rlr = 1;      /* 下限保护 */

    /* 3. 配置预分频 */
    IWDG_SetPrescaler(psc);   // 对应 IWDG_Prescaler_4~256
    /* 4. 配置重装载值 */
    IWDG_SetReload((uint16_t)rlr);

    /* 5. 重装载计数器（立即开始计时） */
    IWDG_ReloadCounter();

    /* 6. 使能看门狗（一旦启用无法关闭，除非复位） */
    IWDG_Enable();
}

/**
  * @brief  喂狗
  * @note   必须在超时时间内调用
  */
void Watchdog_Feed(void)
{
    IWDG_ReloadCounter();
}

/**
  * @brief  检查本次复位是否由看门狗触发
  * @retval 1 - 是；0 - 不是
  * @note   RCC 的 IWDGRSTF 标志位可以判断，读取后自动清除
  */
uint8_t Watchdog_IsResetByIWDG(void)
{
    /* 读取复位标志（读取 RCC_CSR 会自动清除标志） */
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
    {
        RCC_ClearFlag();   // 清除所有复位标志
        return 1;
    }
    RCC_ClearFlag();
    return 0;
}
