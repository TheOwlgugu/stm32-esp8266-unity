#include "DataMode.h"
#include "NodeMode.h"
#include "Config.h"
#include "SystemData.h"
#include "Serial3.h"
#include "Network.h"
#include "LightMode.h"
#include "Delay.h"
// #include "AHT21.h"    // 以后加上
#include "Scd40Mode.h"
#include "Pms7003Mode.h"
#include <stdio.h>

/* I2C 从机地址（7 位） */
#define AHT21_ADDR   0x38

/* 调试：每次采集打印光照值，发布时改成 0 */
#define DATAMODE_DEBUG_PRINT  1

/**
  * @brief  按当前模式初始化传感器
  * @retval 1=全部就绪，0=有设备未实现或未应答
  * @note   调用前 main 已经把传感器电源打开并等过稳定时间
  */
uint8_t DataMode_Init(void)
{
    uint8_t ok = 1;

    Serial3_Printf("Init %s\r\n", g_nodeMode.raw);

    switch (g_nodeMode.data)
    {
        case DATA_PORT_LIGHT:
            LightMode_Init();
            Serial3_SendString("Light OK\r\n");
            break;

        case DATA_PORT_BUZZER:
            /* TODO: 蜂鸣器输出初始化 */
            Serial3_SendString("Buzz TODO\r\n");
            break;

        case DATA_PORT_DS18B20:
            /* TODO: DS18B20 1-Wire 驱动 */
            Serial3_SendString("DS18B20 TODO\r\n");
            ok = 0;
            break;

        default:
            break;
    }

    switch (g_nodeMode.i2c)
    {
        case I2C_PORT_AHT21:
            /* TODO: AHT21 驱动，地址 0x38 */
            Serial3_SendString("AHT21 TODO\r\n");
            ok = 0;
            break;

        case I2C_PORT_SCD40:
            Delay_ms(1000);   /* 上电稳定，SCD40 需要 1 秒 */
            if (Scd40Mode_Init()) Serial3_SendString("SCD40 OK\r\n");
            else { Serial3_SendString("SCD40 NO\r\n"); ok = 0; }
            break;

        default:
            break;
    }

    switch (g_nodeMode.uart2)
    {
        case UART2_PORT_PMS7003:
            Pms7003Mode_Init();
            Serial3_SendString("PMS7003 ON\r\n");
            break;

        default:
            break;
    }

    return ok;
}

/**
  * @brief  传感器上电后需要的稳定时间（毫秒）
  * @note   多个传感器取最大值；只算上电稳定，不含测量转换时间
  */
uint16_t DataMode_GetSettleMs(void)
{
    uint16_t ms = 0;

    switch (g_nodeMode.data)
    {
        case DATA_PORT_LIGHT:   ms = 20;  break;
        case DATA_PORT_BUZZER:  ms = 10;  break;
        case DATA_PORT_DS18B20: ms = 20;  break;
        default: break;
    }

    switch (g_nodeMode.i2c)
    {
        case I2C_PORT_AHT21: if (ms < 100)  ms = 100;  break;
        case I2C_PORT_SCD40: if (ms < 1000) ms = 1000; break;
        default: break;
    }

    switch (g_nodeMode.uart2)
    {
        case UART2_PORT_PMS7003: if (ms < 1000) ms = 1000; break;
        default: break;
    }

    return ms;
}

/**
  * @brief  当前模式是否有传感器需要持续供电
  * @retval 1=需要（SCD40 周期测量），0=可以采完就断电
  */
uint8_t DataMode_NeedsContinuousPower(void)
{
    return (g_nodeMode.i2c == I2C_PORT_SCD40) ||
           (g_nodeMode.uart2 == UART2_PORT_PMS7003);
}

void DataMode_Read_All(void)
{
    /* 不清标志：慢速传感器（如 SCD40）每 5 秒才更新一次，
     * 清掉会让发送时刻刚好错过数据。发送完成后再统一清。 */

    /* 第1位：DATA口 */
    switch (g_nodeMode.data)
    {
        case DATA_PORT_LIGHT:
            sysData.light_val = LightMode_Read();
            sysData.light_updated = 1;
#if DATAMODE_DEBUG_PRINT
            Serial3_Printf("L=%u\r\n", (unsigned int)sysData.light_val);
#endif
            break;

        case DATA_PORT_BUZZER:
            /* 蜂鸣器只输出，不采集 */
            break;

        case DATA_PORT_DS18B20:
            /* DS18B20_Read(&sysData.temp_val); */
            /* sysData.temp_updated = 1; */
            break;

        default:
            break;
    }

    /* 第2位：I2C口 */
    switch (g_nodeMode.i2c)
    {
        case I2C_PORT_AHT21:
            /* AHT21_Read(&sysData.temp_val, &sysData.humi_val); */
            /* sysData.temp_updated = 1; */
            /* sysData.humi_updated = 1; */
            break;

        case I2C_PORT_SCD40:
            if (Scd40Mode_DataReady())
            {
                uint16_t co2;
                float t, h;

                if (Scd40Mode_Read(&co2, &t, &h))
                {
                    sysData.co2_val  = co2;
                    sysData.temp_val = t;
                    sysData.humi_val = h;
                    sysData.co2_updated  = 1;
                    sysData.temp_updated = 1;
                    sysData.humi_updated = 1;
#if DATAMODE_DEBUG_PRINT
                    /* 调试：T/H 为 ×100，C 为 ppm；一行不超过 16 字符 */
                    Serial3_Printf("T%d H%d C%u\r\n",
                                   (int)(t * 100), (int)(h * 100), (unsigned int)co2);
#endif
                }
            }
            break;

        default:
            break;
    }

    /* 第3位：USART2口 */
    switch (g_nodeMode.uart2)
    {
        case UART2_PORT_PMS7003:
            {
                uint16_t pm25;

                if (Pms7003Mode_Read(&pm25))
                {
                    sysData.pm25_val = pm25;
                    sysData.pm25_updated = 1;
#if DATAMODE_DEBUG_PRINT
                    Serial3_Printf("PM=%u\r\n", (unsigned int)pm25);
#endif
                }
            }
            break;

        default:
            break;
    }
}

void DataMode_Send_Data(void)
{
    char msg[32];

    // 按照协议拼接：设备ID(3) + 类型(2) + 数据(5)
    // 类型码：00=光照, 01=温度, 02=CO2, 03=湿度, 04=PM2.5
    // 缩放：光照 0/1；温度 x100；湿度 x100；CO2 ppm；PM2.5 ug/m3
    
    // 如果光照有更新，发送光照 (类型 00)
    if (sysData.light_updated) {
        sprintf(msg, "%s00%05d\n", config.device_id, sysData.light_val);
        Serial3_Printf("[TX] %s", msg);
        Network_Send(msg);
        // 注意：如果你用的是 LoRa，发送间隔太短会失败。后面我们在 main 里控制时间。
    }

    // 如果温度有更新，发送温度 (类型 01)
    // 温度 ×100：25.00°C -> 02500，接收端除以 100（旧固件是 ×1000）
    if (sysData.temp_updated) {
        int temp_int = (int)(sysData.temp_val * 100);
        sprintf(msg, "%s01%05d\n", config.device_id, temp_int);
        Serial3_Printf("[TX] %s", msg);
        Network_Send(msg);
    }

    // 如果湿度有更新，发送湿度 (类型 03)
    if (sysData.humi_updated) {
        int humi_int = (int)(sysData.humi_val * 100);
        sprintf(msg, "%s03%05d\n", config.device_id, humi_int);
        Serial3_Printf("[TX] %s", msg);
        Network_Send(msg);
    }

    // 如果 CO2 有更新，发送 CO2 (类型 02)
    if (sysData.co2_updated) {
        sprintf(msg, "%s02%05d\n", config.device_id, sysData.co2_val);
        Serial3_Printf("[TX] %s", msg);
        Network_Send(msg);
    }

    // 如果 PM2.5 有更新，发送 PM2.5 (类型 04)
    if (sysData.pm25_updated) {
        sprintf(msg, "%s04%05d\n", config.device_id, sysData.pm25_val);
        Serial3_Printf("[TX] %s", msg);
        Network_Send(msg);
    }
}
