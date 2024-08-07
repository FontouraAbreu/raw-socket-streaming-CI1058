#include "utils.h"

args_t parse_args(int argc, char **argv, char *optstring)
{
    int opt;
    args_t args = { NULL };

    if (argc != 3) {
        fprintf(stderr, "Usage: %s -i <interface>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 'i':
            args.interface = optarg;
            break;

        default:
            fprintf(stderr, "Usage: %s -i <interface>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (args.interface == NULL) {
        fprintf(stderr, "Usage: %s -i <interface>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return args;
}
