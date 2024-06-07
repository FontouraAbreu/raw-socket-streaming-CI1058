#include "socket.h"

int connect_raw_socket(char *interface) {
    int raw_socket;

    /* Create the raw socket */
    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (raw_socket < 0) {
        perror("[Warning]: connect_raw_socket(): ");
        return -1;
    }

    struct ifreq ifr;
    /* Get the index of the network device */
    memset(&ifr, 0, sizeof(ifr));
    /* Set the device name */
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);
    /* Get the index */
    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl()");
        close(raw_socket);
        return -1;
    }

    /* Bind the raw socket to the network device */
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifr.ifr_ifindex;
    /* Bind the raw socket */
    if (bind(raw_socket, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind()");
        close(raw_socket);
        return -1;
    }

    struct packet_mreq mr;
    /* Set the raw socket to promiscuous mode */
    memset(&mr, 0, sizeof(mr));          /*Modo Promiscuo*/
    mr.mr_ifindex = ifr.ifr_ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(raw_socket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)	{
    printf("Erro ao fazer setsockopt\n");
    exit(-1);
  }


    return raw_socket;
}