#include "StartLink.h"
#include "Config.h"
#include "Serial3.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ===================================================================
 * 辅助宏：字段名匹配
 * =================================================================== */

/* 精确匹配字段名：如 field_len == strlen("id") && strncmp(field, "id", len)==0 */
#define FIELD_MATCH(name)  (field_len == strlen(name) && \
                            strncmp(field, name, field_len) == 0)

/* 匹配带下划线的字段名：如 "_id" 匹配 "id" */
#define FIELD_MATCH_UNDERSCORE(name) \
    (field_len == strlen(name) + 1 && field[0] == '_' && \
     strncmp(field + 1, name, strlen(name)) == 0)

/* 组合判断：id / ID / _id / device_id 都能匹配 */
#define FIELD_IS(short1, short2, longname) \
    (FIELD_MATCH(short1) || FIELD_MATCH(short2) || \
     FIELD_MATCH_UNDERSCORE(short1) || FIELD_MATCH(longname))

/* ===================================================================
 * 保存并回复
 * =================================================================== */
static void SaveAndReply(const char *field, int field_len, const char *value)
{
    Config_Save();
    Serial3_Printf("OK: %.*s = %s\r\n", field_len, field, value);
}

/* ===================================================================
 * 解析并执行一条 set 指令
 * =================================================================== */
uint8_t SetConfig(char *cmd)
{
    /* 1. 检查前缀 "set:" 或 "SET:" */
    if (strncmp(cmd, "set:", 4) != 0 && strncmp(cmd, "SET:", 4) != 0)
    {
        Serial3_SendString("ERR: not a set command\r\n");
        return 0;
    }

    /* 2. 找等号 */
    char *eq = strchr(cmd, '=');
    if (eq == NULL)
    {
        Serial3_SendString("ERR: missing '='\r\n");
        return 0;
    }

    /* 3. 提取字段名和值 */
    char *field = cmd + 4;            // 跳过 "set:"
    uint8_t field_len = eq - field;   // 字段名长度
    char *value = eq + 1;             // 等号后面的值

    /* 4. 去掉值末尾的换行/回车 */
    uint8_t val_len = strlen(value);
    while (val_len > 0 &&
           (value[val_len - 1] == '\r' || value[val_len - 1] == '\n'))
    {
        value[--val_len] = '\0';
    }

    /* 5. 字段匹配 */
    uint8_t matched = 0;

    /* ---------- device_id（3位数字） ---------- */
    if (FIELD_IS("id", "ID", "device_id"))
    {
        if (strlen(value) != 3)
        {
            Serial3_SendString("ERR: id must be 3 digits\r\n");
            return 0;
        }
        strncpy(config.device_id, value, sizeof(config.device_id) - 1);
        config.device_id[sizeof(config.device_id) - 1] = '\0';
        matched = 1;
    }
    /* ---------- mode（3位数字：类型2位 + 网络1位） ---------- */
    else if (FIELD_IS("mode", "MODE", "mode"))
    {
        if (strlen(value) != 3)
        {
            Serial3_SendString("ERR: mode must be 3 digits (e.g. 001)\r\n");
            return 0;
        }
        /* 校验第3位必须是 0 或 1 */
        if (value[2] != '0' && value[2] != '1')
        {
            Serial3_SendString("ERR: mode[2] must be 0 (WiFi) or 1 (LoRa)\r\n");
            return 0;
        }
        strncpy(config.mode, value, sizeof(config.mode) - 1);
        config.mode[sizeof(config.mode) - 1] = '\0';
        matched = 1;
    }
    /* ---------- interval_ms ---------- */
    else if (FIELD_IS("interval", "INTERVAL", "interval_ms"))
    {
        config.interval_ms = (uint32_t)atol(value);
        matched = 1;
    }
    /* ---------- WiFi SSID ---------- */
    else if (FIELD_IS("wifi", "WIFI", "wifi_ssid"))
    {
        strncpy(config.wifi_ssid, value, sizeof(config.wifi_ssid) - 1);
        config.wifi_ssid[sizeof(config.wifi_ssid) - 1] = '\0';
        matched = 1;
    }
    /* ---------- WiFi 密码 ---------- */
    else if (FIELD_IS("pwd", "PWD", "wifi_pwd"))
    {
        strncpy(config.wifi_pwd, value, sizeof(config.wifi_pwd) - 1);
        config.wifi_pwd[sizeof(config.wifi_pwd) - 1] = '\0';
        matched = 1;
    }
    /* ---------- 服务器 IP ---------- */
    else if (FIELD_IS("ip", "IP", "wifi_ip"))
    {
        strncpy(config.wifi_ip, value, sizeof(config.wifi_ip) - 1);
        config.wifi_ip[sizeof(config.wifi_ip) - 1] = '\0';
        matched = 1;
    }
    /* ---------- 服务器端口 ---------- */
    else if (FIELD_IS("port", "PORT", "wifi_port"))
    {
        config.wifi_port = (uint16_t)atoi(value);
        matched = 1;
    }
    /* ---------- LoRa DevEUI（16位hex） ---------- */
    else if (FIELD_IS("deveui", "DEVEUI", "lora_deveui"))
    {
        if (strlen(value) != 16)
        {
            Serial3_SendString("ERR: deveui must be 16 hex chars\r\n");
            return 0;
        }
        strncpy(config.lora_deveui, value, sizeof(config.lora_deveui) - 1);
        config.lora_deveui[sizeof(config.lora_deveui) - 1] = '\0';
        matched = 1;
    }
    /* ---------- LoRa AppEUI（16位hex） ---------- */
    else if (FIELD_IS("appeui", "APPEUI", "lora_appeui"))
    {
        if (strlen(value) != 16)
        {
            Serial3_SendString("ERR: appeui must be 16 hex chars\r\n");
            return 0;
        }
        strncpy(config.lora_appeui, value, sizeof(config.lora_appeui) - 1);
        config.lora_appeui[sizeof(config.lora_appeui) - 1] = '\0';
        matched = 1;
    }
    /* ---------- LoRa AppKey（32位hex） ---------- */
    else if (FIELD_IS("appkey", "APPKEY", "lora_appkey"))
    {
        if (strlen(value) != 32)
        {
            Serial3_SendString("ERR: appkey must be 32 hex chars\r\n");
            return 0;
        }
        strncpy(config.lora_appkey, value, sizeof(config.lora_appkey) - 1);
        config.lora_appkey[sizeof(config.lora_appkey) - 1] = '\0';
        matched = 1;
    }
    /* ---------- LoRa Class（0/1/2） ---------- */
    else if (FIELD_IS("class", "CLASS", "lora_class"))
    {
        uint8_t c = (uint8_t)atoi(value);
        if (c > 2)
        {
            Serial3_SendString("ERR: class must be 0/1/2\r\n");
            return 0;
        }
        config.lora_class = c;
        matched = 1;
    }

    /* 6. 处理结果 */
    if (matched)
    {
        SaveAndReply(field, field_len, value);
        return 1;
    }
    else
    {
        Serial3_SendString("ERR: unknown field\r\n");
        return 0;
    }
}

/* ===================================================================
 * 显示当前配置（get 指令）
 * =================================================================== */
static void ShowConfig(void)
{
    Serial3_Printf("id=%s\r\n",       config.device_id);
    Serial3_Printf("mode=%s\r\n",     config.mode);
    Serial3_Printf("interval=%d\r\n", config.interval_ms);
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

/* ===================================================================
 * 配置模式主循环
 * =================================================================== */
void StartLink(void)
{
    char cmd_buf[128];
    uint8_t cmd_idx = 0;

    Serial3_SendString("\r\n=== Config Mode ===\r\n");
    Serial3_SendString("Commands:\r\n");
    Serial3_SendString("  set:id=001\r\n");
    Serial3_SendString("  set:mode=001    (00~04 + 0/1)\r\n");
    Serial3_SendString("  set:interval=10000\r\n");
    Serial3_SendString("  set:wifi=SSID\r\n");
    Serial3_SendString("  set:pwd=PASSWORD\r\n");
    Serial3_SendString("  set:ip=192.168.1.100\r\n");
    Serial3_SendString("  set:port=4000\r\n");
	Serial3_SendString("  set:deveui=16 hex chars\r\n");
	Serial3_SendString("  set:appeui=16 hex chars\r\n");
	Serial3_SendString("  set:appkey=32 hex chars\r\n");
    Serial3_SendString("  set:class=0/1/2\r\n");
    Serial3_SendString("  get   - show config\r\n");
    Serial3_SendString("  exit  - quit config mode\r\n");

    while (1)
    {
        if (Serial3_GetRxFlag())
        {
            uint8_t ch = Serial3_GetRxData();

            /* 回显 */
            Serial3_SendByte(ch);

            /* 行结束符 */
            if (ch == '\r' || ch == '\n')
            {
                if (cmd_idx == 0) continue;   // 空行忽略

                cmd_buf[cmd_idx] = '\0';

                /* --- exit --- */
                if (strcmp(cmd_buf, "exit") == 0 ||
                    strcmp(cmd_buf, "EXIT") == 0)
                {
                    Serial3_SendString("\r\nExiting config mode.\r\n");
                    return;
                }
                /* --- get --- */
                else if (strcmp(cmd_buf, "get") == 0 ||
                         strcmp(cmd_buf, "GET") == 0)
                {
                    ShowConfig();
                }
                /* --- set:... --- */
                else if (strncmp(cmd_buf, "set:", 4) == 0 ||
                         strncmp(cmd_buf, "SET:", 4) == 0)
                {
                    SetConfig(cmd_buf);
                }
                /* --- 未知指令 --- */
                else
                {
                    Serial3_SendString("ERR: unknown command\r\n");
                }

                cmd_idx = 0;
                cmd_buf[0] = '\0';
            }
            else
            {
                if (cmd_idx < sizeof(cmd_buf) - 1)
                    cmd_buf[cmd_idx++] = ch;
            }
        }
    }
}
