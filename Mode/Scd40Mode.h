#ifndef __SCD40MODE_H
#define __SCD40MODE_H

#include "stm32f10x.h"

/* SCD40 的 7 位 I2C 地址 */
#define SCD40_I2C_ADDR   0x62

/* 初始化并启动周期测量；1=成功，0=无应答/初始化失败 */
uint8_t Scd40Mode_Init(void);

/* 是否有新测量数据；1=有，0=没有 */
uint8_t Scd40Mode_DataReady(void);

/* 读取一次测量值：CO2(ppm)、温度(℃)、湿度(%RH)；1=成功 */
uint8_t Scd40Mode_Read(uint16_t *co2, float *temp, float *humi);

#endif
