#ifndef __LIGHTMODE_H
#define __LIGHTMODE_H

#include "stm32f10x.h"

/* 读光照：返回 0=Off, 1=On */
int Light_Read(void);

/* 判断光照是否变化：值不同就算变化 */
uint8_t Light_IsChanged(int new_val, int old_val);

#endif
