#ifndef __LIGHTMODE_H
#define __LIGHTMODE_H

#include "stm32f10x.h"

/* 光照传感器（数字 DO，接 PA1）：初始化 */
void LightMode_Init(void);

/* 读光照 DO：返回 0/1（极性取决于模块，遮光前后看变化即可） */
uint8_t LightMode_Read(void);

#endif
