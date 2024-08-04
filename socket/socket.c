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
#define DATA_MAX_LEN 63
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

void buffer_to_message(unsigned char *buffer, packet_t *packet)
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
    unsigned char buffer[sizeof(packet_union_t)] = {0};
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
    unsigned char received_crc = pu.packet.crc;
    pu.packet.crc = 0; // Zero out CRC for validation
    unsigned char computed_crc = calculate_crc8(pu.raw, sizeof(pu.raw) - 1);

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

void build_packet(packet_t *pkt, unsigned char seq_num, unsigned char type, unsigned char *data, size_t data_len)
{
    pkt->starter_mark = STARTER_MARK;
    pkt->size = data_len;
    pkt->seq_num = seq_num;
    pkt->type = type;
    memset(pkt->data, 0, DATA_LEN);
    memcpy(pkt->data, data, data_len);
    pkt->crc = calculate_crc8((unsigned char *)pkt, sizeof(packet_t) - 1);
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
    unsigned char buffer[sizeof(packet_union_t)] = {0};

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

ssize_t send_init_sequence(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state)
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

    printf("recebi ack do init sequence\n");

    // if (is_ack)
    // {
    //     packet_t packet_init_seq;
    //     build_packet(&packet_init_seq, 0, INICIO_SEQ, NULL, 0);
    //     send_ack(_socket, &packet_init_seq, address, connection_state);
    // }

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
unsigned char calculate_crc8(const unsigned char *data, size_t len)
{
    unsigned char crc = 0x00;
    for (size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (unsigned char j = 0; j < 8; ++j)
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
    if (!video)
    {
        perror("Failed to allocate memory for video_t");
        return NULL;
    }
    video->name = NULL;
    video->path = NULL;
    video->size = 0;
    return video;
}

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

void wait_for_init_sequence(int sock, packet_t *packet, connection_t *connection)
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

        // error = check_error(packet);
        // if (error > -1) {
        //     send_packet(sock, &last_packet, &connection.address, connection_state);
        //     break;
        // }

        // recebe um pacote de inicio de sequencia
        if (packet->type == INICIO_SEQ)
        {
            break;
        }

        // if (check_message_parity(packet))
        //     break;
    };
    packet_t packet_ack;

    build_packet(&packet_ack, 0, ACK, NULL, 0);
    ssize_t size_ack = send_ack(sock, &packet_ack, &connection->address, &connection->state);

    if (size_ack < 0)
    {
        fprintf(stderr, "Error on sendto: %s\n", strerror(errno));
        exit(-1);
    }
}

void receive_packet_sequence(int sock, packet_t *packet, connection_t *connection, video_list_t *video_list)
{
    ssize_t size;
    int last_received_seq_num = -1;

    for (;;)
    {
        size = recv(sock, packet, sizeof(packet_t), 0);

        if (size == -1 || packet->starter_mark != STARTER_MARK)
        {
            continue;
        }

        if (packet->type == ACK || packet->type == NACK)
        {
            continue;
        }

        if (packet->seq_num == last_received_seq_num && (packet->type != FIM))
        {
            continue;
        }

        if (packet->type == DADOS)
        {
            printf("[ETHBKP][RCVM] Message received: ");
            print_packet(packet);
            last_received_seq_num = packet->seq_num;

            packet_t packet_ack;
            build_packet(&packet_ack, 0, ACK, NULL, 0);
            send_ack(sock, &packet_ack, &connection->address, &connection->state);

            video_t *video = init_video_t();
            if (video)
            {
                video_list->videos = realloc(video_list->videos, (video_list->num_videos + 1) * sizeof(video_t));
                if (!video_list->videos)
                {
                    // Handle error, possibly free previously allocated resources and exit or return an error
                    free(video);
                }
                else
                {
                    video->name = (char *)malloc(sizeof(char) * packet->size);
                    if (video->name)
                    {
                        memcpy(video->name, packet->data, packet->size);
                        video->size = packet->size;
                        video->path = malloc(sizeof(char) * (strlen(VIDEO_LOCATION) + strlen(video->name) + 1));
                        if (video->path)
                        {
                            strcpy(video->path, VIDEO_LOCATION);
                            strcat(video->path, video->name);
                        }
                        video_list->videos[video_list->num_videos] = *video;
                        video_list->num_videos++;
                    }
                    else
                    {
                        free(video);
                    }
                }
            }
        }

        if (packet->type == FIM)
        {
            printf("Para de receber arquivos\n");
            packet_t packet_ack;
            build_packet(&packet_ack, 0, ACK, NULL, 0);
            send_ack(sock, &packet_ack, &connection->address, &connection->state);
            break;
        }
    }
}

void receive_video_packet_sequence(int sock, packet_t *packet, connection_t *connection, const char *output_filename)
{
    ssize_t size;
    int last_received_seq_num = -1;

    FILE *file = fopen(output_filename, "wb");

    if (!file)
    {
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        return;
    }

    while (1)
    {
        size = recv(sock, packet, sizeof(packet_t), 0);

        if (size == -1)
        {
            perror("recv");
            continue;
        }

        if (packet->starter_mark != STARTER_MARK)
        {
            continue;
        }

        if (packet->type == ACK || packet->type == NACK)
        {
            continue;
        }

        if (packet->seq_num == last_received_seq_num && (packet->type != FIM))
        {
            continue;
        }

        int data_size;
        unsigned char *data = malloc(DATA_MAX_LEN);
        test_alloc(data, "receive_file data buffer");

        if (packet->type == DADOS)
        {
            printf("[ETHBKP][RCVM] Message received: ");
            print_packet(packet);
            data_size = packet->size;
            unsigned char *data = malloc(data_size);
            memcpy(data, packet->data, data_size);

            size_t written = fwrite(
                packet->data,
                sizeof(*packet->data),
                packet->size,
                file);
            if (written != packet->size)
            {
                fprintf(stderr, "Error writing to file\n");
                fclose(file);
                break;
            }
            last_received_seq_num = packet->seq_num;

            packet_t packet_ack;
            build_packet(&packet_ack, 0, ACK, NULL, 0);
            send_ack(sock, &packet_ack, &connection->address, &connection->state);
        }

        if (packet->type == FIM)
        {
            // if (chmod(file, 0644) != 0)
            // {
            //     perror("Erro ao ajustar permissões do arquivo");
            //     exit(EXIT_FAILURE);
            // }

            fclose(file);
            printf("File received successfully\n");

            char command[256];
            snprintf(command, sizeof(command), "sudo vlc %s", output_filename);

            int result = system(command);
            if (result == -1)
            {
                fprintf(stderr, "Error executing system command\n");
            }
            else
            {
                printf("VLC player opened successfully\n");
            }

            break;
        }
    }
}

void test_alloc(void *ptr, const char *msg)
{
    if (!ptr)
    {
        fprintf(stderr, "Error allocating memory for %s\n", msg);
        exit(EXIT_FAILURE);
    }
}

int wait_for_ack_socket(int sockfd, packet_t *packet, struct sockaddr_ll *address, int *state)
{
    ssize_t size;
    int is_ack = 0;
    socklen_t addr_len = sizeof(struct sockaddr_ll);

    while (!is_ack)
    {
        size = recvfrom(sockfd, packet, sizeof(packet_t), 0, (struct sockaddr *)address, &addr_len);
        if (size < 0)
        {
            perror("Erro ao receber pacote");
            continue;
        }

        if (packet->type == ACK)
        {
            is_ack = 1;
        }
        else
        {
            print_packet(packet);
            send_packet(sockfd, packet, address, state); // Reenviar pacote atual em caso de erro
        }
    }

    return is_ack;
}

void send_video(int sock, packet_t *packet, connection_t *connection, char *video_path)
{
    if (!packet || !video_path)
        return;

    FILE *file = fopen(video_path, "rb");
    if (!file)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }

    unsigned char *data = malloc(DATA_MAX_LEN);
    if (!data)
    {
        fprintf(stderr, "Memory allocation error\n");
        fclose(file);
        return;
    }

    ssize_t data_size;
    int current_data_video_index = 0;
    unsigned char aux, aux2;

    while (!feof(file))
    {
        data_size = fread(data, sizeof(*data), DATA_MAX_LEN, file);

        if (data_size <= 0)
            break;

        build_packet(packet, current_data_video_index, DADOS, (unsigned char *)data, data_size);

        int is_ack = 0;
        while (!is_ack)
        {
            if (send_packet(sock, packet, &connection->address, &connection->state) < 0)
            {
                perror("Error sending packet");
                continue;
            }

            is_ack = wait_for_ack_socket(sock, packet, &connection->address, &connection->state);

            printf("is_ack: %d\n", is_ack);

#ifdef DEBUG
            printf("[ETHBKP][SNDMSG] Message sent, is_ack=%d\n\n", is_ack);
#endif
        }

        current_data_video_index++;
    }

    free(data);
    fclose(file);

    build_packet(packet, 0, FIM, NULL, 0);
    if (send_packet(sock, packet, &connection->address, &connection->state) < 0)
    {
        printf("Error: %s\n", strerror(errno));
        return;
    }
}

char *get_video_path(char *video_name)
{
    DIR *d;
    struct dirent *dir;
    char *video_path = NULL;

    d = opendir(VIDEO_LOCATION);
    if (!d)
    {
        perror("Erro ao abrir diretório de vídeos");
        return NULL;
    }

    while ((dir = readdir(d)) != NULL)
    {
        if (strcmp(dir->d_name, video_name) == 0)
        {
            // Construir o caminho completo do vídeo
            video_path = malloc(strlen(VIDEO_LOCATION) + strlen(video_name) + 1);
            if (video_path)
            {
                strcpy(video_path, VIDEO_LOCATION);
                strcat(video_path, video_name);
            }
            break;
        }
        printf("%s\n", dir->d_name);
    }

    printf("video_path %s\n", video_path);

    closedir(d);
    return video_path;
}
