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
        if (connection.state == RECEIVING){
            receive_packet(connection.socket, &packet, &connection.state);
        }

        switch (packet.type)
        {
            case LISTAR:
                printf("Listando videos...\n");
            video_list_t *videos = list_videos();

            if (videos->num_videos == 0)
                {
                    #ifdef DEBUG
                        printf("Erro ao listar videos, enviando pacote de erro\n");
                    #endif
                    build_packet(&packet, 0, ERRO, NULL, 0);
                    if (connection.state == SENDING)
                        send_packet(connection.socket, &packet, &connection.address, &connection.state);
                    break;
                }
            else
            {
#ifdef DEBUG
                printf("Processando videos...\n");
#endif
                process_videos(connection, &packet, videos);
            }

            // send_packet(connection.socket, &packet, &connection.address, &connection.state);
                //     build_packet(&packet, i, LISTAR, video_name, sizeof(video_name));
                //        printf("Enviando video com nome: %s\n", video_name);
                //     send_packet(connection.socket, &packet, &connection.address);
                // }

            free(videos);
            break;
        }
    }

    /* connects to the server */

    // list_videos();

    return EXIT_SUCCESS;
}

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

void process_videos(connection_t connection, packet_t *packet, video_list_t *videos)
{

    for (int i = 0; i < videos->num_videos; i++)
        {
        char *video_name = videos->videos[i].name;
        build_packet(packet, i, LISTAR, video_name, sizeof(video_name));
        if (connection.state == SENDING)
        {
            send_packet(connection.socket, packet, &connection.address, &connection.state);
        }
    }

    build_packet(packet, 0, FIM, NULL, 0);
    if (connection.state == SENDING)
    {
        send_packet(connection.socket, packet, &connection.address, &connection.state);
    }

    // exit(EXIT_SUCCESS);
}
