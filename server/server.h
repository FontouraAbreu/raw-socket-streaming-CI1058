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
#include "../socket/socket.h"
#include "../utils/utils.h"

video_t *list_videos();

#endif // __SERVER_H__
