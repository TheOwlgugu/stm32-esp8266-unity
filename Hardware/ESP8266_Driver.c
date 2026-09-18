#include "ESP8266_Driver.h"
#include "Serial.h"
#include "Delay.h"
#include <string.h>
#include <stdlib.h>

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

    ESP8266_SendCmd("AT+CWQAP\r\n", "OK", 2000);

    char cmd[128];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);

    // ★ 改成等 "WIFI GOT IP"
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
    return WaitForResponse("OK", 5000);
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
