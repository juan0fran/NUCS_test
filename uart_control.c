#include "uart_control.h"

static int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static void set_mincount(int fd, int mcount, int timeout)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    /* if mcount == 0, timed read with timeout ms */
    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = timeout;        /* timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}


/*
 * Methods from Arduino compatible:
 *  - begin(port)
 *  - available(port)
 *  - readBytesUntil(port)
 */

/* UART port initialization */
void begin(const char * device, int baud, unsigned int timeout_ms, serial_parms_t * handler)
{
    handler->fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (handler->fd < 0) {
        printf("Error opening %s: %s\n", device, strerror(errno));
        handler->ret = -1;
        return;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(handler->fd, baud);
    /* Convert milli-seconds to deca-seconds */
    if (timeout_ms == 0){
        /* pure non-blocking */
        set_mincount(handler->fd, 0, 0);                /* set to pure timed read */
    }else if (timeout_ms < 100){
        /* 100ms blocking */
        set_mincount(handler->fd, 0, 1);                /* set to pure timed read */
    }else{
        /* specified timeout ms */
        set_mincount(handler->fd, 0, timeout_ms/100);   /* set to pure timed read */
    }
    handler->timeout = timeout_ms;
    return;
}


int available(serial_parms_t * input_handler)
{
    int bytes_avail;
    ioctl(input_handler->fd, FIONREAD, &bytes_avail);
    return bytes_avail;
}

int read_port(serial_parms_t * input_handler)
{
    return read(input_handler->fd, input_handler->buffer, 1);
}

void clear (serial_parms_t * input_handler)
{
    if(available(input_handler) > 0)
    {
        while(read_port(input_handler) > 0);
    }
}

int readBytesUntil(serial_parms_t * input_handler, char to_find, char * buffer, int max_size)
{
    int cnt = 0;
    if(available(input_handler) > 0){
        while (read_port(input_handler) > 0){
            /* keep reading */
            buffer[cnt] = input_handler->buffer[0];
            if ((char) buffer[cnt] == to_find){
                cnt++;
                return cnt;
            }else{
                cnt++;
                if (cnt >= max_size)
                    return max_size;
            }
        }
    }
    return 0;
}
#if 0
serial_parms_t serial;

#define PREDEFINED_TEST
#ifndef PREDEFINED_TEST
void * send_thread(void *args)
{
    simple_link_packet_t packet;
    simple_link_control_t control;
    link_layer_packet_t ll_packet_buffer;
    link_layer_packet_t *ll_packet;
    int control_byte;
    uint8_t buffer[128];
    int i;
    int cnt, ret;
    cnt = 0;
    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            if (buffer[0] == '0') {
                control_byte = 0;
            }else {
                control_byte = 1;
            }
        }
        ll_packet = (link_layer_packet_t *) &ll_packet_buffer;
        for (i = 0; i < 1500; i++) {
            ll_packet->fields.payload[i] = i % 256;
        }
        ll_packet->fields.len = 1500;
        ll_packet->fields.attribs = 9600;
        ll_packet->fields.crc = 0;
        if (control_byte == 1) {
            if ( (ret = set_simple_link_packet(NULL, 0, control_byte, 0, &packet) ) > 0) {
                cnt++;
                //printf("[PC]: Sending LL packet through Kiss: %d bytes. Cnt: %d\n", ret, cnt);
                if (send_kiss_packet(serial.fd, &packet, ret) == -1) {
                    printf("Not working!\n");
                }
            }
        }else {
            if ( (ret = set_simple_link_packet(ll_packet, ll_packet->fields.len + LINK_LAYER_HEADER_SIZE, control_byte, 4, &packet) ) > 0) {
                cnt++;
                //printf("[PC]: Sending LL packet through Kiss: %d bytes. Cnt: %d\n", ret, cnt);
                if (send_kiss_packet(serial.fd, &packet, ret) == -1) {
                    printf("Not working!\n");
                }
            }
        }
    }
}
#else
void * send_thread(void *args)
{
    simple_link_packet_t packet;
    simple_link_control_t control;
    link_layer_packet_t ll_packet_buffer;
    link_layer_packet_t *ll_packet;
    int control_byte;
    uint8_t buffer[128];
    int i;
    int cnt, ret;
    cnt = 0;
    while (++cnt < 100) {
        if ( (ret = set_simple_link_packet(NULL, 0, 1, 0, &packet) ) > 0) {
            if (send_kiss_packet(serial.fd, &packet, ret) == -1) {
                printf("Not working!\n");
            }
        }
        ll_packet = (link_layer_packet_t *) &ll_packet_buffer;
        for (i = 0; i < 1500; i++) {
            ll_packet->fields.payload[i] = i % 256;
        }
        ll_packet->fields.len = 1500;
        ll_packet->fields.attribs = 9600;
        ll_packet->fields.crc = 0;
        if ( (ret = set_simple_link_packet(ll_packet, ll_packet->fields.len + LINK_LAYER_HEADER_SIZE, control_byte, 4, &packet) ) > 0) {
            if (send_kiss_packet(serial.fd, &packet, ret) == -1) {
                printf("Not working!\n");
            }
        }
        /* wait 6 seconds */
        sleep(6);
    }
    printf("End of test\n");
}
#endif

void * receive_thread(void *args)
{
    comms_hk_data_t *data;
    link_layer_packet_t *ll_packet;
    simple_link_packet_t packet;
    simple_link_control_t control;
    int cnt = 0;
    uint8_t buffer[1500];
    int ret;
    int i;
    prepare_simple_link(&control);
    for (i = 0; i < 1500; i++){
      buffer[i] = i%256;
    }
    while(1) {
        while (read_port(&serial) > 0) {
            //printf("%c", serial.buffer[0]);
            if( (ret = get_simple_link_packet(serial.buffer[0], &control, &packet)) > 0) {
                //printf("Packet received with Config: %d %d\n", packet.fields.config1, packet.fields.config2);
                if (packet.fields.config1 != 0) {
                    if (packet.fields.config1 == 1) {
                        data = (comms_hk_data_t *) packet.fields.payload;
                        /*printf( "External Temp: %d, Internal Temp: %d, Bus Volt: %d\r\n"
                                "Phy RX: %d, Phy RXErr: %d, Phy TX: %d, No TX: %d\r\n"
                                "LL RX: %d LL TX:%d\r\n"
                                "Last RSSI: %f\r\n",
                                data->ext_temp, data->int_temp, data->bus_volt,
                                data->phy_rx_packets, data->phy_rx_errors, data->phy_tx_packets, data->phy_failed_packets,
                                data->ll_rx_packets, data->ll_tx_packets,
                                data->last_rssi);*/
                        printf("Phy TX: %d, No TX: %d, LL TX:%d\r\n",
                                data->phy_tx_packets, data->phy_failed_packets, data->ll_tx_packets);
                        /*printf("Free Stack: %d %d %d %d %d %d\r\n",
                                    data->free_stack[0], data->free_stack[1], data->free_stack[2],
                                    data->free_stack[3], data->free_stack[4], data->free_stack[5]);*/
                    }else {
                        //printf("Packet config received: %d\n", packet.fields.config1);
                        if (packet.fields.config1 == 2) {
                            cnt++;
                            printf("%d sent packets\n", cnt);
                        }
                    }
                }else {
                    printf("Data packet arrived\r\n");
                    ll_packet = (link_layer_packet_t *) packet.fields.payload;
                    printf("Len: %d Attribs: %d\r\n", ll_packet->fields.len, ll_packet->fields.attribs);
                }
            }
        }
    }
}

int main(int argc, char ** argv)
{
    char dev_name[64];
    char opt;
    if (argc == 3){
        strcpy(dev_name, argv[1]);
        opt = argv[2][0];
    }else{
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    if (opt != 't' && opt != 'r') {
        printf("Bad input sintax, specify ./prog_name /dev/...\n");
        exit( -1 );
    }
    begin(dev_name, B115200, 500, &serial);
    /* Clear the input */
    clear(&serial);
    /*
    if (opt == 't') {
        blind_test_tx();
    }else {
        blind_test_rx();
    }
    close(serial.fd);
    return 0;*/

    pthread_t threads[2];
    pthread_create(&threads[0], NULL, send_thread, NULL);
    pthread_create(&threads[1], NULL, receive_thread, NULL);
    while(1);
    close(serial.fd);
}
#endif
