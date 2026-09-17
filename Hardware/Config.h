#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f10x.h"

/* W25Q64 中存放配置的起始地址（0x000000 开始） */
#define CONFIG_FLASH_ADDR   0x000000

/* 魔数，用于判断是否第一次使用 */
#define CONFIG_MAGIC        0x5A5A5A5A

/* 配置结构体（使用 pack(1) 避免字节对齐填充） */
#pragma pack(1)
typedef struct {
    uint32_t magic;            // 魔数，固定为 CONFIG_MAGIC
    char     device_id[16];    // 编号
	  char     wifi_ssid[32];    // WiFi 名称
    char     wifi_pwd[64];     // WiFi 密码
    char     server_ip[16];    // 服务器 IP
    uint16_t server_port;      // 服务器端口
    uint32_t interval_ms;      // 发送间隔（毫秒）
    uint8_t  crc;              // CRC8 校验值（计算范围：除 crc 外的所有字节）
} Config_t;

#pragma pack()
/* 全局配置变量，其他文件可直接引用 */
extern Config_t config;

/* 函数声明 */
void Config_Init(void);        // 上电初始化：读取配置，无效则写入默认值
void Config_Save(void);        // 将当前配置写入 W25Q64
void Config_SetDefault(void);  // 设置默认配置（不写入）

#endif

