#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f10x.h"

#define CONFIG_FLASH_ADDR   0x000000
#define CONFIG_MAGIC        0x5A5A5A5A

#pragma pack(1)
typedef struct {
    uint32_t magic;
    char     device_id[4];       // 设备编号，3位数字 + '\0'
    
    // ★ 新架构模式字符串（4位 + '\0'）
    // 位0(DATA): 0关 1光敏 2蜂鸣器 3DS18B20
    // 位1(I2C):  0关 1AHT21 2SCD40
    // 位2(UART2):0关 1PMS7003
    // 位3(NET):  0 WiFi 1 LoRa
    char     mode[5];            
    
    uint32_t send_interval_ms;   // ★ 统一发信间隔（毫秒），推荐 >= 6000

    /* WiFi 配置 */
    char     wifi_ssid[32];
    char     wifi_pwd[64];
    char     wifi_ip[16];
    uint16_t wifi_port;

    /* LoRa 配置 */
    char     lora_deveui[17];
    char     lora_appeui[17];
    char     lora_appkey[33];
    uint8_t  lora_class;
    uint8_t  lora_provisioned;

    uint8_t  crc;
} Config_t;
#pragma pack()

extern Config_t config;

void Config_Init(void);
void Config_Save(void);
void Config_SetDefault(void);

#endif
