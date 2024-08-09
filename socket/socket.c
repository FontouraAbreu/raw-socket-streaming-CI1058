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
#include <sys/time.h>
#include <sys/stat.h>
#include <pwd.h>
#include "socket.h"
#define DATA_MAX_LEN 63
#define DEBUG
#define NUM_ATTEMPT 4
#define TIMEOUT_S 2
#define TIMEOUT_ERROR -1
#define ESCAPE_CHAR 0xFF

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

    // Timeout do socket
    struct timeval tv;
    tv.tv_sec = TIMEOUT_S;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

    return soquete;
}


void create_packet(packet_t *packet) {
    packet->starter_mark = 0;
    packet->size = 0;
    packet->seq_num = 0;
    packet->type = 0;
    packet->data = NULL;
    packet->crc = 0;

    return packet;
}

void buffer_to_message(unsigned char *buffer, packet_t *packet) {
    if (!buffer || !packet)
        return;

    // message_reset(message);

    packet->starter_mark = buffer[0];
    packet->size = (buffer[1] >> 2) & 0x3F;

    packet->seq_num = ((buffer[1] & 0x03) << 3) + (buffer[2] >> 5);

    packet->type = buffer[2] & 0x1F;

    if (packet->size)
    {
        unsigned char *tempData = malloc(sizeof(unsigned char) * packet->size);
        packet->data = tempData;
        test_alloc(packet->data, "packet->data");

        memcpy(packet->data, &buffer[3], packet->size);
    }

    packet->crc = buffer[packet->size + 3];

    return;
}

void message_to_buffer(packet_t *packet, unsigned char *buffer)
{
    if (!packet || !buffer)
        return;

    int buffer_size = packet->size + 4;

    buffer[0] = packet->starter_mark;
    buffer[1] = (packet->size << 2) + (packet->seq_num >> 3);
    buffer[2] = (packet->seq_num << 5) + (packet->type & 0x1F);

    if (packet->size)
        memcpy(&buffer[3], packet->data, packet->size);

    buffer[buffer_size - 1] = packet->crc;

    return;
}

/*
 * Função que ouve a porta e retorna o pacote recebido
 */
int listen_socket(int _socket, packet_t *packet)
{
    // unsigned char buffer[sizeof(packet_union_t)] = {0};
    unsigned char buffer[DATA_LEN];
    print_packet(packet);
    ssize_t bytes_received = recv(_socket, buffer, sizeof(buffer), 0);

    buffer_to_message(buffer, &packet);


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
        return -2;

#ifdef DEBUG
    printf("[ETHBKP][WACKOE] Waiting acknowledgement\n");
#endif

    ssize_t size = -1;
    int is_ack = 0;
    unsigned char buffer[sizeof(packet_union_t)] = {0};
    int attempt = 0;

    while (is_ack <= 0 && attempt < NUM_ATTEMPT)
    {
        // size = recv(_socket, buffer, sizeof(buffer), 0);
        size = recv(_socket, packet, sizeof(packet_t), 0);
        print_packet(packet);

        // recv returns -1 if a timeout has happened
        if (size == -1)
        {
            attempt++;
            int temp_random = rand() % attempt * attempt;
            usleep(temp_random * 1000);
            printf("Conexão interrompida!!\n\t(%d\\%d)Esperando por: %ds\n", attempt, NUM_ATTEMPT, temp_random);
            continue;
        }
        attempt = 0;

        if (packet->starter_mark != STARTER_MARK)
            continue;

        if (packet->type == ERRO)
        {
            printf("Erro ao receber pacote\n");
            // memcpy(error, packet->data, sizeof(int));
            is_ack = 2;
            break;
        }

        if (packet->type == ERRO_SEM_VIDEOS)
        {
            printf("Erro ao receber pacote: não há videos\n");
            return ERRO_SEM_VIDEOS;
        }

        // if packet type is not ACK
        if (packet->type != ACK && packet->type != NACK)
        {
            continue;
        }

        if (packet->type == ACK)
        {
            printf("Received ACK\n");
            is_ack = 1;
        }

        break;
    }

    if (attempt >= NUM_ATTEMPT)
    {
        printf("Número máximo de tentativas atingido!!\n");
        exit(1);
        return TIMEOUT_ERROR;
    }

    return is_ack;
}

// checks for the substrings 0x81 and 0x88 in the packet data
// if found, uses byte stuffing to escape them
void code_vpn_strings(packet_t *packet)
{
    int i, j;
    unsigned char *tempBuffer = malloc(DATA_MAX_LEN * 2); // Alocando espaço para o pior caso
    if (!tempBuffer)
    {
        fprintf(stderr, "Falha ao alocar memória para codificação\n");
        return;
    }

    for (i = 0, j = 0; i < DATA_MAX_LEN; i++)
    {
        if (packet->data[i] == 0x81 || packet->data[i] == 0x88)
        {
            printf("Found 0x81 or 0x88\n");
            printf("found: %x\n", packet->data[i]);
            tempBuffer[j++] = ESCAPE_CHAR; // Insere byte de escape
            // exit(1);
            // sleep(4);
        }
        tempBuffer[j++] = packet->data[i]; // Copia o byte original
    }

    memcpy(packet->data, tempBuffer, j);
    packet->size = j;

    free(tempBuffer);
}

// checks for the escape character ESCAPE_CHAR in the packet data
// if found, uses byte stuffing to unescape it
void decode_vpn_strings(packet_t *packet)
{
    int i, j;
    unsigned char *tempBuffer = malloc(DATA_MAX_LEN); // Assume que o pacote não encolhe
    if (!tempBuffer)
    {
        fprintf(stderr, "Falha ao alocar memória para decodificação\n");
        return;
    }

    for (i = 0, j = 0; i < packet->size; i++)
    {
        if (packet->data[i] == ESCAPE_CHAR && i + 1 < packet->size)
        {
            i++; // Pula o byte de escape
        }
        tempBuffer[j++] = packet->data[i]; // Copia o byte especial sem modificação
    }

    memcpy(packet->data, tempBuffer, j);
    packet->size = j;

    free(tempBuffer);
}

/*
 * Função que envia um pacote sem esperar por um ACK
 */
ssize_t send_packet_no_ack(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state)
{
    packet_union_t pu;
    memcpy(pu.raw, packet, sizeof(packet_t));

    ssize_t size;
    int is_ack = 0;
    int error = -1;

    // code_vpn_strings(packet);
    size = sendto(_socket, pu.raw, sizeof(packet_t), 0, (struct sockaddr *)address, sizeof(struct sockaddr_ll));
}

ssize_t send_packet(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state)
{
    packet_union_t pu;
    memcpy(pu.raw, packet, sizeof(packet_t));

    ssize_t size;
    int is_ack = 0;
    int error = -1;

    while (is_ack <= 0 && is_ack != TIMEOUT_ERROR)
    {
        // code_vpn_strings(packet);
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

    if (is_ack == TIMEOUT_ERROR)
    {
        return TIMEOUT_ERROR;
    }

    if (is_ack == ERRO_SEM_VIDEOS)
    {
        return ERRO_SEM_VIDEOS;
    }

    return size;
}

ssize_t send_init_sequence(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state)
{
    packet_union_t pu;
    memcpy(pu.raw, packet, sizeof(packet_t));

    ssize_t size;
    int is_ack = 0;
    int error = -1;

    // if a nack is received, resend the packet
    while (!is_ack)
    {
        // code_vpn_strings(packet);
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

    ssize_t size;

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

ssize_t send_nack(int _socket, packet_t *packet, struct sockaddr_ll *address, int *connection_state)
{
    packet_union_t pu;
    memcpy(pu.raw, packet, sizeof(packet_t));

    ssize_t size;

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
    video->duration = 0;
    video->size = 0;
    return video;
}

void receive_packet(int sock, packet_t *packet, connection_t *connection)
{
    ssize_t size;

    int error = -1;

    while (1)
    {
        size = recv(sock, packet, sizeof(packet_t), 0);

        packet_t *packet = malloc(sizeof(packet_t));


        if (size == -1)
        {
            continue;
        }

        // decode_vpn_strings(packet);

        if (packet->starter_mark != STARTER_MARK)
            continue;

        if (packet->type == ACK || packet->type == NACK)
            continue;

#ifdef DEBUG
#endif
        printf("[ETHBKP][RCVM] Message received: ");
        print_packet(packet);

        // error = check_crc(packet);
        // if (error)
        // {
        //     packet_t packet_nack;
        //     build_packet(&packet_nack, 0, ACK, NULL, 0);
        //     send_nack(sock, packet, &connection->address, &connection->state);
        //     continue;
        // }

        break;
    };

    packet_t packet_ack;
    build_packet(&packet_ack, 0, ACK, NULL, 0);
    send_ack(sock, &packet_ack, &connection->address, &connection->state);
}

void wait_for_init_sequence(int sock, packet_t *packet, connection_t *connection)
{
    ssize_t size;

    int error = -1;
    int attempt = 0;

    while (1 && attempt < NUM_ATTEMPT)
    {
        size = recv(sock, packet, sizeof(packet_t), 0);

        if (size == -1)
        {
            attempt++;
            int temp_random = rand() % attempt * attempt;
            usleep(temp_random * 1000);
            printf("Conexão interrompida!!\n\t(%d\\%d)Esperando por: %ds\n", attempt, NUM_ATTEMPT, temp_random);
            continue;
        }
        attempt = 0;

        // decode_vpn_strings(packet);

        if (packet->starter_mark != STARTER_MARK)
            continue;

        if (packet->type == ERRO_SEM_VIDEOS)
        {
            break;
        }

        if (packet->type == ACK || packet->type == NACK)
            continue;

#ifdef DEBUG
        printf("[ETHBKP][RCVM] Message received: ");
#endif

        error = check_crc(packet);
        if (error)
        {
            packet_t packet_nack;
            build_packet(&packet_nack, 0, NACK, NULL, 0);
            unsigned char buffer[DATA_LEN];
            message_to_buffer(&packet, buffer);
            send_nack(sock, packet, &connection->address, &connection->state);
            continue;
        }

        // recebe um pacote de inicio de sequencia
        if (packet->type == INICIO_SEQ)
        {
            break;
        }
    };

    if (attempt >= NUM_ATTEMPT)
    {
        printf("\tmaximum number of timeouts reached!!\n\tPlease try again!\n");
        exit(1);
    }

    packet_t packet_ack;
    build_packet(&packet_ack, 0, ACK, NULL, 0);

    unsigned char buffer[DATA_LEN];
    message_to_buffer(&packet, buffer);

    ssize_t size_ack = send_ack(sock, &packet_ack, &connection->address, &connection->state);

    if (size_ack < 0)
    {
        fprintf(stderr, "Error on sendto: %s\n", strerror(errno));
        exit(-1);
    }
}

void add_video_to_list(video_list_t *video_list, video_t *video)
{
    video_t *new_videos = realloc(video_list->videos, (video_list->num_videos + 1) * sizeof(video_t));
    if (!new_videos)
    {
        free(video);
        return;
    }

    video_list->videos = new_videos;
    video_list->videos[video_list->num_videos] = *video;
    video_list->num_videos++;
}

void receive_packet_sequence(int sock, packet_t *packet, connection_t *connection, video_list_t *video_list)
{
    ssize_t size;
    int attempt = 0;
    ssize_t size_tam;
    video_t *current_video = init_video_t();

    while (1 && attempt < NUM_ATTEMPT)
    {
        size = recv(sock, packet, sizeof(packet_t), 0);
        if (size == -1)
        {
            attempt++;
            int temp_random = rand() % attempt * attempt;
            usleep(temp_random * 1000);
            printf("Conexão interrompida!!\n\t(%d\\%d)Esperando por: %ds\n", attempt, NUM_ATTEMPT, temp_random);
            continue;
        }
        attempt = 0;

        // decode_vpn_strings(packet);

        print_packet(packet);

        // send a NACK response
        if (packet->starter_mark != STARTER_MARK)
        {
            continue;
        }

        if (packet->type == ACK || packet->type == NACK)
        {
            continue;
        }

        if (packet->type == ERRO)
        {
            break;
        }

        // int error = check_crc(packet);
        // if (error)
        // {
        //     printf("Erro no CRC\n");
        //     packet_t packet_nack;
        //     build_packet(&packet_nack, 0, ACK, NULL, 0);
        //     send_nack(sock, packet, &connection->address, &connection->state);
        //     continue;
        // }

        // Verifica se é um descritor (nome do vídeo)
        if (packet->type == DESCRITOR && packet->seq_num == 0)
        {
            print_packet(packet);
            // Supondo que o nome do vídeo esteja nos dados do pacote
            // +1 para o terminador nulo
            current_video->name = malloc(packet->size + 1);
            strcpy(current_video->name, packet->data);

            packet_t packet_ack;
            build_packet(&packet_ack, 0, ACK, NULL, 0);
            send_ack(sock, &packet_ack, &connection->address, &connection->state);
            continue;
        }

        // Verifica se é o tamanho do vídeo
        if (packet->type == TAMANHO && packet->seq_num == 1)
        {
            print_packet(packet);
            // Supondo que o tamanho esteja nos dados do pacote como um inteiro
            current_video->size = *((int *)packet->data); // Ajuste conforme necessário

            packet_t packet_ack;
            build_packet(&packet_ack, 0, ACK, NULL, 0);
            send_ack(sock, &packet_ack, &connection->address, &connection->state);
            continue;
        }

        // Verifica se é a duração do vídeo
        if (packet->type == DURACAO && packet->seq_num == 2)
        {
            printf("[ETHBKP][RCVM] Duração do vídeo recebida: ");
            print_packet(packet);
            video_list->videos = realloc(video_list->videos, (video_list->num_videos + 1) * sizeof(video_t));
            // Supondo que a duração esteja nos dados do pacote como um inteiro
            // Ajuste conforme necessário
            current_video->duration = *((int *)packet->data);

            printf("Adicionando video na lista\n");
            printf("Nome: %s\n", current_video->name);
            printf("Tamanho: %ld\n", current_video->size);
            printf("Duração: %d\n", current_video->duration);
            // add_video_to_list(video_list, &current_video);

            if (video_list != NULL)
            {
                printf("Num videos: %d\n", video_list->num_videos);
                video_list->videos[video_list->num_videos] = *current_video;
                video_list->num_videos++;
            }

            packet_t packet_ack;
            build_packet(&packet_ack, 0, ACK, NULL, 0);
            send_ack(sock, &packet_ack, &connection->address, &connection->state);
            continue;
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

    if (attempt >= NUM_ATTEMPT)
    {
        printf("\tmaximum number of timeouts reached!!\n\tPlease try again!\n");
        exit(1);
    }

    return video_list;
}

int receive_video_packet_sequence(int sock, packet_t *packet, connection_t *connection, const char *output_filename, int expected_size)
{
    ssize_t size;
    int last_received_seq_num = -1;
    int attempt = 0;

    // vai receber
    FILE *file = fopen(output_filename, "wb");

    if (!file)
    {
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        return;
    }

    while (1 && attempt < NUM_ATTEMPT)
    {
        size = recv(sock, packet, sizeof(packet_t), 0);

        if (size == -1)
        {
            attempt++;
            int temp_random = rand() % attempt * attempt;
            usleep(temp_random * 1000);
            printf("Conexão interrompida!!\n\t(%d\\%d)Esperando por: %ds\n", attempt, NUM_ATTEMPT, temp_random);
            continue;
        }
        attempt = 0;

        // decode_vpn_strings(packet);

        if (packet->starter_mark != STARTER_MARK)
        {
            continue;
        }

        if (packet->type == ERRO_NAO_ENCONTRADO)
        {
            printf("Video nao encontrado!! Liste os videos e tente novamente!\n");
            break;
        }

        if (packet->type == ACK || packet->type == NACK)
        {
            continue;
        }

        if (packet->seq_num == last_received_seq_num && (packet->type != FIM))
        {
            continue;
        }

        // int error = check_crc(packet);
        // if (error)
        // {
        //     packet_t packet_nack;
        //     build_packet(&packet_nack, 0, ACK, NULL, 0);
        //     send_nack(sock, packet, &connection->address, &connection->state);
        //     continue;
        // }

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

            unsigned char buffer[DATA_LEN];
            buffer_to_message(buffer, packet);

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
            fclose(file);
            printf("File received successfully\n");

            // Verifica se o tamanho do arquivo recebido é o esperado
            struct stat st;
            stat(output_filename, &st);
            if (st.st_size != expected_size)
            {
                printf("Received file size does not match the expected size\n");
                printf("\tTamanho do video: %ld\n", st.st_size);
                printf("\tTamanho esperado: %d\n", expected_size);
                packet_t packet_ack;
                build_packet(&packet_ack, 0, ACK, NULL, 0);
                send_ack(sock, &packet_ack, &connection->address, &connection->state);
                return -1;
            }
            else
            {
                printf("Received file size matches the expected size\n");
                packet_t packet_ack;
                build_packet(&packet_ack, 0, ACK, NULL, 0);
                send_ack(sock, &packet_ack, &connection->address, &connection->state);
            }

            // Muda as permissões do arquivo para que possa ser executado com o VLC
            if (chmod(output_filename, 0666) != 0)
            {
                perror("Erro ao ajustar permissões do arquivo");
                exit(EXIT_FAILURE);
            }

            // Extrai o usuário logado usando getlogin
            FILE *fp = popen("logname", "r");
            if (fp == NULL)
            {
                perror("Failed to run logname command");
                exit(EXIT_FAILURE);
            }

            char *logged_user = getlogin();
            if (logged_user == NULL)
            {
                perror("Failed to get login name");
                exit(EXIT_FAILURE);
            }

            // Obtém as informações do usuário, como UID e GID
            struct passwd *pwd = getpwnam(logged_user);
            if (pwd == NULL)
            {
                perror("Failed to get user information");
                exit(EXIT_FAILURE);
            }

            uid_t uid = pwd->pw_uid;
            gid_t gid = pwd->pw_gid;

            // Altera o proprietário do arquivo para o usuário logado
            if (chown(output_filename, uid, gid) != 0)
            {
                perror("Failed to change file ownership");
                exit(EXIT_FAILURE);
            }

            char command[256];
            snprintf(command, sizeof(command), "sudo -u %s vlc %s", logged_user, output_filename);

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

    if (attempt >= NUM_ATTEMPT)
    {
        printf("\tmaximum number of timeouts reached!!\n\tPlease try again!\n");
        exit(1);
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
    srand(time(NULL));
    ssize_t size;
    int is_ack = 0;
    socklen_t addr_len = sizeof(struct sockaddr_ll);
    int send_status;

    while (!is_ack)
    {
        size = recvfrom(sockfd, packet, sizeof(packet_t), 0, (struct sockaddr *)address, &addr_len);
        unsigned char buffer[DATA_LEN];
        buffer_to_message(buffer, packet);
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
            // Reenviar pacote atual em caso de erro
            print_packet(packet);
            send_status = send_packet(sockfd, packet, address, state);
        }

        if (send_status == TIMEOUT_ERROR)
        {
            printf("timeout occurred. Finishing program!!\n");
            exit(1);
            return TIMEOUT_ERROR;
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

    while (!feof(file))
    {
        data_size = fread(data, sizeof(*data), DATA_MAX_LEN, file);

        if (data_size <= 0)
            break;

        build_packet(packet, current_data_video_index, DADOS, (unsigned char *)data, data_size);

        unsigned char buffer[DATA_LEN];
        message_to_buffer(packet, data);

        int is_ack = 0;
        while (!is_ack && is_ack != -1)
        {
            if (send_packet(sock, packet, &connection->address, &connection->state) < 0)
            {
                perror("Error sending packet");
                continue;
            }

            is_ack = wait_for_ack_socket(sock, packet, &connection->address, &connection->state);

#ifdef DEBUG
            printf("[ETHBKP][SNDMSG] Message sent, is_ack=%d\n\n", is_ack);
#endif
        }

        if (is_ack == -1)
        {
            printf("received a timeout");
            exit(1);
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
            size_t video_path_len = strlen(VIDEO_LOCATION) + strlen(video_name) + 1;
            // +1 para o terminador nulo
            video_path = malloc(video_path_len + 1);
            if (video_path)
            {
                snprintf(video_path, video_path_len + 1, "%s%s", VIDEO_LOCATION, video_name);
                printf("video_path encontrado %s\n", video_path);
            }
            break;
        }
    }

    if (video_path == NULL)
    {
        printf("Vídeo não encontrado\n");
    }

    closedir(d);
    return video_path;
}

int check_crc(packet_t *packet)
{
    unsigned char received_crc = packet->crc;
    packet->crc = 0;
    unsigned char computed_crc = calculate_crc8((unsigned char *)packet, sizeof(packet_t) - 1);

    return received_crc != computed_crc;
}
