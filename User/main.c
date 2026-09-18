#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "LightSensor.h"
#include "AD.h"
#include "Serial.h"
#include "ESP8266_Driver.h"
#include "MySPI.h"
#include "W25Q64.h"
#include "Config.h"
#include "Serial3.h"
#include "StartLink.h"
#include "DataCtr.h"
#include <string.h>

char Data[32];

int main(void)
{
    /* 模块初始化 */
    OLED_Init();
    LightSensor_Init();
    AD_Init();
    Serial_Init();
    Serial3_Init();       // 只调用一次
    W25Q64_Init();
    Config_Init();

    Serial_SendString("Serial OK!\r\n");
    Delay_ms(500);

    /* ESP8266 初始化 */
    OLED_ShowString(1, 1, "START");
    if (ESP8266_SendCmd("AT\r\n", "OK", 10000))
        OLED_ShowString(1, 1, "OK   ");
    else
    {
        OLED_ShowString(1, 1, "ERROR");
        while (1);
    }

    /* 2秒监听窗口：收到 's' 才进配置模式 */
    uint32_t tick = 0;
    uint8_t got_s = 0;
    Serial3_SendString("Send 's' in 1s to config...\r\n");
    while (tick < 2000)
    {
        if (Serial3_GetRxFlag())
        {
            if (Serial3_GetRxData() == 's') { got_s = 1; break; }
        }
        Delay_ms(1);
        tick++;
    }
    if (got_s)
    {
        ChoseMod();
        Config_Init();
    }

    /* 连接 WiFi */
    OLED_ShowString(2, 1, "Wifi:");
    if (ESP8266_ConnectWiFi(config.wifi_ssid, config.wifi_pwd))
        OLED_ShowString(2, 6, "OK");
    else
    {
        OLED_ShowString(2, 6, "ERROR");
        while (1);
    }

    Delay_ms(500);

    /* 连接 Unity 服务器 */
    OLED_ShowString(3, 1, "Unity:");
    if (ESP8266_ConnectServer(config.server_ip, config.server_port))
        OLED_ShowString(3, 7, "OK");
    else
    {
        OLED_ShowString(3, 7, "ERROR");
        while (1);
    }

    OLED_Clear();
    OLED_ShowString(1, 1, config.device_id);
    OLED_ShowString(2, 1, "Light:");
    OLED_ShowString(3, 1, "TEMP:");

    /* 初始化变化检测 */
    DataCtr_Init();

    /* 主循环 */
    while (1) 
    {
        DataCtr_Update();    // 刷新传感器数据

        /* OLED 显示 */
        uint8_t light = DataCtr_GetLight();
        float   temp  = DataCtr_GetTemperature();

        OLED_ShowNum(3, 6, (int)temp, 2);
        OLED_ShowChar(3, 8, '.');
        OLED_ShowNum(3, 9, (uint16_t)(temp * 100) % 100, 2);

        if (light == 0)
            OLED_ShowString(2, 7, "On ");
        else
            OLED_ShowString(2, 7, "Off");

        /* 判断并发送 */
        Data_Send();

        Delay_ms(DATACTR_LOOP_DELAY_MS);
    }
}
