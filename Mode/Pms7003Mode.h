#ifndef __PMS7003MODE_H
#define __PMS7003MODE_H

#include "stm32f10x.h"

/*
 * PMS7003 传感器模式层
 * 依赖 Hardware/Serial2（USART2，9600 8N1）
 * 负责 32 字节帧、校验、data5（大气 PM2.5）解析
 */
void Pms7003Mode_Init(void);

/*
 * 等待并读取最新一帧有效数据（最多约 1 秒）
 * 返回 1 = 有有效数据，0 = 超时
 */
uint8_t Pms7003Mode_Read(uint16_t *pm25);

#endif
