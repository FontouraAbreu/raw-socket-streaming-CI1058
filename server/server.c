#include "server.h"

#define DEBUG

void build_packet(kermit_frame_t *pkt, uint8_t seq_num, uint8_t type, const uint8_t *data, size_t data_len)
{
    pkt->starter_mark = 0xAA;
    pkt->size = data_len;
    pkt->seq_num = seq_num;
    pkt->type = type;
    memset(pkt->data, 0, DATA_LEN);
    memcpy(pkt->data, data, data_len);
    pkt->crc = calculate_crc8((uint8_t *)pkt, sizeof(kermit_frame_t) - 1);
}

typedef struct {
    int socket;
    kermit_frame_t send_buffer;
    uint8_t sequence_number;
} backup_t;

int wait_ack_or_error(backup_t *backup, int *error) {
    if (!backup)
        return -1;

    // Remove the unused variable declaration
    kermit_frame_t ack;
    ssize_t size;

    size = recv(
        backup->socket,
        &ack,
        sizeof(kermit_frame_t),
        0
    );

    if (size < 0) {
        perror("recv");
        return -1;
    }

    if (ack.type == ACK) {
        return 1;
    } else if (ack.type == ERRO) {
        *error = 1;
        return 1;
    }

    return 0;
}

void print_message(kermit_frame_t *m) {
    if (!m)
        return;

    printf("Starter mark: %x\n", m->starter_mark);
    printf("Size: %d\n", m->size);
    printf("Sequence number: %d\n", m->seq_num);
    printf("Type: %d\n", m->type);
    printf("Data: %s\n", m->data);
    printf("CRC: %x\n", m->crc);
}

ssize_t send_message(backup_t *backup) {
    if (!backup)
        return -1;

    kermit_frame_t *m = &backup->send_buffer;

    // Supondo que build_packet preenche m corretamente
    build_packet(m, backup->sequence_number, 1, (const uint8_t*)"Hello", strlen("Hello"));

    #ifdef DEBUG
    printf("[ETHBKP][SDMSG] Sending message\n");
    print_message(m); // Assumindo que esta função imprime detalhes do pacote
    #endif

    ssize_t size;
    int is_ack = 0;
    int error = -1;

    while (!is_ack) {
        size = send(
            backup->socket,
            m,
            sizeof(kermit_frame_t),
            0
        );

        is_ack = wait_ack_or_error(backup, &error); // Esta função deve verificar se recebeu ACK ou erro

        #ifdef DEBUG
        printf("[ETHBKP][SNDMSG] Message sent, is_ack=%d\n\n", is_ack);
        #endif
    }

    // Atualiza a sequência para o próximo número
    backup->sequence_number++;

    return size;
}

int main()
{
    const char *interface = "lo";
    int raw_socket = create_stream_socket(0);
    printf("Socket: %d\n", raw_socket);
    if (raw_socket < 0)
    {
        fprintf(stderr, "Failed to create and configure raw socket on interface %s\n", interface);
        return EXIT_FAILURE;
    }

    struct ifreq ifr;

    // Get the interface index
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    if (ioctl(raw_socket, SIOCGIFINDEX, &ifr) == -1)
    {
        perror("ioctl");
        close(raw_socket);
        return EXIT_FAILURE;
    }

    // Bind the socket to the interface
    struct sockaddr_ll addr = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_ALL),
        .sll_ifindex = ifr.ifr_ifindex,
    };
    if (bind(raw_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(raw_socket);
        return EXIT_FAILURE;
    }

    backup_t backup = {
        .socket = raw_socket,
        .sequence_number = 0,
    };

    while (1)
    {
        send_message(&backup);
    }

    return EXIT_SUCCESS;

    // return 0;
}