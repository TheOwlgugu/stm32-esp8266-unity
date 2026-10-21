#ifndef __LORAMODE_H
#define __LORAMODE_H

#include "stm32f10x.h"

uint8_t LoRaConnect(void);
uint8_t LoRaSend(const char *msg);

#endif
