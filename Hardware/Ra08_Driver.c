#include "Ra08_Driver.h"
#include "Serial.h"
#include "Delay.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Serial3.h>

#define RA08_BAUD    9600
#define RA08_EOL     "\r\n"
#define RX_BUF_SIZE  256

static char     rx_buf[RX_BUF_SIZE];
static uint16_t rx_index = 0;
static uint8_t  joined   = 0;

static void ClearRxBuffer(void)
{
    memset(rx_buf, 0, RX_BUF_SIZE);
    rx_index = 0;
}

/* 返回 1:匹配 ok  2:匹配 fail  0:超时。ok/fail 传 NULL 则只是收数据 */
static uint8_t WaitFor(const char *ok, const char *fail, uint32_t timeout_ms)
{
    for (uint32_t t = 0; t < timeout_ms; t++)
    {
        while (Serial_Available())
        {
            if (rx_index >= RX_BUF_SIZE - 1)   /* 满了丢掉旧的一半 */
            {
                memmove(rx_buf, rx_buf + rx_index - RX_BUF_SIZE / 2, RX_BUF_SIZE / 2);
                rx_index = RX_BUF_SIZE / 2;
            }
            rx_buf[rx_index++] = Serial_GetByte();
            rx_buf[rx_index] = '\0';

            if (ok   && strstr(rx_buf, ok))   return 1;
            if (fail && strstr(rx_buf, fail)) return 2;
        }
        Delay_ms(1);
    }
    return 0;
}

uint8_t Ra08_SendCmd(const char *cmd, const char *expected, uint32_t timeout_ms)
{
    ClearRxBuffer();
    Serial_SendString((char *)cmd);
    return WaitFor(expected, "ERROR", timeout_ms) == 1;
}

uint8_t Ra08_Init(void)
{
    Serial_SetBaudRate(RA08_BAUD);  // 改成 9600
    Delay_ms(100);
    
    // 使用统一的 SendCmd 测试模块
    if (Ra08_SendCmd("AT+CGMI?" RA08_EOL, "OK", 1000)) {
        Serial3_SendString("[LoRa] Init OK\r\n");
        return 1;
    } else {
        Serial3_SendString("[LoRa] Init Timeout\r\n");
        return 0;
    }
}

/* 只在第一次烧录/换密钥时调用,CSAVE 会写 Flash,不要每次开机都调 */
uint8_t Ra08_Provision(const char *deveui, const char *appeui, const char *appkey)
{
    char cmd[64];

    sprintf(cmd, "AT+CDEVEUI=%s" RA08_EOL, deveui);
    if (!Ra08_SendCmd(cmd, "OK", 1000)) return 0;
    sprintf(cmd, "AT+CAPPEUI=%s" RA08_EOL, appeui);
    if (!Ra08_SendCmd(cmd, "OK", 1000)) return 0;
    sprintf(cmd, "AT+CAPPKEY=%s" RA08_EOL, appkey);
    if (!Ra08_SendCmd(cmd, "OK", 1000)) return 0;

    return Ra08_SendCmd("AT+CCLASS=0" RA08_EOL, "OK", 1000)
        && Ra08_SendCmd("AT+CFREQBANDMASK=0001" RA08_EOL, "OK", 1000)
        && Ra08_SendCmd("AT+CSAVE" RA08_EOL, "OK", 3000);
}

uint8_t Ra08_Join(void)
{
    joined = 0;
    if (!Ra08_SendCmd("AT+CJOIN=1,0,8,8" RA08_EOL, "OK", 2000))
        return 0;

    /* OK 只表示命令被接受,结果稍后以 +CJOIN:OK / +CJOIN:FAIL 上报;最坏 8 次 x 8 秒 */
    if (WaitFor("+CJOIN:OK", "+CJOIN:FAIL", 70000) == 1)
    {
        joined = 1;
        return 1;
    }
    return 0;
}

uint8_t Ra08_IsJoined(void)
{
    return joined;
}

Ra08_SendResult Ra08_SendData(const uint8_t *data, uint8_t len, uint8_t confirm)
{
    char cmd[128];

    if (len == 0 || len > RA08_MAX_PAYLOAD) return RA08_SEND_TOO_LONG;
    if (!joined) return RA08_SEND_NOT_JOINED;

    int n = sprintf(cmd, "AT+DTRX=%d,1,%d,", confirm ? 1 : 0, len);
    for (uint8_t i = 0; i < len; i++)
        n += sprintf(cmd + n, "%02X", data[i]);
    strcpy(cmd + n, RA08_EOL);

    ClearRxBuffer();
    Serial_SendString(cmd);

    uint8_t r = WaitFor("OK+SEND", "ERR+SEND", 3000);
    if (r == 0) return RA08_SEND_TIMEOUT;
    if (r == 2)
    {
        WaitFor(NULL, NULL, 30);   /* 等错误码那几个字符到齐 */
        char *p = strstr(rx_buf, "ERR+SEND:");
        char *end = NULL;
        long code = -1;
        if (p) { code = strtol(p + 9, &end, 16); if (end == p + 9) code = -1; }

        if (code == 0) { joined = 0; return RA08_SEND_NOT_JOINED; }
        if (code == 2) return RA08_SEND_TOO_LONG;
        return RA08_SEND_BUSY;
    }

    /* 确认帧要等网关应答,最长约十几秒 */
    r = WaitFor("OK+SENT", "ERR+SENT", confirm ? 15000 : 5000);
    if (r == 1) return RA08_SEND_OK;
    if (r == 2) return RA08_SEND_NO_ACK;
    return RA08_SEND_TIMEOUT;
}
