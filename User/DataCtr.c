#include "DataCtr.h"
#include "LightSensor.h"
#include "AD.h"
#include "Config.h"
#include "ESP8266_Driver.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* 上次记录的状态（基准值） */
static uint8_t last_light = 0xFF;
static float   last_temp  = -999.0f;

/* 当前传感器数据（由 Update 刷新） */
static uint8_t cur_light  = 0;
static float   cur_temp   = 0.0f;

/* ---- 以下为 Data_Send 内部状态 ---- */
static uint8_t  has_pending   = 0;   // 是否有"待发送的变化"
static uint32_t elapsed_ms    = 0;   // 自开机以来累计毫秒
static uint32_t last_send_ms  = 0;   // 上次发送的时间戳

void DataCtr_Init(void)
{
    cur_light  = LightSensor_Get();
    last_light = cur_light;

    uint16_t ad = AD_GetValue();
    cur_temp   = ((float)ad / 4095 * 60.0f) - 8.0f;
    last_temp  = cur_temp;

    has_pending  = 0;
    elapsed_ms   = 0;
    last_send_ms = 0;
}

void DataCtr_Update(void)
{
    cur_light = LightSensor_Get();
    uint16_t ad = AD_GetValue();
    cur_temp = ((float)ad / 4095 * 60.0f) - 8.0f;
}

uint8_t DataCtr_IsLightChanged(void)
{
    if (cur_light != last_light)
    {
        last_light = cur_light;
        return 1;
    }
    return 0;
}

uint8_t DataCtr_IsTempChanged(void)
{
    float diff = cur_temp - last_temp;
    if (diff < 0) diff = -diff;

    if (diff > TEMP_CHANGE_THRESHOLD)
    {
        last_temp = cur_temp;
        return 1;
    }
    return 0;
}

uint8_t DataCtr_GetLight(void)       { return cur_light; }
float   DataCtr_GetTemperature(void) { return cur_temp;  }

/**
  * @brief  拼接发送字符串
  * @note   格式：ID(2位) + 灯光(1位) + 温度(4位，忽略小数点)
  *         例：0702256 → 编号07，灯光亮(0)，温度22.56℃
  */
void Data_Create(char *Data)
{
    uint8_t id = (uint8_t)(atoi(config.device_id) % 100);
    uint8_t light = cur_light;

    int16_t temp_int = (int16_t)(cur_temp * 100.0f + 0.5f);
    if (temp_int < 0)    temp_int = 0;
    if (temp_int > 9999) temp_int = 9999;

    sprintf(Data, "%02d%d%04d\n", id, light, temp_int);
}

/**
  * @brief  发送数据（有变化 + 时间到才发）
  * @note   每次主循环调用一次，内部自行累计时间
  *         即使光照高频闪烁，也只会在间隔 >= config.interval_ms 时才发一次
  */
void Data_Send(void)
{
    char Data[16];

    /* 1. 累计时间（按主循环延迟估算） */
    elapsed_ms += DATACTR_LOOP_DELAY_MS;

    /* 2. 检测变化，有变化就置 pending 标志 */
    if (DataCtr_IsLightChanged() || DataCtr_IsTempChanged())
    {
        has_pending = 1;
    }

    /* 3. 没有待发送的变化，直接返回 */
    if (!has_pending) return;

    /* 4. 时间还没到，保留 pending，下次再判断 */
    if ((elapsed_ms - last_send_ms) < config.interval_ms) return;

    /* 5. 时间到了，发送 */
    Data_Create(Data);
    ESP8266_SendData(Data);

    /* 6. 更新状态 */
    last_send_ms = elapsed_ms;
    has_pending  = 0;
}
