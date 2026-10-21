#ifndef __STARTLINK_H
#define __STARTLINK_H

#include "stm32f10x.h"

/**
  * @brief  进入配置模式
  * @note   阻塞运行，直到收到 "exit" 才返回
  *         用户通过 USART3 发送 set 指令修改 config，每条修改立即写入 W25Q64
  */
void StartLink(void);

/**
  * @brief  解析一条配置指令
  * @param  cmd  以 '\0' 结尾的指令字符串，格式 "set:字段=值"
  * @retval 1=成功，0=失败
  */
uint8_t SetConfig(char *cmd);

#endif
