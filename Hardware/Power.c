#include "Power.h"
#include "stm32f10x.h"
#include "Delay.h" // 如果用到里面的 SysTick 或者你自己写时钟恢复
#include "Millis.h"
#include "Serial3.h"

/* RTC 是否可用（LSI 起振且寄存器同步成功才为 1）。不可用时 Power_Sleep 退化为普通延时 */
static uint8_t rtc_ready = 0;

/**
  * @brief  等待 RTC 寄存器与 APB1 同步（RSF 置位），带超时
  * @note   RTC 时钟正常时 1 秒内 RSF 必然置位；超时说明 RTC 没有时钟，返回 0
  *         这里不会清 RSF，清 RSF 由调用者决定
  */
static uint8_t RTC_WaitSync(void)
{
    uint32_t timeout = 0x1000000;
    while (RTC_GetFlagStatus(RTC_FLAG_RSF) == RESET)
    {
        if (--timeout == 0) return 0;
    }
    return 1;
}

/**
  * @brief  电源开关引脚初始化
  * @note   PB0 = 传感器 + OLED 总电源，PB1 = LoRa 电源
  *         两路都是 P-MOS 高边：栅极低电平导通，高电平关断
  *         复位后引脚悬空，靠外部上拉保证默认关断
  */
void Power_Rail_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = SENSOR_PWR_PIN | LORA_PWR_PIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 默认全部关断 */
    Power_Sensor_OFF();
    Power_Lora_OFF();
}

/**
  * @brief  RTC 初始化（LSI 时钟源，1Hz 计数，用于定时唤醒）
  * @note   必须在 Power_GetSeconds / Power_Sleep 之前调用
  */
void Power_RTC_Init(void)
{
    /* RTC (用于定时唤醒) */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE); // 允许访问后备区域

    /* ★ LSI 不在备份域里：系统复位（复位键、看门狗、调试器复位）会把 LSION 清零，
     *   而 RTC 时钟源的选择保存在备份域里，复位后依然在。
     *   所以每次启动都必须先重新打开 LSI，否则 RTC 没有时钟，
     *   后面等同步的时候会一直死等，表现为"按复位键后没反应" */
    RCC_LSICmd(ENABLE);
    uint32_t timeout = 100000;
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET && timeout) timeout--;
    if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
    {
        Serial3_SendString("LSI Clock Timeout!\r\n");
        return; // rtc_ready 保持 0，Power_Sleep 会退化为普通延时
    }

    // 如果 RTC 未配置过，则配置 LSI(内部低速时钟) 作为 RTC 时钟源
    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    {
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        RCC_RTCCLKCmd(ENABLE);

        RTC_ClearFlag(RTC_FLAG_RSF);
        if (!RTC_WaitSync())
        {
            Serial3_SendString("RTC Sync Timeout!\r\n");
            return;
        }
        RTC_WaitForLastTask();

        // 设置 RTC 分频，让 RTC 时钟变成 1Hz（每秒走一格）
        // LSI 约 40kHz，40k / 40000 = 1Hz
        RTC_SetPrescaler(40000 - 1);
        RTC_WaitForLastTask();

        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5); // 写入标志，下次上电不再重置
    }
    else
    {
        // 如果配置过，只需等待同步并开启 RTC 时钟
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        RCC_RTCCLKCmd(ENABLE);

        RTC_ClearFlag(RTC_FLAG_RSF);
        if (!RTC_WaitSync())
        {
            Serial3_SendString("RTC Sync Timeout!\r\n");
            return;
        }
    }

    rtc_ready = 1;
}

/**
  * @brief  打开传感器电源 (MOS管导通)
  */
void Power_Sensor_ON(void)
{
    GPIO_ResetBits(SENSOR_PWR_PORT, SENSOR_PWR_PIN); // 拉低，P-MOS 导通
}

/**
  * @brief  关闭传感器电源 (MOS管关断)
  */
void Power_Sensor_OFF(void)
{
    GPIO_SetBits(SENSOR_PWR_PORT, SENSOR_PWR_PIN); // 拉高，P-MOS 关断
}

/**
  * @brief  打开 LoRa 电源 (PB1 拉低，P-MOS 导通)
  * @note   1M 上拉下栅极静态电流约 3.3 微安
  */
void Power_Lora_ON(void)
{
    GPIO_ResetBits(LORA_PWR_PORT, LORA_PWR_PIN);
}

/**
  * @brief  关闭 LoRa 电源 (PB1 拉高，P-MOS 关断)
  */
void Power_Lora_OFF(void)
{
    GPIO_SetBits(LORA_PWR_PORT, LORA_PWR_PIN);
}

/**
  * @brief  获取系统运行秒数（RTC 计数，STOP 休眠期间也在走）
  * @note   millis() 基于 TIM2，STOP 模式下会停，不能用来统计含休眠的时间
  *         唤醒后 RTC 影子寄存器是旧值，这里会等它同步完再读（最多等 1 秒）
  *         RTC 不可用时退化为 millis()/1000
  */
uint32_t Power_GetSeconds(void)
{
    if (rtc_ready && !RTC_WaitSync()) rtc_ready = 0;
    return rtc_ready ? RTC_GetCounter() : (millis() / 1000);
}

/**
  * @brief  进入 STOP 模式休眠指定毫秒
  * @param  ms 休眠时间（毫秒）
  * @note   注意：LSI 精度较差，会有几秒的误差，对于低功耗场景可接受
  *         RTC 以秒为单位，最短休眠 2 秒
  */
void Power_Sleep(uint32_t ms)
{
    uint32_t wakeup_sec = ms / 1000;
    /* ★ 最少睡 2 秒：设闹钟时如果刚好跨过秒边界，1 秒的闹钟会已经过期，
     *   RTC 只在计数等于闹钟值的那一刻触发，错过就要等 2^32 秒，MCU 再也醒不来 */
    if (wakeup_sec < 2) wakeup_sec = 2;

    /* RTC 不可用就用普通延时代替休眠 */
    if (!rtc_ready)
    {
        Delay_ms(ms);
        return;
    }

    /* 0. 读计数前，确保影子寄存器已同步（唤醒后 APB1 时钟停过，里面是睡前的旧值）
     *    ★ 如果拿旧值算闹钟，闹钟会落在"过去"，MCU 会一直睡下去 */
    if (!RTC_WaitSync())
    {
        rtc_ready = 0;
        Delay_ms(ms);
        return;
    }

    /* 1. 等串口把最后一个字节移出去，否则时钟一停最后几个字符会丢 */
    uint32_t tc_timeout = 100000;
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET && tc_timeout) tc_timeout--;

    /* 2. 清除标志并设置新的闹钟 */
    RTC_WaitForLastTask();
    RTC_ClearFlag(RTC_FLAG_ALR); // ★ 修复标志位名字
    RTC_SetAlarm(RTC_GetCounter() + wakeup_sec);
    RTC_WaitForLastTask();

    RTC_ITConfig(RTC_IT_ALR, ENABLE);
    RTC_WaitForLastTask();

    EXTI_ClearITPendingBit(EXTI_Line17); // 清掉残留的 EXTI 挂起位，避免一进 STOP 就被唤醒

    /* 3. 挂起 SysTick，防止休眠时被 tick 唤醒 */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    /* 4. 进入 STOP 模式 (WFI 等待中断) */
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

    /* ---- 以下代码在唤醒后执行 ---- */

        /* 5. 唤醒后恢复系统时钟 */
    SystemInit();

    /* ★ 重新配置 SysTick */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    /* ★ APB1 时钟停过，RTC 影子寄存器已经过期：先清 RSF，
     *   下次读计数（Power_GetSeconds / Power_Sleep）之前会等它重新同步。
     *   这里不死等，是为了让同步和后面的采集过程并行，少等一会儿 */
    RTC_ClearFlag(RTC_FLAG_RSF);

    /* ★ 加一点延时，让硬件稳定一下 */
    Delay_ms(10);

    /* ★ 如果醒来后OLED乱了，可以在这里重新初始化一下（可选，看情况） */
    // OLED_Init();

}

/**
  * @brief  开机保护窗口：复位后先保持唤醒一段时间，不进入 STOP
  * @note   STOP 模式下调试器读写不了芯片，下载程序会报 Flash Timeout。
  *         复位后留出这段时间，调试器就能连上并烧录。时长见 Power.h 的 POWER_BOOT_GRACE_MS
  */
void Power_BootGrace(void)
{
#if POWER_BOOT_GRACE_MS > 0
    Serial3_SendString("Flash window...\r\n");
    Delay_ms(POWER_BOOT_GRACE_MS);
#endif
}
