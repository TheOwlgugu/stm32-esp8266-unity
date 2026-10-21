#ifndef __WIFIMODE_H
#define __WIFIMODE_H

#include "stm32f10x.h"

/**
  * @brief  WiFi 连接全流程
  * @note   测 AT → 连 WiFi → 连 TCP 服务器
  * @retval 1=成功，0=失败
  */
uint8_t WifiConnect(void);

/**
  * @brief  通过 WiFi 发送数据报
  * @param  msg  数据报字符串
  * @retval 1=成功，0=失败
  */
uint8_t WifiSend(const char *msg);

#endif
