#include "server.h"
#include "../utils/utils.h"

#define DEBUG

void build_packet(packet_t *pkt, uint8_t seq_num, uint8_t type, const uint8_t *data, size_t data_len)
{
    pkt->starter_mark = 0xAA;
    pkt->size = data_len;
    pkt->seq_num = seq_num;
    pkt->type = type;
    memset(pkt->data, 0, DATA_LEN);
    memcpy(pkt->data, data, data_len);
    pkt->crc = calculate_crc8((uint8_t *)pkt, sizeof(packet_t) - 1);
}

int main()
{
    const char *interface = "enp6s0";
    int raw_socket = create_stream_socket(0);
    printf("Socket: %d\n", raw_socket);
    if (raw_socket < 0)
    {
        fprintf(stderr, "Failed to create and configure raw socket on interface %s\n", interface);
        return EXIT_FAILURE;
    }

    struct ifreq ifr;

    // Get the interface index
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) == -1)
    {
        perror("ioctl");
        close(raw_socket);
        return EXIT_FAILURE;
    }

    // Bind the socket to the interface
    struct sockaddr_ll addr = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_ALL),
        .sll_ifindex = ifr.ifr_ifindex,
    };
    if (bind(raw_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(raw_socket);
        return EXIT_FAILURE;
    }

    while (1)
    {
        /* code */
    }

    return EXIT_SUCCESS;
}
