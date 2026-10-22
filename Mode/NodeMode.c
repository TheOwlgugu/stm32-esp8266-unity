#include "NodeMode.h"
#include "Config.h"
#include "Serial3.h"

NodeMode_t g_nodeMode;

static const char *const data_names[]  = { "OFF", "LIGHT", "BUZZER", "DS18B20" };
static const char *const i2c_names[]   = { "OFF", "AHT21", "SCD40" };
static const char *const uart2_names[] = { "OFF", "PMS7003" };
static const char *const net_names[]   = { "WiFi", "LoRa" };

/**
  * @brief  把 ASCII 数字转成数值
  * @retval 0~9，非数字返回 0xFF
  */
static uint8_t Digit(char c)
{
    if (c < '0' || c > '9') return 0xFF;
    return (uint8_t)(c - '0');
}

/**
  * @brief  解析 4 位模式字符串
  * @param  raw  例如 "3121"
  * @retval 1=成功，0=失败（g_nodeMode 保持不变）
  */
uint8_t NodeMode_Apply(const char *raw)
{
    if (raw == 0) return 0;

    uint8_t d0 = Digit(raw[0]);
    uint8_t d1 = Digit(raw[1]);
    uint8_t d2 = Digit(raw[2]);
    uint8_t d3 = Digit(raw[3]);

    /* 位数范围与 StartLink 的校验保持一致 */
    if (d0 > 3 || d1 > 2 || d2 > 1 || d3 > 1) return 0;

    g_nodeMode.data   = (DataPort_t)d0;
    g_nodeMode.i2c    = (I2cPort_t)d1;
    g_nodeMode.uart2  = (Uart2Port_t)d2;
    g_nodeMode.net    = (NetType_t)d3;
    g_nodeMode.raw[0] = raw[0];
    g_nodeMode.raw[1] = raw[1];
    g_nodeMode.raw[2] = raw[2];
    g_nodeMode.raw[3] = raw[3];
    g_nodeMode.raw[4] = '\0';
    g_nodeMode.valid  = 1;

    return 1;
}

/**
  * @brief  用 config.mode 刷新运行期模式
  * @note   配置损坏时不能带着随机模式运行：全部关掉，网络默认 WiFi
  */
void NodeMode_LoadFromConfig(void)
{
    if (NodeMode_Apply(config.mode)) return;

    g_nodeMode.data   = DATA_PORT_OFF;
    g_nodeMode.i2c    = I2C_PORT_OFF;
    g_nodeMode.uart2  = UART2_PORT_OFF;
    g_nodeMode.net    = NET_TYPE_WIFI;
    g_nodeMode.raw[0] = '0';
    g_nodeMode.raw[1] = '0';
    g_nodeMode.raw[2] = '0';
    g_nodeMode.raw[3] = '0';
    g_nodeMode.raw[4] = '\0';
    g_nodeMode.valid  = 0;

    Serial3_SendString("ERR: bad mode, use 0000\r\n");
}

/**
  * @brief  DATA/I2C/UART2 是否至少有一个设备开启
  */
uint8_t NodeMode_HasAnySensor(void)
{
    return (g_nodeMode.data  != DATA_PORT_OFF) ||
           (g_nodeMode.i2c   != I2C_PORT_OFF)  ||
           (g_nodeMode.uart2 != UART2_PORT_OFF);
}

const char *NodeMode_DataName(DataPort_t port)
{
    if ((uint8_t)port > (uint8_t)DATA_PORT_DS18B20) return "?";
    return data_names[(uint8_t)port];
}

const char *NodeMode_I2cName(I2cPort_t port)
{
    if ((uint8_t)port > (uint8_t)I2C_PORT_SCD40) return "?";
    return i2c_names[(uint8_t)port];
}

const char *NodeMode_Uart2Name(Uart2Port_t port)
{
    if ((uint8_t)port > (uint8_t)UART2_PORT_PMS7003) return "?";
    return uart2_names[(uint8_t)port];
}

const char *NodeMode_NetName(NetType_t net)
{
    if ((uint8_t)net > (uint8_t)NET_TYPE_LORA) return "?";
    return net_names[(uint8_t)net];
}

/**
  * @brief  打印当前模式
  * @note   每行不超过 16 个字符，OLED 4 行刚好放得下
  */
void NodeMode_Print(void)
{
    Serial3_Printf("Mode %s %s%s\r\n", g_nodeMode.raw, NodeMode_NetName(g_nodeMode.net), g_nodeMode.valid ? "" : "!");
    Serial3_Printf("D:%s\r\n", NodeMode_DataName(g_nodeMode.data));
    Serial3_Printf("I:%s\r\n", NodeMode_I2cName(g_nodeMode.i2c));
    Serial3_Printf("U:%s\r\n", NodeMode_Uart2Name(g_nodeMode.uart2));
}
