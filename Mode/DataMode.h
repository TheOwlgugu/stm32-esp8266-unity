#ifndef __DATAMODE_H
#define __DATAMODE_H

#include "stm32f10x.h"

/* 数据类型枚举（用于 g_dataType） */
#define DATA_LIGHT  0    // 光照
#define DATA_TEMP   1    // 温度
#define DATA_CO2    2    // CO2
#define DATA_HUMI   3    // 湿度
#define DATA_DISP   4    // 位移

/* 全局数据类型（由 DataMode_Init 设置） */
extern uint8_t g_dataType;

/**
  * @brief  初始化数据模式（解析 config.mode，绑定读取函数）
  * @retval 1=成功，0=配置错误
  */
uint8_t DataMode_Init(void);

/**
  * @brief  执行一次数据采集 + 拼包 + 发送
  * @note   在主循环里每次调用一次
  */
void DataMode_Run(void);

#endif
