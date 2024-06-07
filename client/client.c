#include "client.h"


struct sockaddr socket_address;
struct sockaddr_in source,dest;
unsigned char *send_buffer;



int main(int argc, char *argv[]) {
    /* handling arguments */
    int opt;
    char *interface = parse_args(argc, argv, "i:");

    //...

    /* handling arguments */

    /* Create and connect the raw socket */

    int raw_socket = connect_raw_socket(interface);
    memset(buffer, 0, ETH_FRAME_LEN);

    //...

    /* Create and connect the raw socket */
    int buflen, socket_address_length;
    while(1) {
        /* Receive packets */
        socket_address_length = sizeof(socket_address);

        buflen = recvfrom(raw_socket, buffer, ETH_FRAME_LEN, 0, NULL, NULL);


        /* Receive packets */

        switch (packet_mode)
        {
        case ACK:
            /* Send ACK */
            break;

        case NACK:
            /* Send NACK */
            break;
        
        case LISTAR:
            /* Send LISTAR */
            break;

        case BAIXAR:
            /* Send BAIXAR */
            break;

        case PRINTAR:
            /* Send PRINTAR */
            break;

        case DESCRITOR:
            /* Send DESCRITOR */
            break;

        case DADOS:
            /* Send DADOS */
            break;

        case FIM:
            /* Send FIM */
            break;

        case ERRO:
            /* Send ERRO */
            break;
        
        default:
            break;
        }
    }
}