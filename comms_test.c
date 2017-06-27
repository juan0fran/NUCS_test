 #include "uart_control.h"

#define PRINT_INFO 1
#define STORE_INFO 1

static char opt;
static serial_parms_t serial;

static struct statistics_t {
    int sent_packets;
    int received_packets;
}test_statistics;

void send_packet(void)
{
    int i, ret;
    simple_link_packet_t packet;
    link_layer_packet_t ll_packet_buffer;
    link_layer_packet_t *ll_packet;
    ll_packet = (link_layer_packet_t *) &ll_packet_buffer;
    for (i = 0; i < 1500; i++) {
        ll_packet->fields.payload[i] = i % 256;
    }
    ll_packet->fields.len = 1500;
    ll_packet->fields.attribs = 9600;
    ll_packet->fields.crc = 0;
    if ( (ret = set_simple_link_packet(ll_packet, ll_packet->fields.len + LINK_LAYER_HEADER_SIZE, 0, 4, &packet) ) > 0) {
        if (send_kiss_packet(serial.fd, &packet, ret) == -1) {
            printf("Not working!\n");
        }
    }
}

void send_control(void)
{
    int ret;
    simple_link_packet_t packet;
    if ( (ret = set_simple_link_packet(NULL, 0, 1, 0, &packet) ) > 0) {
        if (send_kiss_packet(serial.fd, &packet, ret) == -1) {
            printf("Not working!\n");
        }
    }
}

void store_info(comms_hk_data_t *data)
{
    FILE *fp;
    #if STORE_INFO
    fp = fopen("comms_hk_file.txt", "a+");
    if (fp != NULL) {
        fprintf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f,%d,%d,%d,%d\r\n",
                data->ext_temp, data->int_temp, data->bus_volt,
                data->phy_rx_packets, data->phy_rx_errors, data->phy_tx_packets, data->phy_tx_failed_packets,
                data->ll_rx_packets, data->ll_tx_packets,
                data->actual_rssi, data->last_rssi, data->last_lqi,
                data->free_stack[0], data->free_stack[1], data->free_stack[2], data->free_stack[3]);
    }else {
        perror("fopen");
    }
    fclose(fp);
    #endif
}

int receive_control(comms_hk_data_t *data)
{
    simple_link_packet_t packet;
    simple_link_control_t control;
    int ret;
    while (read_port(&serial) > 0) {
        if( (ret = get_simple_link_packet(serial.buffer[0], &control, &packet)) > 0) {
            if (packet.fields.config1 == 1) {
                /* store comms_hk_data_t somewhere */
                memcpy(data, packet.fields.payload, sizeof(comms_hk_data_t));
                store_info(data);
                return 1;
            }else if (packet.fields.config1 == 2) {
                return 2;
            }
        }
    }
    return 0;
}

int receive_frame(link_layer_packet_t *ll_packet)
{
    simple_link_packet_t packet;
    simple_link_control_t control;
    int ret;
    while (read_port(&serial) > 0) {
        if( (ret = get_simple_link_packet(serial.buffer[0], &control, &packet)) > 0) {
            if (packet.fields.config1 == 0) {
                memcpy(ll_packet, packet.fields.payload, sizeof(link_layer_packet_t));
                return 1;
            }
        }
    }
    return 0;
}

void send_req(void)
{
    int ret;
    simple_link_packet_t packet;
    if ( (ret = set_simple_link_packet(NULL, 0, 2, 0, &packet) ) > 0) {
        if (send_kiss_packet(serial.fd, &packet, ret) == -1) {
            printf("Not working!\n");
        }
    }
}

void send_routine(void)
{
    link_layer_packet_t packet;
    comms_hk_data_t data;
    while(1) {
        send_control();
        if (receive_control(&data) == 1) {
            if (data.tx_remaining > 0) {
                send_packet();
                if ( receive_control(&data) == 2) {
                    #if PRINT_INFO
                    printf("Control packet information --> \t");
                    printf("Spaces in TX queue: %d, ", data.tx_remaining);
                    printf("Free Stack: %d %d %d %d\r\n",
                                data.free_stack[0], data.free_stack[1], data.free_stack[2],
                                data.free_stack[3]);
                    #endif
                    test_statistics.sent_packets++;
                    printf("Packet %d sent correctly\r\n", test_statistics.sent_packets);
                }
            }else {
                printf("Queue is full\r\n");
            }
        }
        sleep(2);
    }
}

void receive_routine(void)
{
    link_layer_packet_t packet;
    comms_hk_data_t data;
    while(1) {
        send_control();
        if (receive_control(&data) == 1) {
            if (data.rx_queued > 0) {
                send_req();
                if (receive_frame(&packet) > 0) {
                    #if PRINT_INFO
                    printf("Control packet information --> \t");
                    printf("Packets in RX queue: %d, ", data.rx_queued);
                    printf("Last RSSI: %0.2f, ", data.last_rssi);
                    printf("Actual RSSI: %0.2f, SNR = %f, ", data.actual_rssi, data.last_rssi - data.actual_rssi);
                    printf("Free Stack: %d %d %d %d\r\n",
                                data.free_stack[0], data.free_stack[1], data.free_stack[2],
                                data.free_stack[3]);
                    #endif
                    test_statistics.received_packets++;
                    printf("New packet received --> \tReceived: %d bytes packet. Total Count: %d\r\n",
                                                    packet.fields.len, test_statistics.received_packets);
                }
            }
        }
        sleep(1);
    }
}

void retransmit_routine(void)
{
    link_layer_packet_t packet;
    comms_hk_data_t data;
    while(1) {
        send_control();
        if (receive_control(&data) == 1) {
            while (data.rx_queued-- > 0) {
                send_req();
                if (receive_frame(&packet) > 0) {
                    #if PRINT_INFO
                    printf("Control packet information --> \t");
                    printf("Packets in RX queue: %d, ", data.rx_queued);
                    printf("Last RSSI: %0.2f, ", data.last_rssi);
                    printf("Actual RSSI: %0.2f, SNR = %f, ", data.actual_rssi, data.last_rssi - data.actual_rssi);
                    printf("Free Stack: %d %d %d %d\r\n",
                                data.free_stack[0], data.free_stack[1], data.free_stack[2],
                                data.free_stack[3]);
                    #endif
                    test_statistics.received_packets++;
                    printf("New packet received --> \tReceived: %d bytes packet. Total Count: %d\r\n",
                                                    packet.fields.len, test_statistics.received_packets);;
                }
                /* send a TX */
                if (data.tx_remaining > 0) {
                    send_packet();
                    if ( receive_control(&data) == 2) {
                        test_statistics.sent_packets++;
                        printf("Packet %d sent correctly\r\n", test_statistics.sent_packets);
                    }
                }
            }
        }
        sleep(1);
    }
}

void send_and_receive_routine(void)
{
    link_layer_packet_t packet;
    comms_hk_data_t data;
    uint32_t start;
    while(1) {
        send_control();
        if (receive_control(&data) == 1) {
            if (data.tx_remaining > 0) {
                send_packet();
                if ( receive_control(&data) == 2) {
                    test_statistics.sent_packets++;
                    printf("Packet %d sent correctly\r\n", test_statistics.sent_packets);
                }
            }
            start = time(NULL);
            do {
                send_control();
                receive_control(&data);
                sleep(1);
            }while(data.rx_queued == 0 && (time(NULL) - start) < 10);
            while (data.rx_queued-- > 0) {
                send_req();
                if (receive_frame(&packet) > 0) {
                    #if PRINT_INFO
                    printf("Control packet information --> \t");
                    printf("Packets in RX queue: %d, ", data.rx_queued);
                    printf("Last RSSI: %0.2f, ", data.last_rssi);
                    printf("Actual RSSI: %0.2f, SNR = %f, ", data.actual_rssi, data.last_rssi - data.actual_rssi);
                    printf("Free Stack: %d %d %d %d\r\n",
                                data.free_stack[0], data.free_stack[1], data.free_stack[2],
                                data.free_stack[3]);
                    #endif
                    test_statistics.received_packets++;
                    printf("New packet received --> \tReceived: %d bytes packet. Total Count: %d\r\n",
                                                    packet.fields.len, test_statistics.received_packets);
                }
            }
        }
        sleep(10 - (time(NULL) - start));
    }
}

int main(int argc, char ** argv)
{
    comms_hk_data_t data;
    int cnt = 0;
    char dev_name[64];
    if (argc == 3){
        strcpy(dev_name, argv[1]);
        opt = argv[2][0];
    }else{
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    if (opt != 't' && opt != 'r' && opt != 'x' && opt != 'w' && opt != 'p') {
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    begin(dev_name, B115200, 500, &serial);
    /* Clear the input */
    clear(&serial);
    test_statistics.sent_packets = 0;
    test_statistics.received_packets = 0;
    printf("Going to %c\r\n", opt);
    if (opt == 't') {
        /* t */
        send_routine();
    }else if (opt == 'r') {
        /* r */
        receive_routine();
    }else if (opt == 'x') {
        /* x */
        retransmit_routine();
    }else if (opt == 'w'){
        /* w */
        send_and_receive_routine();
    }else {
        while(1) {
            send_control();
            if (receive_control(&data) == 1) {
                printf("Control packet information --> \t");
                printf("Packets in RX queue: %d, ", data.rx_queued);
                printf("Last RSSI: %0.2f, ", data.last_rssi);
                printf("Actual RSSI: %0.2f, SNR = %f, ", data.actual_rssi, data.last_rssi - data.actual_rssi);
                printf("Free Stack: %d %d %d %d\r\n",
                            data.free_stack[0], data.free_stack[1], data.free_stack[2],
                            data.free_stack[3]);
            }
            sleep(1);
        }
    }
    close(serial.fd);
}
