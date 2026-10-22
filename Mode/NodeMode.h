#ifndef __NODEMODE_H
#define __NODEMODE_H

#include "stm32f10x.h"

/* DATA 口设备（config.mode 第 0 位） */
typedef enum
{
    DATA_PORT_OFF = 0,
    DATA_PORT_LIGHT,
    DATA_PORT_BUZZER,
    DATA_PORT_DS18B20
} DataPort_t;

/* I2C 口设备（第 1 位） */
typedef enum
{
    I2C_PORT_OFF = 0,
    I2C_PORT_AHT21,
    I2C_PORT_SCD40
} I2cPort_t;

/* USART2 口设备（第 2 位） */
typedef enum
{
    UART2_PORT_OFF = 0,
    UART2_PORT_PMS7003
} Uart2Port_t;

/* 网络类型（第 3 位） */
typedef enum
{
    NET_TYPE_WIFI = 0,
    NET_TYPE_LORA
} NetType_t;

/* 运行期模式：由 config.mode 解析得到，不写回 Flash */
typedef struct
{
    DataPort_t  data;
    I2cPort_t   i2c;
    Uart2Port_t uart2;
    NetType_t   net;
    char        raw[5];      /* 原始 4 位字符串，例如 "3121" */
    uint8_t     valid;       /* 1 = 解析成功，0 = 使用了安全默认值 */
} NodeMode_t;

extern NodeMode_t g_nodeMode;

/* 解析字符串：成功返回 1；失败返回 0，且不修改 g_nodeMode */
uint8_t NodeMode_Apply(const char *raw);

/* 用 config.mode 刷新 g_nodeMode；失败时退回全关 + WiFi 并报错 */
void NodeMode_LoadFromConfig(void);

/* DATA/I2C/UART2 是否至少有一个设备开启 */
uint8_t NodeMode_HasAnySensor(void);

/* 枚举转名字，越界返回 "?" */
const char *NodeMode_DataName(DataPort_t port);
const char *NodeMode_I2cName(I2cPort_t port);
const char *NodeMode_Uart2Name(Uart2Port_t port);
const char *NodeMode_NetName(NetType_t net);

/* 通过 USART3 打印（会镜像到 OLED，4 行 × 16 列） */
void NodeMode_Print(void);

#endif
