#ifndef __TEMPMODE_H
#define __TEMPMODE_H

#include "stm32f10x.h"

/* 温度变化阈值：0.05℃ × 1000 = 50 */
#define TEMP_CHANGE_THRESH   50

/* 读温度：返回 温度 × 1000 */
int Temp_Read(void);

/* 判断温度是否变化：差值超过阈值 */
uint8_t Temp_IsChanged(int new_val, int old_val);

#endif
