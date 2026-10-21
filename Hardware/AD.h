#ifndef __AD_H
#define __AD_H

#include "stm32f10x.h"     // ★ 加这一行

void AD_Init(void);
uint16_t AD_GetValue(void);

#endif
