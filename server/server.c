#include "server.h"
#include "../utils/utils.h"

#define DEBUG

/* server configurations */
#define VIDEO_LOCATION "./videos/"

connection_t connection;

void init_server(char *interface)
{
    connection.state = RECEIVING;
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
    init_server("lo");

    packet_t packet;

    while (1)
    {
        receive_packet(connection.socket, &packet, &connection);
        printf("Entrando para funcoes\n");
        switch (packet.type)
        {
        case LISTAR:
            build_packet(&packet, 0, INICIO_SEQ, NULL, 0);
            send_init_sequence(connection.socket, &packet, &connection.address, &(connection.state));

            printf("Listando videos...\n");
            video_list_t *videos = list_videos();

            if (videos->num_videos == 0)
            {
#ifdef DEBUG
                printf("Erro ao listar videos, enviando pacote de erro\n");
#endif
                build_packet(&packet, 0, ERRO, NULL, 0);
                send_packet(connection.socket, &packet, &connection.address, &connection.state);
                break;
            }
            else
            {
                wait_for_ack(connection.socket, &packet, &connection.address, &connection.state);

                // quando receber o ack, chama a função process_videos
                process_videos(connection, &packet, videos);
                // process_videos(connection, &packet, videos);
            }

            // send_packet(connection.socket, &packet, &connection.address, &connection.state);
            // build_packet(&packet, i, LISTAR, video_name, sizeof(video_name));
            // printf("Enviando video com nome: %s\n", video_name);
            // send_packet(connection.socket, &packet, &connection.address);
            free(videos);

            break;

        case BAIXAR:
            printf("Recebendo pacote de download\n");
            printf("Nome do video escolhido %s:", packet.data);
            break;
        default:
            receive_packet(connection.socket, &packet, &connection);
        }

        // break;
    }
}

/* connects to the server */

// list_videos();

// return EXIT_SUCCESS;
// }

video_list_t *list_videos()
{
    DIR *directory;
    struct dirent *entry;
    video_list_t *video_list = malloc(sizeof(video_list_t));

    if (!video_list)
    {
        perror("Erro ao alocar memória para video_list_t");
        return NULL;
    }

    video_list->videos = NULL;
    video_list->num_videos = 0;

    if ((directory = opendir(VIDEO_LOCATION)) != NULL)
    {
        while ((entry = readdir(directory)) != NULL)
        {
            // Verifica se a entrada é um arquivo regular
            if (entry->d_type == DT_REG)
            {
                // Verifica se a entrada é um arquivo .mp4 ou .avi
                if (strstr(entry->d_name, ".mp4") == NULL && strstr(entry->d_name, ".avi") == NULL)
                {
                    continue;
                }

                // Extrai informações do vídeo
                video_t video;
                video.name = strdup(entry->d_name); // Copia o nome do arquivo para o campo name do video_t
                if (!video.name)
                {
                    perror("Erro ao alocar memória para video->name");
                    closedir(directory);
                    free(video_list->videos);
                    free(video_list);
                    return NULL;
                }
                video.size = entry->d_reclen;                                            // Assumindo que d_reclen contém o tamanho do arquivo
                video.path = malloc(strlen(VIDEO_LOCATION) + strlen(entry->d_name) + 1); // Alocando memória para o caminho completo
                if (!video.path)
                {
                    perror("Erro ao alocar memória para video->path");
                    free(video.name); // Libera memória alocada para o nome do arquivo
                    closedir(directory);
                    free(video_list->videos);
                    free(video_list);
                    return NULL;
                }
                strcpy(video.path, VIDEO_LOCATION);
                strcat(video.path, entry->d_name);

                // Adiciona o video à lista
                video_list->videos = realloc(video_list->videos, (video_list->num_videos + 1) * sizeof(video_t)); // Ajusta o tamanho da lista para acomodar o novo vídeo
                if (!video_list->videos)
                {
                    perror("Erro ao realocar memória para a lista de vídeos");
                    free(video.name); // Libera memória alocada para o nome do arquivo
                    free(video.path); // Libera memória alocada para o caminho do vídeo
                    closedir(directory);
                    free(video_list);
                    return NULL;
                }
                video_list->videos[video_list->num_videos] = video; // Adiciona o vídeo à lista
                video_list->num_videos++;
            }
        }
        closedir(directory);
    }
    else
    {
        perror("Erro ao abrir o diretório");
        free(video_list->videos);
        free(video_list);
        return NULL;
    }

    if (video_list->num_videos == 0)
    {
        printf("Nenhum vídeo encontrado, verifique o diretório de vídeos\n");
        free(video_list->videos);
        free(video_list);
        return NULL;
    }

    return video_list;
}

// wait for ack
int wait_for_ack(int sockfd, packet_t *packet, struct sockaddr_ll *address, int *state)
{
    ssize_t size;
    int is_ack = 0;
    socklen_t addr_len = sizeof(struct sockaddr_ll);

    while (!is_ack)
    {
        size = recvfrom(sockfd, packet, sizeof(packet_t), 0, (struct sockaddr *)address, &addr_len);
        if (size < 0)
        {
            perror("Erro ao receber pacote");
            continue;
        }

        if (packet->type == ACK)
        {
            is_ack = 1;
        }
        else
        {
            print_packet(packet);
            send_packet(sockfd, packet, address, state); // Reenviar pacote atual em caso de erro
        }
    }

    return is_ack;
}

void process_videos(connection_t connection, packet_t *packet, video_list_t *videos)
{
    int current_video_index = 0;
    int total_videos = videos->num_videos;

    while (current_video_index < total_videos)
    {
        // Send the current video packet
        char *video_name = videos->videos[current_video_index].name;
        build_packet(packet, current_video_index, DADOS, (uint8_t *)video_name, strlen(video_name));
        packet_union_t pu;
        memcpy(pu.raw, packet, sizeof(packet_t));

        ssize_t size;
        int is_ack = 0;

        while (!is_ack)
        {
            size = sendto(connection.socket, pu.raw, sizeof(packet_t), 0, (struct sockaddr *)&connection.address, sizeof(connection.address));
            if (size < 0)
            {
                perror("Error sending packet");
                continue;
            }

            is_ack = wait_for_ack(connection.socket, packet, &connection.address, &connection.state);

            printf("is_ack: %d\n", is_ack);

#ifdef DEBUG
            printf("[ETHBKP][SNDMSG] Message sent, is_ack=%d\n\n", is_ack);
#endif
        }

        current_video_index++;
    }

    // All videos have been sent, send the end packet
    printf("Enviando pacote de fim de listagem\n");
    build_packet(packet, 0, FIM, NULL, 0);
    send_packet(connection.socket, packet, &connection.address, &connection.state);
    // exit(EXIT_SUCCESS);
}
