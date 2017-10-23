#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
typedef struct __attribute__ ((__packed__)) comms_hk_data_u {
    struct __attribute__ ((__packed__)) {
        uint16_t ext_temp;
        uint16_t int_temp;
        uint32_t phy_rx_packets;
        uint32_t phy_tx_packets;
        uint16_t phy_rx_errors;
        uint16_t phy_tx_failed_packets;
        uint32_t ll_rx_packets;
        uint32_t ll_tx_packets;
        uint8_t transmit_power;
        uint8_t actual_rssi;
        uint8_t last_rssi;
        uint8_t last_lqi;
    } housekeeping;
    struct __attribute__ ((__packed__)) {
        uint8_t rx_queued;
        uint8_t tx_remaining;
        uint16_t free_stack[4];
        uint16_t used_stack[4];
    } control;
} comms_hk_data_t;

float   rssi_lna_dbm(uint8_t rssi_dec);
float   rssi_raw_dbm(uint8_t rssi_dec);
float   lqi_status(uint8_t lqi);

float convert_temp_u16_f(uint16_t temp);
uint16_t convert_temp_f_u16(float temp);

#endif
