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
                video_t *videos = list_videos();

                if (videos == NULL)
                {
                    #ifdef DEBUG
                        printf("Erro ao listar videos, enviando pacote de erro\n");
                    #endif
                    build_packet(&packet, 0, ERRO, NULL, 0);
                    if (connection.state == SENDING)
                        send_packet(connection.socket, &packet, &connection.address, &connection.state);
                    break;
                }

                while (videos != NULL)
                {
                    for (int i = 0; i < sizeof(videos); i++)
                    {
                        char video_name[DATA_LEN];
                        strncpy(video_name, videos[i].name, sizeof(video_name));
                        build_packet(&packet, i, PRINTAR, video_name, sizeof(video_name));
                        send_packet(connection.socket, &packet, &connection.address, &connection.state);
                    }
                }

                send_packet(connection.socket, &packet, &connection.address, &connection.state);
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

video_t *list_videos()
{
    DIR *directory;
    struct dirent *entry;
    video_t *videos = NULL;
    int videoCount = 0;

    // Open directory
    if ((directory = opendir(VIDEO_LOCATION)) != NULL)
    {
        // loop through directory
        while ((entry = readdir(directory)) != NULL)
        {
            // check if entry is a regular file
            if (entry->d_type == DT_REG)
            {
                // check if entry is a .mp4 or .avi file
                if (strstr(entry->d_name, ".mp4") == NULL && strstr(entry->d_name, ".avi") == NULL)
                {
                    continue;
                }

                // extract video information
                video_t *video = init_video_t();
                video->name = entry->d_name;
                video->size = entry->d_reclen;
                video->path = (char *)malloc(strlen(VIDEO_LOCATION) + strlen(entry->d_name) + 1);
                strcpy(video->path, VIDEO_LOCATION);
                strcat(video->path, entry->d_name);

                // add video to list
                videos = (video_t *)realloc(videos, (videoCount + 1) * sizeof(video_t));
                videos[videoCount] = *video;
                videoCount++;
            }
        }
        closedir(directory);
    }
    else
    {
        perror("Erro ao abrir diretorio");
    }

    if (videoCount == 0)
    {
        printf("Nenhum video encontrado, verifique o diretorio de videos\n");
    }

    #ifdef DEBUG
        for (int i = 0; i < videoCount; i++)
        {
            printf("Video %d\n", i);
            printf("  Name: %s\n", videos[i].name);
            printf("  Path: %s\n", videos[i].path);
            printf("  Size: %d\n", videos[i].size);
        }
    #endif

    return videos;
}
