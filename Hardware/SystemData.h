#ifndef __SYSTEM_DATA_H
#define __SYSTEM_DATA_H

#include "stm32f10x.h"

typedef struct {
    // 数据存储区
    uint8_t light_val;     // 光照 0/1
    float   temp_val;      // 温度
    float   humi_val;      // 湿度
    uint16_t co2_val;      // CO2
    uint16_t pm25_val;     // PM2.5
    
    // 数据更新标志位（1表示本次采集有更新）
    uint8_t light_updated;
    uint8_t temp_updated;
    uint8_t humi_updated;
    uint8_t co2_updated;
    uint8_t pm25_updated;
} SystemData_t;

extern SystemData_t sysData;

void SystemData_Init(void);
void SystemData_ClearFlags(void);

#endif
