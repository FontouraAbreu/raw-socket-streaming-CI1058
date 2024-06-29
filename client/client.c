#include "client.h"


connection_t connection;

void init_client(char *interface){
    connection.state = SENDING;
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

int main() {
    init_client("lo");

    while (1)
    {
        packet_t pkt;
        build_packet(&pkt, 0, 0, "Hello, world!", strlen("Hello, world!"));
        send_packet(connection.socket, &pkt, &connection.address);
    }

    return EXIT_SUCCESS;
}
