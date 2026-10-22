#include "stm32f10x.h"
#include "Delay.h"
#include "Millis.h"
#include "OLED.h"
#include "Serial.h"
#include "Serial3.h"
#include "W25Q64.h"
#include "Config.h"
#include "StartLink.h"
#include "NodeMode.h"
#include "Network.h"
#include "DataMode.h"
#include "SystemData.h"
#include "Power.h"
#include "SoftI2C.h"

/* 调试：上电扫描 I2C 总线，确认 OLED/传感器地址；确认后改成 0 */
#define I2C_SCAN_ON_BOOT  1

int main(void)
{
    /* ========== 1. 电源脚先就位（默认全关） ========== */
    Power_Rail_Init();

    /* 2. 给 OLED + 传感器上电，等电源稳定 */
    Power_Sensor_ON();
    Delay_ms(100);

    /* 3. 总线和屏幕最先初始化，后面才能显示其他外设状态 */
    SoftI2C_Init();
    OLED_Init();
    Serial_Init();
    Serial3_Init();

#if I2C_SCAN_ON_BOOT
    /* 调试：扫描 I2C 总线，有应答的地址打印出来 */
    Serial3_SendString("I2C:");
    for (uint8_t addr = 0x08; addr < 0x78; addr++)
    {
        if (SoftI2C_Probe(addr))
            Serial3_Printf(" %02X", (unsigned int)addr);
    }
    Serial3_SendString("\r\n");
#endif

    /* 4. 配置、运行模式、毫秒计数 */
    W25Q64_Init();
    Config_Init();

    /* 开机 5 秒内从 USART3 发任意字符，进入配置模式 */
    StartLink_BootWindow(POWER_BOOT_GRACE_MS);

    NodeMode_LoadFromConfig();   // 把 config.mode 解析成运行期模式
    NodeMode_Print();            // 串口 + OLED 显示当前模式
    Millis_Init();
    SystemData_Init();
    
    /* 5. 按网络模式控制 LoRa 电源（WiFi 模式关断，避免收到 AT 指令） */
    if (g_nodeMode.net == NET_TYPE_LORA) Power_Lora_ON();
    else                                 Power_Lora_OFF();

    /* 6. 按模式初始化传感器（屏幕显示初始化进度） */
    if (!DataMode_Init())
        Serial3_SendString("Init warn\r\n");

    /* 7. RTC 最后初始化，出错时串口已经可用 */
    Power_RTC_Init();

    /* 没有传感器时，OLED 也一起断电，避免待机耗电 */
    if (!NodeMode_HasAnySensor())
        Power_Sensor_OFF();

    /* EXTI & NVIC 配置 (RTC唤醒) */
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    Serial3_SendString("System boot\r\n");

//    /* ========== 2. 网络连接 (常驻) ========== */
//    g_connType = (g_nodeMode.net == NET_TYPE_LORA) ? NET_LORA : NET_WIFI;
//    
//    if (!Network_Connect()) {
//        Serial3_SendString("Network Connect Failed!\r\n");
//        while(1);
//    }
//    （DataMode_Init 已在上面调用）

    /* ========== 3. 低功耗主循环 ========== */
    // 注意：不能用 millis() 计时，TIM2 在 STOP 模式下会停，休眠的时间不会被算进去
    uint32_t last_send_sec = Power_GetSeconds(); // 记录上次发送时间（RTC 秒数）
    uint32_t tick = 0;

    while (1)
    {
        tick++;
        // OLED 一行只有 16 个字符，所以提示文字尽量短（串口和 OLED 显示同样的内容）
        Serial3_Printf("\r\nWake Tick:%u\r\n", (unsigned int)tick);
        
        /* 3.1 供电与采集 */
        // 如果模式不是全0，说明有传感器要工作
        if (NodeMode_HasAnySensor()) {
            Power_Sensor_ON();

            /* SCD40 周期测量需要持续供电，不能采完就断 */
            if (!DataMode_NeedsContinuousPower())
                Delay_ms(DataMode_GetSettleMs()); // 按当前模式取上电稳定时间

            DataMode_Read_All(); // 去读数据，存入 sysData

            if (!DataMode_NeedsContinuousPower())
                Power_Sensor_OFF(); // 采完立刻断电
        }

        /* 3.2 判断时间闸门是否到了 */
        // 如果还没到发送间隔，就不发
        if (Power_GetSeconds() - last_send_sec >= config.send_interval_ms / 1000) {
            Serial3_SendString("Time to send!\r\n");

            DataMode_Send_Data(); // 把刚才采到的数据打包发出去
            SystemData_ClearFlags(); // 发完再清标志，慢速传感器不会丢数据

            last_send_sec = Power_GetSeconds(); // 更新发送时间
        }

        /* 3.3 进入休眠 */
        // 假设固定每 2 秒醒来一次。你可以把 2000 写进 config 里
        Serial3_SendString("Go to sleep...\r\n");
        Power_Sleep(2000); 
    }
}

