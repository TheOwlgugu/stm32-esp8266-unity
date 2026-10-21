#include "DataMode.h"
#include "Config.h"
#include "Network.h"
#include "Serial3.h"
#include "ESP8266_Driver.h"
#include "Millis.h"
#include "LightMode.h"
#include "TempMode.h"
//#include "Co2Mode.h"
//#include "HumiMode.h"
//#include "DispMode.h"
#include <stdio.h>
#include <string.h>

/* 读取函数指针：返回当前值 */
typedef int     (*ReadFunc)(void);
/* 变化判断函数指针：判断新值是否算变化 */
typedef uint8_t (*ChangedFunc)(int, int);

static ReadFunc    g_readFunc    = NULL;
static ChangedFunc g_changedFunc = NULL;

/* 全局数据类型 */
uint8_t g_dataType = DATA_LIGHT;

/* 变化检测状态 */
static int      last_value   = 0;      /* 上次发送的值 */
static uint32_t last_send_ms = 0;      /* 上次发送的时间 */
static uint8_t  has_pending  = 0;      /* 是否有待发送的变化 */

uint8_t DataMode_Init(void)
{
    g_dataType = (config.mode[0] - '0') * 10 + (config.mode[1] - '0');
    Serial3_Printf("Data mode: %d\r\n", g_dataType);

    switch (g_dataType)
    {
        case DATA_LIGHT:
            g_readFunc    = Light_Read;
            g_changedFunc = Light_IsChanged;
            break;

        case DATA_TEMP:
            g_readFunc    = Temp_Read;
            g_changedFunc = Temp_IsChanged;
            break;

//        case DATA_CO2:
//            g_readFunc    = Co2_Read;
//            g_changedFunc = Co2_IsChanged;
//            break;

        default:
            return 0;
    }

    /* 上电时读一次作为基准，避免上电立刻发 */
    if (g_readFunc != NULL)
    {
        last_value   = g_readFunc();
        last_send_ms = millis();
    }

    return 1;
}

void DataMode_Run(void)
{
    char msg[16];
    char typeStr[3];
    int  value;

    if (g_readFunc == NULL || g_changedFunc == NULL) return;

    /* 1. 读当前值 */
    value = g_readFunc();

    /* 2. 用当前类型自己的规则判断变化 */
    if (g_changedFunc(value, last_value))
        has_pending = 1;

    /* 3. 没有变化，直接返回 */
    if (!has_pending) return;

    /* 4. 距上次发送时间不够，等待（统一用 config.interval_ms 限流） */
    if (millis() - last_send_ms < config.interval_ms) return;

    /* 5. 拼接 + 发送 */
    typeStr[0] = config.mode[0];
    typeStr[1] = config.mode[1];
    typeStr[2] = '\0';

    snprintf(msg, sizeof(msg), "%s%s%05d\n",
             config.device_id, typeStr, value);

    Serial3_Printf("[TX] %s", msg);

    
    if (Network_Send(msg))
    {
        Serial3_SendString("[SEND] OK\r\n");
        last_value   = value;        /* 更新基准值 */
        last_send_ms = millis();     /* 更新时间戳 */
        has_pending  = 0;            /* 清除 pending */
    }
    else
    {
        Serial3_SendString("[SEND] FAIL\r\n");
        /* 发送失败，pending 保留，下次重试 */
    }
}
