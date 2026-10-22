#include "Pms7003Mode.h"
#include "Serial2.h"
#include "Delay.h"

#define PMS_FRAME_LEN   32

/* 帧解析状态 */
static uint8_t  frame[PMS_FRAME_LEN];
static uint8_t  idx = 0;
static uint16_t latest_pm25 = 0;

/**
  * @brief  逐字节拼帧
  * @retval 1 = 收到一帧校验通过的数据
  * @note   帧格式：0x42 0x4D + 长度(0x00 0x1C) + 26 字节数据 + 2 字节校验
  *         校验 = 前 30 字节累加和
  *         data5（第 12~13 字节）= 大气环境 PM2.5，单位 μg/m3
  */
static uint8_t Pms7003_Feed(uint8_t b)
{
    if (idx == 0)
    {
        if (b == 0x42) frame[idx++] = b;
    }
    else if (idx == 1)
    {
        if (b == 0x4D) frame[idx++] = b;
        else if (b != 0x42) idx = 0;   /* 保留 frame[0]=0x42，等下一个 0x4D */
    }
    else
    {
        frame[idx++] = b;

        if (idx >= PMS_FRAME_LEN)
        {
            idx = 0;

            if (frame[2] == 0x00 && frame[3] == 0x1C)
            {
                uint16_t sum = 0;
                uint16_t check;
                uint8_t i;

                for (i = 0; i < 30; i++) sum += frame[i];
                check = ((uint16_t)frame[30] << 8) | frame[31];

                if (sum == check)
                {
                    latest_pm25 = ((uint16_t)frame[12] << 8) | frame[13];
                    return 1;
                }
            }
        }
    }

    return 0;
}

/**
  * @brief  初始化 USART2 和解析状态
  */
void Pms7003Mode_Init(void)
{
    Serial2_Init(9600);
    idx = 0;
}

/**
  * @brief  读取最新 PM2.5（大气环境值，data5）
  * @note   传感器主动输出，帧间隔 200~800ms；这里最多等 1 秒。
  *         一次调用会把缓冲区里所有完整帧都解析掉，返回最后一帧，
  *         避免主循环醒来后拿到的是几秒前的旧值。
  */
uint8_t Pms7003Mode_Read(uint16_t *pm25)
{
    uint16_t timeout = 100;   /* 100 × 10ms = 1s */
    uint8_t  got = 0;

    while (timeout--)
    {
        while (Serial2_Available())
        {
            if (Pms7003_Feed(Serial2_GetByte())) got = 1;
        }

        if (got)
        {
            *pm25 = latest_pm25;
            return 1;
        }

        Delay_ms(10);
    }

    return 0;
}
