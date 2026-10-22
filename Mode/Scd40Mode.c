#include "Scd40Mode.h"
#include "SoftI2C.h"
#include "Delay.h"

/* SCD40 命令（16 位，高字节先发） */
#define SCD40_CMD_START_PERIODIC   0x21B1
#define SCD40_CMD_STOP_PERIODIC    0x3F86
#define SCD40_CMD_DATA_READY       0xE4B8
#define SCD40_CMD_READ_MEASUREMENT 0xEC05

/**
  * @brief  Sensirion CRC8（多项式 0x31，初值 0xFF）
  */
static uint8_t Scd40Mode_Crc(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    uint8_t i, b;

    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (b = 0; b < 8; b++)
        {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x31);
            else            crc = (uint8_t)(crc << 1);
        }
    }

    return crc;
}

/**
  * @brief  发送一个 2 字节命令（带停止条件）
  */
static uint8_t Scd40Mode_WriteCmd(uint16_t cmd)
{
    uint8_t buf[2];

    buf[0] = (uint8_t)(cmd >> 8);
    buf[1] = (uint8_t)(cmd & 0xFF);

    return SoftI2C_Write(SCD40_I2C_ADDR, buf, 2);
}

/**
  * @brief  发命令后读回若干 16 位字（每个字 2 字节 + 1 字节 CRC）
  * @param  words  要读的字数（1=状态，3=测量值）
  * @note   用重复起始，符合 Sensirion 的读时序
  */
static uint8_t Scd40Mode_ReadWords(uint16_t cmd, uint8_t *buf, uint8_t words)
{
    uint8_t i;
    uint8_t len = (uint8_t)(words * 3);

    SoftI2C_Start();
    if (!SoftI2C_SendByte((uint8_t)(SCD40_I2C_ADDR << 1))) { SoftI2C_Stop(); return 0; }
    if (!SoftI2C_SendByte((uint8_t)(cmd >> 8)))            { SoftI2C_Stop(); return 0; }
    if (!SoftI2C_SendByte((uint8_t)(cmd & 0xFF)))          { SoftI2C_Stop(); return 0; }

    Delay_ms(1);                 /* 命令执行时间 */

    SoftI2C_Start();             /* 重复起始 */
    if (!SoftI2C_SendByte((uint8_t)((SCD40_I2C_ADDR << 1) | 0x01)))
    {
        SoftI2C_Stop();
        return 0;
    }

    for (i = 0; i < len; i++)
    {
        buf[i] = SoftI2C_ReadByte((i + 1 < len) ? 1 : 0);
    }

    SoftI2C_Stop();

    for (i = 0; i < words; i++)
    {
        if (Scd40Mode_Crc(&buf[i * 3], 2) != buf[i * 3 + 2]) return 0;
    }

    return 1;
}

/**
  * @brief  初始化 SCD40 并启动周期测量
  * @note   调用前传感器需要已经上电至少 1 秒；周期测量每 5 秒出一次数据
  */
uint8_t Scd40Mode_Init(void)
{
    if (!SoftI2C_IsReady(SCD40_I2C_ADDR)) return 0;

    Scd40Mode_WriteCmd(SCD40_CMD_STOP_PERIODIC);
    Delay_ms(500);

    if (!Scd40Mode_WriteCmd(SCD40_CMD_START_PERIODIC)) return 0;
    Delay_ms(1);

    return 1;
}

/**
  * @brief  查询是否有新的测量数据
  */
uint8_t Scd40Mode_DataReady(void)
{
    uint8_t buf[3];
    uint16_t status;

    if (!Scd40Mode_ReadWords(SCD40_CMD_DATA_READY, buf, 1)) return 0;

    status = ((uint16_t)buf[0] << 8) | buf[1];
    return (status & 0x07FF) ? 1 : 0;
}

/**
  * @brief  读取一次测量结果
  */
uint8_t Scd40Mode_Read(uint16_t *co2, float *temp, float *humi)
{
    uint8_t buf[9];
    uint16_t t_raw, h_raw;

    if (!Scd40Mode_ReadWords(SCD40_CMD_READ_MEASUREMENT, buf, 3)) return 0;

    *co2  = ((uint16_t)buf[0] << 8) | buf[1];
    t_raw = ((uint16_t)buf[3] << 8) | buf[4];
    h_raw = ((uint16_t)buf[6] << 8) | buf[7];

    *temp = -45.0f + 175.0f * (float)t_raw / 65536.0f;
    *humi = 100.0f * (float)h_raw / 65536.0f;

    return 1;
}
