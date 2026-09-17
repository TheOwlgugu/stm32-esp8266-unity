#include "stm32f10x.h"
#include "Config.h"
#include "Serial3.h"
#include "OLED.h"
#include <string.h>
#include <stdlib.h>

/* 声明外部函数 */
extern void Config_Save(void);

/**
  * @brief  解析一条 set 指令并修改 Flash 中的配置
  * @param  cmd  以 '\0' 结尾的字符串，格式："set:xxx=yyy"
  * @retval 1 - 修改成功；0 - 格式错误或字段不匹配
  */
uint8_t SetConfig(char *cmd)
{
    /* 1. 检查前缀是否为 "set:" */
    if (strncmp(cmd, "set:", 4) != 0 && strncmp(cmd, "SET:", 4) != 0)
    {
        Serial3_SendString("ERR: not a set command\r\n");
        return 0;
    }

    /* 2. 找到等号位置 */
    char *eq = strchr(cmd, '=');
    if (eq == NULL)
    {
        Serial3_SendString("ERR: missing '='\r\n");
        return 0;
    }

    /* 3. 提取字段名和值 */
    char *field = cmd + 4;              // 跳过 "set:"
    uint8_t field_len = eq - field;     // 字段名长度

    char *value = eq + 1;               // 等号后面的值

    /* 4. 去掉值末尾的换行/回车 */
    uint8_t val_len = strlen(value);
    while (val_len > 0 && (value[val_len - 1] == '\r' || value[val_len - 1] == '\n'))
    {
        value[--val_len] = '\0';
    }

    /* 5. 字段名匹配（支持简称、全称、带下划线） */
    #define FIELD_MATCH(name)  (field_len == strlen(name) && \
                                strncmp(field, name, field_len) == 0)

    #define FIELD_MATCH_AFTER_UNDERSCORE(name) \
        (field_len == strlen(name) + 1 && field[0] == '_' && \
         strncmp(field + 1, name, strlen(name)) == 0)

    uint8_t matched = 0;

    /* ---- device_id ---- */
    if (FIELD_MATCH("id") || FIELD_MATCH("ID") ||
        FIELD_MATCH_AFTER_UNDERSCORE("id") ||
        FIELD_MATCH("device_id"))
    {
        strncpy(config.device_id, value, sizeof(config.device_id) - 1);
        config.device_id[sizeof(config.device_id) - 1] = '\0';
        matched = 1;
    }
    /* ---- wifi_ssid ---- */
    else if (FIELD_MATCH("wifi") || FIELD_MATCH("WIFI") ||
             FIELD_MATCH_AFTER_UNDERSCORE("wifi") ||
             FIELD_MATCH("wifi_ssid"))
    {
        strncpy(config.wifi_ssid, value, sizeof(config.wifi_ssid) - 1);
        config.wifi_ssid[sizeof(config.wifi_ssid) - 1] = '\0';
        matched = 1;
    }
    /* ---- wifi_pwd ---- */
    else if (FIELD_MATCH("pwd") || FIELD_MATCH("PWD") ||
             FIELD_MATCH_AFTER_UNDERSCORE("pwd") ||
             FIELD_MATCH("wifi_pwd"))
    {
        strncpy(config.wifi_pwd, value, sizeof(config.wifi_pwd) - 1);
        config.wifi_pwd[sizeof(config.wifi_pwd) - 1] = '\0';
        matched = 1;
    }
    /* ---- server_ip ---- */
    else if (FIELD_MATCH("ip") || FIELD_MATCH("IP") ||
             FIELD_MATCH_AFTER_UNDERSCORE("ip") ||
             FIELD_MATCH("server_ip"))
    {
        strncpy(config.server_ip, value, sizeof(config.server_ip) - 1);
        config.server_ip[sizeof(config.server_ip) - 1] = '\0';
        matched = 1;
    }
    /* ---- server_port ---- */
    else if (FIELD_MATCH("port") || FIELD_MATCH("PORT") ||
             FIELD_MATCH_AFTER_UNDERSCORE("port") ||
             FIELD_MATCH("server_port"))
    {
        config.server_port = (uint16_t)atoi(value);
        matched = 1;
    }
    /* ---- interval_ms ---- */
    else if (FIELD_MATCH("interval") || FIELD_MATCH("INTERVAL") ||
             FIELD_MATCH_AFTER_UNDERSCORE("interval") ||
             FIELD_MATCH("interval_ms"))
    {
        config.interval_ms = (uint32_t)atol(value);
        matched = 1;
    }

    /* 6. 处理结果 */
    if (matched)
    {
        Config_Save();
        Serial3_Printf("OK: %.*s = %s\r\n", field_len, field, value);
        return 1;
    }
    else
    {
        Serial3_SendString("ERR: unknown field\r\n");
        return 0;
    }
}

/**
  * @brief  配置模式主循环：接收指令修改配置，收到 "exit" 退出
  * @param  无
  * @retval 无
  * @note   该函数会阻塞运行，直到收到 "exit"
  */
void ChoseMod(void)
{
    char cmd_buf[128];
    uint8_t cmd_idx = 0;

    Serial3_SendString("\r\n=== Config Mode ===\r\n");
    Serial3_SendString("Send 'set:xxx=yyy', type 'get' to view, 'exit' to quit.\r\n");

    while (1)
    {
        if (Serial3_GetRxFlag())
        {
            uint8_t ch = Serial3_GetRxData();

            /* 回显 */
            Serial3_SendByte(ch);

            /* 统一识别 \r 或 \n 作为行结束 */
            if (ch == '\r' || ch == '\n')
            {
                /* 连续的空行直接忽略 */
                if (cmd_idx == 0)
                    continue;

                cmd_buf[cmd_idx] = '\0';

                /* ---- 退出 ---- */
                if (strcmp(cmd_buf, "exit") == 0 || strcmp(cmd_buf, "EXIT") == 0)
                {
                    Serial3_SendString("\r\nExiting config mode.\r\n");
					OLED_Clear();
                    break;
                }
                /* ---- 查看配置 ---- */
                else if (strcmp(cmd_buf, "get") == 0 || strcmp(cmd_buf, "GET") == 0)
                {
                    Serial3_Printf("id=%s\r\n",       config.device_id);
                    Serial3_Printf("wifi=%s\r\n",     config.wifi_ssid);
                    Serial3_Printf("pwd=%s\r\n",      config.wifi_pwd);
                    Serial3_Printf("ip=%s\r\n",       config.server_ip);
                    Serial3_Printf("port=%d\r\n",     config.server_port);
                    Serial3_Printf("interval=%d\r\n", config.interval_ms);
					OLED_ShowString(1, 1, config.device_id);
					OLED_ShowNum(1, 6, config.server_port, 4);	
					OLED_ShowString(2, 1, config.wifi_ssid);
					OLED_ShowString(3, 1, config.wifi_pwd);
					OLED_ShowString(4, 1, config.server_ip);
                }
                /* ---- 设置配置 ---- */
                else if (strncmp(cmd_buf, "set:", 4) == 0 ||
                         strncmp(cmd_buf, "SET:", 4) == 0)
                {
                    SetConfig(cmd_buf);
                }
                /* ---- 未知指令 ---- */
                else
                {
                    Serial3_SendString("ERR: unknown command\r\n");
                }

                /* 清空缓冲区，准备下一条 */
                cmd_idx = 0;
                cmd_buf[0] = '\0';
            }
            else
            {
                /* 普通字符存入缓冲区，防止溢出 */
                if (cmd_idx < sizeof(cmd_buf) - 1)
                {
                    cmd_buf[cmd_idx++] = ch;
                }
            }
        }
    }
}
