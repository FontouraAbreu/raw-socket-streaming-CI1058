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
                    printf("Interface: %s\n", tmp->ifa_name);
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
