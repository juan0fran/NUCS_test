/*
 * link_layer.h
 *
 *  Created on: Mar 27, 2017
 *      Author: gs-ms
 */

#ifndef INC_LINK_LAYER_H_
#define INC_LINK_LAYER_H_

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#ifndef MAC_UNCODED_PACKET_SIZE
#define MAC_UNCODED_PACKET_SIZE 223
#endif
#ifndef MAC_PAYLOAD_SIZE
#define MAC_PAYLOAD_SIZE 219
#endif

#define LINK_LAYER_PACKET_SIZE 		(MAC_PAYLOAD_SIZE * 15)
#define LINK_LAYER_HEADER_SIZE 		8
#define LINK_LAYER_PAYLOAD_SIZE		(LINK_LAYER_PACKET_SIZE - LINK_LAYER_HEADER_SIZE)

typedef union __attribute__ ((__packed__)) link_layer_packet_s {
    uint8_t     raw[LINK_LAYER_PACKET_SIZE];
    /* + 2 which are rssi+lqi */
    struct __attribute__ ((packed)){
    	uint32_t 	attribs;
    	uint16_t 	len;
        uint8_t 	payload[LINK_LAYER_PAYLOAD_SIZE];
        uint16_t 	crc;	/* for the moment 16 bit crc... */
    }fields;
}link_layer_packet_t;

#endif /* INC_LINK_LAYER_H_ */
