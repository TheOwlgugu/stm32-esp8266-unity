#ifndef __NETWORK_H
#define __NETWORK_H

#include "stm32f10x.h"

/* 网络类型枚举 */
#define NET_WIFI  0
#define NET_LORA  1

/* 全局网络类型（由 ConnectMode 设置） */
extern uint8_t g_connType;

/**
  * @brief  连接网络（根据 g_connType 自动分发）
  * @retval 1=成功，0=失败
  */
uint8_t Network_Connect(void);

/**
  * @brief  发送数据报（根据 g_connType 自动分发）
  * @param  msg  以 '\0' 结尾的数据报字符串
  * @retval 1=成功，0=失败
  */
uint8_t Network_Send(const char *msg);

#endif
