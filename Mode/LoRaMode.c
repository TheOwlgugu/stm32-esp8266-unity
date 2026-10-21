#include "LoRaMode.h"
#include "Serial.h"
#include "Serial3.h"
#include "Delay.h"
#include "OLED.h"
#include "Ra08_Driver.h"
#include "Config.h"
#include <string.h>
#include <stdio.h>

/**
  * @brief  10位ASCII字符串 → 10字节数组（用于 Ra08_SendData）
  * @param  str  输入的10位字符串
  * @param  out  输出的10字节数组
  */
static void StringToBytes(const char *str, uint8_t *out)
{
    for (uint8_t i = 0; i < 10; i++)
        out[i] = (uint8_t)str[i];
}

/**
  * @brief  LoRa 连接全流程
  * @note   包含：Init → Provision（首次）→ Join
  */
uint8_t LoRaConnect(void)
{
    Serial3_SendString("=== LoRa Connect ===\r\n");

    /* 1. 初始化 Ra-08（会切换串口波特率到 9600） */
    OLED_ShowString(1, 1, "Init:");
    if (!Ra08_Init())
    {
        OLED_ShowString(1, 6, "FAIL");
        Serial3_SendString("Ra08 init fail\r\n");
        return 0;
    }
    OLED_ShowString(1, 6, "OK  ");

    /* 2. 首次烧录时 Provision（写入 DevEUI/AppEUI/AppKey） */
    if (!config.lora_provisioned)
    {
        Serial3_SendString("Provisioning...\r\n");
        if (!Ra08_Provision(config.lora_deveui,
                            config.lora_appeui,
                            config.lora_appkey))
        {
            Serial3_SendString("Provision fail\r\n");
            return 0;
        }

        /* 设置 Class 和频段掩码 */
        Ra08_SendCmd("AT+CCLASS=0\r\n", "OK", 1000);
        Ra08_SendCmd("AT+CFREQBANDMASK=0001\r\n", "OK", 1000);
        Ra08_SendCmd("AT+CSAVE\r\n", "OK", 3000);   // 保存到 Flash

        /* 标记已配置 */
        config.lora_provisioned = 1;
        Config_Save();
        Serial3_SendString("Provision done\r\n");
    }

    /* 3. 入网 */
    OLED_ShowString(2, 1, "Join:");
    if (!Ra08_Join())
    {
        OLED_ShowString(2, 6, "FAIL");
        Serial3_SendString("Join fail\r\n");
        return 0;
    }
    OLED_ShowString(2, 6, "OK  ");
    Serial3_SendString("Join OK\r\n");

    return 1;
}

/**
  * @brief  LoRa 发送数据
  * @note   字符串 → 字节数组 → Ra08_SendData
  */
uint8_t LoRaSend(const char *msg)
{
    uint8_t payload[10];

    // 只要长度小于 10 就不合法；如果是 11（带 \n），也允许过
    if (strlen(msg) < 10)
    {
        Serial3_SendString("LoRa payload too short\r\n");
        return 0;
    }

    // StringToBytes 内部只取前 10 个字节，会自动忽略第 11 个字节 '\n'
    StringToBytes(msg, payload);

    Ra08_SendResult r = Ra08_SendData(payload, 10, 0);   // 0=非确认模式

    return (r == RA08_SEND_OK) ? 1 : 0;
}
