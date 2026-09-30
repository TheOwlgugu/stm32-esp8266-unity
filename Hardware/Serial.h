#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>

/* ==================== 初始化与发送 ==================== */
void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);

/* ==================== 环形缓冲区接收 ==================== */
uint8_t Serial_Available(void);   // 缓冲区是否有数据
uint8_t Serial_GetByte(void);     // 从缓冲区取一个字节

#endif
