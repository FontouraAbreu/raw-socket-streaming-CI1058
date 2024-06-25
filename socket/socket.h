#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PACKET_LEN 16
#define DATA_LEN 8

typedef struct {
    uint8_t starter_mark;
    uint8_t size: 6;
    uint8_t seq_num: 5;
    uint8_t type: 5;
    uint8_t data[DATA_LEN];
    uint8_t crc;
} kermit_frame_t;

// Tipo das mensagens (5 bits)
#define ACK 0 // bx00000
#define NACK 1 // bx00001
#define LISTAR 10 // bx01010
#define BAIXAR 11 // bx01011
#define PRINTAR 16 // bx10000
#define DESCRITOR 17 // bx10001
#define DADOS 18 // bx10010
#define FIM 30 // bx11110
#define ERRO 31 // bx11111

int create_stream_socket(int interface);

uint8_t calculate_crc8(const uint8_t *data, size_t len);

#endif // SOCKET_H
