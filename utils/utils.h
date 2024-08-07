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
#include <getopt.h>
#include "../server/server.h"
#include "../socket/socket.h"
#include "../client/client.h"

typedef struct {
    char *interface;
} args_t;

args_t parse_args(int argc, char **argv, char *optstring);

#endif // __UTILS_H__
