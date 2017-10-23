#include "utils.h"

#include <stdio.h>

float convert_temp_u16_f(uint16_t temp)
{
    int8_t msb;
    uint8_t lsb;
    msb = (temp>>8)&0xFF;
    lsb = temp&0xFF;
    if (msb < 0) {
        return (float) (1.0*msb - 1.0/256.0*lsb);
    }else {
        return (float) (1.0*msb + 1.0/256.0*lsb);
    }
}

uint16_t convert_temp_f_u16(float temp)
{
    int msb = (int) temp;
    printf("MSB: %d\n", msb);
    float decpart = temp - msb;
    if (decpart < 0.0) {
        decpart = decpart * -1.0;
    }
    printf("DECpart: %f\n", decpart);
    int lsb = (int) (decpart * 256);
    printf("LSB: %u\n", lsb);
    return ((msb&0xFF) << 8) | (lsb&0xff);
}

// ------------------------------------------------------------------------------------------------
// Calculate RSSI in dBm from decimal RSSI read out of RSSI status register
float rssi_raw_dbm(uint8_t rssi_dec)
// ------------------------------------------------------------------------------------------------
{
        if (rssi_dec < 128)
        {
                return (rssi_dec / 2.0) - 74.0;
        }
        else
        {
                return ((rssi_dec - 256) / 2.0) - 74.0;
        }
}

// ------------------------------------------------------------------------------------------------
// Calculate RSSI in dBm from decimal RSSI read out of RSSI status register
float rssi_lna_dbm(uint8_t rssi_dec)
// ------------------------------------------------------------------------------------------------
{
        if (rssi_dec < 128)
        {
                return (rssi_dec / 2.0) - 89.9;
        }
        else
        {
                return ((rssi_dec - 256) / 2.0) - 89.9;
        }
}

float lqi_status(uint8_t lqi)
{
        return (1.0 - ((float) lqi/127.0)) * 100.0;
}
