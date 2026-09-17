#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "LightSensor.h"
#include "AD.h"
#include "Serial.h"
#include "ESP8266_Driver.h"
#include "MySPI.h"
#include "W25Q64.h"
#include "Config.h"
#include "Serial3.h"
#include "StartLink.h"
#include <string.h>

uint16_t ADValue;			//定义AD值变量
float Temperature;			//定义温度变量
char Data[32];

int main(void)
{
	/*模块初始化*/
	OLED_Init();
	LightSensor_Init();
	AD_Init();
	Serial_Init();
	Serial3_Init();
	W25Q64_Init();
	Serial3_Init();
	Config_Init();
	
	Serial_SendString("Serial OK!\r\n");
	
	Delay_ms(500);
	
	/*ESP8266初始化*/
	OLED_ShowString(1, 1, "START");
	
	// ========== 第一步：测试 ESP8266 是否响应 ==========
    if (ESP8266_SendCmd("AT\r\n", "OK", 10000))
    {
        OLED_ShowString(1, 1, "OK   ");
    }
    else
    {
        OLED_ShowString(1, 1, "ERROR");
        while (1);  // 死循环，等待复位
    }
	
	//ChoseMod();//进入设置模式
	Config_Init();
	
	 // ========== 第二步：连接 WiFi ==========
    OLED_ShowString(2, 1, "Wifi:");
    if (ESP8266_ConnectWiFi(config.wifi_ssid, config.wifi_pwd))
    {
        OLED_ShowString(2, 6, "OK");
    }
    else
    {
        OLED_ShowString(2, 6, "ERROR");
        while (1);
    }
		
	Delay_ms(500);
	// ========== 第三步：连接 Unity TCP 服务器 ==========
	OLED_ShowString(3, 1, "Unity:");
    if (ESP8266_ConnectServer(config.server_ip, config.server_port))
    {
        OLED_ShowString(3, 7, "OK");
    }
    else
    {
        OLED_ShowString(3, 7, "ERROR");
        while (1);
    }

    OLED_Clear();
	
	
	/*OLED显示*/
	
	OLED_ShowString(1, 1, config.device_id);
	
	OLED_ShowString(2, 1, "Light:");
	
	OLED_ShowString(3, 1, "TEMP:");
	
	OLED_ShowString(4, 1, "AD:");
	
	while (1)
	{
    ADValue = AD_GetValue();
    Temperature = ((float)ADValue / 4095 * 60.0) - 8.0;
    OLED_ShowNum(4, 4, ADValue, 4);
    OLED_ShowNum(3, 6, Temperature, 2);
    OLED_ShowChar(3, 8, '.');
    OLED_ShowNum(3, 9, (uint16_t)(Temperature * 100) % 100, 2);

    if (LightSensor_Get() == 0) {
        OLED_ShowString(2, 7, "On ");
        sprintf(Data, "sL1T%.2f", Temperature);
    }
    else {
        OLED_ShowString(2, 7, "Off");
        sprintf(Data, "sL0T%.2f", Temperature);
    }

    /* ★★★ 关键：发送给 ESP8266 → Unity ★★★ */
    if (ESP8266_SendData(Data))
        OLED_ShowString(4, 1, "OK ");
    else
        OLED_ShowString(4, 1, "ERR");

    Delay_ms(config.interval_ms);   // 用配置里的间隔
	}
}
