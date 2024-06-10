#include "client.h"


struct sockaddr socket_address;
struct sockaddr_in source,dest;
unsigned char *send_buffer;



int main() {
    const char *interface = "lo";
    int raw_socket = connect_raw_socket(interface);

    if (raw_socket < 0) {
        fprintf(stderr, "Failed to create and configure raw socket on interface %s\n", interface);
        return EXIT_FAILURE;
    }

    kermit_frame_t packet;
    struct sockaddr_ll src_addr;
    socklen_t src_addr_len = sizeof(src_addr);

    while (1) {
        ssize_t num_bytes = recvfrom(raw_socket, &packet, sizeof(packet), 0, (struct sockaddr *)&src_addr, &src_addr_len);
        if (num_bytes < 0) {
            perror("recvfrom");
            close(raw_socket);
            return EXIT_FAILURE;
        }

        uint8_t calculated_crc = calculate_crc8((uint8_t *)&packet, sizeof(packet) - 1);
        if (packet.starter_mark == 0xAA && packet.crc == calculated_crc) {
            printf("Received valid packet: ");
            for (int i = 0; i < packet.size; ++i) {
                printf("%c", packet.data[i]);
            }
            printf("\n");
        } else {
            printf("Received invalid packet\n");
        }
    }

    return EXIT_SUCCESS;
}