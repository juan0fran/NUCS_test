 #include "uart_control.h"

#define PRINT_INFO 1
#define STORE_INFO 1

#define BLIND_TEST_DURATION         1000
#define RETRANSMIT_TEST_DURATION    500

static char prefix_path[128];
#define COMMS_HK_DATA "comms_hk_file.txt"
#define COMMS_TEST_DATA "comms_test_file.txt"

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
    for (i = 0; i < 1740; i++) {
        ll_packet->fields.payload[i] = i % 256;
    }
    ll_packet->fields.len = 1740;
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

void store_test_info()
{
    FILE *fp;
    char path[256];
    #if STORE_INFO
    if (prefix_path[0] == '\0') {
        return;
    }
    sprintf(path, "%s_%s", prefix_path, COMMS_TEST_DATA);
    fp = fopen(path, "a+");
    if (fp != NULL) {
        fprintf(fp, "%d,%d\r\n", test_statistics.sent_packets, test_statistics.received_packets);
    }else {
        perror("fopen store data");
        exit(1);
    }
    fclose(fp);
    #endif
}

#if 0
void store_info(comms_hk_data_t *data)
{
    FILE *fp;
    char path[256];
    #if STORE_INFO
    if (prefix_path[0] == '\0') {
        return;
    }
    sprintf(path, "%s_%s", prefix_path, COMMS_HK_DATA);
    fp = fopen(path, "a+");
    if (fp != NULL) {
        fprintf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f,%d,%d,%d,%d\r\n",
                data->ext_temp, data->int_temp, data->bus_volt,
                data->phy_rx_packets, data->phy_rx_errors, data->phy_tx_packets, data->phy_tx_failed_packets,
                data->ll_rx_packets, data->ll_tx_packets,
                data->actual_rssi, data->last_rssi, data->last_lqi,
                data->free_stack[0], data->free_stack[1], data->free_stack[2], data->free_stack[3]);
    }else {
        perror("fopen store info");
        exit(1);
    }
    fclose(fp);
    #endif
}
#else
void store_info(comms_hk_data_t *data)
{
    /* do nothing */
}
#endif

int receive_control(comms_hk_data_t *data)
{
    simple_link_packet_t packet;
    simple_link_control_t control;
    int ret;
    prepare_simple_link(&control);

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
    prepare_simple_link(&control);

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
    while(test_statistics.sent_packets < BLIND_TEST_DURATION) {
        send_control();
        if (receive_control(&data) == 1) {
            if (data.control.tx_remaining > 0) {
                send_packet();
                if ( receive_control(&data) == 2) {
                    #if PRINT_INFO
                    printf("Control packet information --> \t");
                    printf("Temperatures: %f, %f, ",
                                convert_temp_u16_f(data.housekeeping.ext_temp),
                                convert_temp_u16_f(data.housekeeping.int_temp));
                    printf("Free Stack: %d %d %d %d\r\n",
                                data.control.free_stack[0], data.control.free_stack[1],
                                data.control.free_stack[2], data.control.free_stack[3]);
                    #endif
                    test_statistics.sent_packets++;
                    printf("Packet %d sent correctly\r\n", test_statistics.sent_packets);
                    store_test_info();
                }
            }else {
                printf("Queue is full\r\n");
            }
        }
        sleep(4);
    }
}

void receive_routine(void)
{
    link_layer_packet_t packet;
    comms_hk_data_t data;
    while(test_statistics.received_packets < BLIND_TEST_DURATION) {
        send_control();
        if (receive_control(&data) == 1) {
            if (data.control.rx_queued > 0) {
                send_req();
                if (receive_frame(&packet) > 0) {
                    #if PRINT_INFO
                    printf("Control packet information --> \t");
                    printf("Temperatures: %f, %f, ",
                                convert_temp_u16_f(data.housekeeping.ext_temp),
                                convert_temp_u16_f(data.housekeeping.int_temp));
                    printf("Last LQI: %0.2f, ", lqi_status(data.housekeeping.last_lqi));
                    printf("Last RSSI: %0.2f, ", rssi_lna_dbm(data.housekeeping.last_rssi));
                    printf("Actual RSSI: %0.2f, SNR = %f, ",
                            rssi_lna_dbm(data.housekeeping.actual_rssi),
                            rssi_lna_dbm(data.housekeeping.last_rssi) - rssi_lna_dbm(data.housekeeping.actual_rssi));
                    printf("Free Stack: %d %d %d %d\r\n",
                                data.control.free_stack[0], data.control.free_stack[1],
                                data.control.free_stack[2], data.control.free_stack[3]);
                    #endif
                    test_statistics.received_packets++;
                    printf("New packet received --> \tReceived: %d bytes packet. Total Count: %d\r\n",
                                                    packet.fields.len, test_statistics.received_packets);
                    store_test_info();
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
    while(test_statistics.sent_packets < RETRANSMIT_TEST_DURATION) {
        send_control();
        if (receive_control(&data) == 1) {
            if (data.control.rx_queued > 0) {
                send_req();
                if (receive_frame(&packet) > 0) {
                    #if PRINT_INFO
                    printf("Control packet information --> \t");
                    printf("Temperatures: %f, %f, ",
                                convert_temp_u16_f(data.housekeeping.ext_temp),
                                convert_temp_u16_f(data.housekeeping.int_temp));
                    printf("Last LQI: %0.2f, ", lqi_status(data.housekeeping.last_lqi));
                    printf("Last RSSI: %0.2f, ", rssi_lna_dbm(data.housekeeping.last_rssi));
                    printf("Actual RSSI: %0.2f, SNR = %f, ",
                            rssi_lna_dbm(data.housekeeping.actual_rssi), rssi_lna_dbm(data.housekeeping.last_rssi) - rssi_lna_dbm(data.housekeeping.actual_rssi));
                    printf("Free Stack: %d %d %d %d\r\n",
                                data.control.free_stack[0], data.control.free_stack[1],
                                data.control.free_stack[2], data.control.free_stack[3]);
                    #endif
                    test_statistics.received_packets++;
                    printf("New packet received --> \tReceived: %d bytes packet. Total Count: %d\r\n",
                                                    packet.fields.len, test_statistics.received_packets);
                }
                /* send a TX */
                if (data.control.tx_remaining > 0) {
                    send_packet();
                    if ( receive_control(&data) == 2) {
                        test_statistics.sent_packets++;
                        printf("Packet %d sent correctly\r\n", test_statistics.sent_packets);
                    }
                }
                store_test_info();
            }
        }else {
            printf("Receive control != 1\n");
        }
        sleep(1);
    }
}

void send_and_receive_routine(void)
{
    link_layer_packet_t packet;
    comms_hk_data_t data;
    uint32_t start;
    while(test_statistics.sent_packets < RETRANSMIT_TEST_DURATION) {
        send_control();
        if (receive_control(&data) == 1) {
            if (data.control.tx_remaining > 0) {
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
            }while(data.control.rx_queued == 0 && (time(NULL) - start) < 8);
            if (data.control.rx_queued > 0) {
                send_req();
                if (receive_frame(&packet) > 0) {
                    #if PRINT_INFO
                    printf("Control packet information --> \t");
                    printf("Temperatures: %f, %f, ",
                                convert_temp_u16_f(data.housekeeping.ext_temp),
                                convert_temp_u16_f(data.housekeeping.int_temp));
                    printf("Last LQI: %0.2f, ", lqi_status(data.housekeeping.last_lqi));
                    printf("Last RSSI: %0.2f, ", rssi_lna_dbm(data.housekeeping.last_rssi));
                    printf("Actual RSSI: %0.2f, SNR = %f, ",
                            rssi_lna_dbm(data.housekeeping.actual_rssi), rssi_lna_dbm(data.housekeeping.last_rssi) - rssi_lna_dbm(data.housekeeping.actual_rssi));
                    printf("Free Stack: %d %d %d %d\r\n",
                                data.control.free_stack[0], data.control.free_stack[1],
                                data.control.free_stack[2], data.control.free_stack[3]);
                    #endif
                    test_statistics.received_packets++;
                    printf("New packet received --> \tReceived: %d bytes packet. Total Count: %d\r\n",
                                                    packet.fields.len, test_statistics.received_packets);
                }
            }
            store_test_info();
        }
        sleep(8 - (time(NULL) - start));
    }
}

int main(int argc, char ** argv)
{
    comms_hk_data_t data;
    int cnt = 0;
    char dev_name[64];
    if (argc == 4){
        strcpy(dev_name, argv[1]);
        opt = argv[2][0];
        strcpy(prefix_path, argv[3]);
    }else if (argc == 3) {
        strcpy(dev_name, argv[1]);
        opt = argv[2][0];
        prefix_path[0] = '\0';
    }else{
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    if (opt != 't' && opt != 'r' && opt != 'x' && opt != 'w' && opt != 'p') {
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    begin(dev_name, B115200, 250, &serial);
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
                printf("Temperatures: %f, %f, ",
                            convert_temp_u16_f(data.housekeeping.ext_temp),
                            convert_temp_u16_f(data.housekeeping.int_temp));
                printf("Last LQI: %0.2f, ", lqi_status(data.housekeeping.last_lqi));
                printf("Last RSSI: %0.2f, ", rssi_lna_dbm(data.housekeeping.last_rssi));
                printf("Actual RSSI: %0.2f, SNR = %f, ",
                        rssi_lna_dbm(data.housekeeping.actual_rssi), rssi_lna_dbm(data.housekeeping.last_rssi) - rssi_lna_dbm(data.housekeeping.actual_rssi));
                printf("Free Stack: %d %d %d %d\r\n",
                            data.control.free_stack[0], data.control.free_stack[1],
                            data.control.free_stack[2], data.control.free_stack[3]);
                printf("Control -> %d packets in rx\n", data.control.rx_queued);
            }
            //sleep(1);
            exit(1);
        }
    }
    close(serial.fd);
}
