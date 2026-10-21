#ifndef __RA08_DRIVER_H
#define __RA08_DRIVER_H
#include "stm32f10x.h"

#define RA08_MAX_PAYLOAD  51   /* 按最低速率 DR0 的上限,所有速率都安全 */

typedef enum {
    RA08_SEND_OK = 0,
    RA08_SEND_TIMEOUT,
    RA08_SEND_NOT_JOINED,
    RA08_SEND_BUSY,
    RA08_SEND_TOO_LONG,
    RA08_SEND_NO_ACK
} Ra08_SendResult;

uint8_t         Ra08_Init(void);
uint8_t         Ra08_SendCmd(const char *cmd, const char *expected, uint32_t timeout_ms);
uint8_t         Ra08_Provision(const char *deveui, const char *appeui, const char *appkey);
uint8_t         Ra08_Join(void);
uint8_t         Ra08_IsJoined(void);
Ra08_SendResult Ra08_SendData(const uint8_t *data, uint8_t len, uint8_t confirm);

#endif
