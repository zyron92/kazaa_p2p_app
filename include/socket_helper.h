#ifndef __SOCKET_HELPER_H__
#define __SOCKET_HELPER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "csapp.h"
#include "msg_type.h"

#define MAX_BUFFER 4096

/*
 * Creating a server socket, binding it to 0.0.0.0:portNumber and preparing it
 * Binding to all interfaces with the port number specified
 * Child : for listening file download request
 * Super : for listening file info sharing & hello
 */
int server_listening(int portNumber);

/*
 * Creating a client socket, and connecting it to ipAddress:portNum
 */
int client_connecting(const char *ipAddress, int portNum);

/*
 * Getting client socket information
 */
int get_connected_client_info(struct sockaddr_in *client_socket_info,
                              char **ipAddress, int *portNum);

/*
 * Return the ip address after the resolution of domain name if it is a domain
 * name, or else return itself if it is already an ip address
 *
 * Free is required
 */
char *resolve_domain_name(const char *domainName);

#endif
