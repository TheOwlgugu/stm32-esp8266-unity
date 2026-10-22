#include "Network.h"
#include "WiFiMode.h"
#include "LoRaMode.h"
#include "Serial.h"
#include "Delay.h"

uint8_t g_connType = NET_WIFI;

/* 连接成功后才允许发送，避免未连接时反复触发 WiFi 重连 */
static uint8_t net_ready = 0;

uint8_t Network_Connect(void)
{
    uint8_t ok = 0;

    if (g_connType == NET_WIFI)
    {
        Serial_SetBaudRate(115200);   // ★ 切到 115200
        Delay_ms(100);
        ok = WifiConnect();
    }
    else if (g_connType == NET_LORA)
    {
        Serial_SetBaudRate(9600);     // ★ 切到 9600
        Delay_ms(100);
        ok = LoRaConnect();
    }

    net_ready = ok;
    return ok;
}

uint8_t Network_Send(const char *msg)
{
    if (!net_ready) return 0;

    if (g_connType == NET_WIFI)
        return WifiSend(msg);
    else if (g_connType == NET_LORA)
        return LoRaSend(msg);
    return 0;
}
