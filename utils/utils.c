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
