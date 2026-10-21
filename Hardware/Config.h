#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f10x.h"

/* W25Q64 中存放配置的起始地址 */
#define CONFIG_FLASH_ADDR   0x000000

/* 魔数，用于判断是否第一次使用 */
#define CONFIG_MAGIC        0x5A5A5A5A

/* 配置结构体（pack(1) 避免对齐填充） */
#pragma pack(1)
typedef struct {
    /* ========== 通用配置 ========== */
    uint32_t magic;              // 魔数，固定 CONFIG_MAGIC
    char     device_id[4];       // 设备编号，3位数字 + '\0'
    char     mode[4];            // 模式，3位字符 + '\0'
                                 //   前2位：数据类型（00~04）
                                 //   第3位：网络类型（0=WiFi，1=LoRa）
    uint32_t interval_ms;        // 发送间隔（毫秒）

    /* ========== WiFi 配置 ========== */
    char     wifi_ssid[32];      // WiFi 名称
    char     wifi_pwd[64];       // WiFi 密码
    char     wifi_ip[16];        // 服务器 IP
    uint16_t wifi_port;          // 服务器端口

    /* ========== LoRa 配置 ========== */
    char     lora_deveui[17];    // DevEUI，16位hex + '\0'
    char     lora_appeui[17];    // AppEUI，16位hex + '\0'
    char     lora_appkey[33];    // AppKey，32位hex + '\0'
    uint8_t  lora_class;         // Class（0=A, 1=B, 2=C）
    uint8_t  lora_provisioned;   // 是否已 Provision 过（0=否，1=是）

    /* ========== 校验 ========== */
    uint8_t  crc;                // CRC8 校验值
} Config_t;
#pragma pack()

/* 全局配置变量，其他文件通过 config.xxx 访问 */
extern Config_t config;

/* 函数声明 */
void Config_Init(void);        // 上电：读配置，无效则写默认值
void Config_Save(void);        // 保存配置到 W25Q64
void Config_SetDefault(void);  // 填充默认配置（不写 Flash）

#endif
