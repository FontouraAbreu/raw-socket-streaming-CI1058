#ifndef SOCKET_H
#define SOCKET_H
#define _DEFAULT_SOURCE // para usar o DT_REG e DT_DIR

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
#include <pthread.h>
#include <dirent.h>

#define VIDEO_LOCATION "./videos/"

#define DATA_LEN 63
#define STARTER_MARK 0x7E
#define DUPLICATE_PACKET -6

typedef enum {
    RECEIVING = 1,
    SENDING = -1,
} state_t;

typedef enum {
    NOT_FOUND = 0,
    FULL_DISK = 1,
} errors_type_t;

typedef enum {
    NOT_FOUND_MSG,
    FULL_DISK_MSG,
} errors_msg_t;

// const char *error_messages[] = {
//     "Vídeo não encontrado",
//     "Disco cheio",
// };

typedef struct {
    unsigned char starter_mark: 8;
    unsigned char size: 6;
    unsigned char seq_num: 5;
    unsigned char type: 5;
    unsigned char data[DATA_LEN];
    unsigned char crc: 8;
} packet_t;

typedef union {
    packet_t packet;
    unsigned char raw[sizeof(packet_t)];
} packet_union_t;

typedef struct {
    state_t state;
    int socket;
    struct sockaddr_ll address;
} connection_t;

typedef struct {
    char *name; // file name
    char *path; // absolute path to video file
    long size; // in bytes
    int duration; // in seconds
} video_t;

typedef struct {
    video_t *videos;
    int num_videos;
} video_list_t;

// Tipo das mensagens (5 bits)
#define ACK 0 // bx00000
#define NACK 1 // bx00001
#define LISTAR 10 // bx01010
#define BAIXAR 11 // bx01011
#define PRINTAR 16 // bx10000
#define DESCRITOR 17 // bx10001
#define TAMANHO 20 // bx10010
#define DURACAO 21 // bx10011
#define DADOS 18 // bx10010
#define ERRO_NAO_ENCONTRADO 27 // bx11011
#define ERRO_SEM_VIDEOS 28 // bx11100
#define INICIO_SEQ 29 // bx11101
#define FIM 30 // bx11110
#define ERRO 31 // bx11111


unsigned char calculate_crc8(const unsigned char *data, size_t len);
int ConexaoRawSocket(char *device);
int listen_socket(int _socket, packet_t *packet);

video_t *init_video_t();

void build_packet(packet_t *pkt, unsigned char seq_num, unsigned char type, unsigned char *data, size_t data_len);
ssize_t send_packet(int sock, packet_t *packet, struct sockaddr_ll *address, int *connection_state);
ssize_t send_packet_no_ack(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state);
void receive_packet(int sock, packet_t *packet, connection_t *connection);
void receive_packet_sequence(int sock, packet_t *packet, connection_t *connection, video_list_t *video_list);
void wait_for_init_sequence(int sock, packet_t *packet, connection_t *connection);
ssize_t send_init_sequence(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state);

// Função para enviar mensagens
ssize_t send_message(int socket, packet_t *packet);
ssize_t send_ack(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state);
ssize_t send_nack(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state);
void print_packet(packet_t *pkt);
int wait_for_ack_socket(int sockfd, packet_t *packet, struct sockaddr_ll *address, int *state);
int check_crc(packet_t *packet);

//Função para enviar um video
void send_video(int sock, packet_t *packet, connection_t *connection, char *video_path);
int receive_video_packet_sequence(int sock, packet_t *packet, connection_t *connection, const char *output_filename, int expected_size);

//retorna path do video selecionado
char* get_video_path(char *video_name);
void test_alloc(void *ptr, const char *msg);


#endif // SOCKET_H
