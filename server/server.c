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
    unsigned char dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast address for example

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
    args_t args = parse_args(argc, argv, "i:f:");

    init_server(args.interface);

    packet_t packet;

    while (1)
    {
        receive_packet(connection.socket, &packet, &connection);
        switch (packet.type)
        {
        case LISTAR:

            printf("Listando videos...\n");
            video_list_t *videos = list_videos();

            if (videos->num_videos == 0)
            {
                printf("Erro ao listar videos, enviando pacote de erro\n");
                build_packet(&packet, 0, ERRO_SEM_VIDEOS, NULL, 0);
                send_packet_no_ack(connection.socket, &packet, &connection.address, &connection.state);
                break;
            }
            else
            {
                build_packet(&packet, 0, INICIO_SEQ, NULL, 0);
                send_init_sequence(connection.socket, &packet, &connection.address, &(connection.state));
                wait_for_ack(connection.socket, &packet, &connection.address, &connection.state);

                // quando receber o ack, chama a função process_videos
                process_videos(connection, &packet, videos);
                // process_videos(connection, &packet, videos);
            }

            free(videos);

            break;

        case BAIXAR:
            printf("Recebendo pacote de download\n");
            printf("Nome do video escolhido %s:\n\n\n", packet.data);

            build_packet(&packet, 0, INICIO_SEQ, NULL, 0);
            send_init_sequence(connection.socket, &packet, &connection.address, &(connection.state));

            char *video_path = get_video_path(packet.data);
            if (video_path)
            {
                send_video(connection.socket, &packet, &connection, video_path);
            }
            else
            {
                printf("Erro ao encontrar o video %s\n", packet.data);
                build_packet(&packet, 0, ERRO_NAO_ENCONTRADO, NOT_FOUND, 0);
                send_packet(connection.socket, &packet, &connection.address, &connection.state);
            }

            break;
        // default:
        //     receive_packet(connection.socket, &packet, &connection);
        }

    }
}

// pega duração do video
int get_video_duration(char *video_location, char *video_name)
{
    char command[1024];
    sprintf(command, "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s%s", video_location, video_name);

    FILE *fp = popen(command, "r");
    if (!fp)
    {
        perror("Erro ao executar o comando ffprobe");
        return -1;
    }

    char duration_str[10];
    fgets(duration_str, sizeof(duration_str), fp);
    pclose(fp);

    return atoi(duration_str);
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
                video.size = entry->d_reclen;

                //pega a duração do video
                video.duration = get_video_duration(VIDEO_LOCATION, entry->d_name);
                // extracts the video path from entry
                video.path = get_video_path(entry->d_name);

                video.size = get_file_size(video.path);
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
    return video_list;
}

long get_file_size(char *filename) {
    struct stat file_status;
    if (stat(filename, &file_status) < 0) {
        return -1;
    }

    return file_status.st_size;
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
    int num_seq = 0;
    int total_videos = videos->num_videos;

    while (current_video_index < total_videos)
    {
        // Send the current video packet
        char *video_name = videos->videos[current_video_index].name;
        int video_name_length = strlen(video_name);
        int video_duration = videos->videos[current_video_index].duration;
        int video_size = videos->videos[current_video_index].size;

        printf("Enviando pacote de video %s\n", video_name);
        printf("Duração: %d\n", video_duration);
        printf("Tamanho: %d\n", video_size);

        // Send the video name packet
        build_packet(packet, num_seq, DESCRITOR, (unsigned char *)video_name, video_name_length);
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

            printf("RECEBEU ACK DO NOME DO VIDEO: %d\n", is_ack);

        #ifdef DEBUG
            printf("[ETHBKP][SNDMSG] Message sent, is_ack=%d\n\n", is_ack);
        #endif
        }

        // Send the video size packet
        num_seq++;
        build_packet(packet, num_seq, TAMANHO, (unsigned char *)&video_size, sizeof(int));
        memcpy(pu.raw, packet, sizeof(packet_t));

        is_ack = 0;

        while (!is_ack)
        {
            printf("Enviando pacote de tamanho\n");
            size = sendto(connection.socket, pu.raw, sizeof(packet_t), 0, (struct sockaddr *)&connection.address, sizeof(connection.address));
            if (size < 0)
            {
                perror("Error sending packet");
                continue;
            }

            is_ack = wait_for_ack(connection.socket, packet, &connection.address, &connection.state);

            printf("RECEBEU ACK DO TAMANHO DO VIDEO: %d\n", is_ack);

        #ifdef DEBUG
            printf("[ETHBKP][SNDMSG] Message sent, is_ack=%d\n\n", is_ack);
        #endif
        }

        // Send the video duration packet
        num_seq++;
        build_packet(packet, num_seq, DURACAO, (unsigned char *)&video_duration, sizeof(int));
        memcpy(pu.raw, packet, sizeof(packet_t));

        is_ack = 0;

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

        num_seq = 0;
        current_video_index++;
    }

    // All videos have been sent, send the end packet
    printf("Enviando pacote de fim de listagem\n");
    build_packet(packet, 0, FIM, NULL, 0);
    send_packet(connection.socket, packet, &connection.address, &connection.state);
    // exit(EXIT_SUCCESS);
}
