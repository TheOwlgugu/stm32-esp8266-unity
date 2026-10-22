#include "SystemData.h"
#include <string.h>

SystemData_t sysData;

void SystemData_Init(void)
{
    memset(&sysData, 0, sizeof(SystemData_t));
}

void SystemData_ClearFlags(void)
{
    sysData.light_updated = 0;
    sysData.temp_updated = 0;
    sysData.humi_updated = 0;
    sysData.co2_updated = 0;
    sysData.pm25_updated = 0;
}
