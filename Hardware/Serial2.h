#ifndef __SERIAL2_H
#define __SERIAL2_H

#include "stm32f10x.h"

/*
 * 通用 USART2 驱动
 * PA2 = TX，PA3 = RX
 * 默认给 PMS7003 用，其他 USART2 传感器也可以复用
 */
void Serial2_Init(uint32_t baud);

void Serial2_SendByte(uint8_t Byte);
void Serial2_SendString(const char *String);

uint8_t Serial2_Available(void);
uint8_t Serial2_GetByte(void);

#endif
