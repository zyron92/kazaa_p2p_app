#include "get_options.h"

/*
 * Get options and initialize variables
 */
int getOptions(int argc, char **argv, unsigned int *listeningPortNum,
               char **superIp, unsigned int *superPortNum)
{
    int opt;
    static struct option longOptions[] =
            {
                    {"s_ip",   required_argument, 0, 'i'},
                    {"s_port", required_argument, 0, 's'},
                    {"l_port", required_argument, 0, 'p'},
                    {0,        0,                 0, 0}
            };

    //Check for all options
    while (1) {
        opt = getopt_long(argc, argv, "i:s:p:", longOptions, NULL);

        //End of the options
        if (opt == -1) {
            break;
        }

        //Check for each option
        switch (opt) {
            case 'i':
                *superIp = calloc(strlen(optarg) + 1, sizeof(char));
                strncpy(*superIp, optarg, strlen(optarg));
                *superIp = resolve_domain_name(*superIp);
                break;
            case 's':
                sscanf(optarg, "%d", superPortNum);
                break;
            case 'p':
                sscanf(optarg, "%d", listeningPortNum);
                break;
            case '?':
                //in case any error
                return ERR;
            default:
                abort();
                return ERR;
        }
    }
    return OK;
}
