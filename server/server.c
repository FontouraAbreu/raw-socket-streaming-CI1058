#include "server.h"

#define DEBUG

int main() {
    char *device = "lo";
    int sockfd = connect_raw_socket(device);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    packet_t packet = {0xAA, 5, 1, 1, "Hello", 0};
    send_packet(sockfd, &packet);

    close(sockfd);
    return 0;
}
