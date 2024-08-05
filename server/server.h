#ifndef __SERVER_H__
#define __SERVER_H__
#define _DEFAULT_SOURCE // para usar o DT_REG e DT_DIR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dirent.h>
#include <sys/stat.h>
#include "../socket/socket.h"
#include "../utils/utils.h"

video_list_t *list_videos();

void process_videos(connection_t connection, packet_t *packet, video_list_t *videos);
long get_file_size(char *filename);

#endif // __SERVER_H__
