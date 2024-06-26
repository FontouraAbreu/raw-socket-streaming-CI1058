#include "utils.h"

char *parse_args(int argc, char **argv, char *optstring)
{
	int opt;
	char *interface = NULL;

	while ((opt = getopt(argc, argv, optstring)) != -1)
	{
		switch (opt)
		{
		case 'i':
			interface = interface;
			break;

		default:
			fprintf(stderr, "Usage: %s -i <interface>\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	return interface;
}

void print_packet(packet_t *m) {
    if (!m)
        return;

    printf("Starter mark: %x\n", m->starter_mark);
    printf("Size: %d\n", m->size);
    printf("Sequence number: %d\n", m->seq_num);
    printf("Type: %d\n", m->type);
    printf("Data: %s\n", m->data);
    printf("CRC: %x\n", m->crc);
}
