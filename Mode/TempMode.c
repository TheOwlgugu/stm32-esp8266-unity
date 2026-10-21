#include "TempMode.h"
#include "AD.h"

int Temp_Read(void)
{
    uint16_t ad = AD_GetValue();
    float t = ((float)ad / 4095 * 60.0f) - 8.0f;
    return (int)(t * 1000);
}

uint8_t Temp_IsChanged(int new_val, int old_val)
{
    int diff = new_val - old_val;
    if (diff < 0) diff = -diff;
    return (diff > TEMP_CHANGE_THRESH);
}
