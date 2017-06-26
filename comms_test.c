 #include "uart_control.h"

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

int receive_control(comms_hk_data_t *data)
{
    simple_link_packet_t packet;
    simple_link_control_t control;
    int ret;
    while (read_port(&serial) > 0) {
        if( (ret = get_simple_link_packet(serial.buffer[0], &control, &packet)) > 0) {
            if (packet.fields.config1 == 1) {
                memcpy(data, packet.fields.payload, sizeof(comms_hk_data_t));
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
                    printf("Packet %d sent correctly\r\n", test_statistics.sent_packets++);
                }
            }
        }
        sleep(6);
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
                    printf("Control packet information --> \t");
                    printf("Free Stack: %d %d %d %d %d %d\r\n",
                                data.free_stack[0], data.free_stack[1], data.free_stack[2],
                                data.free_stack[3], data.free_stack[4], data.free_stack[5]);
                    printf("New packet received --> \tReceived: %d bytes packet. Total Count: %d\r\n",
                                                    packet.fields.len, test_statistics.received_packets++);
                }
            }
        }
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
                    printf("Control packet information --> \t");
                    printf("Free Stack: %d %d %d %d %d %d\r\n",
                                data.free_stack[0], data.free_stack[1], data.free_stack[2],
                                data.free_stack[3], data.free_stack[4], data.free_stack[5]);
                    printf("New packet received --> \tReceived: %d bytes packet. Total Count: %d\r\n",
                                                    packet.fields.len, test_statistics.received_packets++);
                }
                /* send a TX */
                if (data.tx_remaining > 0) {
                    send_packet();
                    if ( receive_control(&data) == 2) {
                        printf("Packet %d sent correctly\r\n", test_statistics.sent_packets++);
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
                    printf("Packet %d sent correctly\r\n", test_statistics.sent_packets++);
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
                    printf("Control packet information --> \t");
                    printf("Free Stack: %d %d %d %d %d %d\r\n",
                                data.free_stack[0], data.free_stack[1], data.free_stack[2],
                                data.free_stack[3], data.free_stack[4], data.free_stack[5]);
                    printf("New packet received --> \tReceived: %d bytes packet. Total Count: %d\r\n",
                                                    packet.fields.len, test_statistics.received_packets++);
                }
            }
        }
        sleep(10 - (time(NULL) - start));
    }
}

int main(int argc, char ** argv)
{
    comms_hk_data_t data;
    link_layer_packet_t packet;
    int cnt = 0;
    char dev_name[64];
    if (argc == 3){
        strcpy(dev_name, argv[1]);
        opt = argv[2][0];
    }else{
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    if (opt != 't' && opt != 'r' && opt != 'x' && opt != 'w') {
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    begin(dev_name, B115200, 500, &serial);
    /* Clear the input */
    clear(&serial);
    test_statistics.sent_packets = 0;
    test_statistics.received_packets = 0;
    if (opt == 't') {
        /* t */
        send_routine();
    }else if (opt == 'r') {
        /* r */
        receive_routine();
    }else if (opt == 'x') {
        /* x */
        retransmit_routine();
    }else {
        /* w */
        send_and_receive_routine();
    }
    close(serial.fd);
}

#if 0

static int cnt_tx = 0;
static int cnt_rx = 0;

void receive_and_print(void)
{
    comms_hk_data_t *data;
    simple_link_packet_t packet;
    simple_link_control_t control;
    link_layer_packet_t ll_packet_buffer;
    link_layer_packet_t *ll_packet;
    int ret;
    while (read_port(&serial) > 0) {
        if( (ret = get_simple_link_packet(serial.buffer[0], &control, &packet)) > 0) {
            if (packet.fields.config1 != 0) {
                if (packet.fields.config1 == 1) {
                    data = (comms_hk_data_t *) packet.fields.payload;
                    printf( "External Temp: %d, Internal Temp: %d, Bus Volt: %d\r\n"
                            "Phy RX: %d, RXErr: %d, Phy TX: %d, TXErr: %d\r\n"
                            "LL RX: %d LL TX:%d\r\n"
                            "Last RSSI: %f\r\n",
                            data->ext_temp, data->int_temp, data->bus_volt,
                            data->phy_rx_packets, data->phy_rx_errors, data->phy_tx_packets, data->phy_failed_packets,
                            data->ll_rx_packets, data->ll_tx_packets,
                            data->last_rssi);
                    printf("Free Stack: %d %d %d %d %d %d\r\n",
                                data->free_stack[0], data->free_stack[1], data->free_stack[2],
                                data->free_stack[3], data->free_stack[4], data->free_stack[5]);
                }else {
                    if (packet.fields.config1 == 2) {
                        cnt_tx++;
                        printf("Packet has been sent (%d sent packets)\r\n", cnt_tx);
                        send_control();
                    }
                }
            }else {
                cnt_rx++;
                printf("Data packet arrived: %d\r\n", cnt_rx);
                ll_packet = (link_layer_packet_t *) packet.fields.payload;
                printf("Len: %d Attribs: %d\r\n", ll_packet->fields.len, ll_packet->fields.attribs);
                send_control();
            }
        }
    }
}

void send_and_wait_packet(void)
{
    comms_hk_data_t *data;
    simple_link_packet_t packet;
    simple_link_control_t control;
    link_layer_packet_t ll_packet_buffer;
    link_layer_packet_t *ll_packet;
    int i, ret;
    for (i = 0; i < 100; i++) {
        send_packet();
        while (read_port(&serial) > 0) {
            if( (ret = get_simple_link_packet(serial.buffer[0], &control, &packet)) > 0) {
                if (packet.fields.config1 != 0) {
                    if (packet.fields.config1 == 1) {
                        data = (comms_hk_data_t *) packet.fields.payload;
                        printf( "External Temp: %d, Internal Temp: %d, Bus Volt: %d\r\n"
                                "Phy RX: %d, RXErr: %d, Phy TX: %d, TXErr: %d\r\n"
                                "LL RX: %d LL TX:%d\r\n"
                                "Last RSSI: %f\r\n",
                                data->ext_temp, data->int_temp, data->bus_volt,
                                data->phy_rx_packets, data->phy_rx_errors, data->phy_tx_packets, data->phy_failed_packets,
                                data->ll_rx_packets, data->ll_tx_packets,
                                data->last_rssi);
                        printf("Free Stack: %d %d %d %d %d %d\r\n",
                                    data->free_stack[0], data->free_stack[1], data->free_stack[2],
                                    data->free_stack[3], data->free_stack[4], data->free_stack[5]);
                    }else {
                        if (packet.fields.config1 == 2) {
                            cnt_tx++;
                            printf("Packet has been sent (%d sent packets)\r\n", cnt_tx);
                            send_control();
                        }
                    }
                }else {
                    cnt_rx++;
                    printf("Data packet arrived: %d\r\n", cnt_rx);
                    ll_packet = (link_layer_packet_t *) packet.fields.payload;
                    printf("Len: %d Attribs: %d\r\n", ll_packet->fields.len, ll_packet->fields.attribs);
                    send_control();
                }
            }
        }
    }
}

void wait_and_retransmit_packet(void)
{
    comms_hk_data_t *data;
    simple_link_packet_t packet;
    simple_link_control_t control;
    link_layer_packet_t ll_packet_buffer;
    link_layer_packet_t *ll_packet;
    int ret;
    uint32_t elapsed_time;
    uint32_t timer = time(NULL);
    while (read_port(&serial) > 0) {
        if( (ret = get_simple_link_packet(serial.buffer[0], &control, &packet)) > 0) {
            if (packet.fields.config1 != 0) {
                if (packet.fields.config1 == 1) {
                    data = (comms_hk_data_t *) packet.fields.payload;
                    printf( "External Temp: %d, Internal Temp: %d, Bus Volt: %d\r\n"
                            "Phy RX: %d, RXErr: %d, Phy TX: %d, TXErr: %d\r\n"
                            "LL RX: %d LL TX:%d\r\n"
                            "Last RSSI: %f\r\n",
                            data->ext_temp, data->int_temp, data->bus_volt,
                            data->phy_rx_packets, data->phy_rx_errors, data->phy_tx_packets, data->phy_failed_packets,
                            data->ll_rx_packets, data->ll_tx_packets,
                            data->last_rssi);
                    printf("Free Stack: %d %d %d %d %d %d\r\n",
                                data->free_stack[0], data->free_stack[1], data->free_stack[2],
                                data->free_stack[3], data->free_stack[4], data->free_stack[5]);
                }else {
                    if (packet.fields.config1 == 2) {
                        cnt_tx++;
                        printf("Packet has been sent (%d sent packets)\r\n", cnt_tx);
                        send_control();
                    }
                }
            }else {
                cnt_rx++;
                printf("Data packet arrived: %d\r\n", cnt_rx);
                ll_packet = (link_layer_packet_t *) packet.fields.payload;
                printf("Len: %d Attribs: %d\r\n", ll_packet->fields.len, ll_packet->fields.attribs);
                send_packet();
            }
        }
    }
    elapsed_time = time(NULL) - timer;
    if (elapsed_time < 10) {
        sleep(10 - elapsed_time);
    }
}

int main(int argc, char ** argv)
{
    int cnt = 0;
    char dev_name[64];
    if (argc == 3){
        strcpy(dev_name, argv[1]);
        opt = argv[2][0];
    }else{
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    if (opt != 't' && opt != 'r' && opt != 'x' && opt != 'w') {
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    begin(dev_name, B115200, 500, &serial);
    /* Clear the input */
    clear(&serial);
    if (opt == 't') {           /* packet sending */
        while(cnt++ < 100) {
            printf("Sending packet: %d\r\n", cnt);
            send_packet();
            receive_and_print();
            sleep(5);
        }
    }else if (opt == 'r') {     /* packet receiving */
        while(1) {
            receive_and_print();
        }
    }else if (opt == 'x') {     /* packet retransmit (rx) */
        while(1) {
            wait_and_retransmit_packet();
        }
    }else {
        while(++cnt < 100) {    /* packet transmit and wait transmit */
            printf("Sending packet: %d\r\n", cnt);
            send_and_wait_packet();
        }
    }
    close(serial.fd);
}
#endif
