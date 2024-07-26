#include "client.h"

connection_t connection;

typedef enum {
    LIST = 1,
    DOWNLOAD = 2,
    QUIT = 3,
} states_client_t;

void init_client(char *interface)
{
    connection.state = SENDING;
    connection.socket = ConexaoRawSocket(interface);

    // Destination MAC address (example, use the actual destination address)
    uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast address for example

    // Prepare sockaddr_ll structure
    memset(&connection.address, 0, sizeof(connection.address));
    connection.address.sll_family = AF_PACKET;
    connection.address.sll_protocol = htons(ETH_P_ALL);
    connection.address.sll_ifindex = if_nametoindex(interface);
    printf("ifindex: %d\n", connection.address.sll_ifindex);
    connection.address.sll_halen = ETH_ALEN;
    memcpy(connection.address.sll_addr, dest_mac, ETH_ALEN);
}

int main(int argc, char **argv)
{
    /* connects to the server */
    // char *interface = parse_args(argc, argv, "i:");
    init_client("lo");

    int op = show_menu();
    while (
    {
        switch (op)
        {
        case 1:
            video_t *videos = get_videos();

            packet_t packet;
            receive_packet_sequence(connection.socket, &packet, &connection);

            // Check if packet sequence number is valid
            // if (packet.seq_num <= last_received_seq_num)
            // {
            //     printf("Número de sequência menor que o último pacote recebido. Parando recepção.\n");
            //     break;
            // }

            // Process the received packet
            if (packet.type == ERRO)
            {
                printf("Erro ao listar videos\n");
                break;
            }

            if (packet.type != ACK)
                print_packet(&packet);

            continue;

        case 2:
            printf("Baixando videos...\n");
            break;
        case 3:
            printf("Saindo...\n");
            packet_t packet_end;
            build_packet(&packet_end, 0, FIM, NULL, 0);

            send_packet(connection.socket, &packet_end, &connection.address, &connection.state);
            break;
        default:
            printf("Opcao invalida\n");
            break;
        }
        op = show_menu();
    }

    packet_t packet;
    // receive_packet(sockfd, &packet);
    // close(sockfd);

    return 0;
}

/*
 * Função que lista os videos disponíveis no servidor
 * Retorna um ponteiro para um array de video_t
 */
video_t *get_videos()
{
    packet_t packet;
    packet.starter_mark = STARTER_MARK;
    packet.size = 0;
    packet.seq_num = 0;
    packet.type = LISTAR;
    memset(packet.data, 0, DATA_LEN);
    packet.crc = calculate_crc8((uint8_t *)&packet, sizeof(packet_t) - 1);
    video_t *videos = NULL;

    printf("Enviando comando de listagem de videos...\n");
    send_packet(connection.socket, &packet, &connection.address, &connection.state);

    // wait for the server to send the list of videos, with ACK
    // receive_packet(connection.socket, &packet, &connection);

    if (packet.type == ACK)
    {
        printf("ACK received\n");
    }

    return videos;
}

int show_menu()
{
    printf("1. Listar videos\n");
    printf("2. Baixar videos\n");
    printf("3. Sair\n");
    printf("Escolha uma opcao: ");
    int opcao;
    scanf("%d", &opcao);
    return opcao;
}
