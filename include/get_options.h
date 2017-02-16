#ifndef __GET_OPTIONS_H__
#define __GET_OPTIONS_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

#include "msg_type.h"
#include "socket_helper.h"

/*
 * Get options and initialize variables
 */
int getOptions(int argc, char **argv, unsigned int *listeningPortNum,
               char **superIp, unsigned int *superPortNum);

#endif