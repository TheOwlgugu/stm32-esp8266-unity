#include "Network.h"
#include "WiFiMode.h"
#include "LoRaMode.h"
#include "Serial.h"
#include "Delay.h"

uint8_t g_connType = NET_WIFI;

uint8_t Network_Connect(void)
{
    if (g_connType == NET_WIFI)
    {
        Serial_SetBaudRate(115200);   // ¡ï ÇÐµ½ 115200
        Delay_ms(100);
        return WifiConnect();
    }
    else if (g_connType == NET_LORA)
    {
        Serial_SetBaudRate(9600);     // ¡ï ÇÐµ½ 9600
        Delay_ms(100);
        return LoRaConnect();
    }
    return 0;
}

uint8_t Network_Send(const char *msg)
{
    if (g_connType == NET_WIFI)
        return WifiSend(msg);
    else if (g_connType == NET_LORA)
        return LoRaSend(msg);
    return 0;
}
