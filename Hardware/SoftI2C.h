#ifndef __SOFTI2C_H
#define __SOFTI2C_H

#include "stm32f10x.h"

/* 软件 I2C：PB8 = SCL，PB9 = SDA（开漏，依赖总线上的上拉电阻） */
void    SoftI2C_Init(void);
void    SoftI2C_Start(void);
void    SoftI2C_Stop(void);

/* 发送/接收一个字节；SendByte 返回 1=收到 ACK，0=NACK/超时 */
uint8_t SoftI2C_SendByte(uint8_t byte);
uint8_t SoftI2C_ReadByte(uint8_t ack);      /* ack=1 回 ACK，0 回 NACK */

/* 以下接口的 dev 都是 7 位地址（不含读写位），返回 1=成功，0=失败 */
uint8_t SoftI2C_Write(uint8_t dev, const uint8_t *buf, uint16_t len);
uint8_t SoftI2C_Read(uint8_t dev, uint8_t *buf, uint16_t len);
uint8_t SoftI2C_WriteReg(uint8_t dev, uint8_t reg, uint8_t value);
uint8_t SoftI2C_ReadReg(uint8_t dev, uint8_t reg, uint8_t *value);
uint8_t SoftI2C_IsReady(uint8_t dev);
uint8_t SoftI2C_Probe(uint8_t dev);   /* 慢速探测，用于 I2C 总线扫描 */

#endif
