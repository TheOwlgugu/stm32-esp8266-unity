#include "Config.h"
#include "W25Q64.h"
#include <string.h>

/* 全局配置变量 */
Config_t config;

/**
  * @brief   CRC8 校验（多项式 0x07）
  * @note    用于校验配置数据完整性，防止 Flash 损坏
  */
static uint8_t CRC8(const uint8_t *data, uint32_t len)
{
    uint8_t crc = 0x00;
    while (len--)
    {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/**
  * @brief   设置默认配置（只填内存，不写 Flash）
  * @note    第一次上电或配置损坏时调用
  */
void Config_SetDefault(void)
{
    memset(&config, 0, sizeof(Config_t));
    config.magic = CONFIG_MAGIC;

    /* --- 通用默认值 --- */
    strncpy(config.device_id, "001", sizeof(config.device_id) - 1);
    strncpy(config.mode,      "000", sizeof(config.mode) - 1);  // 光照 + LoRa
    config.interval_ms = 10000;   // LoRa 占空比限制 ≥6 秒，默认 10 秒

    /* --- WiFi 默认值 --- */
    strncpy(config.wifi_ssid, "main",   sizeof(config.wifi_ssid) - 1);
    strncpy(config.wifi_pwd,  "admin1234",    sizeof(config.wifi_pwd)  - 1);
    strncpy(config.wifi_ip,   "169.254.169.110", sizeof(config.wifi_ip)   - 1);
    config.wifi_port = 4000;

/* ---------- LoRa 配置 ---------- */
    strncpy(config.lora_deveui, "1d4517b623310c86", sizeof(config.lora_deveui) - 1);  // ★
    strncpy(config.lora_appeui, "248df2c2303ac457", sizeof(config.lora_appeui) - 1);  // ★
    strncpy(config.lora_appkey, "8a5b1303a0ccc45ac2b9aca1263d2c27", sizeof(config.lora_appkey) - 1);  // ★
    config.lora_class = 0;         // 默认 Class A
    config.lora_provisioned = 0;   // 未配置过
}

/**
  * @brief   保存配置到 W25Q64
  * @note    先算 CRC，再擦除扇区，最后写入
  */
void Config_Save(void)
{
    /* 1. 计算 CRC（不包含 crc 字段本身） */
    config.crc = CRC8((uint8_t*)&config, sizeof(Config_t) - 1);

    /* 2. 擦除扇区（W25Q64 写入前必须先擦除） */
    W25Q64_SectorErase(CONFIG_FLASH_ADDR);

    /* 3. 写入配置（200字节 < 页大小256，一次写完） */
    W25Q64_PageProgram(CONFIG_FLASH_ADDR, (uint8_t*)&config, sizeof(Config_t));
}

/**
  * @brief   初始化配置：读 Flash，校验，无效则写默认值
  * @note    在 main 里调用一次
  */
void Config_Init(void)
{
    /* 1. 从 W25Q64 读取配置到内存 */
    W25Q64_ReadData(CONFIG_FLASH_ADDR, (uint8_t*)&config, sizeof(Config_t));

    /* 2. 校验魔数和 CRC */
    uint8_t crc_calc = CRC8((uint8_t*)&config, sizeof(Config_t) - 1);
    if (config.magic != CONFIG_MAGIC || config.crc != crc_calc)
    {
        /* 3. 校验失败（首次上电或数据损坏），写默认值 */
        Config_SetDefault();
        Config_Save();
    }
}
