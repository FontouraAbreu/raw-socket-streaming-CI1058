#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "../server/server.h"
#include "../socket/socket.h"
#include "../client/client.h"

char *parse_args(int argc, char **argv, char *optstring);

void print_packet(packet_t *packet);

#endif // __UTILS_H__
