#include "server.h"
#include "../utils/utils.h"

#define DEBUG

/* server configurations */
#define VIDEO_LOCATION "./videos/"


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



int main(int argc, char **argv) {
    /* connects to the server */
    char *interface = parse_args(argc, argv, "i:");
    int sockfd = connect_raw_socket(interface);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    /* connects to the server */

    list_videos();

    return EXIT_SUCCESS;
}

video_t *list_videos() {
    DIR *directory;
    struct dirent *entry;
    video_t *videos = NULL;

    // Open directory
    if ((directory = opendir(VIDEO_LOCATION)) != NULL) {
        // loop through directory
        while ((entry = readdir(directory)) != NULL) {
            // check if entry is a regular file
            if (entry->d_type == DT_REG) {
                // check if entry is a .mp4 or .avi file
                if (strstr(entry->d_name, ".mp4") == NULL && strstr(entry->d_name, ".avi") == NULL) {
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
                videos = (video_t *)realloc(videos, sizeof(video_t) + sizeof(video_t));
                videos = video;
            }
        }
        closedir(directory);
    } else {
        perror("Erro ao abrir diretorio");
    }

    #ifdef DEBUG
        while (videos != NULL) {
            printf("Video: %s\n", videos->name);
            printf("Size: %d\n", videos->size);
            printf("Path: %s\n", videos->path);
            videos++;
        }
    #endif

    return videos;
}

