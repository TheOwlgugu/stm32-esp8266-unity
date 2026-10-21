#include "LightMode.h"
#include "LightSensor.h"

int Light_Read(void)
{
    return (LightSensor_Get() == 0) ? 1 : 0;
}

uint8_t Light_IsChanged(int new_val, int old_val)
{
    return (new_val != old_val);
}
