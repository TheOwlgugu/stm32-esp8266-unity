#include "stm32f10x.h"
#include "Delay.h"
#include "Millis.h"
#include "OLED.h"
#include "Serial.h"
#include "Serial3.h"
#include "W25Q64.h"
#include "Config.h"
#include "StartLink.h"
#include "Network.h"
#include "DataMode.h"
#include "Watchdog.h"

int main(void)
{
    /* ========== 1. 外设初始化（只做一次） ========== */
    OLED_Init();
    Serial_Init();
    Serial3_Init();
    W25Q64_Init();
    Config_Init();
    Millis_Init();

    Serial3_SendString("System boot\r\n");
    Delay_ms(500);

    if (Watchdog_IsResetByIWDG())
        Serial3_SendString("Last reset: Watchdog\r\n");

    /* ========== 2. 大循环：连接失败就回配置模式 ========== */
    while (1)
    {
        /* ---------- 2.1 2秒监听窗口 ---------- */
        uint32_t tick = 0;
        uint8_t got_s = 0;
        Serial3_SendString("Send 's' in 2s to config...\r\n");

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
            StartLink();       // 配置模式（收到 exit 才返回）
            Config_Init();     // 重新加载配置
        }

        /* ---------- 2.2 解析网络类型 ---------- */
        char netType = config.mode[2];
        if (netType == '0')      g_connType = NET_WIFI;
        else if (netType == '1') g_connType = NET_LORA;
        else
        {
            Serial3_SendString("mode[2] must be 0 or 1\r\n");
            Serial3_SendString("Retry in 3s...\r\n");
            Delay_ms(3000);
            continue;   // 回到 while 开头，重新进入监听窗口
        }

        Serial3_Printf("Network: %s\r\n",
                       g_connType == NET_WIFI ? "WiFi" : "LoRa");
        Serial3_SendString("Connecting...\r\n");

        /* ---------- 2.3 连接网络 ---------- */
        OLED_Clear();
        OLED_ShowString(1, 1, "Connecting...");

        if (!Network_Connect())
        {
            Serial3_SendString("Connect failed! Back to config mode.\r\n");
            OLED_Clear();
            OLED_ShowString(1, 1, "Connect FAIL");
            OLED_ShowString(2, 1, "Back to config");
            OLED_ShowString(3, 1, "in 3s...");
            Delay_ms(3000);
            continue;   // ★ 回到 while 开头，重新进入监听窗口
        }

        /* ---------- 2.4 初始化数据模式 ---------- */
        if (!DataMode_Init())
        {
            Serial3_SendString("DataMode init failed!\r\n");
            OLED_Clear();
            OLED_ShowString(1, 1, "DataMode FAIL");
            Delay_ms(3000);
            continue;
        }

        /* ---------- 2.5 启动看门狗 ---------- */
        Watchdog_Init(30000);

        Serial3_SendString("=== Enter main loop ===\r\n");
        OLED_Clear();
        OLED_ShowString(1, 1, config.device_id);

        /* ---------- 2.6 主循环 ---------- */
        while (1)
        {
            DataMode_Run();
            Watchdog_Feed();
            Delay_ms(500);
        }
    }
}
