#include "server.h"

int main(int argc, char *argv[]) {
    /* handling arguments */

    //...

    /* handling arguments */

    /* Create and connect the raw socket */

    //...

    /* Create and connect the raw socket */

    while(1) {
        /* Receive packets */

        //...

        /* Receive packets */

        /* extract header information */

        //...

        /* extract header information */


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
