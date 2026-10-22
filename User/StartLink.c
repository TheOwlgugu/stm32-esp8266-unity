#include "StartLink.h"
#include "Config.h"
#include "Serial3.h"
#include "Delay.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FIELD_MATCH(name)  (field_len == strlen(name) && strncmp(field, name, field_len) == 0)
#define FIELD_MATCH_UNDERSCORE(name) (field_len == strlen(name) + 1 && field[0] == '_' && strncmp(field + 1, name, strlen(name)) == 0)
#define FIELD_IS(short1, short2, longname) (FIELD_MATCH(short1) || FIELD_MATCH(short2) || FIELD_MATCH_UNDERSCORE(short1) || FIELD_MATCH(longname))

static void SaveAndReply(const char *field, int field_len, const char *value)
{
    Config_Save();
    Serial3_Printf("OK: %.*s = %s\r\n", field_len, field, value);
}

uint8_t SetConfig(char *cmd)
{
    if (strncmp(cmd, "set:", 4) != 0 && strncmp(cmd, "SET:", 4) != 0) return 0;

    char *eq = strchr(cmd, '=');
    if (eq == NULL) return 0;

    char *field = cmd + 4;
    uint8_t field_len = eq - field;
    char *value = eq + 1;

    uint8_t val_len = strlen(value);
    while (val_len > 0 && (value[val_len - 1] == '\r' || value[val_len - 1] == '\n')) {
        value[--val_len] = '\0';
    }

    uint8_t matched = 0;

    /* ---------- device_id ---------- */
    if (FIELD_IS("id", "ID", "device_id"))
    {
        if (strlen(value) != 3) { Serial3_SendString("ERR: id must be 3 digits\r\n"); return 0; }
        strncpy(config.device_id, value, sizeof(config.device_id) - 1);
        matched = 1;
    }
    /* ---------- mode（4位组合校验） ---------- */
    else if (FIELD_IS("mode", "MODE", "mode"))
    {
        if (strlen(value) != 4) {
            Serial3_SendString("ERR: mode must be 4 digits (e.g. 3121)\r\n");
            return 0;
        }
        // DATA口(0~3), I2C口(0~2), USART2(0~1), NET(0~1)
        if (value[0] < '0' || value[0] > '3' ||
            value[1] < '0' || value[1] > '2' ||
            value[2] < '0' || value[2] > '1' ||
            value[3] < '0' || value[3] > '1') 
        {
            Serial3_SendString("ERR: mode format invalid\r\n");
            return 0;
        }
        strncpy(config.mode, value, sizeof(config.mode) - 1);
        matched = 1;
    }
    /* ---------- interval（发信间隔） ---------- */
    else if (FIELD_IS("interval", "INTERVAL", "send_interval"))
    {
        uint32_t ms = (uint32_t)atol(value);
        if (ms < 6000) {
            Serial3_SendString("ERR: interval must be >= 6000 ms (LoRa limit)\r\n");
            return 0;
        }
        config.send_interval_ms = ms;
        matched = 1;
    }
    /* ---------- WiFi 配置 ---------- */
    else if (FIELD_IS("wifi", "WIFI", "wifi_ssid"))
    {
        strncpy(config.wifi_ssid, value, sizeof(config.wifi_ssid) - 1);
        matched = 1;
    }
    else if (FIELD_IS("pwd", "PWD", "wifi_pwd"))
    {
        strncpy(config.wifi_pwd, value, sizeof(config.wifi_pwd) - 1);
        matched = 1;
    }
    else if (FIELD_IS("ip", "IP", "wifi_ip"))
    {
        strncpy(config.wifi_ip, value, sizeof(config.wifi_ip) - 1);
        matched = 1;
    }
    else if (FIELD_IS("port", "PORT", "wifi_port"))
    {
        config.wifi_port = (uint16_t)atoi(value);
        matched = 1;
    }
    /* ---------- LoRa 配置 ---------- */
    else if (FIELD_IS("deveui", "DEVEUI", "lora_deveui"))
    {
        if (strlen(value) != 16) { Serial3_SendString("ERR: deveui must be 16 hex chars\r\n"); return 0; }
        strncpy(config.lora_deveui, value, sizeof(config.lora_deveui) - 1);
        matched = 1;
    }
    else if (FIELD_IS("appeui", "APPEUI", "lora_appeui"))
    {
        if (strlen(value) != 16) { Serial3_SendString("ERR: appeui must be 16 hex chars\r\n"); return 0; }
        strncpy(config.lora_appeui, value, sizeof(config.lora_appeui) - 1);
        matched = 1;
    }
    else if (FIELD_IS("appkey", "APPKEY", "lora_appkey"))
    {
        if (strlen(value) != 32) { Serial3_SendString("ERR: appkey must be 32 hex chars\r\n"); return 0; }
        strncpy(config.lora_appkey, value, sizeof(config.lora_appkey) - 1);
        matched = 1;
    }
    else if (FIELD_IS("class", "CLASS", "lora_class"))
    {
        uint8_t c = (uint8_t)atoi(value);
        if (c > 2) { Serial3_SendString("ERR: class must be 0/1/2\r\n"); return 0; }
        config.lora_class = c;
        matched = 1;
    }

    if (matched) { SaveAndReply(field, field_len, value); return 1; }
    else { Serial3_SendString("ERR: unknown field\r\n"); return 0; }
}

static void ShowConfig(void)
{
    Serial3_Printf("id=%s\r\n",       config.device_id);
    Serial3_Printf("mode=%s\r\n",     config.mode);
    Serial3_Printf("interval=%d\r\n", config.send_interval_ms);  // 注意这里名字变了
    Serial3_Printf("wifi=%s\r\n",     config.wifi_ssid);
    Serial3_Printf("pwd=%s\r\n",      config.wifi_pwd);
    Serial3_Printf("ip=%s\r\n",       config.wifi_ip);
    Serial3_Printf("port=%d\r\n",     config.wifi_port);
    Serial3_Printf("deveui=%s\r\n",   config.lora_deveui);
    Serial3_Printf("appeui=%s\r\n",   config.lora_appeui);
    Serial3_Printf("appkey=%s\r\n",   config.lora_appkey);
    Serial3_Printf("class=%d\r\n",    config.lora_class);
    Serial3_Printf("provisioned=%d\r\n", config.lora_provisioned);
}

void StartLink(void)
{
    char cmd_buf[128];
    uint8_t cmd_idx = 0;

    Serial3_SendString("\r\n=== Config Mode ===\r\n");
    Serial3_SendString("Commands:\r\n");
    Serial3_SendString("  set:id=001\r\n");
    Serial3_SendString("  set:mode=XXXX  (DATA 0-3, I2C 0-2, UART 0-1, NET 0-1)\r\n");
    Serial3_SendString("                 (1=Light, 2=Buzzer, 3=DS18B20)\r\n");
    Serial3_SendString("                 (1=AHT21, 2=SCD40)\r\n");
    Serial3_SendString("                 (1=PMS7003)\r\n");
    Serial3_SendString("                 (0=WiFi, 1=LoRa)\r\n");
    Serial3_SendString("  set:interval=60000  (ms, >= 6000)\r\n");
    Serial3_SendString("  set:wifi=SSID\r\n");
    Serial3_SendString("  set:pwd=PASSWORD\r\n");
    Serial3_SendString("  set:ip=192.168.1.100\r\n");
    Serial3_SendString("  set:port=4000\r\n");
    Serial3_SendString("  set:deveui=16 hex\r\n");
    Serial3_SendString("  set:appeui=16 hex\r\n");
    Serial3_SendString("  set:appkey=32 hex\r\n");
    Serial3_SendString("  set:class=0/1/2\r\n");
    Serial3_SendString("  get   - show config\r\n");
    Serial3_SendString("  exit  - quit\r\n");

    while (1)
    {
        if (Serial3_Available())
        {
            uint8_t ch = Serial3_GetByte();
            Serial3_SendByte(ch);
            if (ch == '\r' || ch == '\n')
            {
                if (cmd_idx == 0) continue;
                cmd_buf[cmd_idx] = '\0';

                if (strcmp(cmd_buf, "exit") == 0 || strcmp(cmd_buf, "EXIT") == 0) {
                    Serial3_SendString("\r\nExiting...\r\n"); return;
                }
                else if (strcmp(cmd_buf, "get") == 0 || strcmp(cmd_buf, "GET") == 0) {
                    ShowConfig();
                }
                else if (strncmp(cmd_buf, "set:", 4) == 0 || strncmp(cmd_buf, "SET:", 4) == 0) {
                    SetConfig(cmd_buf);
                }
                else { Serial3_SendString("ERR: unknown command\r\n"); }
                cmd_idx = 0; cmd_buf[0] = '\0';
            }
            else {
                if (cmd_idx < sizeof(cmd_buf) - 1) cmd_buf[cmd_idx++] = ch;
            }
        }
    }
}

/**
  * @brief  开机配置窗口
  * @param  ms  等待时间（毫秒）
  * @note   收到任意字节就把该字节丢掉并进入 StartLink，避免把噪声当成命令
  */
void StartLink_BootWindow(uint16_t ms)
{
    Serial3_Printf("Cfg? %u ms\r\n", (unsigned int)ms);

    for (uint16_t i = 0; i < ms / 10; i++)
    {
        if (Serial3_Available())
        {
            (void)Serial3_GetByte();
            StartLink();
            return;
        }
        Delay_ms(10);
    }
}
