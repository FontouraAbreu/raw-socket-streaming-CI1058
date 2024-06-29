#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "../socket/socket.h"
#include "../utils/utils.h"

//enum que contem o MENU com as opcoes
typedef enum {
    LISTAR = 10, // bx01010
    BAIXAR = 11,
    PRINTAR = 16,
    DESCRITOR = 17,
    DADOS = 18,
    FIM = 30,
    ERRO = 31
} Actions_t;

void show_menu();

    

#endif // __CLIENT_H__