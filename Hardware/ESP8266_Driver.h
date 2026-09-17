#ifndef __ESP8266_DRIVER_H
#define __ESP8266_DRIVER_H

#include "stm32f10x.h"
#include <stdint.h>

void ESP8266_Init(void);
uint8_t ESP8266_SendCmd(const char *cmd, const char *expected_response, uint32_t timeout_ms);
uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *pwd);
uint8_t ESP8266_GetIP(char *ip_buf);
uint8_t ESP8266_ConnectServer(const char *ip, uint16_t port);
uint8_t ESP8266_SendData(const char *data);
uint8_t ESP8266_EnterTransparentMode(void);
void ESP8266_ExitTransparentMode(void);
void ESP8266_CloseConnection(void);
void ESP8266_Reset(void);

#endif
