#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include "socket.h"
#define DEBUG

packet_t last_packet = {
    .starter_mark = 0,
    .size = 0,
    .seq_num = 0,
    .type = 0,
    .data = {0},
    .crc = 0};

packet_t penult_packet = {
    .starter_mark = 0,
    .size = 0,
    .seq_num = 0,
    .type = 0,
    .data = {0},
    .crc = -1};

int ConexaoRawSocket(char *device)
{
    int soquete;
    struct ifreq ir;
    struct sockaddr_ll endereco;
    struct packet_mreq mr;

    soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); /*cria socket*/
    if (soquete == -1)
    {
        printf("Erro no Socket\n");
        exit(-1);
    }

    memset(&ir, 0, sizeof(struct ifreq)); /*dispositivo eth0*/
    memcpy(ir.ifr_name, device, sizeof(device));
    if (ioctl(soquete, SIOCGIFINDEX, &ir) == -1)
    {
        printf("Erro no ioctl\n");
        exit(-1);
    }

    memset(&endereco, 0, sizeof(endereco)); /*IP do dispositivo*/
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ir.ifr_ifindex;
    if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1)
    {
        printf("Erro no bind\n");
        exit(-1);
    }

    memset(&mr, 0, sizeof(mr)); /*Modo Promiscuo*/
    mr.mr_ifindex = ir.ifr_ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)
    {
        printf("Erro ao fazer setsockopt\n");
        exit(-1);
    }

    return soquete;
}

void buffer_to_message(uint8_t *buffer, packet_t *packet)
{
    packet_union_t pu;
    memcpy(pu.raw, buffer, sizeof(packet_union_t));
    packet->starter_mark = pu.packet.starter_mark;
    packet->size = pu.packet.size;
    packet->seq_num = pu.packet.seq_num;
    packet->type = pu.packet.type;
    memcpy(packet->data, pu.packet.data, sizeof(packet->data));
    packet->crc = pu.packet.crc;
}

// Listen to a socket, if a packet is received, it is copied to the output packet
int listen_socket(int _socket, packet_t *packet)
{
    uint8_t buffer[sizeof(packet_union_t)] = {0};
    ssize_t bytes_received = recv(_socket, buffer, sizeof(buffer), 0);

// check if it is a duplicate packet using ifrindex
#ifdef DEBUG
    printf("Waiting for packet\n");
#endif

    ssize_t size = -1;
    int is_ack = 0;

    for (;;)
    {
        size = recv(_socket, buffer, sizeof(buffer), 0);

        if (size == -1)
        {
#ifdef DEBUG
            printf("No message received after timeout: errno=%d\n", errno);
#endif
            break;
        }

        if (packet->starter_mark != STARTER_MARK)
            continue;

        if (packet->type == ERRO)
        {
            printf("Erro ao receber pacote\n");
            // memcpy(error, packet->data, sizeof(int));
            is_ack = 2;
            break;
        }

        if (packet->type != ACK && packet->type != NACK)
            continue;

#ifdef DEBUG
        printf("Received %s\n", packet->type == ACK ? "ACK" : "NACK");
#endif

        if (packet->type == ACK)
        {
            printf("Received ACK\n");
            is_ack = 1;
        }

        break;
    }

    if (bytes_received < 0)
    {
        fprintf(stderr, "Error on recv: %s\n", strerror(errno));
        exit(-1);
    }

    if (bytes_received == 0)
        return NACK;

    // Deserialize into packet_union_t
    packet_union_t pu;
    memcpy(pu.raw, buffer, bytes_received);

    // Validate packet
    uint8_t received_crc = pu.packet.crc;
    pu.packet.crc = 0; // Zero out CRC for validation
    uint8_t computed_crc = calculate_crc8(pu.raw, sizeof(pu.raw) - 1);

    // error handling
    if (received_crc != computed_crc)
        return NACK;

    // Copy packet to output
    packet->size = pu.packet.size;
    packet->seq_num = pu.packet.seq_num;
    packet->type = pu.packet.type;
    memcpy(packet->data, pu.packet.data, sizeof(packet->data));

    return is_ack;
}

void build_packet(packet_t *pkt, uint8_t seq_num, uint8_t type, uint8_t *data, size_t data_len)
{
    pkt->starter_mark = STARTER_MARK;
    pkt->size = data_len;
    pkt->seq_num = seq_num;
    pkt->type = type;
    memset(pkt->data, 0, DATA_LEN);
    memcpy(pkt->data, data, data_len);
    pkt->crc = calculate_crc8((uint8_t *)pkt, sizeof(packet_t) - 1);
}

void print_packet(packet_t *pkt)
{
    printf("Packet:\n");
    printf("  Starter mark: %x\n", pkt->starter_mark);
    printf("  Size: %d\n", pkt->size);
    printf("  Sequence number: %d\n", pkt->seq_num);
    printf("  Type: %d\n", pkt->type);
    printf("  Data: %s\n", pkt->data);
    printf("  CRC: %x\n", pkt->crc);
}

int wait_ack_or_error(packet_t *packet, int *error, int _socket)
{
    if (!packet)
        return -1;

#ifdef DEBUG
    printf("[ETHBKP][WACKOE] Waiting acknowledgement\n");
#endif

    ssize_t size = -1;
    int is_ack = 0;
    uint8_t buffer[sizeof(packet_union_t)] = {0};

    for (;;)
    {
        // size = recv(_socket, buffer, sizeof(buffer), 0);
        size = recv(_socket, packet, sizeof(packet_t), 0);

        if (size == -1)
        {
            break;
        }

        // buffer_to_message(backup->recv_buffer, backup->recv_message);

        if (packet->starter_mark != STARTER_MARK)
            continue;

        print_packet(packet);
        if (packet->type == ERRO)
        {
            printf("Erro ao receber pacote\n");
            // memcpy(error, packet->data, sizeof(int));
            is_ack = 2;
            break;
        }

        if (packet->type != ACK && packet->type != NACK)
            continue;

        if (packet->type == ACK)
        {
            printf("Received ACK\n");
            is_ack = 1;
        }

        break;
    }

    return is_ack;
}

ssize_t send_packet(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state)
{
    packet_union_t pu;
    memcpy(pu.raw, packet, sizeof(packet_t));

    connection_state = *connection_state * -1;

    ssize_t size;
    int is_ack = 0;
    int error = -1;

    while (!is_ack)
    {
        size = sendto(_socket, pu.raw, sizeof(packet_t), 0, (struct sockaddr *)address, sizeof(struct sockaddr_ll));

        is_ack = wait_ack_or_error(packet, &error, _socket);

        printf("is_ack: %d\n", is_ack);

#ifdef DEBUG
        printf("[ETHBKP][SNDMSG] Message sent, is_ack=%d\n\n", is_ack);
#endif
    }

    if (size < 0)
    {
        fprintf(stderr, "Error on sendto: %s\n", strerror(errno));
        exit(-1);
    }

    return size;
}

ssize_t send_ack(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state)
{
    packet_union_t pu;
    memcpy(pu.raw, packet, sizeof(packet_t));

    connection_state = *connection_state * -1;

    ssize_t size;
    int error = -1;

    printf("Sending packet\n");
    print_packet(packet);
    size = sendto(_socket, pu.raw, sizeof(packet_t), 0, (struct sockaddr *)address, sizeof(struct sockaddr_ll));

    if (size < 0)
    {
        fprintf(stderr, "Error on sendto: %s\n", strerror(errno));
        exit(-1);
    }

    return size;
}

// CRC-8 calculation function
uint8_t calculate_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; ++j)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

video_t *init_video_t()
{
    video_t *video = (video_t *)malloc(sizeof(video_t));
    video->name = NULL;
    video->path = NULL;
    video->size = 0;
    return video;
}

// ssize_t size;
//     message_t *m = backup->recv_message;
//     int error = -1;

//     int valid_message;

//     for (;;) {
//         size = recv(backup->socket, backup->recv_buffer, BUFFER_MAX_LEN, 0);

//         if (size == -1 || backup->recv_buffer[0] != START_MARKER)
//             continue;

//         buffer_to_message(backup->recv_buffer, m);

//         if (m->type == ACK || m->type == NACK)
//             continue;

//         if (m->sequence == backup->sequence - 1) {
//             send_acknowledgement(backup, 1);
//             continue;
//         }

//         if (
//             m->type != RESET_SEQUENCE &&
//             m->sequence != backup->sequence
//         )
//             continue;

//         #ifdef DEBUG
//         printf("[ETHBKP][RCVM] Message received: ");
//         print_message(backup->recv_message);
//         #endif

//         valid_message = check_message_parity(m);

//         error = check_error(m);
//         if (error > -1) {
//             send_error(backup, error);
//             break;
//         }

//         send_acknowledgement(backup, valid_message);

//         if (valid_message)
//             break;
//     };

//     if (m->type == RESET_SEQUENCE)
//         backup->sequence = 1;
//     else
//         update_sequence(backup);

//     return size;

// chega erro

void receive_packet(int sock, packet_t *packet, connection_t *connection)
{
    ssize_t size;

    int error = -1;

    for (;;)
    {
        size = recv(sock, packet, sizeof(packet_t), 0);

        if (size == -1 || packet->starter_mark != STARTER_MARK)
            continue;

        printf("type %d\n", packet->type);
        if (packet->type == ACK || packet->type == NACK)
            continue;

            // if (packet->seq_num == penult_packet.seq_num) {
            //     send_packet(sock, &last_packet, &connection->address, &connection->state);
            //     continue;
            // }

            // if (packet->type != RESET_SEQUENCE && packet->seq_num != last_packet.seq_num)
            //     continue;

#ifdef DEBUG
#endif
        printf("[ETHBKP][RCVM] Message received: ");
        print_packet(packet);

        // error = check_error(packet);
        // if (error > -1) {
        //     send_packet(sock, &last_packet, &connection.address, connection_state);
        //     break;
        // }

        // envia um ACK
        break;

        // if (check_message_parity(packet))
        //     break;
    };
    packet_t packet_ack;

    build_packet(&packet_ack, 0, ACK, NULL, 0);
    send_ack(sock, &packet_ack, &connection->address, &connection->state);
}

void receive_packet_sequence(int sock, packet_t *packet, connection_t *connection)
{
    ssize_t size;

    int error = -1;
    int last_received_seq_num = -1;

    for (;;)
    {
        size = recv(sock, packet, sizeof(packet_t), 0);

        if (size == -1 || packet->starter_mark != STARTER_MARK)
            continue;

        if (packet->type == ACK || packet->type == NACK)
            continue;

        if (packet->seq_num == last_received_seq_num && (packet->type != FIM))
        {
            continue;
        }

        // if (packet->type != RESET_SEQUENCE && packet->seq_num != last_packet.seq_num)
        //     continue;

#ifdef DEBUG
#endif
        printf("[ETHBKP][RCVM] Message received: ");
        print_packet(packet);

        // error = check_error(packet);
        // if (error > -1) {
        //     send_packet(sock, &last_packet, &connection.address, connection_state);
        //     break;
        // }


        // envia um ACK
        if (packet->type == FIM)
        {
            printf("Para de receber arquivos");
            break;
        }

        packet_t packet_ack;
        build_packet(&packet_ack, 0, ACK, NULL, 0);
        send_ack(sock, &packet_ack, &connection->address, &connection->state);
        last_received_seq_num++;

        // if (check_message_parity(packet))
        //     break;
    };

    // printf("Enviando ack apos receber tudinho");
    // build_packet(&packet_ack, 0, ACK, NULL, 0);
    // send_ack(sock, &packet_ack, &connection->address, &connection->state);
}
