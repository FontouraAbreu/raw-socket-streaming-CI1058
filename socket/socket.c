#include "socket.h"
#include "ConexaoRawSocket.c"
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <unistd.h>
#define TEST

int create_stream_socket(int interface){
    int socket;
    struct ifaddrs *addrs, *tmp;

    if (interface) {
        socket = ConexaoRawSocket("lo");
    } else {
        getifaddrs(&addrs);
        tmp = addrs;

        while (tmp) {
            if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET) {
                if (tmp->ifa_name[0] == 'e' || tmp->ifa_name[0] == 'E') {
                    socket = ConexaoRawSocket(tmp->ifa_name);
                    break;
                }
            }
            tmp = tmp->ifa_next;
        }

        freeifaddrs(addrs);
    }

    return socket;
}

// CRC-8 calculation function
uint8_t calculate_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// ssize_t send_message( *backup) {
//     if (!backup)
//         return -1;

//     packet_t *m = &backup->send_buffer;

//     // Supondo que build_packet preenche m corretamente
//     build_packet(m, backup->sequence_number, 1, (const uint8_t*)"Hello", strlen("Hello"));

//     #ifdef DEBUG
//     printf("[ETHBKP][SDMSG] Sending message\n");
//     // print_packet(m);
//     #endif

//     ssize_t size;
//     int is_ack = 0;
//     int error = -1;

//     while (!is_ack) {
//         size = send(backup->socket, m, sizeof(packet_t), 0);

//         is_ack = wait_ack_or_error(backup, &error); // Esta função deve verificar se recebeu ACK ou erro

//         #ifdef DEBUG
//         printf("[ETHBKP][SNDMSG] Message sent, is_ack=%d\n\n", is_ack);
//         #endif
//     }

//     // Atualiza a sequência para o próximo número
//     backup->sequence_number++;

//     return size;
// }
