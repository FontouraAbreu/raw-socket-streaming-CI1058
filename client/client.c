#include "client.h"

int main(int argc, char *argv[]) {
    int raw_socket;

    raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
}