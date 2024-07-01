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

#define DATA_LEN 63

typedef enum {
    RECEIVING = 1,
    SENDING,
} state_t;


#define STARTER_MARK 0x7E
typedef struct {
    uint8_t starter_mark: 8;
    uint8_t size: 6;
    uint8_t seq_num: 5;
    uint8_t type: 5;
    uint8_t data[DATA_LEN];
    uint8_t crc: 8;
} packet_t;

typedef union {
    packet_t packet;
    uint8_t raw[sizeof(packet_t)];
} packet_union_t;

typedef struct {
    state_t state;
    int socket;
    struct sockaddr_ll address;
} connection_t;

typedef struct {
    char *name; // file name
    char *path; // absolute path to video file
    int size; // in bytes
} video_t;

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

uint8_t calculate_crc8(const uint8_t *data, size_t len);
int ConexaoRawSocket(char *device);
int listen_socket(int _socket, packet_t *packet);

video_t *init_video_t();

ssize_t send_packet(int sock, packet_t *packet, struct sockaddr_ll *address);
void receive_packet(int sock, packet_t *packet);

// Função para enviar mensagens
ssize_t send_message(int socket, packet_t *packet);

#endif // SOCKET_H
