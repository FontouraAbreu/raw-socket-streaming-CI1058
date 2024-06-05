#include "socket.h"

int connect_raw_socket(char *interface) {
    int raw_socket;
    struct ifreq ifr;

    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (raw_socket < 0) {
        perror("socket()");
        return -1;
    }

    /* Get the index of the network device */
    memset(&ifr, 0, sizeof(ifr));
    /* Set the device name */
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);

    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl()");
        close(raw_socket);
        return -1;
    }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifr.ifr_ifindex;

    if (bind(raw_socket, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind()");
        close(raw_socket);
        return -1;
    }

    return raw_socket;
}