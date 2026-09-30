#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include "stm32f10x.h"

/* 默认超时时间（毫秒） */
#define WATCHDOG_DEFAULT_TIMEOUT_MS   4000

/**
  * @brief  初始化独立看门狗（IWDG）
  * @param  timeout_ms  超时时间（毫秒），范围约 1~26000ms
  * @note   超时后自动复位 STM32
  */
void Watchdog_Init(uint16_t timeout_ms);

/**
  * @brief  喂狗（重装载计数器）
  * @note   必须在超时时间内调用，否则复位
  */
void Watchdog_Feed(void);

/**
  * @brief  检查看门狗是否由 IWDG 复位触发
  * @retval 1 - 是看门狗复位；0 - 不是
  * @note   上电时调用，可用于判断上次是否异常复位
  */
uint8_t Watchdog_IsResetByIWDG(void);

#endif
