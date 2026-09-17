#include "Config.h"
#include "W25Q64.h"
#include <string.h>

/* 全局配置变量 */
Config_t config;

/**
  * @brief   CRC8 校验（多项式 0x07）
  * @param   data 数据指针
  * @param   len  数据长度
  * @retval  CRC8 值
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
  * @brief   设置默认配置（仅填充结构体，不写入 Flash）
  * @param   无
  * @retval  无
  */
void Config_SetDefault(void)
{
    memset(&config, 0, sizeof(Config_t));
    config.magic = CONFIG_MAGIC;
    strncpy(config.device_id,  "001",           sizeof(config.device_id) - 1);
    strncpy(config.wifi_ssid,  "DefaultWiFi",   sizeof(config.wifi_ssid) - 1);
    strncpy(config.wifi_pwd,   "DefaultPWD",    sizeof(config.wifi_pwd)  - 1);
    strncpy(config.server_ip,  "192.168.1.100", sizeof(config.server_ip) - 1);
    config.server_port = 8888;
    config.interval_ms = 5000;
    // CRC 在 Config_Save 中计算并填充
}

/**
  * @brief   保存配置到 W25Q64
  * @param   无
  * @retval  无
  * @note    写入前会擦除整个 4KB 扇区，配置数据远小于扇区大小
  */
void Config_Save(void)
{
    // 1. 计算 CRC（计算范围：从 magic 到 interval_ms，即除 crc 外的所有字节）
    config.crc = CRC8((uint8_t*)&config, sizeof(Config_t) - 1);

    // 2. 擦除扇区（W25Q64 写入前必须擦除）
    W25Q64_SectorErase(CONFIG_FLASH_ADDR);

    // 3. 写入配置
    W25Q64_PageProgram(CONFIG_FLASH_ADDR, (uint8_t*)&config, sizeof(Config_t));
}

/**
  * @brief   初始化配置：从 W25Q64 读取，校验失败则写入默认值
  * @param   无
  * @retval  无
  */
void Config_Init(void)
{
    // 1. 从 W25Q64 读取配置
    W25Q64_ReadData(CONFIG_FLASH_ADDR, (uint8_t*)&config, sizeof(Config_t));

    // 2. 校验魔数和 CRC
    uint8_t crc_calc = CRC8((uint8_t*)&config, sizeof(Config_t) - 1);
    if (config.magic != CONFIG_MAGIC || config.crc != crc_calc)
    {
        // 校验失败，使用默认配置并保存
        Config_SetDefault();
        Config_Save();
    }
}
