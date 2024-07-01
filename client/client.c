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

int main(int argc, char **argv) {
    /* connects to the server */
    char *interface = parse_args(argc, argv, "i:");
    int sockfd = connect_raw_socket(interface);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    /* connects to the server */

    int opcao_escolhida = show_menu();
    while (opcao_escolhida != 2){
        // Deals with the user's choice
        switch (opcao_escolhida){
            case 1:
                printf("Listando videos...\n");

                break;
            case 2:
                printf("Saindo...\n");
                break;
            default:
                printf("Opcao invalida\n");
                break;
        }
        opcao_escolhida = show_menu();
    }

    packet_t packet;
    receive_packet(sockfd, &packet);
    close(sockfd);

    return 0;
}




/*
* Função que lista os videos disponíveis no servidor
* Retorna um ponteiro para um array de video_t
*/
video_t *get_videos() {
    packet_t packet;
    packet.starter_mark = STARTER_MARK;
    packet.size = 0;
    packet.seq_num = 0;
    packet.type = LISTAR;
    packet.data[0] = '\0';
    packet.crc = calculate_crc8((uint8_t *)&packet, sizeof(packet_t) - 1);
}

int show_menu(){
    printf("1. Listar videos\n");
    printf("2. Baixar videos\n");
    printf("3. Sair\n");
    printf("Escolha uma opcao: ");
    int opcao;
    scanf("%d", &opcao);
    return opcao;
}
