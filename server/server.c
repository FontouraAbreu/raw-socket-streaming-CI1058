#include "server.h"

#define DEBUG

void build_packet(kermit_frame_t *pkt, uint8_t seq_num, uint8_t type, const uint8_t *data, size_t data_len) {
    pkt->starter_mark = 0xAA;
    pkt->size = data_len;
    pkt->seq_num = seq_num;
    pkt->type = type;
    memset(pkt->data, 0, DATA_LEN);
    memcpy(pkt->data, data, data_len);
    pkt->crc = calculate_crc8((uint8_t *)pkt, sizeof(kermit_frame_t) - 1);
}

int main() {
    const char *interface = "enp4s0";
    const char *dest_mac_str = "ff:ff:ff:ff:ff:ff";  // Broadcast or specific MAC
    int raw_socket = connect_raw_socket(interface);

    if (raw_socket < 0) {
        fprintf(stderr, "Failed to create and configure raw socket on interface %s\n", interface);
        return EXIT_FAILURE;
    }

    struct ifreq ifr;

    // Get the interface index
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) == -1) {
        perror("ioctl");
        close(raw_socket);
        return EXIT_FAILURE;
    }

    struct sockaddr_ll dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sll_family = AF_PACKET;
    dest_addr.sll_protocol = htons(ETH_P_ALL);
    dest_addr.sll_ifindex = ifr.ifr_ifindex;
    dest_addr.sll_halen = ETH_ALEN; // Length of destination address
    dest_addr.sll_pkttype = PACKET_HOST; // Packet type

 // Convert MAC address string to binary
    sscanf(dest_mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &dest_addr.sll_addr[0], &dest_addr.sll_addr[1], &dest_addr.sll_addr[2], 
           &dest_addr.sll_addr[3], &dest_addr.sll_addr[4], &dest_addr.sll_addr[5]);

    kermit_frame_t packet;
    uint8_t data[DATA_LEN] = "Hello";
    build_packet(&packet, 1, 1, data, strlen((char *)data));

    #ifdef DEBUG
        printf("Packet to be sent:\n");
        printf("Starter mark: 0x%02X\n", packet.starter_mark);
        printf("Size: %d\n", packet.size);
        printf("Sequence number: %d\n", packet.seq_num);
        printf("Type: %d\n", packet.type);
        printf("Data: %s\n", packet.data);
        printf("CRC: 0x%02X\n", packet.crc);
    #endif


    ssize_t sent_size = sendto(raw_socket, &packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (sent_size < 0) {
        perror("sendto");
        close(raw_socket);
        return EXIT_FAILURE;
    }

    printf("Packet sent successfully, sent %zd bytes.\n", sent_size);
    close(raw_socket);
    return EXIT_SUCCESS;
}