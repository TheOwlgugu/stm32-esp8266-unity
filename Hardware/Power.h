#ifndef __POWER_H
#define __POWER_H

#include "stm32f10x.h"

/* 传感器 + OLED 总电源：PB0，P-MOS 高边，低电平导通 */
#define SENSOR_PWR_PORT   GPIOB
#define SENSOR_PWR_PIN    GPIO_Pin_0

/* LoRa 电源：PB1，1M 上拉到 3.3V，默认关断；低电平导通 */
#define LORA_PWR_PORT     GPIOB
#define LORA_PWR_PIN      GPIO_Pin_1

void Power_Rail_Init(void);        // 只初始化电源开关引脚，默认全部关断
void Power_RTC_Init(void);         // 初始化 LSI + RTC

void Power_Sensor_ON(void);
void Power_Sensor_OFF(void);
void Power_Lora_ON(void);
void Power_Lora_OFF(void);

void Power_Sleep(uint32_t ms);
uint32_t Power_GetSeconds(void);   // 系统运行秒数（RTC，含休眠时间）

/* 开机后保持唤醒的时间（毫秒），给调试器/烧录留出连接窗口；发布时设为 0 即可关闭 */
#define POWER_BOOT_GRACE_MS  5000
void Power_BootGrace(void);

#endif
