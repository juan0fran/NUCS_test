#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

typedef struct __attribute__ ((__packed__)) comms_hk_data_u {
	int32_t 	ext_temp;
	int32_t 	int_temp;
	uint32_t 	bus_volt;
	uint32_t	phy_rx_packets;
	uint32_t 	phy_rx_errors;
	uint32_t	phy_tx_packets;
	uint32_t	phy_tx_failed_packets;
	uint32_t 	ll_rx_packets;
	uint32_t 	ll_tx_packets;
	uint8_t 	rx_queued;
	uint8_t		tx_remaining;
	uint8_t		trx_status;
	uint16_t	free_stack[6];
	uint16_t	used_stack[6];
	float 		last_rssi;
	float 		last_lqi;
}comms_hk_data_t;

#endif
