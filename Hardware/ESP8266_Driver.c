#include "ESP8266_Driver.h"
#include "Serial.h"
#include "Delay.h"
#include <string.h>
#include <stdlib.h>
#include "MySPI.h"
#include "W25Q64.h"
#include "Config.h"
#include "Serial3.h"

#define RX_BUF_SIZE  256

static char rx_buf[RX_BUF_SIZE];
static uint16_t rx_index = 0;

static void ClearRxBuffer(void)
{
    memset(rx_buf, 0, RX_BUF_SIZE);
    rx_index = 0;
}

static uint8_t WaitForResponse(const char *expected, uint32_t timeout_ms)
{
    uint32_t cnt = 0;
    // ★ 删掉了这里的 ClearRxBuffer()

    while (cnt < timeout_ms)
    {
        while (Serial_Available())
        {
            uint8_t byte = Serial_GetByte();
            if (rx_index < RX_BUF_SIZE - 1)
            {
                rx_buf[rx_index++] = byte;
                rx_buf[rx_index] = '\0';
            }
        }
        if (expected != NULL && strstr(rx_buf, expected) != NULL)
            return 1;
        Delay_ms(1);
        cnt++;
    }
    return 0;
}

void ESP8266_Init(void)
{
    ESP8266_SendCmd("AT\r\n", "OK", 2000);
}

uint8_t ESP8266_SendCmd(const char *cmd, const char *expected_response, uint32_t timeout_ms)
{
    ClearRxBuffer();
    Serial_SendString((char*)cmd);
    uint8_t ret = WaitForResponse(expected_response, timeout_ms);
    Delay_ms(500);   // ★ 新增：每条指令后延时
    return ret;
}

uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *pwd)
{
    if (!ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK", 2000))
        return 0;

    // ★ 开启 WiFi 自动重连
    ESP8266_SendCmd("AT+CWAUTOCONN=1\r\n", "OK", 2000);

    ESP8266_SendCmd("AT+CWQAP\r\n", "OK", 2000);

    char cmd[128];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);

    if (ESP8266_SendCmd(cmd, "WIFI GOT IP", 25000))
        return 1;

    return 0;
}

uint8_t ESP8266_GetIP(char *ip_buf)
{
    ClearRxBuffer();
    Serial_SendString("AT+CIFSR\r\n");
    if (WaitForResponse("+CIFSR:STAIP,\"", 3000))
    {
        char *start = strstr(rx_buf, "+CIFSR:STAIP,\"");
        if (start)
        {
            start += strlen("+CIFSR:STAIP,\"");
            char *end = strchr(start, '\"');
            if (end)
            {
                int len = end - start;
                if (len < 16)
                {
                    memcpy(ip_buf, start, len);
                    ip_buf[len] = '\0';
                    return 1;
                }
            }
        }
    }
    return 0;
}

uint8_t ESP8266_ConnectServer(const char *ip, uint16_t port)
{
    char cmd[64];
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", ip, port);

    // ★ 新增：重试 3 次
    for (uint8_t i = 0; i < 3; i++)
    {
        if (ESP8266_SendCmd(cmd, "CONNECT", 10000))
            return 1;
        Delay_ms(1000);
    }
    return 0;
}

uint8_t ESP8266_SendData(const char *data)
{
    uint16_t len = strlen(data);
    char cmd[24];
    sprintf(cmd, "AT+CIPSEND=%d\r\n", len);

    ClearRxBuffer();
    Serial_SendString(cmd);
    if (!WaitForResponse(">", 3000))
        return 0;

    Serial_SendString((char*)data);

    // ★ 等待 "SEND OK" 而不是只等 "OK"
    if (!WaitForResponse("SEND OK", 5000))
        return 0;

    return 1;
}
uint8_t ESP8266_EnterTransparentMode(void)
{
    if (!ESP8266_SendCmd("AT+CIPMUX=0\r\n", "OK", 2000))
        return 0;
    if (!ESP8266_SendCmd("AT+CIPMODE=1\r\n", "OK", 2000))
        return 0;

    ClearRxBuffer();
    Serial_SendString("AT+CIPSEND\r\n");
    return WaitForResponse(">", 3000);
}

/**
  * @brief   检查 TCP 连接是否还活着
  * @retval  1 - 连接正常；0 - 已断开
  */
uint8_t ESP8266_CheckTCP(void)
{
    ClearRxBuffer();
    Serial_SendString("AT+CIPSTATUS\r\n");

    uint32_t cnt = 0;
    while (cnt < 2000)
    {
        while (Serial_Available())
        {
            uint8_t byte = Serial_GetByte();
            if (rx_index < RX_BUF_SIZE - 1)
            {
                rx_buf[rx_index++] = byte;
                rx_buf[rx_index] = '\0';
            }
        }

        char *p = strstr(rx_buf, "STATUS:");
        if (p != NULL)
        {
            p += 7;    // 跳过 "STATUS:"
            if (*p == '3')   // ★ 3 = TCP 已连接
                return 1;
            else
                return 0;    // 其他状态都视为断开
        }

        Delay_ms(1);
        cnt++;
    }
    return 0;
}

/**
  * @brief   断线重连
  * @retval  1 - 重连成功；0 - 失败
  */
uint8_t ESP8266_Reconnect(void)
{
    Serial3_SendString("Try Reconnecting...\r\n");

    // 1. 关闭旧连接（如果还在）
    ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK", 3000);
    Delay_ms(500);

    // 2. 检查 WiFi 是否还在
    if (!ESP8266_SendCmd("AT+CWJAP?\r\n", "OK", 3000))
    {
        Serial3_SendString("WiFi Break,Reconnect WiFi...\r\n");
        if (!ESP8266_ConnectWiFi(config.wifi_ssid, config.wifi_pwd))
        {
            Serial3_SendString("WiFi Fail\r\n");
            return 0;
        }
    }

    // 3. 重新连接 TCP 服务器
    Delay_ms(500);
    if (!ESP8266_ConnectServer(config.server_ip, config.server_port))
    {
        Serial3_SendString("Servaer Fail\r\n");
        return 0;
    }

    Serial3_SendString("Server Reconnection\r\n");
    return 1;
}

void ESP8266_ExitTransparentMode(void)
{
    Serial_SendString("+++");
    Delay_ms(500);
}

void ESP8266_CloseConnection(void)
{
    ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK", 3000);
}

void ESP8266_Reset(void)
{
    ESP8266_SendCmd("AT+RST\r\n", "OK", 5000);
}
