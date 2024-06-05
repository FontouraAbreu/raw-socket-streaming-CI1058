#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


struct kermit_frame_t {
    unsigned start;
    unsigned size;
    unsigned seq;
    unsigned type;
    unsigned data[64];
    unsigned crc_8;
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


#endif // SOCKET_H