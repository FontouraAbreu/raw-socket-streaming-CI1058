#include "client.h"

int main(int argc, char *argv[]) {
    int raw_socket;

    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL);

    if (raw_socket < 0) {
        perror("socket()");
        return -1;
    }


}