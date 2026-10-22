#include "SoftI2C.h"
#include "Delay.h"

/* 引脚：PB8 = SCL，PB9 = SDA，开漏输出，依赖总线上的上拉电阻 */
#define I2C_GPIO          GPIOB
#define I2C_SCL_PIN       GPIO_Pin_8
#define I2C_SDA_PIN       GPIO_Pin_9

/* 等应答的最大循环次数，防止从机不在线时死等 */
#define I2C_ACK_TIMEOUT   250

/* 每个半周期的延时（微秒），2us 约 100~150kHz，兼容 SSD1306/SH1106 */
#define SOFTI2C_DELAY_US  2

#define SCL_H()           GPIO_SetBits(I2C_GPIO, I2C_SCL_PIN)
#define SCL_L()           GPIO_ResetBits(I2C_GPIO, I2C_SCL_PIN)
#define SDA_H()           GPIO_SetBits(I2C_GPIO, I2C_SDA_PIN)
#define SDA_L()           GPIO_ResetBits(I2C_GPIO, I2C_SDA_PIN)
#define SDA_READ()        GPIO_ReadInputDataBit(I2C_GPIO, I2C_SDA_PIN)

/**
  * @brief  软件 I2C 初始化
  * @note   时序与原来的 OLED 驱动保持一致，不额外加延时
  */
void SoftI2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_Init(I2C_GPIO, &GPIO_InitStructure);

    SCL_H();
    SDA_H();
}

/**
  * @brief  起始条件：SCL 高电平期间 SDA 由高变低
  */
void SoftI2C_Start(void)
{
    SDA_H(); Delay_us(SOFTI2C_DELAY_US);
    SCL_H(); Delay_us(SOFTI2C_DELAY_US);
    SDA_L(); Delay_us(SOFTI2C_DELAY_US);
    SCL_L(); Delay_us(SOFTI2C_DELAY_US);
}

/**
  * @brief  停止条件：SCL 高电平期间 SDA 由低变高
  */
void SoftI2C_Stop(void)
{
    SDA_L(); Delay_us(SOFTI2C_DELAY_US);
    SCL_H(); Delay_us(SOFTI2C_DELAY_US);
    SDA_H(); Delay_us(SOFTI2C_DELAY_US);
}

/**
  * @brief  发送一个字节并读取从机应答
  * @retval 1=ACK，0=NACK 或超时
  */
uint8_t SoftI2C_SendByte(uint8_t byte)
{
    uint8_t i;
    uint16_t timeout;
    uint8_t ack;

    for (i = 0; i < 8; i++)
    {
        if (byte & 0x80) SDA_H();
        else             SDA_L();
        byte <<= 1;
        Delay_us(SOFTI2C_DELAY_US);
        SCL_H();
        Delay_us(SOFTI2C_DELAY_US);
        SCL_L();
    }

    /* 第 9 个时钟：释放 SDA，由从机拉低表示应答 */
    SDA_H();
    Delay_us(SOFTI2C_DELAY_US);
    SCL_H();
    Delay_us(SOFTI2C_DELAY_US);
    timeout = I2C_ACK_TIMEOUT;
    while (SDA_READ() && --timeout);
    ack = (SDA_READ() == 0) ? 1 : 0;
    SCL_L();
    Delay_us(SOFTI2C_DELAY_US);

    return ack;
}

/**
  * @brief  接收一个字节
  * @param  ack  1=发送 ACK（还要继续读），0=发送 NACK（最后一个字节）
  */
uint8_t SoftI2C_ReadByte(uint8_t ack)
{
    uint8_t i;
    uint8_t value = 0;

    SDA_H();                    /* 主机释放 SDA，由从机驱动 */
    Delay_us(SOFTI2C_DELAY_US);
    for (i = 0; i < 8; i++)
    {
        SCL_H();
        Delay_us(SOFTI2C_DELAY_US);
        value <<= 1;
        if (SDA_READ()) value |= 0x01;
        SCL_L();
        Delay_us(SOFTI2C_DELAY_US);
    }

    /* 第 9 个时钟：主机回 ACK/NACK */
    if (ack) SDA_L();
    else     SDA_H();
    Delay_us(SOFTI2C_DELAY_US);
    SCL_H();
    Delay_us(SOFTI2C_DELAY_US);
    SCL_L();
    Delay_us(SOFTI2C_DELAY_US);
    SDA_H();
    Delay_us(SOFTI2C_DELAY_US);

    return value;
}

/**
  * @brief  向从机写一串数据
  * @note   某个字节没有应答时仍把时序走完（与旧 OLED 驱动兼容），
  *         但返回值会告诉调用者是否全部应答
  */
uint8_t SoftI2C_Write(uint8_t dev, const uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint8_t ok = 1;

    SoftI2C_Start();
    ok &= SoftI2C_SendByte((uint8_t)(dev << 1));
    for (i = 0; i < len; i++)
    {
        ok &= SoftI2C_SendByte(buf[i]);
    }
    SoftI2C_Stop();

    return ok;
}

/**
  * @brief  从从机读一串数据
  */
uint8_t SoftI2C_Read(uint8_t dev, uint8_t *buf, uint16_t len)
{
    uint16_t i;

    if (len == 0) return 0;

    SoftI2C_Start();
    if (!SoftI2C_SendByte((uint8_t)((dev << 1) | 0x01)))
    {
        SoftI2C_Stop();
        return 0;
    }

    for (i = 0; i < len; i++)
    {
        buf[i] = SoftI2C_ReadByte((i + 1 < len) ? 1 : 0);
    }

    SoftI2C_Stop();
    return 1;
}

/**
  * @brief  写一个 8 位寄存器
  */
uint8_t SoftI2C_WriteReg(uint8_t dev, uint8_t reg, uint8_t value)
{
    uint8_t buf[2];

    buf[0] = reg;
    buf[1] = value;

    return SoftI2C_Write(dev, buf, 2);
}

/**
  * @brief  读一个 8 位寄存器（写寄存器地址后用重复起始读回）
  */
uint8_t SoftI2C_ReadReg(uint8_t dev, uint8_t reg, uint8_t *value)
{
    SoftI2C_Start();
    if (!SoftI2C_SendByte((uint8_t)(dev << 1)))
    {
        SoftI2C_Stop();
        return 0;
    }
    if (!SoftI2C_SendByte(reg))
    {
        SoftI2C_Stop();
        return 0;
    }

    SoftI2C_Start();            /* 重复起始 */
    if (!SoftI2C_SendByte((uint8_t)((dev << 1) | 0x01)))
    {
        SoftI2C_Stop();
        return 0;
    }

    *value = SoftI2C_ReadByte(0);
    SoftI2C_Stop();

    return 1;
}

/**
  * @brief  探测设备是否存在
  * @retval 1=有应答，0=无应答
  */
uint8_t SoftI2C_IsReady(uint8_t dev)
{
    uint8_t ack;

    SoftI2C_Start();
    ack = SoftI2C_SendByte((uint8_t)(dev << 1));
    SoftI2C_Stop();

    return ack;
}

/**
  * @brief  慢速探测一个 7 位地址（约 100kHz），用于扫描总线
  * @param  dev  7 位从机地址
  * @retval 1=有应答，0=无应答
  * @note   和正常读写分开，加了延时，避免因为总线太快而误判
  */
uint8_t SoftI2C_Probe(uint8_t dev)
{
    uint8_t i;
    uint8_t ack;
    uint8_t addr = (uint8_t)(dev << 1);

    /* 起始条件（带延时） */
    SDA_H(); Delay_us(2);
    SCL_H(); Delay_us(2);
    SDA_L(); Delay_us(2);
    SCL_L(); Delay_us(2);

    for (i = 0; i < 8; i++)
    {
        if (addr & 0x80) SDA_H();
        else             SDA_L();
        addr <<= 1;
        Delay_us(2);
        SCL_H(); Delay_us(2);
        SCL_L(); Delay_us(2);
    }

    /* 第 9 个时钟读应答 */
    SDA_H(); Delay_us(2);
    SCL_H(); Delay_us(2);
    ack = (SDA_READ() == 0) ? 1 : 0;
    SCL_L(); Delay_us(2);

    /* 停止条件（带延时） */
    SDA_L(); Delay_us(2);
    SCL_H(); Delay_us(2);
    SDA_H(); Delay_us(2);

    return ack;
}
