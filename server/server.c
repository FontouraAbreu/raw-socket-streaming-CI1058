#include "server.h"
#include "../utils/utils.h"

#define DEBUG

connection_t connection;

void init_server(char *interface){
    connection.state = RECEIVING;
    connection.socket = ConexaoRawSocket(interface);

    // Destination MAC address (example, use the actual destination address)
    uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast address for example

    // Prepare sockaddr_ll structure
    memset(&connection.address, 0, sizeof(connection.address));
    connection.address.sll_family = AF_PACKET;
    connection.address.sll_protocol = htons(ETH_P_ALL);
    connection.address.sll_ifindex = if_nametoindex(interface);
    connection.address.sll_halen = ETH_ALEN;
    memcpy(connection.address.sll_addr, dest_mac, ETH_ALEN);
}

int main()
{
    init_server("lo");


    while (1)
    {
        packet_t pkt;
        listen_socket(connection.socket, &pkt);

        if (pkt.starter_mark == STARTER_MARK)
        {
            printf("Received packet: %s\n", pkt.data);
        }
    }

    return EXIT_SUCCESS;
}
