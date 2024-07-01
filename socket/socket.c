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
#define TEST


int ConexaoRawSocket(char *device)
{
  int soquete;
  struct ifreq ir;
  struct sockaddr_ll endereco;
  struct packet_mreq mr;

  soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));  	/*cria socket*/
  if (soquete == -1) {
    printf("Erro no Socket\n");
    exit(-1);
  }

  memset(&ir, 0, sizeof(struct ifreq));  	/*dispositivo eth0*/
  memcpy(ir.ifr_name, device, sizeof(device));
  if (ioctl(soquete, SIOCGIFINDEX, &ir) == -1) {
    printf("Erro no ioctl\n");
    exit(-1);
  }


  memset(&endereco, 0, sizeof(endereco)); 	/*IP do dispositivo*/
  endereco.sll_family = AF_PACKET;
  endereco.sll_protocol = htons(ETH_P_ALL);
  endereco.sll_ifindex = ir.ifr_ifindex;
  if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1) {
    printf("Erro no bind\n");
    exit(-1);
  }


  memset(&mr, 0, sizeof(mr));          /*Modo Promiscuo*/
  mr.mr_ifindex = ir.ifr_ifindex;
  mr.mr_type = PACKET_MR_PROMISC;
  if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)	{
    printf("Erro ao fazer setsockopt\n");
    exit(-1);
  }

  return soquete;
}

// Listen to a socket, if a packet is received, it is copied to the output packet
int listen_socket(int _socket, packet_t *packet)
{
  uint8_t buffer[sizeof(packet_union_t)] = {0};
  ssize_t bytes_received = recv(_socket, buffer, sizeof(buffer), 0);

  // Check for errors
  if (bytes_received < 0)
  {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return -1;
    else
    {
      fprintf(stderr, "Error on recv: %s\n", strerror(errno));
      exit(-1);
    }
  }
  else if (bytes_received == 0)
    return -1;

  // check if packet starts with START_MARKER and has at least the size of a packet_t
  if (buffer[0] != STARTER_MARK || bytes_received < sizeof(packet_t))
    return -1;

  // Deserialize into packet_union_t
  packet_union_t pu;
  memcpy(pu.raw, buffer, bytes_received);

  // Validate packet
  uint8_t received_crc = pu.packet.crc;
  pu.packet.crc = 0; // Zero out CRC for validation
  uint8_t computed_crc = calculate_crc8(pu.raw, sizeof(pu.raw) - 1);

  if (received_crc != computed_crc)
    return NACK;

  // Copy packet to output
  packet->size = pu.packet.size;
  packet->seq_num = pu.packet.seq_num;
  packet->type = pu.packet.type;
  memcpy(packet->data, pu.packet.data, sizeof(packet->data));

  return ACK;
}

void build_packet(packet_t *pkt, uint8_t seq_num, uint8_t type, const uint8_t *data, size_t data_len)
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

ssize_t send_packet(int _socket, packet_t *packet, struct sockaddr_ll *address)
{
    packet_union_t pu;
    memcpy(pu.raw, packet, sizeof(packet_t));

    ssize_t bytes_sent = sendto(_socket, pu.raw, sizeof(packet_t), 0, (struct sockaddr *)address, sizeof(struct sockaddr_ll));

    if (bytes_sent < 0)
    {
        fprintf(stderr, "Error on sendto: %s\n", strerror(errno));
        exit(-1);
    }

    print_packet(packet);

    return bytes_sent;
}


// CRC-8 calculation function
uint8_t calculate_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

video_t *init_video_t() {
    video_t *video = (video_t *)malloc(sizeof(video_t));
    video->name = NULL;
    video->path = NULL;
    video->size = 0;
    return video;
}

void receive_packet(int sock, packet_t *packet) {
    int status = listen_socket(sock, packet);
    if (status == -1) {
        printf("Erro ao receber pacote\n");
        exit(EXIT_FAILURE);
    }
}
