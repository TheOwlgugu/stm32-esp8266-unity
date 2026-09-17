/**
  * @file    ESP8266_Driver.c
  * @brief   ESP8266 Wi-Fi 模块驱动（基于串口AT指令）
  * @note    依赖 Serial.c 提供的串口发送/接收功能，以及 Delay.c 提供的延时函数
  */

#include "ESP8266_Driver.h"   // 驱动头文件，包含函数声明
#include "Serial.h"           // 串口驱动，用于发送AT指令和接收回复
#include "Delay.h"            // 延时函数（毫秒级）
#include <string.h>           // 字符串操作（strlen, strstr, memcpy等）
#include <stdlib.h>           // 标准库（sprintf等）

/* 接收缓冲区大小（字节） */
#define RX_BUF_SIZE  256

/* 静态接收缓冲区，用于存储从ESP8266返回的字符串 */
static char rx_buf[RX_BUF_SIZE];
/* 当前已接收字节的索引（写入位置） */
static uint16_t rx_index = 0;

/**
  * @brief   清空接收缓冲区，重置索引
  * @param   无
  * @retval  无
  */
static void ClearRxBuffer(void)
{
    // 将缓冲区全部置零
    memset(rx_buf, 0, RX_BUF_SIZE);
    // 索引归零，表示从头部开始写入
    rx_index = 0;
}

/**
  * @brief   等待ESP8266返回指定的字符串（或超时）
  * @param   expected   期望收到的子字符串（如"OK"），若为NULL则不检查，仅清空缓冲区
  * @param   timeout_ms 超时时间（毫秒）
  * @retval  1 - 在超时前收到了期望的字符串；0 - 超时未收到
  * @note    此函数会阻塞运行，直到收到目标字符串或超时
  *          串口接收中断会将收到的字节存入 rx_buf，并置位 Serial_RxFlag
  */
static uint8_t WaitForResponse(const char *expected, uint32_t timeout_ms)
{
    uint32_t cnt = 0;
    ClearRxBuffer();
    while (cnt < timeout_ms)
    {
        while (Serial_Available())      // 一口气把缓冲区里的数据全读完
        {
            uint8_t byte = Serial_GetByte();
            if (rx_index < RX_BUF_SIZE - 1)
            {
                rx_buf[rx_index++] = byte;
                rx_buf[rx_index] = '\0';
            }
        }
        if (expected != NULL && strstr(rx_buf, expected) != NULL)
            return 1;
        Delay_ms(1);
        cnt++;
    }
    return 0;
}

/**
  * @brief   ESP8266 初始化（发送AT测试指令）
  * @param   无
  * @retval  无
  * @note    该函数会发送"AT\r\n"并等待"OK"回复，用于检测模块是否正常通信
  */
void ESP8266_Init(void)
{
    // 调用发送命令函数，等待"OK"回复，超时2秒
    ESP8266_SendCmd("AT\r\n", "OK", 2000);
}

/**
  * @brief   发送AT指令并等待指定回复
  * @param   cmd               要发送的指令字符串（需以"\r\n"结尾）
  * @param   expected_response 期望的回复子串（如"OK"），可为NULL
  * @param   timeout_ms        超时时间（毫秒）
  * @retval  1 - 收到期望回复；0 - 超时或回复内容不匹配
  */
uint8_t ESP8266_SendCmd(const char *cmd, const char *expected_response, uint32_t timeout_ms)
{
    ClearRxBuffer();                 // 清空缓冲区，确保本次接收不受之前干扰
    Serial_SendString((char*)cmd);   // 通过串口发送指令
    return WaitForResponse(expected_response, timeout_ms); // 等待并返回结果
}

/**
  * @brief   连接WiFi热点
  * @param   ssid  热点名称（SSID）
  * @param   pwd   密码
  * @retval  1 - 连接成功；0 - 连接失败
  * @note    首先设置WiFi模式为Station，然后发送连接指令，等待"WIFI GOT IP"
  */
uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *pwd)
{
    /* 1. 设为 Station 模式 */
    if (!ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK", 2000))
        return 0;

    /* 2. 断开旧连接 */
    ESP8266_SendCmd("AT+CWQAP\r\n", "OK", 2000);

    /* 3. 连接新 WiFi（只发一次） */
    char cmd[128];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);
    if (ESP8266_SendCmd(cmd, "WIFI CONNECTED", 15000))
        return 1;

    return 0;
}

/**
  * @brief   获取ESP8266的IP地址
  * @param   ip_buf  用于存放IP地址的字符数组（至少16字节）
  * @retval  1 - 成功获取并存入ip_buf；0 - 获取失败
  * @note    解析"AT+CIFSR"返回的"+CIFSR:STAIP,"192.168.x.x""
  */
uint8_t ESP8266_GetIP(char *ip_buf)
{
    ClearRxBuffer();
    Serial_SendString("AT+CIFSR\r\n");
    // 等待响应中包含"+CIFSR:STAIP,\""
    if (WaitForResponse("+CIFSR:STAIP,\"", 3000))
    {
        // 在rx_buf中查找 "+CIFSR:STAIP,\"" 子串
        char *start = strstr(rx_buf, "+CIFSR:STAIP,\"");
        if (start)
        {
            // 跳过前缀，指向IP地址的第一个字符
            start += strlen("+CIFSR:STAIP,\"");
            // 查找IP地址结束的双引号
            char *end = strchr(start, '\"');
            if (end)
            {
                int len = end - start;       // IP地址长度
                if (len < 16)                // 确保长度合理
                {
                    memcpy(ip_buf, start, len); // 拷贝IP字符串
                    ip_buf[len] = '\0';        // 添加结束符
                    return 1;
                }
            }
        }
    }
    return 0;
}

/**
  * @brief   建立TCP连接到指定服务器
  * @param   ip    服务器IP地址（字符串，如"192.168.1.100"）
  * @param   port  端口号
  * @retval  1 - 连接成功；0 - 连接失败
  * @note    发送"AT+CIPSTART="TCP","IP",PORT"，等待"CONNECT"
  */
uint8_t ESP8266_ConnectServer(const char *ip, uint16_t port)
{
    char cmd[64];
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", ip, port);
    // 等待"CONNECT"，超时10秒
    return ESP8266_SendCmd(cmd, "CONNECT", 10000);
}

/**
  * @brief   在标准模式下发送数据（非透传）
  * @param   data   要发送的数据字符串（不含"\r\n"）
  * @retval  1 - 发送成功；0 - 发送失败
  * @note    首先发送"AT+CIPSEND=长度"，等待">"提示符，
  *          然后发送数据，等待"OK"确认
  */
uint8_t ESP8266_SendData(const char *data)
{
    uint16_t len = strlen(data);
    char cmd[24];
    sprintf(cmd, "AT+CIPSEND=%d\r\n", len);

    ClearRxBuffer();
    Serial_SendString(cmd);              // 发送长度指令
    if (!WaitForResponse(">", 3000))     // 等待 ">" 提示符
        return 0;

    Serial_SendString((char*)data);      // 发送实际数据
    return WaitForResponse("OK", 5000);  // 等待发送完成确认
}

/**
  * @brief   进入透传模式
  * @param   无
  * @retval  1 - 进入成功；0 - 失败
  * @note    需先设为单连接模式，再开启透传，最后发送"AT+CIPSEND"进入透传
  */
uint8_t ESP8266_EnterTransparentMode(void)
{
    // 设为单连接模式（如果已是单连接可跳过）
    if (!ESP8266_SendCmd("AT+CIPMUX=0\r\n", "OK", 2000))
        return 0;
    // 开启透传模式
    if (!ESP8266_SendCmd("AT+CIPMODE=1\r\n", "OK", 2000))
        return 0;

    ClearRxBuffer();
    Serial_SendString("AT+CIPSEND\r\n");
    // 等待 ">" 提示符，表示已进入透传
    return WaitForResponse(">", 3000);
}

/**
  * @brief   退出透传模式
  * @param   无
  * @retval  无
  * @note    发送 "+++"（不加换行），然后延时等待模块退出
  */
void ESP8266_ExitTransparentMode(void)
{
    Serial_SendString("+++");   // 发送三个加号，不加\r\n
    Delay_ms(500);              // 等待模块退出透传
}

/**
  * @brief   关闭当前TCP连接
  * @param   无
  * @retval  无
  * @note    发送"AT+CIPCLOSE"，等待"OK"确认（但不强制检查）
  */
void ESP8266_CloseConnection(void)
{
    ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK", 3000);
}

/**
  * @brief   复位ESP8266模块
  * @param   无
  * @retval  无
  * @note    发送"AT+RST"，等待"OK"确认（超时5秒）
  */
void ESP8266_Reset(void)
{
    ESP8266_SendCmd("AT+RST\r\n", "OK", 5000);
}
