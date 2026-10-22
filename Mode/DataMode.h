#ifndef __DATAMODE_H
#define __DATAMODE_H

#include "stm32f10x.h"

// 初始化配置的传感器
uint8_t DataMode_Init(void);

// 当前模式下传感器需要的上电稳定时间（毫秒）
uint16_t DataMode_GetSettleMs(void);

// 当前模式是否有传感器需要持续供电（SCD40）
uint8_t DataMode_NeedsContinuousPower(void);

// 执行一次全量采集，把数据存入 sysData
void DataMode_Read_All(void);

// 把 sysData 里的最新数据打包发送
void DataMode_Send_Data(void);

#endif
