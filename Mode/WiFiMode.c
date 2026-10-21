#include "WiFiMode.h"
#include "Serial.h"
#include "Serial3.h"
#include "Delay.h"
#include "OLED.h"
#include "ESP8266_Driver.h"
#include "Config.h"

/**
  * @brief  WiFi 连接全流程
  * @note   包含：AT测试 → 连接WiFi → 连接TCP服务器
  */
uint8_t WifiConnect(void)
{
    Serial3_SendString("=== WiFi Connect ===\r\n");

    /* ★ 关键：WiFi 模式用 115200，切换回来 */
    Serial_SetBaudRate(115200);
    Delay_ms(100);
	
	/* 1. 测 AT */
    OLED_ShowString(1, 1, "AT:");
    if (!ESP8266_SendCmd("AT\r\n", "OK", 3000))
    {
        OLED_ShowString(1, 6, "FAIL");
        Serial3_SendString("AT fail\r\n");
        return 0;
    }
    OLED_ShowString(1, 6, "OK  ");

    /* 2. 连接 WiFi */
    OLED_ShowString(2, 1, "Wifi:");
    if (!ESP8266_ConnectWiFi(config.wifi_ssid, config.wifi_pwd))
    {
        OLED_ShowString(2, 6, "FAIL");
        Serial3_SendString("WiFi fail\r\n");
        return 0;
    }
    OLED_ShowString(2, 6, "OK  ");
    Serial3_SendString("WiFi OK\r\n");

    Delay_ms(500);

    /* 3. 连接 TCP 服务器 */
    OLED_ShowString(3, 1, "TCP:");
    if (!ESP8266_ConnectServer(config.wifi_ip, config.wifi_port))
    {
        OLED_ShowString(3, 6, "FAIL");
        Serial3_SendString("TCP fail\r\n");
        return 0;
    }
    OLED_ShowString(3, 6, "OK  ");
    Serial3_SendString("TCP OK\r\n");

    return 1;
}

/**
  * @brief  WiFi 发送数据
  * @note   直接调 ESP8266_SendData，失败自动重连
  */
uint8_t WifiSend(const char *msg)
{
    uint8_t ret = ESP8266_SendData(msg);

    if (!ret)
    {
        /* 发送失败，尝试重连一次 */
        Serial3_SendString("Send fail, reconnecting...\r\n");
        if (ESP8266_Reconnect())
        {
            /* 重连成功后重发一次 */
            Delay_ms(500);
            ret = ESP8266_SendData(msg);
        }
    }

    return ret;
}
