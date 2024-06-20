#include "client.h"


struct sockaddr socket_address;
struct sockaddr_in source,dest;
unsigned char *send_buffer;


int main() {
    char *device = "lo";
    int sockfd = connect_raw_socket(device);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    packet_t packet;
    receive_packet(sockfd, &packet);
    close(sockfd);

    return 0;
}