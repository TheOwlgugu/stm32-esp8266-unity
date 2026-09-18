#ifndef __DATACTR_H
#define __DATACTR_H

#include "stm32f10x.h"

/* 温度变化阈值（摄氏度） */
#define TEMP_CHANGE_THRESHOLD   0.3f

/* 主循环延迟，用于估算经过时间（必须和 main 里的 Delay_ms 一致） */
#define DATACTR_LOOP_DELAY_MS   500

void DataCtr_Init(void);
void DataCtr_Update(void);

uint8_t DataCtr_IsLightChanged(void);
uint8_t DataCtr_IsTempChanged(void);

uint8_t DataCtr_GetLight(void);
float   DataCtr_GetTemperature(void);

void Data_Create(char *Data);

/**
  * @brief  发送数据（有变化 + 时间到才发）
  * @param  无
  * @retval 无
  * @note   每次主循环调用一次，内部自行累计时间
  */
void Data_Send(void);

#endif
