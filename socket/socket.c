#include "socket.h"

#define TEST

int connect_raw_socket(const char *interface) {
    int raw_socket;

    /* Create the raw socket */
    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (raw_socket < 0) {
        perror("socket() - AF_PACKET");
        return -1;
    }

    /* Get the index of the network interface */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);

    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl() - SIOCGIFINDEX");
        return -1;
    }

    /* Bind the raw socket to the network interface */
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_halen = ETH_ALEN;

    if (bind(raw_socket, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind() - AF_PACKET");
        return -1;
    }

    return raw_socket;
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