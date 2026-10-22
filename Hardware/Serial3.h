#ifndef __SERIAL3_H
#define __SERIAL3_H

#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>

/* 调试用：�?1 时，USART3 发出的每个字节同时镜像到 OLED�? �?x 16 列，自动滚屏�?
 * 需要先调用 OLED_Init()；调试完把它改成 0 即可关闭 */
#define SERIAL3_MIRROR_OLED  1

void Serial3_Init(void);
void Serial3_SendByte(uint8_t Byte);
void Serial3_SendArray(uint8_t *Array, uint16_t Length);
void Serial3_SendString(char *String);
void Serial3_Printf(char *format, ...);

uint8_t Serial3_Available(void);
uint8_t Serial3_GetByte(void);

#endif
