#include "utils.h"
#include <unistd.h> // Add this line to include the declaration for "optarg"
#include <getopt.h> // Add this line to include the declaration for "optarg"

args_t parse_args(int argc, char **argv, char *optstring)
{
    int opt;
    args_t args = { NULL, NULL };

    if (argc != 5) {
        fprintf(stderr, "Usage: %s -i <interface> -f <folder>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 'i':
            args.interface = optarg;
            break;
        case 'f':
            args.folder = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s -i <interface> -f <folder>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (args.interface == NULL || args.folder == NULL) {
        fprintf(stderr, "Usage: %s -i <interface> -f <folder>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return args;
}
