#include "client.h"

connection_t connection;

#define VIDEO_CLIENT_LOCATION "./videos_client/testevideo.mp4"

typedef enum
{
    LIST = 1,
    DOWNLOAD = 2,
    QUIT = 3,
} states_client_t;

void init_client(char *interface)
{
    connection.state = SENDING;
    connection.socket = ConexaoRawSocket(interface);

    // Destination MAC address (example, use the actual destination address)
    // Broadcast address for example
    unsigned char dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Prepare sockaddr_ll structure
    memset(&connection.address, 0, sizeof(connection.address));
    connection.address.sll_family = AF_PACKET;
    connection.address.sll_protocol = htons(ETH_P_ALL);
    connection.address.sll_ifindex = if_nametoindex(interface);
    connection.address.sll_halen = ETH_ALEN;
    memcpy(connection.address.sll_addr, dest_mac, ETH_ALEN);
}

int main(int argc, char **argv)
{
    // Parse arguments
    args_t args = parse_args(argc, argv, "i:");

    init_client(args.interface);
    video_list_t *video_list = malloc(sizeof(video_list_t));

    int op = show_menu();
    while (op != QUIT)
    {
        printf("Opcao escolhida: %d\n", op);
        switch (op)
        {
        case LIST:
            // Solicita a lista de vídeos disponíveis no servidor
            video_t *videos = request_videos();

            packet_t packet;
            wait_for_init_sequence(connection.socket, &packet, &connection);

            if (packet.type == ERRO)
            {
                printf("Erro ao listar videos, tente novamente!\n");
                break;
            }

            if (packet.type == ERRO_SEM_VIDEOS)
            {
                printf("Nenhum video disponivel\n");
                break;
            }

            if (packet.type != ACK)
                print_packet(&packet);

            if (packet.type == INICIO_SEQ)
            {
                printf("Recebendo videos...\n");
                receive_packet_sequence(connection.socket, &packet, &connection, video_list);
            }

            if (packet.type == FIM)
            {
                printf("Videos disponiveis:\n");
                for (int i = 0; i < video_list->num_videos; i++)
                {
                    printf("%d. %s\n", i + 1, video_list->videos[i].name);
                    if (video_list->videos[i].size > 1024 * 1024)
                    {
                        printf("Tamanho: %.2f MB\n", (float)video_list->videos[i].size / (1024 * 1024));
                    }
                    else
                    {
                        printf("Tamanho: %.2f KB\n", (float)video_list->videos[i].size / 1024);
                    }
                    printf("Duracao: %ds\n", video_list->videos[i].duration);
                }
            }

            break;

        case DOWNLOAD:
            printf("Escolha o video que deseja baixar: ");
            if (video_list->num_videos == 0)
            {
                printf("Nenhum video disponivel, liste os videos para ver o catalogo\n");
                break;
            }

            int chosen_video_index;
            scanf("%d", &chosen_video_index);

            if (chosen_video_index < 1 || chosen_video_index > video_list->num_videos)
            {
                printf("Opcao invalida\n");
                break;
            }

            video_t chosen_video = video_list->videos[chosen_video_index - 1];
            char *video_path = malloc(strlen(VIDEO_CLIENT_LOCATION) + strlen(chosen_video.name) + 1);
            printf("Baixando video %s\n", chosen_video.name);

            request_download(chosen_video.name);

            wait_for_init_sequence(connection.socket, &packet, &connection);

            if (packet.type != ACK)
                print_packet(&packet);

            if (packet.type == INICIO_SEQ)
            {
                printf("Recebendo dados do video...\n");
                receive_video_packet_sequence(connection.socket, &packet, &connection, VIDEO_CLIENT_LOCATION, chosen_video.size);
            }

            break;
        case QUIT:
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
    op = show_menu();

    return 0;
}

/*
 * Função que lista os videos disponíveis no servidor
 * Retorna um ponteiro para um array de video_t
 */
video_t *request_videos()
{
    packet_t packet;
    packet.starter_mark = STARTER_MARK;
    packet.size = 0;
    packet.seq_num = 0;
    packet.type = LISTAR;
    memset(packet.data, 0, DATA_LEN);
    packet.crc = calculate_crc8((unsigned char *)&packet, sizeof(packet_t) - 1);
    video_t *videos = NULL;

    int status = send_packet(connection.socket, &packet, &connection.address, &connection.state);

    if (status == ERRO_SEM_VIDEOS)
    {
        printf("Nenhum video disponivel\n");
        return videos;
    }

    if (packet.type == ACK)
    {
        printf("Videos request sent\n");
    }

    return videos;
}

/*
 * Função que solicita o download de um video ao servidor
 */
void request_download(char *video_name)
{
    packet_t packet;
    packet.starter_mark = STARTER_MARK;
    packet.size = 0;
    packet.seq_num = 0;
    packet.type = BAIXAR;
    memcpy(packet.data, video_name, strlen(video_name) + 1);
    printf("Enviando pacote de download com nome %s\n\n", packet.data);
    packet.crc = calculate_crc8((unsigned char *)&packet, sizeof(packet_t) - 1);

    send_packet(connection.socket, &packet, &connection.address, &connection.state);
}

/*
 * Função que exibe o menu de opções
 */
int show_menu()
{
    printf("1. Listar videos\n");
    printf("2. Baixar videos\n");
    printf("3. Sair\n");
    printf("Escolha uma opcao: ");
    int op;
    scanf("%d", &op);
    return op;
}
