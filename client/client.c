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

void show_menu(){
    printf("Menu:\n");
    printf("1 - Listar arquivos\n");
    printf("2 - Baixar arquivo\n");
    printf("3 - Printar arquivo\n");
    printf("4 - Descritor de arquivo\n");
    printf("5 - Dados de arquivo\n");
    printf("6 - Fim\n");
    printf("Escolha uma opção: ");
}

int main() {
    init_client("lo");
    while (1)
    {
        show_menu();
        int choice;
        scanf("%d", &choice);
        packet_t pkt;
        build_packet(&pkt, 0, 0, "Hello, world!", strlen("Hello, world!"));
        send_packet(connection.socket, &pkt, &connection.address);
        switch (choice)
        {
            case 1:
                printf("\n\nListing files...\n");
                // Call the function to list files
                break;
            case 2:
                printf("\n\nDownloading file...\n");
                // Call the function to download file
                break;
            case 3:
                printf("\n\nPrinting file...\n");
                // Call the function to print file
                break;
            case 4:
                printf("\n\nFile descriptor...\n");
                // Call the function to get file descriptor
                break;
            case 5:
                printf("\n\nFile data...\n");
                // Call the function to get file data
                break;
            case 6:
                printf("\n\nExiting...\n");
                return EXIT_SUCCESS;
            default:
                printf("\n\nInvalid choice. Please try again.\n");
                break;
        }
    }
    return EXIT_SUCCESS;
}

